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

#include "LoopCG.hpp"
#include "Node.hpp"
#include "Operator.hpp"

using namespace llvm;

namespace Hobbit {
  class gemm : public Operator {
  public:
    gemm(uint64_t N, uint64_t M, uint64_t K, ast::Tensor *A, ast::Tensor *B,
         ast::Tensor *C, Value *alpha, Value *beta)
        : N_(N), M_(M), K_(K), A_(A), B_(B), C_(C), alpha_(alpha), beta_(beta) {
      CHECK_EQ(A_->GetType(), B_->GetType());
      CHECK_EQ(A_->GetType(), C_->GetType());
      CHECK_EQ(alpha_->getType(), A_->GetType());
      CHECK_EQ(beta_->getType(), A_->GetType());
    }

    void InsertIntoFunction(Function *func) override {
      util::LoopMD loopMD;
      loopMD.vector_width = 4;
      loopMD.unroll_count = 32;

      LLVMContext &ctx = func->getContext();

      BasicBlock *gemm_prehead =
          BasicBlock::Create(ctx, "hobbit.gemm.prehead", func);
      BasicBlock *gemm_posttail =
          BasicBlock::Create(ctx, "hobbit.gemm.posttail", func);

      IRBuilder<> builder(gemm_prehead);
      Value *zero, *one, *K, *N, *M;
      zero = builder.getInt64(0);
      one = builder.getInt64(1);
      N = builder.getInt64(N_);
      M = builder.getInt64(M_);
      K = builder.getInt64(K_);

      util::LoopInfo loopinfo_N = util::EmitLoop("hobbit.gemm.N", gemm_prehead,
                                                 gemm_posttail, zero, N, one);
      loopinfo_N.body_bb->removeFromParent();
      util::AddLoopMetadata(loopinfo_N.cond, loopMD);

      util::LoopInfo loopinfo_M =
          util::EmitLoop("hobbit.gemm.M", loopinfo_N.head_bb,
                         loopinfo_N.tail_bb, zero, M, one);
      util::AddLoopMetadata(loopinfo_M.cond, loopMD);

      Value *C_idx =
          C_->At({loopinfo_N.ind_var, loopinfo_M.ind_var}, loopinfo_M.body_bb);
      Value *C_gep = builder.CreateInBoundsGEP(C_->GetValue(), C_idx);

      Value *C_elt;
      if (C_->GetType()->isFloatingPointTy()) {
        Value *C_elt =
            builder.CreateFMul(beta_, builder.CreateAlignedLoad(C_gep, 32));
      }
      if (C_->GetType()->isIntegerTy()) {
        C_elt = builder.CreateMul(beta_, builder.CreateAlignedLoad(C_gep, 32));
      }

      util::LoopInfo loopinfo_K =
          util::EmitLoop("hobbit.gemm.K", loopinfo_M.body_bb,
                         loopinfo_M.tail_bb, zero, K, one);
      util::AddLoopMetadata(loopinfo_K.cond, loopMD);

      BasicBlock *body_bb = loopinfo_K.body_bb;
      builder.SetInsertPoint(body_bb);

      Value *A_idx = A_->At({loopinfo_N.ind_var, loopinfo_K.ind_var}, body_bb);
      Value *B_idx = B_->At({loopinfo_K.ind_var, loopinfo_M.ind_var}, body_bb);

      Value *A_elt = builder.CreateAlignedLoad(
          builder.CreateInBoundsGEP(A_->GetValue(), A_idx), 32);
      Value *B_elt = builder.CreateAlignedLoad(
          builder.CreateInBoundsGEP(B_->GetValue(), B_idx), 32);

      if (A_->GetType()->isFloatingPointTy()) {
        Value *tmp =
            builder.CreateFMul(builder.CreateFMul(A_elt, B_elt), alpha_);
        C_elt = builder.CreateFAdd(C_elt, tmp);
      }

      if (A_->GetType()->isIntegerTy()) {
        Value *tmp = builder.CreateMul(A_elt, B_elt);
        C_elt = builder.CreateAdd(C_elt, tmp);
      }

      builder.CreateAlignedStore(C_elt, C_gep, 32);
      builder.CreateBr(loopinfo_K.tail_bb);
    }

  private:
    uint64_t N_, M_, K_;
    ast::Tensor *A_, *B_, *C_;
    Value *alpha_, *beta_;
  };
} // namespace Hobbit
