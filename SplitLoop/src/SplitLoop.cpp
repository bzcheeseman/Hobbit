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
#include <llvm/IR/Instruction.h>

namespace {

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

      if (single_block_loop) {
        return SplitSingleBlockLoop(phi, loop_increment_instr, LI, DT);
      }

      /*
       * Add function to split multi block loops
       *  - insert new BB with new phi
       *  - reset branch after initial phi
       */

      return false;
    }

  private:
    bool SplitSingleBlockLoop(PHINode *phi, BinaryOperator *loop_increment, LoopInfo &LI, DominatorTree &DT) {
      LLVMContext &ctx = phi->getContext();
      Function *parent_func = phi->getFunction();
      Type *phi_type = phi->getType();

      BasicBlock *old_phi_block = phi->getParent();
      old_phi_block->setName("hobbit.loop.entry"+std::to_string(ID));

      BasicBlock *new_inner_block = SplitBlock(old_phi_block, phi, &DT, &LI);
      new_inner_block->setName("hobbit.loop.body"+std::to_string(ID));

      BasicBlock *new_exit_block = SplitBlock(new_inner_block, loop_increment, &DT, &LI);
      new_exit_block->setName("hobbit.loop.exit"+std::to_string(ID));

      TerminatorInst *term = new_inner_block->getTerminator();
      term->removeFromParent();

      // modify loop increment to increase by chunk_size
      loop_increment->setOperand(1, ConstantInt::get(phi_type, chunk_size));

      Value *zero = ConstantInt::get(phi_type, 0);

      // Wrap the inner block with loop code
      PHINode *new_phi = PHINode::Create(phi_type, 2, "hobbit.inner.idx"+std::to_string(ID), &*new_inner_block->begin());
      new_phi->addIncoming(zero, old_phi_block);

      llvm::IRBuilder<> builder(new_inner_block);
      Value *new_idx = builder.CreateNUWAdd(phi, new_phi);
      dyn_cast<BinaryOperator>(new_idx)->moveAfter(new_phi);

      // replace uses of phi with phi + new_phi
      SmallVector<User *, 8> phi_users (phi->user_begin(), phi->user_end());
      for (auto user : phi_users) {
        GetElementPtrInst *gep_instr = dyn_cast<GetElementPtrInst>(user);
        if (!gep_instr) continue;
        gep_instr->replaceUsesOfWith(phi, new_idx);
      }

      // Increment the new phi properly
      Value *next_idx = builder.CreateAdd(new_phi, builder.CreateBitCast(builder.getInt64(1), phi_type));
      Value *exit_cmp = builder.CreateICmpULT(next_idx, builder.getInt64(chunk_size));
      builder.CreateCondBr(exit_cmp, new_inner_block, new_exit_block);
      new_phi->addIncoming(next_idx, new_inner_block);

      // TODO: inside the new exit block we *must* clean up the leftovers after looping through chunks

      return true;
    }

  private:
    uint64_t chunk_size = 32;
  };
}

char SplitLoop::ID = 0;
static RegisterPass<SplitLoop> X("split-loop", "Split Loop");
