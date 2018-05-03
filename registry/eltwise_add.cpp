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
#include <ast/DataStorage.hpp>

#include "utils/LoopCG.hpp"
#include "ops/Operator.hpp"

using namespace llvm;

// TODO: Make this so that we don't have to #include a .cpp

namespace Hobbit {
  class eltwise_add : public ops::Operator {
  public:
    eltwise_add(ast::Tensor *A, ast::Tensor *B) : A_(A), B_(B) {
      CHECK_EQ(A_->Type(), B_->Type());

      CHECK_EQ(A_->GetShape().NDim(), B_->GetShape().NDim());

      // Doesn't actually check if the dimensions are the same
      CHECK_EQ(A_->GetShape().Size(), B_->GetShape().Size());
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

      BasicBlock *gemm_prehead_pred;
      IRBuilder<> builder(ctx);
      if ((gemm_prehead_pred = gemm_prehead->getSinglePredecessor())) {
        builder.SetInsertPoint(gemm_prehead_pred);
        builder.CreateBr(gemm_prehead);
      }

      builder.SetInsertPoint(gemm_prehead);
      Value *zero, *one, *size;
      zero = builder.getInt64(0);
      one = builder.getInt64(1);
      size = builder.getInt64(A_->GetShape().Size());

      util::LoopInfo loopinfo_i = util::EmitLoop(
              "hobbit.eltwise_add.i", prehead, posttail, zero, size, one, false);
      util::AddLoopMetadata(loopinfo_i.cond, loopMD);

//      Value *A_idx = A_->GetShape().At({loopinfo_N.ind_var, loopinfo_K.ind_var}, body_bb);
//      Value *B_idx = B_->GetShape().At({loopinfo_K.ind_var, loopinfo_M.ind_var}, body_bb);

//      Value *A_elt = builder.CreateAlignedLoad(
//              builder.CreateInBoundsGEP(A_->Value(), A_idx), 32);
//      Value *B_elt = builder.CreateAlignedLoad(
//              builder.CreateInBoundsGEP(B_->Value(), B_idx), 32);

//      if (A_->Type()->isFloatingPointTy()) {
//        Value *tmp = builder.CreateFAdd(builder.CreateFMul(A_elt, B_elt), alpha_);
//        C_elt = builder.CreateFAdd(C_elt, tmp);
//      }
//
//      if (A_->Type()->isIntegerTy()) {
//        Value *tmp = builder.CreateMul(builder.CreateMul(A_elt, B_elt), alpha_);
//        C_elt = builder.CreateAdd(C_elt, tmp);
//      }

//      builder.CreateAlignedStore(C_elt, C_gep, 32);
//      builder.CreateBr(loopinfo_K.tail_bb);
//
      return posttail;
    }

  private:
    ast::Tensor *A_, *B_;
  };
} // namespace Hobbit