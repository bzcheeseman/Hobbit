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
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include <codegen/Module.hpp>
#include <graph/Variable.hpp>
#include <ops/Operator.hpp>
#include <utils/LoopCG.hpp>

using namespace llvm;

Hobbit::ops::gemm::gemm(Hobbit::Module *module, Hobbit::graph::Variable *A,
                        Hobbit::graph::Variable *B,
                        Hobbit::graph::Variable *alpha,
                        Hobbit::graph::Variable *beta)
    : Operator(module), N_(A->GetShape().Dim((uint64_t)0)),
      M_(B->GetShape().Dim(1)), K_(A->GetShape().Dim(1)), A_(A), B_(B),
      alpha_(alpha), beta_(beta) {
  CHECK_EQ(A_->GetType(), B_->GetType());
  CHECK_EQ(A_->GetType(), alpha_->GetType());
  CHECK_EQ(A_->GetType(), beta_->GetType());

  CHECK_EQ(A_->GetShape().NDim(), 2);
  CHECK_EQ(B_->GetShape().NDim(), 2);

  CHECK_EQ(alpha_->GetShape().Size(), 1); // todo: remove alpha/beta?
  CHECK_EQ(beta_->GetShape().Size(), 1);

  CHECK_EQ(B_->GetShape().Dim((uint64_t)0), K_);

  C_ = m_module_->GetVariable("hobbit.gemm.output", {N_, M_}, A_->GetType());
}

Hobbit::graph::Variable *Hobbit::ops::gemm::GetOutputVariable() const {
  return C_;
}

llvm::BasicBlock *
Hobbit::ops::gemm::InsertIntoFunction(llvm::Function *func,
                                      llvm::BasicBlock *previous) {

  LLVMContext &ctx = func->getContext();
  IRBuilder<> builder(ctx);

  if (!C_->GetVal()) {
    builder.SetInsertPoint(&func->getEntryBlock());
    llvm::Value *c_val = builder.CreateAlloca(
        C_->GetType(), builder.getInt64(N_ * M_), C_->GetName());
    C_->SetVal(c_val);
  }

  BasicBlock *gemm_prehead =
      BasicBlock::Create(ctx, "hobbit.gemm.prehead", func);
  BasicBlock *gemm_posttail =
      BasicBlock::Create(ctx, "hobbit.gemm.posttail", func);

  builder.SetInsertPoint(previous);
  builder.CreateBr(gemm_prehead);

  builder.SetInsertPoint(gemm_prehead);

  Value *zero, *one, *K, *N, *M;
  zero = builder.getInt64(0);
  one = builder.getInt64(1);
  N = builder.getInt64(N_);
  M = builder.getInt64(M_);
  K = builder.getInt64(K_);

  util::LoopMD k_md = {4, 32};
  util::LoopInfo loopinfo_K = util::EmitLoop(
      "hobbit.gemm.K", gemm_prehead, gemm_posttail, zero, K, one, false);
  util::AddLoopMetadata(loopinfo_K.cond, k_md);

  util::LoopMD n_md = {4, 32};
  util::LoopInfo loopinfo_N =
      util::EmitLoop("hobbit.gemm.N", loopinfo_K.head_bb, loopinfo_K.tail_bb,
                     zero, N, one, false);
  util::AddLoopMetadata(loopinfo_N.cond, n_md);

  util::LoopMD m_md = {4, 32};
  util::LoopInfo loopinfo_M = util::EmitLoop(
      "hobbit.gemm.M", loopinfo_N.head_bb, loopinfo_N.tail_bb, zero, M, one);
  util::AddLoopMetadata(loopinfo_M.cond, m_md);

  BasicBlock *body_bb = loopinfo_M.body_bb;
  builder.SetInsertPoint(body_bb);

  Value *A_idx =
      A_->GetShape().At({loopinfo_N.ind_var, loopinfo_K.ind_var}, body_bb);
  Value *B_idx =
      B_->GetShape().At({loopinfo_K.ind_var, loopinfo_M.ind_var}, body_bb);
  Value *C_idx =
      C_->GetShape().At({loopinfo_N.ind_var, loopinfo_M.ind_var}, body_bb);

  Value *A_elt = builder.CreateAlignedLoad(
      builder.CreateInBoundsGEP(A_->GetVal(), A_idx), 32);
  Value *B_elt = builder.CreateAlignedLoad(
      builder.CreateInBoundsGEP(B_->GetVal(), B_idx), 32);
  Value *C_elt = builder.CreateAlignedLoad(
      builder.CreateInBoundsGEP(C_->GetVal(), C_idx), 32);

  if (A_->GetType()->isFloatingPointTy()) {
    Value *tmp =
        builder.CreateFMul(builder.CreateFMul(A_elt, B_elt), alpha_->GetVal());
    C_elt = builder.CreateFAdd(C_elt, tmp);
  }

  if (A_->GetType()->isIntegerTy()) {
    Value *tmp =
        builder.CreateMul(builder.CreateMul(A_elt, B_elt), alpha_->GetVal());
    C_elt = builder.CreateAdd(C_elt, tmp);
  }

  builder.CreateAlignedStore(C_elt, builder.CreateInBoundsGEP(C_->GetVal(), C_idx), 32);
  builder.CreateBr(loopinfo_M.tail_bb);

  return gemm_posttail;
}
