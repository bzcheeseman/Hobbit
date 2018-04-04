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
#include <llvm/IR/IRBuilder.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/IR/Dominators.h>


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

  class SplitLoop : public LoopPass {
  public:
    static char ID; // Pass ID, replacement for typeid
    SplitLoop() : LoopPass(ID) {}

    void getAnalysisUsage(AnalysisUsage &Info) const override {
      Info.addRequired<LoopInfoWrapperPass>();
      Info.addRequired<DominatorTreeWrapperPass>();
    }

    bool doInitialization(Loop *L, LPPassManager &LPM) override {
      // set the Value * to be the chunk increment of the loop
      return false;
    }

    bool runOnLoop(Loop *L, LPPassManager &) override {
      PHINode *phi = L->getCanonicalInductionVariable();
      if (!phi) return false;

      LLVMContext &ctx = phi->getContext();
      Function *parent_func = phi->getFunction();

      // Loop BBs
      BasicBlock *loop_phi = phi->getParent();
      BasicBlock *loop_exit = L->getLoopLatch();
      bool single_block_loop = L->isLoopLatch(loop_phi);

      // Loop information
      TerminatorInst *loop_exit_instr = dyn_cast<TerminatorInst>(&*--(loop_exit->end()));
      BinaryOperator *loop_increment_instr = dyn_cast<BinaryOperator>(
              dyn_cast<ICmpInst>(loop_exit_instr->getOperand(0))->getOperand(0)
      );
      Value *loop_range_begin = phi->getIncomingValue(0);

      // LLVM loop information
      LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

      phi->print(outs());
      loop_exit_instr->print(outs());
      loop_increment_instr->print(outs());

      outs() << "\n";

      // Add function to split single block loops
      /*
       * Add function to split multi block loops
       *  - insert new BB with new phi
       *  - reset branch after initial phi
       */

      return false;
    }



  private:
    Value *chunk_increment;
  };
}

char SplitLoop::ID = 0;
static RegisterPass<SplitLoop> X("split-loop", "Split Loop");
