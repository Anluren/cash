#pragma once

#include "bitbase.h"

namespace ch {
namespace internal {

class ch_sbool : public ch_sbitbase<ch_sbool> {
public:
  using traits = system_traits<1, false, ch_sbool, ch_bool>;
  using base = ch_sbitbase<ch_sbool>;
  using base::operator=;

  ch_sbool(const system_buffer& buffer
          = make_system_buffer(1, idname<ch_sbool>()))
    : buffer_(buffer) {
    assert(1 == buffer->size());
  }

  template <typename U,
            CH_REQUIRE(std::is_integral_v<U>)>
  ch_sbool(const U& other)
    : ch_sbool(make_system_buffer(1, idname<ch_sbool>())) {
    this->operator=(other);
  }

  template <typename U,
            CH_REQUIRE(is_bitvector_extended_type_v<U>)>
  explicit ch_sbool(U&& other)
    : ch_sbool(make_system_buffer(1, idname<ch_sbool>())) {
    this->operator=(make_system_buffer(sdata_type(1 , std::forward<U>(other))));
  }

  template <typename U,
            CH_REQUIRE(ch_width_v<U> == 1)>
  explicit ch_sbool(const ch_sbitbase<U>& other)
    : ch_sbool(make_system_buffer(1, idname<ch_sbool>())) {
    this->operator=(reinterpret_cast<const U&>(other));
  }

  ch_sbool(const ch_sbool& other)
    : ch_sbool(make_system_buffer(1, idname<ch_sbool>())) {
    this->operator=(other);
  }

  ch_sbool(ch_sbool&& other) : buffer_(std::move(other.buffer_)) {}

  ch_sbool& operator=(const ch_sbool& other) {
    system_accessor::assign(*this, other);
    return *this;
  }

  ch_sbool& operator=(ch_sbool&& other) {
    system_accessor::move(*this, std::move(other));
    return *this;
  }

protected:

  void do_print(std::ostream& out) const {
    auto oldflags = out.flags();
    out << std::boolalpha;
    out << static_cast<bool>(*this);
    out.flags(oldflags);
  }

  const system_buffer& __buffer() const {
    return buffer_;
  }

  system_buffer buffer_;

  friend class system_accessor;
};

///////////////////////////////////////////////////////////////////////////////

class ch_bool : public ch_bitbase<ch_bool> {
public:
  using traits = logic_traits<1, false, ch_bool, ch_sbool>;
  using base = ch_bitbase<ch_bool>;
  using base::operator=;

  ch_bool(const logic_buffer& buffer
          = make_logic_buffer(1, idname<ch_bool>()))
    : buffer_(buffer) {
    assert(1 == buffer.size());
  }

  template <typename U,
            CH_REQUIRE(std::is_integral_v<U>)>
  ch_bool(const U& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(other);
  }

  template <typename U,
            CH_REQUIRE(ch_width_v<U> == 1)>
  explicit ch_bool(const ch_sbitbase<U>& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(reinterpret_cast<const U&>(other));
  }

  ch_bool(const ch_sbool& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(other);
  }

  ch_bool(const ch_sbit<1>& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(other);
  }

  ch_bool(const ch_sint<1>& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(other);
  }

  ch_bool(const ch_suint<1>& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(other);
  }

  template <typename U,
            CH_REQUIRE(ch_width_v<U> == 1)>
  explicit ch_bool(const ch_bitbase<U>& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(reinterpret_cast<const U&>(other));
  }

  ch_bool(const ch_bool& other)
    : ch_bool(make_logic_buffer(1, idname<ch_bool>())) {
    this->operator=(other);
  }

  ch_bool(ch_bool&& other) : buffer_(std::move(other.buffer_)) {}

  ch_bool& operator=(const ch_bool& other) {
    CH_API_ENTRY(1);
    logic_accessor::assign(*this, other);
    return *this;
  }

  ch_bool& operator=(ch_bool&& other) {
    CH_API_ENTRY(1);
    logic_accessor::move(*this, std::move(other));
    return *this;
  }

protected:

  void do_print(ch_ostream& out) const {
    out.write(get_lnode(*this), 'b');
  }

  const logic_buffer& __buffer() const {
    return buffer_;
  }

  logic_buffer buffer_;

  friend class logic_accessor;
};

}
}