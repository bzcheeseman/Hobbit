//
// Created by Aman LaChapelle on 5/26/18.
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

#include "compile/Optimize.hpp"
#include "codegen/Module.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <polly/RegisterPasses.h>

void Hobbit::compile::Optimize::Initialize(unsigned int opt_level) {
  llvm::StringMap<llvm::cl::Option *> &Map = llvm::cl::getRegisteredOptions();

  assert(Map.count("polly-target") > 0);
  Map["polly-target"]->setValueStr("cpu");
  assert(Map.count("polly-position") > 0);
  Map["polly-position"]->setValueStr("before-vectorizer");

  llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
  polly::initializePollyPasses(Registry);

  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = opt_level;
  PMBuilder.MergeFunctions = true;
  PMBuilder.LoopVectorize = true;
  PMBuilder.DisableUnrollLoops = false;
  PMBuilder.SLPVectorize = true;

  polly::registerPollyPasses(m_pass_manager_);
  PMBuilder.populateModulePassManager(m_pass_manager_);
}

void Hobbit::compile::Optimize::Run(Hobbit::Module *m,
                                    const std::string &target_triple,
                                    const std::string &cpu,
                                    const std::string &features) {

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  LLVMInitializeNVPTXTarget();
  LLVMInitializeNVPTXTargetInfo();
  LLVMInitializeNVPTXTargetMC();
  LLVMInitializeNVPTXAsmPrinter();

//  llvm::StringMap<llvm::cl::Option *> &Map = llvm::cl::getRegisteredOptions();
//
//  assert(Map.count("polly-target") > 0);
//  Map["polly-target"]->setValueStr("gpu");
//  assert(Map.count("polly-position") > 0);
//  Map["polly-position"]->setValueStr("before-vectorizer");

//  llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
//  polly::initializePollyPasses(Registry);

  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = 3;
  PMBuilder.LoopVectorize = true;

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
      target->createTargetMachine(target_triple, cpu, features, options, RM);

  target_machine->adjustPassManager(PMBuilder);
//  polly::registerPollyPasses(m_pass_manager_);
  PMBuilder.populateModulePassManager(m_pass_manager_);  // for some reason the pass manager isn't doing the thing...works with opt -O3

  llvm::Module &module = m->SetTarget(target_triple, target_machine->createDataLayout());

  m_pass_manager_.run(module);
}
