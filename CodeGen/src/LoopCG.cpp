//
// Created by Aman LaChapelle on 3/31/18.
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


#include "LoopCG.hpp"

#include "Node.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

Hobbit::LoopCG::LoopCG(Hobbit::Index *idx, const Hobbit::LoopMD &loop_md)
        : head_called_(false), tail_called_(false), loopmd_(loop_md)
{
  idx_ = idx;
  while (idx->parent_ != nullptr) {
    idx_ = idx->parent_;
  } // want to store the parent-est Index *
}

llvm::BasicBlock *Hobbit::LoopCG::EmitHead(const std::string &name, llvm::BasicBlock *phi_bb) {
  llvm::LLVMContext &ctx = phi_bb->getContext();
  llvm::Function *parent_function = phi_bb->getParent();

  llvm::BasicBlock *working_bb = phi_bb;
  llvm::IRBuilder<> builder(working_bb);

  int iter = 1;
  do {
    idx_->phi_ = builder.CreatePHI(builder.getInt64Ty(), 2, name);
    idx_->phi_->addIncoming(builder.getInt64(idx_->start_), working_bb->getSinglePredecessor());

    working_bb = llvm::BasicBlock::Create(ctx, name+"."+std::to_string(iter), parent_function);

    builder.CreateBr(working_bb);

    idx_ = idx_->child_;
    iter++;
  } while (idx_->child_ != nullptr);

  head_called_ = true;
  return working_bb;
}

llvm::Value *Hobbit::LoopCG::GetIDX(llvm::BasicBlock *BB) {
  if (!head_called_) LOG(FATAL) << "Must call EmitHead before attempting to get an IDX";
  llvm::IRBuilder<> builder(BB);

  llvm::Value *idx = idx_->phi_;
  Index *parent = idx_->parent_;
  while (parent != nullptr) {
    idx = builder.CreateAdd(idx, parent->phi_);
    parent = parent->parent_;
  }

  return idx;
}

llvm::BasicBlock *Hobbit::LoopCG::EmitTail(llvm::BasicBlock *exit_bb) {

  if (!head_called_) LOG(FATAL) << "Must call EmitHead before attempting to close the loop";

  llvm::LLVMContext &ctx = exit_bb->getContext();
  llvm::Function *parent_function = exit_bb->getParent();

  llvm::BasicBlock *working_bb = exit_bb;
  llvm::IRBuilder<> builder(working_bb);

  int iter = 1;
  do {
    llvm::Value *increment = builder.getInt64(idx_->stride_);
    llvm::Value *next_idx_var = builder.CreateAdd(idx_->phi_, increment);
    llvm::Value *end_cond =
            builder.CreateICmpEQ(next_idx_var, builder.getInt64(idx_->end_));

    working_bb = llvm::BasicBlock::Create(ctx, working_bb->getName()+"."+std::to_string(iter), parent_function);

    llvm::BranchInst *br = builder.CreateCondBr(end_cond, working_bb, idx_->phi_->getParent());
    idx_->phi_->addIncoming(next_idx_var, exit_bb);
    this->AddLoopMetadata(br);

    idx_ = idx_->child_;
    iter++;
  } while (idx_->child_ != nullptr);

  tail_called_ = true;
  return working_bb;
}

void Hobbit::LoopCG::AddLoopMetadata(llvm::BranchInst *loop_end_br) {
  llvm::LLVMContext &ctx = loop_end_br->getContext();

  llvm::SmallVector<llvm::Metadata *, 4> args;
  // Reserve operand 0 for loop id self reference.
  auto TempNode = llvm::MDNode::getTemporary(ctx, llvm::None);
  args.push_back(TempNode.get());

  llvm::Metadata *vecMD[] = {
          llvm::MDString::get(ctx, "llvm.loop.vectorize.width"),
          llvm::ConstantAsMetadata::get(
                  llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), loopmd_.vector_width))};
  llvm::Metadata *unrollMD[] = {
          llvm::MDString::get(ctx, "llvm.loop.unroll.count"),
          llvm::ConstantAsMetadata::get(
                  llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), loopmd_.unroll_count))};
  args.push_back(llvm::MDNode::get(ctx, vecMD));
  args.push_back(llvm::MDNode::get(ctx, unrollMD));

  llvm::MDNode *LoopID = llvm::MDNode::get(ctx, args);
  LoopID->replaceOperandWith(0, LoopID);

  loop_end_br->setMetadata("llvm.loop", LoopID);
}
