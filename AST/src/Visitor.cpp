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

#include <glog/logging.h>

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/Verifier.h>


Hobbit::ast::Visitor *
Hobbit::ast::Visitor::Create(llvm::LLVMContext *ctx,
                             const std::string &module_name) {
  return new Visitor(ctx, module_name);
}

llvm::LLVMContext *Hobbit::ast::Visitor::GetContext() { return ctx_; }

llvm::Module *Hobbit::ast::Visitor::GetModule() { return module_.get(); }

llvm::Function *Hobbit::ast::Visitor::GetFunction(Hobbit::ast::Function *key) {
  return func_table_.at(key);
}

void Hobbit::ast::Visitor::PushFunction(Hobbit::ast::Function *key,
                                        llvm::Function *val) {
  if (func_table_.find(key) != func_table_.end())
    LOG(FATAL) << "Function already exists in the table!";

  func_table_[key] = val;
}

Hobbit::ast::Visitor::Visitor(llvm::LLVMContext *ctx,
                              const std::string &module_name)
    : ctx_(std::move(ctx)),
      module_(llvm::make_unique<llvm::Module>(module_name, *ctx_)) {
  ;
}

void Hobbit::ast::Visitor::FinalizeFunction(Hobbit::ast::Function *key) {
  llvm::verifyFunction(*func_table_.at(key), &llvm::errs());
}

void Hobbit::ast::Visitor::Finalize(unsigned int opt_level,
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

  pm.run(*module_); // why does this segfault?

  llvm::verifyModule(*module_, &llvm::errs());
}
