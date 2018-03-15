//
// Created by Aman LaChapelle on 3/7/18.
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

#ifndef HOBBIT_REDUCTIONFUNCTORS_HPP
#define HOBBIT_REDUCTIONFUNCTORS_HPP

#include <cstdint>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "../../Buffer/include/Shape.hpp"
#include "Buffer.hpp"
#include "Functor.hpp"

namespace Hobbit {
  class SumReduction : public Functor {
  public:
    explicit SumReduction(llvm::Type *output_type, const uint64_t &n_elements)
        : output_type_(output_type), n_elements_(n_elements) {}

    inline Workspace AllocOutput(llvm::BasicBlock *BB) override {
      Workspace out;
      out.PushBuffer(new Buffer(BB, output_type_, Shape(1, 1, 1)));

      return out;
    }

    inline Buffer *Emit(Function *f, Buffer *input, Workspace &workspace,
                     bool emit_inline) override {
      emit_inline ? EmitInline_(f, input, workspace.GetBuffer(0)) : EmitPHI_(f, input, workspace.GetBuffer(0));
      return workspace.GetBuffer(0);
    }

  private:
    void EmitInline_(Function *f, Buffer *input, Buffer *output) {
      if (output_type_ != input->GetType())
        throw std::runtime_error("Input and output must be the same type!");
      if (output->GetShape() != Shape(1, 1, 1))
        throw std::runtime_error("Output shape must match specified shape!");

      llvm::BasicBlock *BB = f->AddBB("SumReduction");
      llvm::IRBuilder<> builder(BB);

      uint64_t total_size = input->GetShape().GetSize();
      uint64_t leftovers = total_size % n_elements_;
      uint64_t chunked = total_size - leftovers;

      internal::SumReduction sum_reduce_kernel(n_elements_);

      llvm::Value *input_value = input->GetValue();
      llvm::Value *output_value = output->GetValue();

      builder.CreateStore(
          builder.CreateBitCast(builder.getInt64(0), output_type_),
          output_value);

      for (uint64_t i = 0; i < chunked; i += n_elements_) {
        sum_reduce_kernel.Emit(BB, {input}, {output}, builder.getInt64(i));
      }

      if (leftovers > 0) {
        internal::SumReduction sum_reduce_kernel_leftovers(leftovers);
        sum_reduce_kernel_leftovers.Emit(BB, {input}, {output},
                                         builder.getInt64(chunked));
      }
    }

    void EmitPHI_(Function *f, Buffer *input, Buffer *output) {
      if (output_type_ != input->GetType())
        throw std::runtime_error("Input and output must be the same type!");
      if (output->GetShape() != Shape(1, 1, 1))
        throw std::runtime_error("Output shape must match specified shape!");

      llvm::BasicBlock *entryBB = f->AddBB("LargeSumRedEntry");
      llvm::BasicBlock *loopBB = f->AddBB("LargeSumReduce");
      llvm::BasicBlock *exitBB = f->AddBB("LargeSumRedExit");

      uint64_t total_size = input->GetShape().GetSize();
      uint64_t leftovers = total_size % n_elements_;
      uint64_t chunked = total_size - leftovers;

      internal::SumReduction sum_reduce_kernel(n_elements_);

      llvm::IRBuilder<> builder(entryBB);
      llvm::Value *output_value = output->GetValue();
      builder.CreateAlignedStore(
          builder.CreateBitCast(builder.getInt64(0), output_type_),
          output_value, 4);
      builder.CreateBr(loopBB);

      builder.SetInsertPoint(loopBB);
      llvm::PHINode *idx_var =
          builder.CreatePHI(builder.getInt64Ty(), 2, "LargeSumRedIndex");
      idx_var->addIncoming(builder.getInt64(0), entryBB);

      sum_reduce_kernel.Emit(loopBB, {input}, {output}, idx_var);

      llvm::Value *next_idx_var =
          builder.CreateAdd(idx_var, builder.getInt64(n_elements_));
      llvm::Value *end_condition =
          builder.CreateICmpEQ(next_idx_var, builder.getInt64(chunked));

      builder.CreateCondBr(end_condition, exitBB, loopBB);
      idx_var->addIncoming(next_idx_var, loopBB);

      builder.SetInsertPoint(exitBB);

      if (leftovers > 0) {
        internal::SumReduction sum_reduce_kernel_leftovers(leftovers);
        sum_reduce_kernel_leftovers.Emit(exitBB, {input}, {output},
                                         builder.getInt64(chunked));
      }
    }

  private:
    llvm::Type *output_type_;
    uint64_t n_elements_;
  };
}

#endif // HOBBIT_REDUCTIONFUNCTORS_HPP
