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
#include <polly/RegisterPasses.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

void Hobbit::compile::Optimize::Initialize(unsigned int opt_level) {
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = opt_level;
  PMBuilder.MergeFunctions = true;
  PMBuilder.LoopVectorize = true;
  PMBuilder.DisableUnrollLoops = false;
  PMBuilder.SLPVectorize = true;

  polly::registerPollyPasses(m_pass_manager_);
  PMBuilder.populateModulePassManager(m_pass_manager_);
}

void Hobbit::compile::Optimize::Run(Hobbit::Module *m, const std::string &target_triple, const std::string &cpu,
                                    const std::string &features) {
  m_pass_manager_.run(m->Finalize(target_triple, cpu, features));
}
