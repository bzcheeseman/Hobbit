//
// Created by Aman LaChapelle on 5/19/18.
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

#include <compile/JIT.hpp>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

void Hobbit::compile::JIT::InitializeLLVM() {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
}

const llvm::Target *
Hobbit::compile::JIT::InitTarget_(const std::string &target_triple) {
  std::string error;
  return llvm::TargetRegistry::lookupTarget(target_triple, error);
}

void Hobbit::compile::JIT::BuildEngine(const std::string &target_triple,
                                       const std::string &cpu,
                                       const std::string &features) {
  const llvm::Target *target = InitTarget_(target_triple);

  llvm::TargetOptions opt;
  opt.AllowFPOpFusion = llvm::FPOpFusion::Fast;
  auto RM = llvm::Optional<llvm::Reloc::Model>();
  m_target_machine_ =
      target->createTargetMachine(target_triple, cpu, features, opt, RM);

  llvm::EngineBuilder engine_builder;
  m_engine_ = engine_builder.create(m_target_machine_);
}
