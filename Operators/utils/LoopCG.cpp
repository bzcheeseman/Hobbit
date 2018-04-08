//
// Created by Aman LaChapelle on 4/8/18.
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

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include "LoopCG.hpp"

Hobbit::util::LoopInfo
Hobbit::util::EmitLoop(const std::string &name, llvm::BasicBlock *loop_prehead,
                       llvm::BasicBlock *loop_posttail, llvm::Value *loop_start,
                       llvm::Value *loop_end, llvm::Value *loop_increment) {
  llvm::LLVMContext &ctx = loop_prehead->getContext();
  llvm::Function *parent_func = loop_prehead->getParent();

  llvm::BasicBlock *head_bb =
      llvm::BasicBlock::Create(ctx, name + ".head", parent_func);
  llvm::BasicBlock *body_bb =
      llvm::BasicBlock::Create(ctx, name + ".body", parent_func);
  llvm::BasicBlock *tail_bb =
      llvm::BasicBlock::Create(ctx, name + ".tail", parent_func);

  llvm::IRBuilder<> builder(loop_prehead);
  builder.CreateBr(head_bb);

  // Create loop head
  builder.SetInsertPoint(head_bb);
  llvm::PHINode *phi =
      builder.CreatePHI(builder.getInt64Ty(), 2, name + ".idx");
  phi->addIncoming(loop_start, loop_prehead);
  builder.CreateBr(body_bb);

  // Create loop tail
  builder.SetInsertPoint(tail_bb);
  llvm::Value *next_idx = builder.CreateAdd(phi, loop_increment);
  llvm::Value *end_cond = builder.CreateICmpULT(next_idx, loop_end);
  llvm::BranchInst *br = builder.CreateCondBr(end_cond, head_bb, loop_posttail);

  return LoopInfo{phi, head_bb, body_bb, tail_bb, br};
}

void Hobbit::util::AddLoopMetadata(llvm::BranchInst *loop_end_br,
                                   Hobbit::util::LoopMD loopMD) {
  llvm::LLVMContext &ctx = loop_end_br->getContext();

  llvm::SmallVector<llvm::Metadata *, 4> args;
  // Reserve operand 0 for loop id self reference.
  auto TempNode = llvm::MDNode::getTemporary(ctx, llvm::None);
  args.push_back(TempNode.get());

  llvm::Metadata *vecMD[] = {
      llvm::MDString::get(ctx, "llvm.loop.vectorize.width"),
      llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(ctx), loopMD.vector_width))};
  llvm::Metadata *unrollMD[] = {
      llvm::MDString::get(ctx, "llvm.loop.unroll.count"),
      llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(ctx), loopMD.unroll_count))};
  args.push_back(llvm::MDNode::get(ctx, vecMD));
  args.push_back(llvm::MDNode::get(ctx, unrollMD));

  llvm::MDNode *LoopID = llvm::MDNode::get(ctx, args);
  LoopID->replaceOperandWith(0, LoopID);

  loop_end_br->setMetadata("llvm.loop", LoopID);
}
