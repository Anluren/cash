#include "lnodeimpl.h"
#include "bit.h"
#include "ioimpl.h"
#include "proxyimpl.h"
#include "litimpl.h"
#include "tickable.h"
#include "regimpl.h"
#include "cdomain.h"
#include "context.h"

using namespace ch::internal;

const char* ch::internal::to_string(lnodetype type) {
  static const char* sc_names[] = {
    CH_LNODE_ENUM(CH_LNODE_NAME)
  };
  return sc_names[CH_LNODE_INDEX(type)];
}

lnodeimpl::lnodeimpl(context* ctx, lnodetype type, uint32_t size)
  : ctx_(ctx)
  , id_(ctx->node_id())
  , type_(type)
  , value_(size)
{}

lnodeimpl::~lnodeimpl() {}

lnodeimpl* lnodeimpl::get_slice(uint32_t offset, uint32_t length) {
  assert(length <= value_.get_size());
  if (value_.get_size() == length)
    return this;
  return ctx_->createNode<proxyimpl>(lnode(this), offset, length);
}

void lnodeimpl::print(std::ostream& out, uint32_t level) const {
  out << "#" << id_ << " <- " << this->get_type() << value_.get_size();
  uint32_t n = srcs_.size();
  if (n > 0) {
    out << "(";
    for (uint32_t i = 0; i < n; ++i) {
      if (i > 0)
        out << ", ";
      out << "#" << srcs_[i].get_id();
    }
    out << ")";
  }
  if (level == 2) {
    out << " = " << value_;
  }
}

///////////////////////////////////////////////////////////////////////////////

undefimpl::undefimpl(context* ctx, uint32_t size)
  : lnodeimpl(ctx, type_undef, size)
{}

const bitvector& undefimpl::eval(ch_tick) {
  CH_ABORT("undefined node: %d!", id_);
  return value_;
}

///////////////////////////////////////////////////////////////////////////////

lnode::lnode() : impl_(nullptr) {}

lnode::lnode(const lnode& rhs) : impl_(rhs.impl_) {}

lnode::lnode(uint32_t size) : impl_(nullptr) {
  // force initialization inside conditional blocks
  if (ctx_curr()->conditional_enabled()) {
    this->ensureInitialized(size, true);
  }
}

lnode::lnode(uint32_t size, const lnode& src, unsigned src_offset) : impl_(nullptr) {
  assert(!src.is_empty());
  this->ensureInitialized(size, false);
  auto proxy = dynamic_cast<proxyimpl*>(impl_);
  proxy->add_source(0, src, src_offset, size);
}

lnode::lnode(lnodeimpl* impl) : impl_(impl) {
  assert(impl);
}

lnode::lnode(const bitvector& value) {
  impl_ = ctx_curr()->get_literal(value);
}

lnode::~lnode() {
  this->clear();
}

void lnode::clear() {
  impl_ = nullptr;
}

const lnode& lnode::ensureInitialized(uint32_t size) const {
  this->ensureInitialized(size, true);
  return *this;
}

lnode& lnode::operator=(const lnode& rhs) {
  impl_ = rhs.impl_;
  return *this;
}

bool lnode::is_empty() const {
  return (nullptr == impl_);
}

uint32_t lnode::get_id() const {
  return impl_ ? impl_->get_id() : 0; 
}

context* lnode::get_ctx() const {
  assert(impl_);
  return impl_->get_ctx();
}

uint32_t lnode::get_size() const {
  return impl_ ? impl_->get_size() : 0;
}

const bitvector& lnode::eval(ch_tick t) {
  assert(impl_);
  return impl_->eval(t);
}

void lnode::set_impl(lnodeimpl* impl) {
  assert(impl);
  impl_ = impl;
}

lnodeimpl* lnode::get_impl() const {
  assert(impl_);
  return impl_;
}

void lnode::ensureInitialized(uint32_t size, bool initialize) const {
  if (nullptr == impl_) {
    if (initialize) {
      impl_ = ctx_curr()->createNode<proxyimpl>(
            ctx_curr()->createNode<undefimpl>(size));
    } else {
      impl_ = ctx_curr()->createNode<proxyimpl>(size);
    }
  }
  assert(impl_->get_size() >= size);
}

void lnode::write(uint32_t dst_offset,
                  const lnode& src,
                  uint32_t src_offset,
                  uint32_t length,
                  uint32_t size) {
  assert(!src.is_empty());
  assert(size > dst_offset);
  assert(size >= dst_offset + length);
  context* ctx = src.get_ctx();  

  auto proxy = dynamic_cast<proxyimpl*>(impl_);
  if (nullptr == proxy) {
    lnodeimpl* impl = nullptr;
    std::swap<lnodeimpl*>(impl, impl_);
    this->ensureInitialized(size, nullptr == impl);
    proxy = dynamic_cast<proxyimpl*>(impl_);
    if (impl) {
      proxy->add_source(0, impl, 0, size);
    }
    ctx->remove_from_locals(impl_);
  }

  if (ctx->conditional_enabled(impl_)) {
    const auto& slices = proxy->get_update_slices(dst_offset, length);
    for (const auto& slice : slices) {
      lnodeimpl* src_impl = src.get_impl();
      if (slice.second != src.get_size()) {
        assert(slice.first >= dst_offset);
        uint32_t offset = src_offset + (slice.first - dst_offset);
        src_impl = ctx->createNode<proxyimpl>(src, offset, slice.second);
      }
      ctx->conditional_assign(*this, src_impl, slice.first, slice.second);
    }
  } else {
    proxy->add_source(dst_offset, src, src_offset, length);
  }
}

const bitvector& lnode::get_data() const {
  assert(impl_);
  return impl_->get_value();
}

bitvector& lnode::get_data() {
  assert(impl_);
  return impl_->get_value();
}

bool lnode::get_bool(unsigned i) const {
  assert(impl_);
  return impl_->get_bool(i);
}

void lnode::set_bool(unsigned i, bool value) {
  assert(impl_);
  impl_->set_bool(i, value);
}

lnodeimpl* lnode::clone(uint32_t size) const {
  return impl_ ? impl_->get_slice(0, size) : nullptr;
}

std::ostream& ch::internal::operator<<(std::ostream& out, lnodetype type) {
  out << to_string(type);
  return out;
}

std::ostream& ch::internal::operator<<(std::ostream& out, const lnode& node) {
  out << node.get_data();
  return out;
}
