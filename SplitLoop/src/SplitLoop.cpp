//
// Created by Aman LaChapelle on 4/1/18.
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

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <cxxabi.h>

namespace {
  inline std::string demangle(llvm::SmallString<1028> name)
  {
    int status = -1;

    size_t len = 0;

    llvm::outs() << name.c_str() << "\n";

    char *demangled = abi::__cxa_demangle(name.c_str(), nullptr, &len, &status);

    llvm::outs() << status << "\n";

    return std::string(demangled);
  }

  using namespace llvm;

  class SplitLoopPass : public LoopPass {
  public:
    static char ID; // Pass ID, replacement for typeid
    SplitLoopPass() : LoopPass(ID) {}

    bool runOnLoop(Loop *L, LPPassManager &) override {
      for (auto &BB : L->blocks()) {
        for (auto instr = BB->begin(); instr != BB->end(); instr++) {
//          errs() << instr->getName() << "\n";
          CallInst *call = dyn_cast<CallInst>(instr);
//          SmallString<1028> name;
          if (call) {
            L->print(outs());
            outs() << "\n";
            call->getArgOperand(0)->print(outs());
            outs() << "\n";
            call->print(outs());
            outs() << "\n";
//            errs() << call->getName() << "\n";
//            name.assign(call->getName());
//            name = demangle(name);
//            outs() << name << "\n";
          }
        }
      }

      return false;
    }
  };
}

char SplitLoopPass::ID = 0;
static RegisterPass<SplitLoopPass> X("split-loop", "Split Loop");
