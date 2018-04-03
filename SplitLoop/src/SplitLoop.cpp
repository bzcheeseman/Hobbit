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
#include <llvm/IR/IRBuilder.h>

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
//      Info.addRequiredID(PromoteMemoryToRegister::ID);
    }

    bool runOnLoop(Loop *L, LPPassManager &) override {
      PHINode *phi = L->getCanonicalInductionVariable();

      if (!phi) return false;

      LLVMContext &ctx = phi->getContext();
      Function *parent_func = phi->getFunction();
      BasicBlock *current_phi_block = phi->getParent();

      Value *loop_range_begin = phi->getIncomingValueForBlock(L->getLoopPredecessor());

      Value *loop_increment = phi->getIncomingValueForBlock(current_phi_block);
      loop_increment = dyn_cast<BinaryOperator>(loop_increment)->getOperand(1);
      loop_increment->print(outs());

      BranchInst *end_br = dyn_cast<BranchInst>(&*(--phi->getParent()->end()));
      Value *loop_range_end = dyn_cast<ICmpInst>(end_br->getOperand(0))->getOperand(1);

      LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      BasicBlock *inner_bb = BasicBlock::Create(ctx, current_phi_block->getName()+".inner", parent_func);

      IRBuilder<> builder(inner_bb);
      builder.CreateBr(current_phi_block);

      Loop *new_inner = LI.getLoopFor(inner_bb);

      parent_func->print(outs());

      return false;
    }
  };
}

char SplitLoop::ID = 0;
static RegisterPass<SplitLoop> X("split-loop", "Split Loop");
