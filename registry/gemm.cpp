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

#include <ops/gemm.hpp>

#include <glog/logging.h>
#include <graph/DataStorage.hpp>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include <codegen/Module.hpp>
#include <graph/Node.hpp>
#include <ops/Operator.hpp>
#include <utils/LoopCG.hpp>

using namespace llvm;

Hobbit::ops::gemm::gemm(Hobbit::codegen::Module *module, uint64_t N, uint64_t M,
                        uint64_t K, Hobbit::graph::Variable *A,
                        Hobbit::graph::Variable *B, Value *alpha, Value *beta)
    : Operator(module), N_(N), M_(M), K_(K), A_(A), B_(B), alpha_(alpha),
      beta_(beta) {
  CHECK_EQ(A_->GetType(), B_->GetType());

  CHECK_EQ(alpha_->getType(), A_->GetType());
  CHECK_EQ(beta_->getType(), A_->GetType());

  CHECK_EQ(A_->GetShape().NDim(), 2);
  CHECK_EQ(B_->GetShape().NDim(), 2);

  CHECK_EQ(A_->GetShape().Dim((uint64_t)0), N_);
  CHECK_EQ(A_->GetShape().Dim(1), K_);

  CHECK_EQ(B_->GetShape().Dim((uint64_t)0), K_);
  CHECK_EQ(B_->GetShape().Dim(1), M_);

  *C_ = m_module_->GetVariable("gemm.output", {N_, M_}, A_->GetType());
}

Hobbit::graph::Variable *Hobbit::ops::gemm::GetOutputVariable() const {
  return C_;
}

llvm::BasicBlock *Hobbit::ops::gemm::InsertIntoFunction(Function *func) {
  util::LoopMD loopMD;
  loopMD.vector_width = 4;
  loopMD.unroll_count = 32;

  LLVMContext &ctx = func->getContext();

  BasicBlock *gemm_prehead =
      BasicBlock::Create(ctx, "hobbit.gemm.prehead", func);
  BasicBlock *gemm_posttail =
      BasicBlock::Create(ctx, "hobbit.gemm.posttail", func);

  BasicBlock *gemm_prehead_pred;
  IRBuilder<> builder(ctx);
  if ((gemm_prehead_pred = gemm_prehead->getSinglePredecessor())) {
    builder.SetInsertPoint(gemm_prehead_pred);
    builder.CreateBr(gemm_prehead);
  }

  builder.SetInsertPoint(gemm_prehead);

  Value *zero, *one, *K, *N, *M;
  zero = builder.getInt64(0);
  one = builder.getInt64(1);
  N = builder.getInt64(N_);
  M = builder.getInt64(M_);
  K = builder.getInt64(K_);

  util::LoopInfo loopinfo_N = util::EmitLoop(
      "hobbit.gemm.N", gemm_prehead, gemm_posttail, zero, N, one, false);
  util::AddLoopMetadata(loopinfo_N.cond, loopMD);

  util::LoopInfo loopinfo_M = util::EmitLoop(
      "hobbit.gemm.M", loopinfo_N.head_bb, loopinfo_N.tail_bb, zero, M, one);
  util::AddLoopMetadata(loopinfo_M.cond, loopMD);

  builder.SetInsertPoint(loopinfo_M.body_bb);
  Value *C_idx = C_->GetShape().At({loopinfo_N.ind_var, loopinfo_M.ind_var},
                                   loopinfo_M.body_bb);
  Value *C_gep = builder.CreateInBoundsGEP(C_->GetVal(), C_idx);

  Value *C_elt = builder.CreateAlignedLoad(C_gep, 32);
  if (C_->GetType()->isFloatingPointTy()) {
    C_elt = builder.CreateFMul(beta_, C_elt);
  }
  if (C_->GetType()->isIntegerTy()) {
    C_elt = builder.CreateMul(beta_, C_elt);
  }

  util::LoopInfo loopinfo_K = util::EmitLoop(
      "hobbit.gemm.K", loopinfo_M.body_bb, loopinfo_M.tail_bb, zero, K, one);
  util::AddLoopMetadata(loopinfo_K.cond, loopMD);

  BasicBlock *body_bb = loopinfo_K.body_bb;
  builder.SetInsertPoint(body_bb);

  Value *A_idx =
      A_->GetShape().At({loopinfo_N.ind_var, loopinfo_K.ind_var}, body_bb);
  Value *B_idx =
      B_->GetShape().At({loopinfo_K.ind_var, loopinfo_M.ind_var}, body_bb);

  Value *A_elt = builder.CreateAlignedLoad(
      builder.CreateInBoundsGEP(A_->GetVal(), A_idx), 32);
  Value *B_elt = builder.CreateAlignedLoad(
      builder.CreateInBoundsGEP(B_->GetVal(), B_idx), 32);

  if (A_->GetType()->isFloatingPointTy()) {
    Value *tmp = builder.CreateFMul(builder.CreateFMul(A_elt, B_elt), alpha_);
    C_elt = builder.CreateFAdd(C_elt, tmp);
  }

  if (A_->GetType()->isIntegerTy()) {
    Value *tmp = builder.CreateMul(builder.CreateMul(A_elt, B_elt), alpha_);
    C_elt = builder.CreateAdd(C_elt, tmp);
  }

  builder.CreateAlignedStore(C_elt, C_gep, 32);
  builder.CreateBr(loopinfo_K.tail_bb);

  return gemm_posttail;
}
