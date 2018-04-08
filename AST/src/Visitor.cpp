//
// Created by Aman LaChapelle on 3/24/18.
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

#include "Visitor.hpp"
#include "Node.hpp"

#include <glog/logging.h>

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

Hobbit::Function *Hobbit::Function::Create(const std::string &name) {
  Function *f = new Function;
  f->name_ = "hobbit." + name;
  return f;
}

Hobbit::ast::Tensor *
Hobbit::Function::GetNewArg(const std::string &name,
                            llvm::SmallVector<uint64_t, 4> dims,
                            llvm::Type *type) {
  // Create a new ast::Tensor
  ast::Tensor *arg = ast::Tensor::Create(name, nullptr, std::move(dims), type);
  // Add the tensor to the arg table
  arg_table_.push_back(arg);
  return arg;
}

Hobbit::ast::Tensor *
Hobbit::Function::GetNewAlloca(const std::string &name,
                               llvm::SmallVector<uint64_t, 4> dims,
                               llvm::Type *type) {
  // Create a new ast::Tensor
  ast::Tensor *arg = ast::Tensor::Create(name, nullptr, std::move(dims), type);
  // Add the tensor to the arg table
  alloca_table_.push_back(arg);
  return arg;
}

void Hobbit::Function::SetArg(Hobbit::ast::Tensor *t) {
  // If we've already added this arg to the alloca table then erase it - it
  // should be an arg
  for (auto &alloca : alloca_table_) {
    if (t == alloca)
      alloca_table_.erase(&alloca);
  }
  arg_table_.push_back(t);
}

void Hobbit::Function::PushNode(Hobbit::ast::Node *node) {
  if (child_ == nullptr) {
    child_ = std::move(node);
    last_node_ = child_;
    return;
  }
}

Hobbit::Visitor *Hobbit::Visitor::Create(llvm::LLVMContext *ctx,
                                         const std::string &module_name) {
  return new Visitor(ctx, module_name);
}

llvm::LLVMContext *Hobbit::Visitor::GetContext() { return ctx_; }

llvm::Module *Hobbit::Visitor::GetModule() { return module_.get(); }

llvm::Function *Hobbit::Visitor::GetFunction(Hobbit::Function *key) {
  return func_table_.at(key);
}

void Hobbit::Visitor::PushFunction(Hobbit::Function *key, llvm::Function *val) {
  if (func_table_.find(key) != func_table_.end())
    LOG(FATAL) << "Function already exists in the table!";

  func_table_[key] = val;
}

Hobbit::Visitor::Visitor(llvm::LLVMContext *ctx, const std::string &module_name)
    : ctx_(std::move(ctx)),
      module_(llvm::make_unique<llvm::Module>(module_name, *ctx_)) {
  ;
}

void Hobbit::Visitor::FinalizeFunction(Hobbit::Function *key) {
  llvm::verifyFunction(*func_table_.at(key), &llvm::errs());
}

void Hobbit::Visitor::Finalize(unsigned int opt_level,
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

  llvm::verifyModule(*module_, &llvm::errs());

  llvm::legacy::PassManager pm;
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = opt_level;

  PMBuilder.populateModulePassManager(pm);
  target_machine->adjustPassManager(PMBuilder);

  pm.run(*module_);

  llvm::verifyModule(*module_, &llvm::errs());
}
