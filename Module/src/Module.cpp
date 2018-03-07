//
// Created by Aman LaChapelle on 3/4/18.
//
// Hobbit
// Copyright (c) 2018 Aman LaChapelle
// Full license at Hobbit/LICENSE.txt
//

/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#include "Module.hpp"

Hobbit::Module::Module(const std::string &name, llvm::LLVMContext &ctx)
    : ctx_(&ctx), module_(llvm::make_unique<llvm::Module>(name, ctx)) {
  module_->setTargetTriple(llvm::sys::getDefaultTargetTriple());
  llvm::ArrayRef<llvm::Type *> malloc_arg_type = {
      llvm::Type::getInt64Ty(*ctx_)};
  llvm::FunctionType *malloc_ft = llvm::FunctionType::get(
      llvm::Type::getInt8PtrTy(*ctx_), malloc_arg_type, false);
  malloc_ = llvm::Function::Create(malloc_ft, llvm::Function::ExternalLinkage,
                                   "malloc", module_.get());
}

Hobbit::Buffer *Hobbit::Module::GetIntConstant(const std::string &function_name,
                                               uint64_t *ptr,
                                               const uint8_t &int_width,
                                               const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;
  llvm::IRBuilder<> builder(entry_);

  llvm::Type *type = llvm::Type::getIntNTy(*ctx_, int_width);

  uint64_t size = shape.GetSize();

  llvm::Value *alloca_size = builder.getInt64(size);

  llvm::Value *out = builder.CreateAlloca(type, alloca_size);
  for (uint32_t i = 0; i < size; i++) {
    llvm::Value *elt = builder.getInt(llvm::APInt(int_width, ptr[i]));
    builder.CreateAlignedStore(elt, builder.CreateGEP(out, builder.getInt32(i)),
                               4);
  }

  out->setName("hobbit.compiletimebuffer.INT" + std::to_string(int_width));

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *
Hobbit::Module::GetHalfConstant(const std::string &function_name, float *ptr,
                                const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  llvm::IRBuilder<> builder(entry_);

  llvm::Type *type = llvm::Type::getHalfTy(*ctx_);

  uint64_t size = shape.GetSize();

  llvm::Value *alloca_size = builder.getInt64(size);

  llvm::Value *out = builder.CreateAlloca(type, alloca_size);
  for (uint64_t i = 0; i < size; i++) {
    llvm::Value *elt = llvm::ConstantFP::get(type, (double)ptr[i]);
    builder.CreateAlignedStore(elt, builder.CreateGEP(out, builder.getInt64(i)),
                               4);
  }

  out->setName("hobbit.compiletimebuffer.HALF");

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *
Hobbit::Module::GetFloatConstant(const std::string &function_name, float *ptr,
                                 const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  llvm::IRBuilder<> builder(entry_);

  llvm::Type *type = llvm::Type::getFloatTy(*ctx_);

  uint64_t size = shape.GetSize();

  llvm::Value *alloca_size = builder.getInt64(size);

  llvm::Value *out = builder.CreateAlloca(type, alloca_size);
  for (uint64_t i = 0; i < size; i++) {
    llvm::Value *elt = llvm::ConstantFP::get(type, (double)ptr[i]);
    builder.CreateAlignedStore(elt, builder.CreateGEP(out, builder.getInt64(i)),
                               4);
  }

  out->setName("hobbit.compiletimebuffer.FLOAT");

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *
Hobbit::Module::GetDoubleConstant(const std::string &function_name, double *ptr,
                                  const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  llvm::IRBuilder<> builder(entry_);

  llvm::Type *type = llvm::Type::getDoubleTy(*ctx_);

  uint64_t size = shape.GetSize();

  llvm::Value *alloca_size = builder.getInt64(size);

  llvm::Value *out = builder.CreateAlloca(type, alloca_size);
  for (uint64_t i = 0; i < size; i++) {
    llvm::Value *elt = llvm::ConstantFP::get(type, ptr[i]);
    builder.CreateAlignedStore(elt, builder.CreateGEP(out, builder.getInt64(i)),
                               4);
  }

  out->setName("hobbit.compiletimebuffer.DOUBLE");

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *Hobbit::Module::GetVariable(const std::string &function_name,
                                            llvm::Type *scalar_type,
                                            const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  return new Buffer(entry_, scalar_type, shape);
}

void Hobbit::Module::CreateFunction(const std::string &name,
                                    llvm::Type *return_type,
                                    llvm::ArrayRef<llvm::Type *> args_types) {

  if (function_table_.find(name) != function_table_.end())
    throw std::runtime_error("Function already exists in table!");

  internal::Function func;

  llvm::FunctionType *function_type =
      llvm::FunctionType::get(return_type, args_types, false);
  func.llvm_function = llvm::cast<llvm::Function>(
      module_->getOrInsertFunction(name, function_type));
  func.entry_block =
      llvm::BasicBlock::Create(*ctx_, name + ".entry", func.llvm_function);

  function_table_[name] = std::move(func);
}

void Hobbit::Module::PrintModule() { module_->print(llvm::outs(), nullptr); }

Hobbit::Buffer *
Hobbit::Module::InsertOperation(const std::string &function_name,
                                Hobbit::Operation *op, Hobbit::Buffer *input) {

  internal::Function &f = function_table_.at(function_name);

  llvm::BasicBlock *bb = f.AddBB(*ctx_, op->GetName());

  op->Emit(f.entry_block, bb, input);

  return input;
}

void Hobbit::Module::FinalizeFunction(const std::string &function_name,
                                      Hobbit::Buffer *return_value) {
  internal::Function &f = function_table_.at(function_name);

  llvm::BasicBlock *exit_bb = f.AddBB(*ctx_, "exit");
  llvm::IRBuilder<> builder(exit_bb);

  llvm::Type *ptr_type = llvm::PointerType::get(return_value->GetType(), 0);
  llvm::Value *ptr, *size;
  ptr = llvm::Constant::getNullValue(ptr_type);
  size = builder.CreateGEP(ptr, builder.getInt64(1));
  size = builder.CreatePtrToInt(size, llvm::Type::getInt64Ty(*ctx_));
  llvm::Value *array_size = builder.CreateMul(
      size, builder.getInt64(return_value->GetShape().GetSize()));

  llvm::Value *mallocd_array = builder.CreateCall(malloc_, array_size);
  mallocd_array = builder.CreateBitCast(
      mallocd_array, llvm::PointerType::get(return_value->GetType(), 0));

  uint64_t return_size = return_value->GetShape().GetSize();
  for (uint64_t i = 0; i < return_size; i++) {
    llvm::Value *mallocd_elt =
        builder.CreateGEP(mallocd_array, builder.getInt64(i));
    llvm::Value *buffer_elt = builder.CreateLoad(
        builder.CreateGEP(return_value->GetValue(), builder.getInt64(i)));
    builder.CreateStore(buffer_elt,
                        mallocd_elt); // is this right? Store into the gep?
  }

  builder.CreateRet(mallocd_array);
}

void Hobbit::Module::FinalizeModule(unsigned opt_level) {

  llvm::IRBuilder<> builder(*ctx_);

  for (auto &f : function_table_) {
    builder.SetInsertPoint(f.second.entry_block);
    builder.CreateBr(*f.second.bb.begin());
    for (auto bb = f.second.bb.begin(); bb != f.second.bb.end() - 1; bb++) {
      builder.SetInsertPoint(*bb);
      builder.CreateBr(*(bb + 1));
    }
    llvm::verifyFunction(*f.second.llvm_function);
  }

  llvm::verifyModule(*module_);

  llvm::legacy::PassManager PM;
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = opt_level;
  PMBuilder.DisableUnrollLoops = false;
  PMBuilder.populateModulePassManager(llvm::cast<llvm::PassManagerBase>(PM));

  PM.run(*module_);
}

Hobbit::Buffer *
Hobbit::Module::GetBufferFromInputs(const std::string &function_name,
                                    const uint32_t &ptr_idx,
                                    const Hobbit::Shape &shape) {
  internal::Function &f = function_table_.at(function_name);
  auto func_iter = f.llvm_function->arg_begin();
  uint32_t i = 0;
  llvm::Value *function_ptr_input;
  while (i <= ptr_idx) {
    function_ptr_input = &(*func_iter++);
    i++;
  }

  llvm::Type *buffer_type = function_ptr_input->getType();
  if (buffer_type->isPointerTy()) {
    buffer_type = buffer_type->getPointerElementType();
  }

  return new Buffer(function_ptr_input, buffer_type, shape, nullptr);
}

void Hobbit::Module::PrepareJIT() {
  std::string error_str;

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeNativeTarget();

  llvm::EngineBuilder engineBuilder(std::move(module_));
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::JIT);
  //  engineBuilder.setMCJITMemoryManager(llvm::make_unique<llvm::SectionMemoryManager>());
  //  engineBuilder.setMCPU("x86-64");
  engine_ = engineBuilder.create(); // why is engine_ null?
  prepare_called_ = true;
}

uint64_t Hobbit::Module::GetFunctionPtr(const std::string &name) {
  assert(prepare_called_);
  return engine_->getFunctionAddress(name);
}
