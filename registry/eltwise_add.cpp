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
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <glog/logging.h>
#include <graph/DataStorage.hpp>

#include "utils/LoopCG.hpp"
#include "ops/Operator.hpp"

using namespace llvm;

namespace Hobbit {
  class eltwise_add : public ops::Operator {
  public:
    eltwise_add(graph::Tensor *A, graph::Tensor *B, graph::Tensor *C) : A_(A), B_(B), C_(C) {
      CHECK_EQ(A_->Type(), B_->Type());
      CHECK_EQ(A_->Type(), C_->Type());

      CHECK_EQ(A_->GetShape().NDim(), B_->GetShape().NDim());
      CHECK_EQ(A_->GetShape().NDim(), C_->GetShape().NDim());

      // Doesn't actually check if the dimensions are the same
      CHECK_EQ(A_->GetShape().Size(), B_->GetShape().Size());
      CHECK_EQ(A_->GetShape().Size(), C_->GetShape().Size());
    }

    OperatorType GetOperatorType() const override { return eltwiseAddID; }

    static inline bool classof(const Operator *op) {
      return op->GetOperatorType() == eltwiseAddID;
    }

    llvm::BasicBlock *InsertIntoFunction(Function *func) override {
      util::LoopMD loopMD;
      loopMD.vector_width = 4;
      loopMD.unroll_count = 32;

      LLVMContext &ctx = func->getContext();

      BasicBlock *prehead =
              BasicBlock::Create(ctx, "hobbit.eltwise_add.prehead", func);
      BasicBlock *posttail =
              BasicBlock::Create(ctx, "hobbit.eltwise_add.posttail", func);

      BasicBlock *prehead_pred;
      IRBuilder<> builder(ctx);
      if ((prehead_pred = prehead->getSinglePredecessor())) {
        builder.SetInsertPoint(prehead_pred);
        builder.CreateBr(prehead);
      }

      builder.SetInsertPoint(prehead);
      Value *zero, *one, *size;
      zero = builder.getInt64(0);
      one = builder.getInt64(1);
      size = builder.getInt64(A_->GetShape().Size());

      util::LoopInfo loopinfo_i = util::EmitLoop(
              "hobbit.eltwise_add.i", prehead, posttail, zero, size, one, false);
      util::AddLoopMetadata(loopinfo_i.cond, loopMD);

      const graph::Shape a_shape = A_->GetShape();
      graph::Shape flat = a_shape.Flatten(prehead);

      Value *idx = flat.At({loopinfo_i.ind_var}, loopinfo_i.body_bb);

      Value *C_gep = builder.CreateInBoundsGEP(C_->Value(), idx);

      Value *A_elt = builder.CreateAlignedLoad(
              builder.CreateInBoundsGEP(A_->Value(), idx), 32);
      Value *B_elt = builder.CreateAlignedLoad(
              builder.CreateInBoundsGEP(B_->Value(), idx), 32);

      Value *C_elt;
      if (A_->Type()->isFloatingPointTy()) {
        C_elt = builder.CreateFAdd(A_elt, B_elt);
      }
      if (A_->Type()->isIntegerTy()) {
        C_elt = builder.CreateAdd(A_elt, B_elt);
      }

      builder.CreateAlignedStore(C_elt, C_gep, 32);
      builder.CreateBr(posttail);

      return posttail;
    }

  private:
    graph::Tensor *A_, *B_, *C_;
  };
} // namespace Hobbit