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
    : ctx_(&ctx), module_(llvm::make_unique<llvm::Module>(name, ctx)) {}

Hobbit::Buffer *Hobbit::Module::GetIntConstant(const std::string &function_name, uint64_t *ptr,
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
    builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt32(i)));
  }

  out->setName("hobbit.compiletimebuffer.INT" + std::to_string(int_width));

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *Hobbit::Module::GetHalfConstant(const std::string &function_name, float *ptr,
                                                const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  llvm::IRBuilder<> builder(entry_);

  llvm::Type *type = llvm::Type::getHalfTy(*ctx_);

  uint64_t size = shape.GetSize();

  llvm::Value *alloca_size = builder.getInt64(size);

  llvm::Value *out = builder.CreateAlloca(type, alloca_size);
  for (uint32_t i = 0; i < size; i++) {
    llvm::Value *elt = llvm::ConstantFP::get(type, (double)ptr[i]);
    builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt32(i)));
  }

  out->setName("hobbit.compiletimebuffer.HALF");

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *Hobbit::Module::GetFloatConstant(const std::string &function_name, float *ptr,
                                                 const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  llvm::IRBuilder<> builder(entry_);

  llvm::Type *type = llvm::Type::getFloatTy(*ctx_);

  uint64_t size = shape.GetSize();

  llvm::Value *alloca_size = builder.getInt64(size);

  llvm::Value *out = builder.CreateAlloca(type, alloca_size);
  for (uint32_t i = 0; i < size; i++) {
    llvm::Value *elt = llvm::ConstantFP::get(type, (double)ptr[i]);
    builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt32(i)));
  }

  out->setName("hobbit.compiletimebuffer.FLOAT");

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *Hobbit::Module::GetDoubleConstant(const std::string &function_name, double *ptr,
                                                  const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  llvm::IRBuilder<> builder(entry_);

  llvm::Type *type = llvm::Type::getDoubleTy(*ctx_);

  uint64_t size = shape.GetSize();

  llvm::Value *alloca_size = builder.getInt64(size);

  llvm::Value *out = builder.CreateAlloca(type, alloca_size);
  for (uint32_t i = 0; i < size; i++) {
    llvm::Value *elt = llvm::ConstantFP::get(type, ptr[i]);
    builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt32(i)));
  }

  out->setName("hobbit.compiletimebuffer.DOUBLE");

  return new Buffer(out, type, shape, nullptr);
}

Hobbit::Buffer *Hobbit::Module::GetVariable(const std::string &function_name, llvm::Type *scalar_type,
                                            const Hobbit::Shape &shape) {
  llvm::BasicBlock *entry_ = function_table_.at(function_name).entry_block;

  return new Buffer(entry_, scalar_type, shape);
}

void Hobbit::Module::CreateFunction(const std::string &name, llvm::Type *return_type,
                                    llvm::ArrayRef<llvm::Type *> args_types) {

  if (function_table_.find(name) != function_table_.end()) throw std::runtime_error("Function already exists in table!");

  internal::Function func;

  llvm::FunctionType *function_type =
          llvm::FunctionType::get(return_type, args_types, false);
  func.llvm_function = llvm::cast<llvm::Function>(
          module_->getOrInsertFunction(name, function_type));
  func.entry_block = llvm::BasicBlock::Create(*ctx_, name+".entry", func.llvm_function);

  function_table_[name] = std::move(func);

}

void Hobbit::Module::PrintModule() {
  module_->print(llvm::outs(), nullptr);
}

Hobbit::Buffer *
Hobbit::Module::InsertOperation(const std::string &function_name, Hobbit::Operation *op, Hobbit::Buffer *input) {

  internal::Function &f = function_table_.at(function_name);

  llvm::IRBuilder<> builder (*ctx_);

  if (f.bb.empty()) {
    builder.SetInsertPoint(f.entry_block);
  }
  else {
    builder.SetInsertPoint(*(f.bb.end()-1));
  }

  llvm::BasicBlock *bb = f.AddBB(*ctx_, op->GetName());

  op->Emit(f.entry_block, bb, input);

  return input;
}

void Hobbit::Module::FinalizeFunction(const std::string &function_name, Hobbit::Buffer *return_value) {
  internal::Function &f = function_table_.at(function_name);

  llvm::BasicBlock *exit_bb = f.AddBB(*ctx_, "exit");
  llvm::IRBuilder<> builder(exit_bb);

  builder.CreateRet(return_value->GetValue());
}

void Hobbit::Module::FinalizeModule() {

  llvm::IRBuilder<> builder(*ctx_);

  for (auto &f : function_table_) {
    builder.SetInsertPoint(f.second.entry_block);
    builder.CreateBr(*f.second.bb.begin());
    for (auto bb = f.second.bb.begin(); bb != f.second.bb.end()-1; bb++) {
      builder.SetInsertPoint(*bb);
      builder.CreateBr(*(bb+1));
    }
    llvm::verifyFunction(*f.second.llvm_function);
  }

  llvm::verifyModule(*module_);

}

Hobbit::Buffer *Hobbit::Module::GetBufferFromInputs(const std::string &function_name, const uint32_t &ptr_idx,
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

  module_->setTargetTriple(llvm::sys::getDefaultTargetTriple());
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
