//
// Created by Aman LaChapelle on 2/22/18.
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

#ifndef HOBBIT_ELEMENTWISEFUNCTORS_HPP
#define HOBBIT_ELEMENTWISEFUNCTORS_HPP

#include <cstdint>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "Buffer.hpp"
#include "Functor.hpp"
#include "Kernel.hpp"

namespace Hobbit {
  class ElementWiseProduct : public Functor {
  public:
    explicit ElementWiseProduct(Buffer *c, const uint64_t &n_elements)
        : c_(c), n_elements_(n_elements) {}

    inline Buffer Emit(Function *f, Buffer *input, bool emit_inline) override {
      Buffer out = Buffer(f->entry_block, c_->GetType(), c_->GetShape());
      emit_inline ? EmitInline_(f, input, &out) : EmitPHI_(f, input, &out);
      return out;
    }

  private:
    void EmitInline_(Function *f, Buffer *input, Buffer *output) {
      if (c_->GetType() != input->GetType())
        throw std::runtime_error(
            "Both Variable and Constant must be the same type!");
      if (c_->GetShape() != input->GetShape())
        throw std::runtime_error(
            "Both Variable and Constant must be the same shape!");
      if (output->GetType() != input->GetType())
        throw std::runtime_error(
            "Both input and output must be the same type!");
      if (output->GetShape() != input->GetShape())
        throw std::runtime_error(
            "Both input and output must be the same shape!");

      llvm::BasicBlock *BB = f->AddBB("EWiseProduct");
      llvm::IRBuilder<> builder(BB);

      const Shape &shape = c_->GetShape();

      uint64_t total_size = shape.GetSize();
      uint64_t leftovers = total_size % n_elements_;
      uint64_t chunked = total_size - leftovers;

      internal::ElementWiseProduct prod_kernel(n_elements_);

      for (uint64_t i = 0; i < chunked; i += n_elements_) {
        prod_kernel.Emit(BB, {input, c_}, {output}, builder.getInt64(i));
      }

      if (leftovers > 0) {
        internal::ElementWiseProduct prod_kernel_leftovers(leftovers);
        prod_kernel_leftovers.Emit(BB, {input, c_}, {output},
                                   builder.getInt64(chunked));
      }
    }

    void EmitPHI_(Function *f, Buffer *input, Buffer *output) {
      if (c_->GetType() != input->GetType())
        throw std::runtime_error(
            "Both Variable and Constant must be the same type!");
      if (c_->GetShape() != input->GetShape())
        throw std::runtime_error(
            "Both Variable and Constant must be the same shape!");
      if (output->GetType() != input->GetType())
        throw std::runtime_error(
            "Both input and output must be the same type!");
      if (output->GetShape() != input->GetShape())
        throw std::runtime_error(
            "Both input and output must be the same shape!");

      llvm::BasicBlock *entryBB = f->AddBB("LargeEwiseEntry");
      llvm::BasicBlock *loopBB = f->AddBB("LargeEwiseProduct");
      llvm::BasicBlock *exitBB = f->AddBB("LargeEwiseExit");

      uint64_t n_elements = 4;

      internal::ElementWiseProduct prod_kernel(n_elements);

      llvm::IRBuilder<> builder(entryBB);
      builder.CreateBr(loopBB);

      const Shape &shape = c_->GetShape();

      uint64_t total_size = shape.GetSize();
      uint64_t leftovers = total_size % n_elements_;
      uint64_t chunked = total_size - leftovers;

      builder.SetInsertPoint(loopBB);
      llvm::PHINode *var =
          builder.CreatePHI(builder.getInt64Ty(), 2, "LargeEwiseProductIndex");
      var->addIncoming(builder.getInt64(0), entryBB);

      prod_kernel.Emit(loopBB, {input, c_}, {output}, var);

      llvm::Value *nextvar =
          builder.CreateAdd(var, builder.getInt64(n_elements));
      llvm::Value *end_condition =
          builder.CreateICmpEQ(nextvar, builder.getInt64(chunked));
      builder.CreateCondBr(end_condition, exitBB, loopBB);
      var->addIncoming(nextvar, loopBB);

      builder.SetInsertPoint(exitBB);
      if (leftovers > 0) {
        internal::ElementWiseProduct prod_kernel_leftovers(leftovers);
        prod_kernel_leftovers.Emit(exitBB, {input, c_}, {output},
                                   builder.getInt64(chunked));
      }
    }

  private:
    Buffer *c_;
    uint64_t n_elements_;
  };
}

#endif // HOBBIT_ELEMENTWISEFUNCTORS_HPP
