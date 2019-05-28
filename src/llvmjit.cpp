#include "llvmjit.h"
#include "common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#pragma GCC diagnostic pop

#define	JIT_TYPE_BOOL   6
extern const jit_type_t jit_type_bool;

class _jit_type : public ch::internal::refcounted {
public:
  _jit_type()
    : kind_(JIT_TYPE_INVALID)
    , impl_(nullptr)
  {}

  _jit_type(const _jit_type& other)
   : kind_(other.kind_)
   , impl_(other.impl_)
  {}

  void init(int kind, llvm::Type* impl) {
    kind_ = kind;
    impl_ = impl;
  }

  auto kind() const {
    return kind_;
  }

  auto impl() const {
    return impl_;
  }

  static jit_type_t from_llvm_type(llvm::Type* type) {
    switch (type->getTypeID()) {
    case llvm::Type::VoidTyID:
      return jit_type_void;
    case llvm::Type::PointerTyID:
      return jit_type_ptr;
    case llvm::Type::IntegerTyID: {
      auto size = llvm::cast<llvm::IntegerType>(type)->getBitWidth();
      switch (size) {
      case 1:
        return jit_type_bool;
      case 8:
        return jit_type_int8;
      case 16:
        return jit_type_int16;
      case 32:
        return jit_type_int32;
      case 64:
        return jit_type_int64;
      default:
        assert(false);
        return nullptr;
      }
    }
    default:
      assert(false);
      return nullptr;
    }
  }

protected:
  int kind_;
  llvm::Type* impl_;
};

_jit_type jit_type_void_def;
_jit_type jit_type_bool_def;
_jit_type jit_type_int8_def;
_jit_type jit_type_int16_def;
_jit_type jit_type_int32_def;
_jit_type jit_type_int64_def;
_jit_type jit_type_ptr_def;

const jit_type_t jit_type_void  = &jit_type_void_def;
const jit_type_t jit_type_bool  = &jit_type_bool_def;
const jit_type_t jit_type_int8  = &jit_type_int8_def;
const jit_type_t jit_type_int16 = &jit_type_int16_def;
const jit_type_t jit_type_int32 = &jit_type_int32_def;
const jit_type_t jit_type_int64 = &jit_type_int64_def;
const jit_type_t jit_type_ptr   = &jit_type_ptr_def;

///////////////////////////////////////////////////////////////////////////////

class _jit_signature : public _jit_type {
public:
  _jit_signature(jit_type_t return_type,
                 jit_type_t* arg_types,
                 unsigned int num_args)
    : _jit_type(*return_type)
    , return_type_(return_type)
    , arg_types_(num_args) {
    for (unsigned int i = 0; i < num_args; ++i) {
      arg_types_[i] = arg_types[i];
    }
  }

  auto return_type() const {
    return return_type_;
  }

  const auto& arg_types() const {
    return arg_types_;
  }

private:
  jit_type_t return_type_;
  std::vector<jit_type_t> arg_types_;
};

///////////////////////////////////////////////////////////////////////////////

class _jit_value {
public:
  _jit_value(llvm::Value* impl, jit_type_t type)
    : impl_(impl)
    , alloc_(nullptr)
    , type_(type)
  {}

  _jit_value(llvm::AllocaInst* alloc, jit_type_t type)
    : impl_(nullptr)
    , alloc_(alloc)
    , type_(type)
  {}

  auto impl() const {
    assert(impl_);
    return impl_;
  }

  auto alloc() const {
    assert(alloc_);
    return alloc_;
  }

  auto is_mutable() const {
    return (alloc_ != nullptr);
  }

  auto type() const {
    return type_;
  }

private:
  llvm::Value* impl_;
  llvm::AllocaInst* alloc_;
  jit_type_t type_;
};

///////////////////////////////////////////////////////////////////////////////

class _jit_block {
public:
  _jit_block(_jit_function* func, llvm::BasicBlock* impl)
    : func_(func)
    , impl_(impl)
  {}

  auto func() const {
    return func_;
  }

  auto impl() const {
    return impl_;
  }

private:
  _jit_function* func_;
  llvm::BasicBlock* impl_;
};

///////////////////////////////////////////////////////////////////////////////

class _jit_context {
public:

  _jit_context(const llvm::orc::JITTargetMachineBuilder& JTMB,
               llvm::TargetMachine* TM,
               const llvm::DataLayout& DL)
    : thread_context_(llvm::make_unique<llvm::LLVMContext>())
    , context_(thread_context_.getContext())
    , builder_(*context_)
    , object_layer_(ex_session_, []() { return llvm::make_unique<llvm::SectionMemoryManager>(); })
    , compile_layer_(ex_session_, object_layer_, llvm::orc::ConcurrentIRCompiler(JTMB))
    , mangle_(ex_session_, DL)
    , target_(TM) {
    //--
    ex_session_.getMainJITDylib().setGenerator(
      llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(DL)));

    //--
    module_ = std::make_unique<llvm::Module>("llvmjit", *context_);
    module_->setDataLayout(DL);

    //--
    jit_type_void_def.init(JIT_TYPE_VOID, llvm::Type::getVoidTy(*context_));
    jit_type_bool_def.init(JIT_TYPE_BOOL, llvm::Type::getInt1Ty(*context_));
    jit_type_int8_def.init(JIT_TYPE_INT8, llvm::Type::getInt8Ty(*context_));
    jit_type_int16_def.init(JIT_TYPE_INT16, llvm::Type::getInt16Ty(*context_));
    jit_type_int32_def.init(JIT_TYPE_INT32, llvm::Type::getInt32Ty(*context_));
    jit_type_int64_def.init(JIT_TYPE_INT64, llvm::Type::getInt64Ty(*context_));
    jit_type_ptr_def.init(JIT_TYPE_PTR, llvm::Type::getInt8PtrTy(*context_));
  }

  ~_jit_context() {}

  auto impl() const {
    return context_;
  }

  auto target() const {
    return target_.get();
  }

  auto module() const {
    return module_.get();
  }

  auto builder() {
    return &builder_;
  }

  int compile(llvm::Function* func) {
    {
      static llvm::raw_os_ostream os(std::cerr);
      if (llvm::verifyFunction(*func, &os))
        return 0;
    }
    {
      llvm::PassManagerBuilder pmb;
      pmb.OptLevel = 3;
      pmb.SizeLevel = 0;
      pmb.LoopVectorize = true;
      pmb.SLPVectorize = true;

      llvm::legacy::FunctionPassManager fpm(module_.get());
      llvm::legacy::PassManager pm;
      pmb.populateFunctionPassManager(fpm);
      pmb.populateModulePassManager(pm);
      fpm.doInitialization();
      fpm.run(*func);
      pm.run(*module_);
    }
    return 1;
  }

  void* closure(const std::string& name) {
    auto err = compile_layer_.add(
          ex_session_.getMainJITDylib(),
          llvm::orc::ThreadSafeModule(std::move(module_), thread_context_));
    if (err)
      return nullptr;
    auto ret = ex_session_.lookup(
      {&ex_session_.getMainJITDylib()}, mangle_(name));
    if (!ret)
      return nullptr;
    return (void*)ret->getAddress();
  }

private:

  llvm::orc::ExecutionSession ex_session_;
  llvm::orc::ThreadSafeContext thread_context_;
  llvm::LLVMContext* context_;
  llvm::IRBuilder<> builder_;
  llvm::orc::RTDyldObjectLinkingLayer object_layer_;
  llvm::orc::IRCompileLayer compile_layer_;
  llvm::orc::MangleAndInterner mangle_;
  std::unique_ptr<llvm::TargetMachine> target_;
  std::unique_ptr<llvm::Module> module_;
};

///////////////////////////////////////////////////////////////////////////////

class _jit_function {
public:
  _jit_function(_jit_context* ctx,
                llvm::Function* impl,
                jit_type_t return_type)
    : ctx_(ctx)
    , impl_(impl)
    , return_type_(return_type)
    , args_(impl->arg_size())
    , cur_label_(0) {
    int i = 0;
    for (auto& arg : impl->args()) {
      auto type = _jit_type::from_llvm_type(arg.getType());
      args_[i] = std::make_unique<_jit_value>(&arg, type);
      ++i;
    }
    auto bb = llvm::BasicBlock::Create(*ctx->impl(), "entry", impl);
    ctx->builder()->SetInsertPoint(bb);
    labels_.emplace(cur_label_, bb);
  }

  auto ctx() const {
    return ctx_;
  }

  auto impl() const {
    return impl_;
  }

  auto return_type() {
    return return_type_;
  }

  auto arg(unsigned int index) const {
    return args_[index].get();
  }

  auto resolve_value(jit_value_t value, jit_type_t type = nullptr) {
    llvm::Value* ret;
    if (value->is_mutable()) {
      ret = ctx_->builder()->CreateLoad(value->alloc());
    } else {
      ret = value->impl();
    }
    if (type
     && value->type()->kind() != type->kind()) {
      assert(jit_type_get_size(value->type()) <= jit_type_get_size(type));
      ret = ctx_->builder()->CreateIntCast(ret, type->impl(), false);
    }
    return ret;
  }

  auto create_value(jit_type_t type) {
    llvm::IRBuilder<> builder(&impl_->getEntryBlock(),
                              impl_->getEntryBlock().begin());
    auto alloc = builder.CreateAlloca(type->impl());
    variables_.emplace_back(std::make_unique<_jit_value>(alloc, type));
    return variables_.back().get();
  }

  auto create_value(llvm::Value* value) {
    auto type = _jit_type::from_llvm_type(value->getType());
    variables_.emplace_back(std::make_unique<_jit_value>(value, type));
    return variables_.back().get();
  }

  llvm::BasicBlock* create_block(jit_label_t* label, bool reuse = false) {
    if (!label)
      return nullptr;
    auto l = *label;
    if (l != jit_label_undefined) {
      auto it = labels_.find(l);
      if (it == labels_.end())
        return nullptr;
      return it->second;
    }
    if (reuse) {
      auto bb = labels_[cur_label_];
      if (0 == bb->size()) {
        *label = cur_label_;
        return bb;
      }
    }
    auto bb = llvm::BasicBlock::Create(*ctx_->impl());
    l = labels_.size();
    labels_.emplace(l, bb);
    *label = l;
    return bb;
  }

  llvm::BasicBlock* get_current_block() {
    return labels_[cur_label_];
  }

  void set_current_block(llvm::BasicBlock* block,
                         jit_label_t label,
                         bool detached = false) {
    if (!detached) {
      auto bb = labels_[cur_label_];
      if (nullptr == bb->getTerminator()) {
        ctx_->builder()->CreateBr(block);
      }
    }
    impl_->getBasicBlockList().push_back(block);
    ctx_->builder()->SetInsertPoint(block);
    cur_label_ = label;
  }

  auto cur_label() const {
    return cur_label_;
  }

private:
  _jit_context* ctx_;
  llvm::Function* impl_;
  jit_type_t return_type_;
  std::vector<std::unique_ptr<_jit_value>> args_;
  std::vector<std::unique_ptr<_jit_value>> variables_;
  std::unordered_map<jit_label_t, llvm::BasicBlock*> labels_;
  jit_label_t cur_label_;
};

///////////////////////////////////////////////////////////////////////////////

auto resolve_pointer(_jit_context* ctx, llvm::Value* value, llvm::Type* type) {
  if (value->getType() != type) {
    assert(type->getTypeID() == llvm::Type::IntegerTyID);
    auto size = llvm::cast<llvm::IntegerType>(type)->getBitWidth();
    switch (size) {
    case 8:
      type = llvm::Type::getInt8PtrTy(*ctx->impl());
      break;
    case 16:
      type = llvm::Type::getInt16PtrTy(*ctx->impl());
      break;
    case 32:
      type = llvm::Type::getInt32PtrTy(*ctx->impl());
      break;
    case 64:
      type = llvm::Type::getInt64PtrTy(*ctx->impl());
      break;
    default:
      assert(false);
      break;
    }
    return ctx->builder()->CreateBitCast(value, type);
  }
  return value;
}

///////////////////////////////////////////////////////////////////////////////

jit_context_t jit_context_create() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  auto JTMB = llvm::orc::JITTargetMachineBuilder::detectHost();
  if (!JTMB) {
    std::cerr << "JITTargetMachineBuilder::detectHost() failed" << std::endl;
    return nullptr;
  }

  auto TM = JTMB->createTargetMachine();
  if (!TM) {
    std::cerr << "JITTargetMachineBuilder::createTargetMachine() failed" << std::endl;
    return nullptr;
  }

  auto DL = (*TM)->createDataLayout();

  return new _jit_context(*JTMB, TM->release(), DL);
}

void jit_context_destroy(jit_context_t context) {
  delete context;
}

void jit_context_build_start(jit_context_t context) {
  CH_UNUSED(context);
}

void jit_context_build_end(jit_context_t context) {
  CH_UNUSED(context);
}

///////////////////////////////////////////////////////////////////////////////

jit_function_t jit_function_create(jit_context_t context, jit_type_t signature) {
  auto j_sig = reinterpret_cast<_jit_signature*>(signature);
  std::vector<llvm::Type*> args(j_sig->arg_types().size());
  for (unsigned i = 0; i < j_sig->arg_types().size(); ++i) {
    args[i] = j_sig->arg_types()[i]->impl();
  }
  auto sig = llvm::FunctionType::get(j_sig->impl(), args, false);
  auto func = llvm::Function::Create(sig,
                                     llvm::Function::ExternalLinkage,
                                     "eval",
                                     context->module());  
  return new _jit_function(context, func, j_sig->return_type());
}

int jit_function_compile(jit_function_t func) {
  auto ctx = func->ctx();
  return ctx->compile(func->impl());
}

void *jit_function_to_closure(jit_function_t func) {
  auto ctx = func->ctx();
  return ctx->closure("eval");
}

jit_block_t jit_function_get_current(jit_function_t func) {
  CH_UNUSED(func);
  CH_TODO();
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

jit_type_t jit_type_create_signature(jit_abi_t abi,
                                     jit_type_t return_type,
                                     jit_type_t *args,
                                     unsigned int num_args,
                                     int incref) {
  CH_UNUSED(abi, incref);
  auto sig = new _jit_signature(return_type, args, num_args);
  sig->acquire();
  return sig;
}

void jit_type_free(jit_type_t type) {
  type->release();
}

int jit_type_get_kind(jit_type_t type) {
  return type->kind();
}

jit_nuint jit_type_get_size(jit_type_t type) {
  auto kind = type->kind();
  switch (kind) {
  case JIT_TYPE_VOID:
    return 0;
  case JIT_TYPE_BOOL:
    return 1;
  case JIT_TYPE_INT8:
    return 1;
  case JIT_TYPE_INT16:
    return 2;
  case JIT_TYPE_INT32:
    return 4;
  case JIT_TYPE_INT64:
    return 8;
  case JIT_TYPE_PTR:
    return 8;
  default:
    assert(false);
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

jit_value_t jit_value_create(jit_function_t func, jit_type_t type) {
  return func->create_value(type);
}

jit_type_t jit_value_get_type(jit_value_t value) {
  return value->type();
}

jit_value_t jit_value_get_param(jit_function_t func, unsigned int param) {
  return func->arg(param);
}

jit_value_t jit_value_create_int_constant(jit_function_t func,
                                          jit_long const_value,
                                          jit_type_t type) {
  auto builder = func->ctx()->builder();
  auto kind = type->kind();
  switch (kind) {
  case JIT_TYPE_BOOL:
    return func->create_value(builder->getInt1(const_value));
  case JIT_TYPE_INT8:
    return func->create_value(builder->getInt8(const_value));
  case JIT_TYPE_INT16:
    return func->create_value(builder->getInt16(const_value));
  case JIT_TYPE_INT32:
    return func->create_value(builder->getInt32(const_value));
  case JIT_TYPE_INT64:
    return func->create_value(builder->getInt64(const_value));
  case JIT_TYPE_PTR:
    return func->create_value(builder->getInt64(const_value));
  default:
    assert(false);
  }
  return nullptr;
}

jit_long jit_value_get_int_constant(jit_value_t value) {
  auto const_value = llvm::dyn_cast<llvm::ConstantInt>(value->impl());
  return const_value->getSExtValue();
}

int jit_value_is_constant(jit_value_t value) {
  if (value->is_mutable())
    return false;
  return (llvm::dyn_cast<llvm::ConstantInt>(value->impl()) != nullptr);
}

///////////////////////////////////////////////////////////////////////////////

jit_value_t jit_insn_add(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateAdd(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_sub(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();  
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateSub(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_mul(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateMul(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_sdiv(jit_function_t func,
                          jit_value_t value1,
                          jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateSDiv(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_udiv(jit_function_t func,
                          jit_value_t value1,
                          jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateUDiv(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_srem(jit_function_t func,
                          jit_value_t value1,
                          jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateSRem(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_urem(jit_function_t func,
                          jit_value_t value1,
                          jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateURem(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_neg(jit_function_t func, jit_value_t value) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto in = func->resolve_value(value);
  auto res = builder->CreateNeg(in);
  return func->create_value(res);
}

jit_value_t jit_insn_and(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateAnd(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_or(jit_function_t func,
                        jit_value_t value1,
                        jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateOr(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_xor(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateXor(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_not(jit_function_t func, jit_value_t value) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto in = func->resolve_value(value);
  auto res = builder->CreateNot(in);
  return func->create_value(res);
}

jit_value_t jit_insn_shl(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2, value1->type());
  auto res = builder->CreateShl(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_shr(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2, value1->type());
  auto res = builder->CreateAShr(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_ushr(jit_function_t func,
                          jit_value_t value1,
                          jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2, value1->type());
  auto res = builder->CreateLShr(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_eq(jit_function_t func,
                        jit_value_t value1,
                        jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpEQ(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_ne(jit_function_t func,
                        jit_value_t value1,
                        jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpNE(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_slt(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpSLT(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_ult(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpULT(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_sle(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpSLE(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_ule(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpULE(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_sgt(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpSGT(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_ugt(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpUGT(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_sge(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpSGE(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_uge(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto lhs = func->resolve_value(value1);
  auto rhs = func->resolve_value(value2);
  auto res = builder->CreateICmpUGE(lhs, rhs);
  return func->create_value(res);
}

jit_value_t jit_insn_to_bool(jit_function_t func, jit_value_t value1) {
  CH_UNUSED(func, value1);
  CH_TODO();
  return nullptr;
}

jit_value_t jit_insn_to_not_bool(jit_function_t func, jit_value_t value1) {
  CH_UNUSED(func, value1);
  CH_TODO();
  return nullptr;
}

jit_value_t jit_insn_abs(jit_function_t func, jit_value_t value1) {
  CH_UNUSED(func, value1);
  CH_TODO();
  return nullptr;
}

jit_value_t jit_insn_min(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  CH_UNUSED(func, value1, value2);
  CH_TODO();
  return nullptr;
}

jit_value_t jit_insn_max(jit_function_t func,
                         jit_value_t value1,
                         jit_value_t value2) {
  CH_UNUSED(func, value1, value2);
  CH_TODO();
  return nullptr;
}

int jit_insn_store(jit_function_t func, jit_value_t dest, jit_value_t value) {
  assert(dest->is_mutable());
  auto ctx = func->ctx();
  auto builder = ctx->builder();  
  auto in = func->resolve_value(value);
  auto addr = resolve_pointer(ctx, dest->alloc(), in->getType());
  builder->CreateStore(in, addr);
  return 1;
}

jit_value_t jit_insn_load_relative(jit_function_t func,
                                   jit_value_t base_addr,
                                   jit_nint offset,
                                   jit_type_t type) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto addr  = func->resolve_value(base_addr);
  if (offset) {
    auto idx = builder->getInt32(offset);
    addr = builder->CreateInBoundsGEP(jit_type_int8->impl(), addr, idx);
  }
  auto value = builder->CreateLoad(type->impl(), addr);
  return func->create_value(value);
}

int jit_insn_store_relative(jit_function_t func,
                            jit_value_t base_addr,
                            jit_nint offset,
                            jit_value_t value) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto in = func->resolve_value(value);
  auto addr = func->resolve_value(base_addr);
  if (offset) {
    auto idx = builder->getInt32(offset);
    addr = builder->CreateInBoundsGEP(jit_type_int8->impl(), addr, idx);
  }
  addr = resolve_pointer(ctx, addr, in->getType());
  auto inst = builder->CreateStore(in, addr);
  return (inst != nullptr);
}

jit_value_t jit_insn_add_relative(jit_function_t func,
                                  jit_value_t base_addr,
                                  jit_nint offset) {
  assert(jit_type_ptr == base_addr->type());
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto idx = builder->getInt32(offset);
  auto addr = func->resolve_value(base_addr);
  addr = builder->CreateInBoundsGEP(jit_type_int8->impl(), addr, idx);
  return func->create_value(addr);
}

jit_value_t jit_insn_load_elem(jit_function_t func,
                               jit_value_t base_addr,
                               jit_value_t index,
                               jit_type_t elem_type) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto idx = func->resolve_value(index);
  auto addr = func->resolve_value(base_addr);
  addr = builder->CreateInBoundsGEP(elem_type->impl(), addr, idx);
  auto value = builder->CreateLoad(elem_type->impl(), addr);
  return func->create_value(value);
}

jit_value_t jit_insn_load_elem_address(jit_function_t func,
                                       jit_value_t base_addr,
                                       jit_value_t index,
                                       jit_type_t elem_type) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto idx = func->resolve_value(index);
  auto addr = func->resolve_value(base_addr);
  auto value = builder->CreateInBoundsGEP(elem_type->impl(), addr, idx);
  return func->create_value(value);
}

int jit_insn_store_elem(jit_function_t func,
                        jit_value_t base_addr,
                        jit_value_t index,
                        jit_value_t value) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto in = func->resolve_value(value);
  auto elem_type = value->type();
  auto addr = func->resolve_value(base_addr);
  auto idx = func->resolve_value(index);
  addr = builder->CreateInBoundsGEP(elem_type->impl(), addr, idx);
  addr = resolve_pointer(ctx, addr, elem_type->impl());
  auto inst = builder->CreateStore(in, addr);
  return (inst != nullptr);
}

jit_value_t jit_insn_address_of(jit_function_t func, jit_value_t value) {
  CH_UNUSED(func, value);
  CH_TODO();
  return nullptr;
}

void jit_insn_set_marker(jit_function_t func, const char* name) {
  CH_UNUSED(func, name);
}

int jit_insn_branch(jit_function_t func, jit_label_t *label) {
  auto BB = func->create_block(label);
  if (!BB)
    return 0;
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  builder->CreateBr(BB);
  return 1;
}

jit_value_t jit_insn_select(jit_function_t func,
                            jit_value_t cond,
                            jit_value_t case_true,
                            jit_value_t case_false) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto type = case_true->type();
  auto dst = func->create_value(type);
  auto cc = func->resolve_value(cond);
  auto ctype = cond->type();
  if (ctype->kind() != JIT_TYPE_BOOL) {
    auto zero = jit_value_create_int_constant(func, 0, ctype);
    cc = builder->CreateICmpNE(cc, zero->impl());
  }
  auto tt = func->resolve_value(case_true);
  auto ff = func->resolve_value(case_false);
  auto res = builder->CreateSelect(cc, tt, ff);
  auto inst = builder->CreateStore(res, dst->alloc());
  assert(inst != nullptr);
  return dst;
}

jit_value_t jit_insn_switch(jit_function_t func,
                            jit_value_t key,
                            const jit_value_t* preds,
                            const jit_value_t* values,
                            unsigned int num_cases,
                            jit_value_t def_value) {
  assert(num_cases != 0);
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto type = values[0]->type();
  auto dst = func->create_value(type);
  auto k = func->resolve_value(key);
  jit_label_t l_defBB(jit_label_undefined);
  auto defBB = func->create_block(&l_defBB);
  jit_label_t l_exitBB(jit_label_undefined);
  auto exitBB = func->create_block(&l_exitBB);
  auto sw = builder->CreateSwitch(k, defBB, num_cases);
  for (unsigned int i = 0; i < num_cases; ++i) {
    jit_label_t l_caseBB(jit_label_undefined);
    auto caseBB = func->create_block(&l_caseBB);
    {
      // case block
      func->set_current_block(caseBB, l_caseBB, true);
      auto value = func->resolve_value(values[i]);
      auto inst = builder->CreateStore(value, dst->alloc());
      assert(inst != nullptr);
      builder->CreateBr(exitBB);
    }
    auto pred = func->resolve_value(preds[i]);
    auto i_pred = llvm::dyn_cast<llvm::ConstantInt>(pred);
    sw->addCase(i_pred, caseBB);
  }
  {
    // default block
    func->set_current_block(defBB, l_defBB, true);
    auto value = func->resolve_value(def_value);
    auto inst = builder->CreateStore(value, dst->alloc());
    assert(inst != nullptr);
    builder->CreateBr(exitBB);
  }
  // exit block
  func->set_current_block(exitBB, l_exitBB, true);
  return dst;
}

int jit_insn_branch_if(jit_function_t func,
                       jit_value_t value,
                       jit_label_t *label) {
  jit_label_t l_elseBB(jit_label_undefined);
  auto thenBB = func->create_block(label);
  auto elseBB = func->create_block(&l_elseBB);
  if (!thenBB || !elseBB)
    return 0;
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto type = value->type();
  auto cond = func->resolve_value(value);
  if (type->kind() != JIT_TYPE_BOOL) {
    auto zero = jit_value_create_int_constant(func, 0, type);
    cond = builder->CreateICmpNE(cond, zero->impl());
  }
  builder->CreateCondBr(cond, thenBB, elseBB);
  func->set_current_block(elseBB, l_elseBB);
  return 1;
}

int jit_insn_branch_if_not(jit_function_t func,
                           jit_value_t value,
                           jit_label_t *label) {
  jit_label_t l_elseBB(jit_label_undefined);
  auto thenBB = func->create_block(label);
  auto elseBB = func->create_block(&l_elseBB);
  if (!thenBB || !elseBB)
    return 0;
  auto ctx = func->ctx();
  auto builder = ctx->builder();  
  auto type = value->type();
  auto cond = func->resolve_value(value);
  if (type->kind() != JIT_TYPE_BOOL) {
    auto zero = jit_value_create_int_constant(func, 0, type);
    cond = builder->CreateICmpNE(cond, zero->impl());
  }
  builder->CreateCondBr(cond, elseBB, thenBB);
  func->set_current_block(elseBB, l_elseBB);
  return 1;
}

int jit_insn_label(jit_function_t func, jit_label_t *label) {
  auto bb = func->create_block(label);
  if (!bb)
    return 0;
  func->set_current_block(bb, *label);
  return 1;
}

int jit_insn_label_tight(jit_function_t func, jit_label_t *label) {
  auto cur_label = func->cur_label();
  auto bb = func->create_block(label, true);
  if (!bb)
    return 0;
  if (*label != cur_label) {
    func->set_current_block(bb, *label);
  }
  return 1;
}

int jit_insn_new_block(jit_function_t func) {
  CH_UNUSED(func);
  CH_TODO();
  return 0;
}

int jit_insn_return(jit_function_t func, jit_value_t value) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto in = func->resolve_value(value);
  builder->CreateRet(in);
  return 1;
}

int jit_insn_move_blocks_to_end(jit_function_t func,
                                jit_label_t from_label,
                                jit_label_t to_label) {
  CH_UNUSED(func, from_label, to_label);
  CH_TODO();
  return 0;
}

int jit_insn_memcpy(jit_function_t func,
                    jit_value_t dest,
                    jit_value_t src,
                    jit_value_t size) {
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto ll_dest = func->resolve_value(dest);
  auto ll_src = func->resolve_value(src);
  auto ll_size = func->resolve_value(size);
  auto inst = builder->CreateMemCpy(ll_dest, 1, ll_src, 1, ll_size);
  assert(inst != nullptr);
  return 1;
}

jit_value_t jit_insn_call_native(jit_function_t func,
                                 const char *name,
                                 void *native_func,
                                 jit_type_t signature,
                                 jit_value_t *args,
                                 unsigned int num_args,
                                 int flags) {
  CH_UNUSED(native_func);
  assert(flags == JIT_CALL_NOTHROW);
  auto ctx = func->ctx();
  auto builder = ctx->builder();

  llvm::Function* ext_func;
  {
    auto j_sig = reinterpret_cast<_jit_signature*>(signature);
    std::vector<llvm::Type*> args(j_sig->arg_types().size());
    for (unsigned i = 0; i < j_sig->arg_types().size(); ++i) {
      args[i] = j_sig->arg_types()[i]->impl();
    }
    auto sig = llvm::FunctionType::get(j_sig->impl(), args, false);
    ext_func = llvm::Function::Create(sig,
                                      llvm::Function::ExternalLinkage,
                                      name,
                                      ctx->module());
  }
  llvm::Value* res;
  {
    std::vector<llvm::Value*> ll_args(num_args);
    for (unsigned int i = 0; i < num_args; ++i) {
      ll_args[i] = func->resolve_value(args[i]);
    }
    res = builder->CreateCall(ext_func, ll_args);
  }
  return func->create_value(res);
}

jit_value_t jit_insn_sext(jit_function_t func,
                          jit_value_t value,
                          jit_type_t type) {
  auto vtype   = jit_value_get_type(value);
  auto vt_size = jit_type_get_size(vtype);
  auto t_size  = jit_type_get_size(type);
  assert(vt_size <= t_size);
  if (vt_size == t_size)
    return value;
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto in = func->resolve_value(value);
  auto res = builder->CreateSExt(in, type->impl());
  return func->create_value(res);
}

jit_value_t jit_insn_convert(jit_function_t func,
                             jit_value_t value,
                             jit_type_t type,
                             int overflow_check) {
  if (overflow_check) {
    CH_TODO();
    return nullptr;
  }
  if (value->type()->kind() == type->kind())
    return value;  
  auto ctx = func->ctx();
  auto builder = ctx->builder();
  auto in = func->resolve_value(value);
  auto res = builder->CreateIntCast(in, type->impl(), false);
  return func->create_value(res);
}

///////////////////////////////////////////////////////////////////////////////

int jit_dump_ast(FILE *stream, jit_function_t func, const char *name) {
  CH_UNUSED(name);
  std::stringstream ss;
  {
    llvm::raw_os_ostream os(ss);
    auto module = func->ctx()->module();
    module->print(os, nullptr);
  }
  fprintf(stream, "%s", ss.str().c_str());
  return 1;
}

int jit_dump_asm(FILE *stream, jit_function_t func, const char *name) {
  CH_UNUSED(name);
  llvm::SmallVector<char, 0> sv;
  {
    auto ctx = func->ctx();
    auto module = ctx->module();
    auto target = ctx->target();;
    llvm::legacy::PassManager pass;
    llvm::raw_svector_ostream os(sv);
    if (target->addPassesToEmitFile(
          pass, os, nullptr, llvm::TargetMachine::CGFT_AssemblyFile))
      return 0;
    pass.run(*module);
  }
  fprintf(stream, "%s", sv.data());
  return 1;
}
