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

#include <ops/eltwise_add.hpp>

#include <glog/logging.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include <codegen/Module.hpp>
#include <graph/Node.hpp>
#include <ops/Operator.hpp>
#include <utils/LoopCG.hpp>

using namespace llvm;

Hobbit::ops::eltwise_add::eltwise_add(codegen::Module *m,
                                      Hobbit::graph::Variable *A,
                                      Hobbit::graph::Variable *B)
    : Operator(m), A_(A), B_(B) {
  CHECK_EQ(A_->GetType(), B_->GetType());

  CHECK_EQ(A_->GetShape().NDim(), B_->GetShape().NDim());

  // Doesn't actually check if the dimensions are the same
  CHECK_EQ(A_->GetShape().Size(), B_->GetShape().Size());

  C_ = m_module_->GetVariable("hobbit.eltwise_add.output", &A_->GetShape(),
                              A_->GetType());
}

Hobbit::graph::Variable *Hobbit::ops::eltwise_add::GetOutputVariable() const {
  return C_;
}

llvm::BasicBlock *
Hobbit::ops::eltwise_add::InsertIntoFunction(llvm::Function *func,
                                             llvm::BasicBlock *previous) {
  util::LoopMD loopMD;
  loopMD.vector_width = 4;
  loopMD.unroll_count = 32;

  LLVMContext &ctx = func->getContext();
  IRBuilder<> builder(ctx);

  builder.SetInsertPoint(&func->getEntryBlock());
  llvm::Value *c_val = builder.CreateAlloca(
      C_->GetType(), builder.getInt64(A_->GetShape().Size()), C_->GetName());
  C_->SetVal(c_val);

  BasicBlock *prehead =
      BasicBlock::Create(ctx, "hobbit.eltwise_add.prehead", func);
  BasicBlock *posttail =
      BasicBlock::Create(ctx, "hobbit.eltwise_add.posttail", func);

  builder.SetInsertPoint(previous);
  builder.CreateBr(prehead);

  builder.SetInsertPoint(prehead);
  Value *zero, *one, *size;
  zero = builder.getInt64(0);
  one = builder.getInt64(1);
  size = builder.getInt64(A_->GetShape().Size());

  const graph::Shape a_shape = A_->GetShape();
  graph::Shape flat = a_shape.Flatten(prehead);

  util::LoopInfo loopinfo_i = util::EmitLoop("hobbit.eltwise_add.i", prehead,
                                             posttail, zero, size, one, true);
  util::AddLoopMetadata(loopinfo_i.cond, loopMD);

  builder.SetInsertPoint(loopinfo_i.body_bb);
  Value *idx = flat.At({loopinfo_i.ind_var}, loopinfo_i.body_bb);

  Value *C_gep = builder.CreateInBoundsGEP(C_->GetVal(), idx);

  Value *A_elt = builder.CreateAlignedLoad(
      builder.CreateInBoundsGEP(A_->GetVal(), idx), 32);
  Value *B_elt = builder.CreateAlignedLoad(
      builder.CreateInBoundsGEP(B_->GetVal(), idx), 32);

  Value *C_elt;
  if (A_->GetType()->isFloatingPointTy()) {
    C_elt = builder.CreateFAdd(A_elt, B_elt);
  }
  if (A_->GetType()->isIntegerTy()) {
    C_elt = builder.CreateAdd(A_elt, B_elt);
  }

  builder.CreateAlignedStore(C_elt, C_gep, 32);
  builder.CreateBr(loopinfo_i.tail_bb);

  return posttail;
}
