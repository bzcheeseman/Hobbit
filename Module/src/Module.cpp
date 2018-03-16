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
    builder.CreateAlignedStore(elt, builder.CreateGEP(out, builder.getInt64(i)),
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
                                    llvm::ArrayRef<llvm::Type *> args_types,
                                    bool last_is_output) {

  if (function_table_.find(name) != function_table_.end())
    throw std::runtime_error("Function already exists in table!");
  if (last_is_output && !return_type->isVoidTy())
    throw std::runtime_error(
        "Trying to return from function and pass output pointer!");

  Function func;

  func.ctx_ = ctx_;
  func.last_is_output = last_is_output;
  llvm::FunctionType *function_type =
      llvm::FunctionType::get(return_type, args_types, false);
  func.llvm_function = llvm::cast<llvm::Function>(
      module_->getOrInsertFunction(name, function_type));
  func.entry_block =
      llvm::BasicBlock::Create(*ctx_, name + ".entry", func.llvm_function);

  function_table_[name] = std::move(func);
}

void Hobbit::Module::PrintModule(llvm::raw_ostream &out_stream) {
  module_->print(out_stream, nullptr);
}

void Hobbit::Module::PrintModule(llvm::raw_fd_ostream &out_stream) {
  llvm::WriteBitcodeToFile(module_.get(), out_stream);
  out_stream.flush();
}

Hobbit::Buffer *
Hobbit::Module::InsertOperation(const std::string &function_name,
                                Hobbit::Operation *op, Hobbit::Buffer *input,
                                bool emit_inline) {

  Buffer temp = op->Emit(&function_table_.at(function_name), input, emit_inline);
  return new Buffer(temp.GetValue(), temp.GetShape());
}

void Hobbit::Module::FinalizeFunction(const std::string &function_name,
                                      Hobbit::Buffer *return_value,
                                      Buffer *output) {
  Function &f = function_table_.at(function_name);

  llvm::BasicBlock *exit_bb = f.AddBB("exit");
  llvm::IRBuilder<> builder(exit_bb); // return_value gets invalidated!?!?!?!

  if (f.last_is_output) {
    if (output == nullptr)
      throw std::runtime_error("Nothing to store output in!");
    if (return_value->GetShape().GetSize() == 1) {
      builder.CreateStore(builder.CreateLoad(return_value->GetValue()),
                          output->GetValue());
    }

    llvm::Type *ptr_type = llvm::PointerType::get(return_value->GetType(), 0);
    llvm::Value *ptr, *size;
    ptr = llvm::Constant::getNullValue(ptr_type);
    size = builder.CreateGEP(ptr, builder.getInt64(1));
    size = builder.CreatePtrToInt(size, llvm::Type::getInt64Ty(*ctx_));
    llvm::Value *array_size = builder.CreateMul(
        size, builder.getInt64(return_value->GetShape().GetSize()));

    builder.CreateMemCpy(output->GetValue(), return_value->GetValue(),
                         array_size, 4);
    builder.CreateRetVoid();
    return;
  }

  if (return_value->GetShape().GetSize() == 1) {
    builder.CreateRet(builder.CreateLoad(return_value->GetValue()));
    return;
  }

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

  builder.CreateMemCpy(mallocd_array, return_value->GetValue(), array_size, 4);

  builder.CreateRet(mallocd_array);
}

void Hobbit::Module::FinalizeModule(unsigned opt_level) {

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::IRBuilder<> builder(*ctx_);

  for (auto &f : function_table_) {
    builder.SetInsertPoint(f.second.entry_block);
    builder.CreateBr(*f.second.bb.begin());
    for (auto bb = f.second.bb.begin(); bb != f.second.bb.end() - 1; bb++) {
      if (llvm::dyn_cast<llvm::BranchInst>(--(*bb)->end()))
        continue;
      builder.SetInsertPoint(*bb);
      builder.CreateBr(*(bb + 1));
    }
    llvm::verifyFunction(*f.second.llvm_function);
  }

  auto TargetTriple = llvm::sys::getDefaultTargetTriple();
  module_->setTargetTriple(TargetTriple);

  std::string Error;
  auto target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  // TODO: decide how to do target decisions
  auto CPU = "corei7-avx";
  auto features = "avx2";
  llvm::TargetMachine *target_machine =
      target->createTargetMachine(TargetTriple, CPU, features, options, RM);

  module_->setDataLayout(target_machine->createDataLayout());
  module_->setTargetTriple(TargetTriple);

  llvm::legacy::PassManager PM;
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = opt_level;
  PMBuilder.MergeFunctions = true;

  PMBuilder.populateModulePassManager(llvm::cast<llvm::PassManagerBase>(PM));
  target_machine->adjustPassManager(PMBuilder);

  PM.run(*module_);

  llvm::verifyModule(*module_);
}

Hobbit::Buffer *
Hobbit::Module::GetBufferFromInputs(const std::string &function_name,
                                    const uint32_t &ptr_idx,
                                    const Hobbit::Shape &shape) {
  Function &f = function_table_.at(function_name);
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

  llvm::EngineBuilder engineBuilder(std::move(module_));
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::JIT);
  engineBuilder.setMCPU("x86-64");
  engine_ = engineBuilder.create();

  prepare_called_ = true;
}

uint64_t Hobbit::Module::GetFunctionPtr(const std::string &name) {
  assert(prepare_called_);
  return engine_->getFunctionAddress(name);
}
