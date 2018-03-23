//
// Created by Aman LaChapelle on 3/17/18.
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

#include "Node.hpp"

Hobbit::core::Node::Node(const std::initializer_list<Symbol *> &args,
                         const std::string &node_name)
    : args_(args), name_(node_name) {}

Hobbit::core::Node::Node(std::vector<Symbol *> args,
                         const std::string &node_name)
    : args_(std::move(args)), name_(node_name) {}

Hobbit::Tensor *Hobbit::core::Alloca::GetOutput() {
  return new Tensor(args_[0]);
}

llvm::Value *Hobbit::core::Alloca::Emit(llvm::Function *func) {
  llvm::BasicBlock *BB = &func->getEntryBlock();

  llvm::IRBuilder<> builder(BB);

  llvm::Type *arg_type = args_[0]->type;
  if (arg_type->isPointerTy()) {
    arg_type = arg_type->getPointerElementType();
  }

  llvm::Type *arr_type =
      llvm::ArrayType::get(arg_type, args_[0]->shape.GetSize());

  llvm::AllocaInst *output_alloca =
      builder.CreateAlloca(arr_type, builder.getInt64(1), "hobbit.alloca");
  output_alloca->setAlignment(32);

  return output_alloca;
}

Hobbit::Tensor *Hobbit::core::Sdot::GetOutput() {
  Tensor *output_tensor =
      Variable::Create(args_[0]->parent_func, args_[0]->type, Shape(1, 1, 1));

  args_.push_back(output_tensor->GetSymbol());

  return output_tensor;
}

// TODO: emit only kernel, write pass that sets up looping
llvm::Value *Hobbit::core::Sdot::Emit(llvm::Function *func) {
  llvm::BasicBlock *entryBB =
      llvm::BasicBlock::Create(func->getContext(), "hobbit.sdot.entry", func);
  llvm::BasicBlock *loopBB =
      llvm::BasicBlock::Create(func->getContext(), "hobbit.sdot.loop", func);
  llvm::BasicBlock *exitBB =
      llvm::BasicBlock::Create(func->getContext(), "hobbit.sdot.exit", func);

  llvm::IRBuilder<> builder(entryBB);

  llvm::Type *arg_type = args_[0]->type;
  uint64_t vector_size = args_[0]->shape.GetSize();
  if (arg_type->isPointerTy()) {
    arg_type = arg_type->getPointerElementType();
  }

  llvm::Value *zero = builder.CreateBitCast(builder.getInt64(0), arg_type);
  llvm::Value *end_size = builder.getInt64(vector_size);

  llvm::Value *lhs = (llvm::Value *)args_[0]->buffer;
  llvm::Value *rhs = (llvm::Value *)args_[1]->buffer;
  llvm::Value *output = (llvm::Value *)args_[2]->buffer;

  builder.CreateBr(loopBB);

  builder.SetInsertPoint(loopBB);

  llvm::PHINode *idx_var =
      builder.CreatePHI(builder.getInt64Ty(), 2, "hobbit.sdot.idx");
  idx_var->addIncoming(builder.getInt64(0), entryBB);
  // from here is the kernel
  llvm::PHINode *accumulator =
      builder.CreatePHI(arg_type, 2, "hobbit.sdot.accumulator");
  accumulator->addIncoming(zero, entryBB);

  llvm::Value *lhs_elt, *rhs_elt;
  if (lhs->getType()->isArrayTy()) {
    lhs_elt = builder.CreateExtractElement(lhs, idx_var);
  } else {
    lhs_elt = builder.CreateAlignedLoad(builder.CreateGEP(lhs, idx_var), 32);
  }

  if (rhs->getType()->isArrayTy()) {
    rhs_elt = builder.CreateExtractElement(rhs, idx_var);
  } else {
    rhs_elt = builder.CreateAlignedLoad(builder.CreateGEP(rhs, idx_var), 32);
  }

  llvm::Value *accumulator_next;
  if (arg_type->isIntegerTy()) {
    accumulator_next =
        builder.CreateAdd(accumulator, builder.CreateMul(lhs_elt, rhs_elt));
  }
  if (arg_type->isFloatingPointTy()) {
    accumulator_next =
        builder.CreateFAdd(accumulator, builder.CreateFMul(lhs_elt, rhs_elt));
  }

  llvm::Value *next_idx_var = builder.CreateAdd(idx_var, builder.getInt64(1));

  idx_var->addIncoming(next_idx_var, loopBB);
  accumulator->addIncoming(accumulator_next, loopBB);

  llvm::SmallVector<llvm::Metadata *, 4> Args;
  // Reserve operand 0 for loop id self reference.
  auto TempNode = llvm::MDNode::getTemporary(builder.getContext(), llvm::None);
  Args.push_back(TempNode.get());

  llvm::Metadata *vecMD[] = {
      llvm::MDString::get(builder.getContext(), "llvm.loop.vectorize.width"),
      llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(builder.getContext()), 8))};
  Args.push_back(llvm::MDNode::get(builder.getContext(), vecMD));

  llvm::MDNode *LoopID = llvm::MDNode::get(builder.getContext(), Args);
  LoopID->replaceOperandWith(0, LoopID);

  llvm::Value *end_cond = builder.CreateICmpEQ(next_idx_var, end_size);
  llvm::BranchInst *br = builder.CreateCondBr(end_cond, exitBB, loopBB);
  br->setMetadata("llvm.loop", LoopID);

  builder.SetInsertPoint(exitBB);
  builder.CreateAlignedStore(accumulator_next, output, 32);

  return output;
}

Hobbit::Tensor *Hobbit::core::Sdot::Register(Hobbit::Function *f,
                                             std::vector<Tensor *> args) {
  // Adds itself to the symbol table on creation
  Tensor *output =
      Variable::Create(f, args[0]->GetType(),
                       Shape(1, 1, 1)); // TODO: get rid of the unique ptr crap
  f->AddNode(this);                     // add myself to its table
  return output;
}
