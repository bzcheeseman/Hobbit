//
// Created by Aman LaChapelle on 3/17/18.
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

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "Module.hpp"
#include "Symbol.hpp"
#include "Tensor.hpp"

llvm::LLVMContext *Hobbit::Module::GetContext() { return ctx_; }

Hobbit::Module::Module(const std::string &name, llvm::LLVMContext &ctx)
    : name_(name), ctx_(&ctx),
      module_(llvm::make_unique<llvm::Module>(name, ctx)) {}

llvm::Function *Hobbit::Module::GetFunction(const std::string &name,
                                            const std::vector<Tensor *> &args) {
  std::vector<llvm::Type *> arg_types;
  for (auto &arg : args) {
    arg_types.push_back(arg->GetSymbol()->type);
  }

  llvm::FunctionType *ft =
      llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx_), arg_types, false);
  llvm::Function *out =
      llvm::cast<llvm::Function>(module_->getOrInsertFunction(name, ft));
  llvm::BasicBlock *entryBB = llvm::BasicBlock::Create(
      *ctx_, "hobbit." + name_ + "." + name + ".entry", out);

  // Set up the args properly
  llvm::Function::arg_iterator iter = out->arg_begin();
  int idx = 0;
  while (iter != out->arg_end()) {
    args[idx]->GetSymbol()->buffer = &(*iter++);
    idx++;
  }

  return out;
}

void Hobbit::Module::Print() { module_->print(llvm::outs(), nullptr); }

void Hobbit::Module::FinalizeFunction(llvm::Function *f) {
  llvm::BasicBlock *exit_bb = llvm::BasicBlock::Create(
      *ctx_, "hobbit." + name_ + "." + f->getName() + ".exit", f);
  llvm::IRBuilder<> builder(exit_bb);

  if (f->getReturnType() == llvm::Type::getVoidTy(*ctx_))
    builder.CreateRetVoid();

  for (llvm::Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
    llvm::BasicBlock *BB = &(*bb);
    if (llvm::dyn_cast<llvm::BranchInst>(--(BB->end())))
      continue;
    builder.SetInsertPoint(BB);
    builder.CreateBr(&(*(++bb)));
  }
  llvm::verifyFunction(*f);
}

void Hobbit::Module::FinalizeModule(unsigned int opt_level,
                                    const std::string &target_triple,
                                    const std::string &cpu,
                                    const std::string &features) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  module_->setTargetTriple(target_triple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
      target->createTargetMachine(target_triple, cpu, features, options, RM);

  module_->setDataLayout(target_machine->createDataLayout());
  module_->setTargetTriple(target_triple);

  llvm::legacy::PassManager PM;
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = opt_level;
  PMBuilder.MergeFunctions = true;
  PMBuilder.LoopVectorize = true;
  PMBuilder.SLPVectorize = true;

  PMBuilder.populateModulePassManager(llvm::cast<llvm::PassManagerBase>(PM));
  target_machine->adjustPassManager(PMBuilder);

  PM.run(*module_);

  llvm::verifyModule(*module_);
}

void *Hobbit::Module::GetFunctionPtr(const std::string &name) {
  std::string error_str;

  llvm::EngineBuilder engineBuilder(std::move(module_));
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::JIT);
  engineBuilder.setMCPU("x86-64");
  llvm::ExecutionEngine *engine = engineBuilder.create();

  return (void *)engine->getFunctionAddress(name);
}
