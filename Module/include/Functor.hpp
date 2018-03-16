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

#ifndef HOBBIT_FUNCTOR_HPP
#define HOBBIT_FUNCTOR_HPP

#include <llvm/IR/IRBuilder.h>

#include "Buffer.hpp"
#include "Function.hpp"
#include "Kernel.hpp"

namespace Hobbit {
  class Functor {
  public:
    virtual Buffer Emit(Function *f, Buffer *input, bool emit_inline) = 0;
  };

  // TODO: add data preloading

  class Dot : public Functor {
  public:
    explicit Dot(Buffer *c, const uint64_t &n_elements) : c_(c), n_elements_(n_elements) {}

    Buffer Emit(Function *f, Buffer *input, bool emit_inline) override {

      Buffer out = emit_inline ? EmitInline_(f, input) : EmitPHI_(f, input);
      return out;
    }

  private:
    Buffer EmitInline_(Function *f, Buffer *input) { // this functor is just squaring things
      if (c_->GetType() != input->GetType())
        throw std::runtime_error(
                "Both Variable and Constant must be the same type!");
      if (c_->GetShape() != input->GetShape())
        throw std::runtime_error(
                "Both Variable and Constant must be the same shape!");

      Buffer temp (f->entry_block, c_->GetType(), c_->GetShape());
      Buffer sum_vec_buf (f->entry_block, c_->GetType(), Shape(1, 1, 4));
      Buffer output (f->entry_block, c_->GetType(), Shape(1, 1, 1));

      llvm::IRBuilder<> builder(f->entry_block);

      builder.CreateStore(builder.CreateBitCast(builder.getInt64(0), c_->GetType()), output.GetValue());

      llvm::BasicBlock *BB = f->AddBB("hobbit.dot");
      builder.SetInsertPoint(BB);

      const Shape &shape = c_->GetShape();

      uint64_t total_size = shape.GetSize();
      uint64_t leftovers = total_size % n_elements_;
      uint64_t chunked = total_size - leftovers;

      internal::ElementWiseProduct prod_kernel(n_elements_);
      internal::SumReduction reduce_kernel(n_elements_);

      for (uint64_t i = 0; i < chunked; i += n_elements_) {
        prod_kernel.Emit(BB, {input, c_}, {&temp}, builder.getInt64(i));
        reduce_kernel.Emit(BB, {&temp}, {&sum_vec_buf}, builder.getInt64(i));
      }

      if (leftovers > 0) {
        internal::ElementWiseProduct prod_kernel_leftovers(leftovers);
        prod_kernel_leftovers.Emit(BB, {input, c_}, {&temp},
                                   builder.getInt64(chunked));

        internal::SumReduction sum_kernel_leftovers(leftovers);
        sum_kernel_leftovers.Emit(BB, {&temp}, {&sum_vec_buf},
                                   builder.getInt64(chunked));
      }

      llvm::Value *vector_hsum;
      llvm::Value *loaded_output;
      llvm::Type *vec_type = llvm::VectorType::get(c_->GetType(), n_elements_);
      if (c_->GetType()->isIntegerTy()) {
        vector_hsum = builder.CreateAddReduce(builder.CreateAlignedLoad(
                builder.CreateBitCast(sum_vec_buf.GetValue(), vec_type->getPointerTo()), 32));
        builder.CreateStore(vector_hsum, output.GetValue());
      }
      if (c_->GetType()->isFloatingPointTy()) {
        vector_hsum = builder.CreateFAddReduce(
                llvm::UndefValue::get(vec_type),
                builder.CreateAlignedLoad(
                        builder.CreateBitCast(sum_vec_buf.GetValue(), vec_type->getPointerTo()), 32));
        builder.CreateStore(vector_hsum, output.GetValue());
      }

      return output;
    }

    Buffer EmitPHI_(Function *f, Buffer *input) {
      if (c_->GetType() != input->GetType())
        throw std::runtime_error(
                "Both Variable and Constant must be the same type!");
      if (c_->GetShape() != input->GetShape())
        throw std::runtime_error(
                "Both Variable and Constant must be the same shape!");

      Buffer temp (f->entry_block, c_->GetType(), c_->GetShape());
      Buffer sum_vec_buf (f->entry_block, c_->GetType(), Shape(1, 1, 4));
      Buffer output (f->entry_block, c_->GetType(), Shape(1, 1, 1));

      llvm::IRBuilder<> builder(f->entry_block);

      builder.CreateStore(builder.CreateBitCast(builder.getInt64(0), c_->GetType()), output.GetValue());

      llvm::BasicBlock *entryBB = f->AddBB("hobbit.dot.entry");
      llvm::BasicBlock *loopBB = f->AddBB("hobbit.dot.loop");
      llvm::BasicBlock *exitBB = f->AddBB("hobbit.dot.exit");

      internal::ElementWiseProduct prod_kernel(n_elements_);
      internal::SumReduction reduce_kernel(n_elements_);

      builder.SetInsertPoint(entryBB);
      builder.CreateBr(loopBB);

      const Shape &shape = c_->GetShape();

      uint64_t total_size = shape.GetSize();
      uint64_t leftovers = total_size % n_elements_;
      uint64_t chunked = total_size - leftovers;

      builder.SetInsertPoint(loopBB);
      llvm::PHINode *var =
              builder.CreatePHI(builder.getInt64Ty(), 2, "hobbit.dot.index");
      var->addIncoming(builder.getInt64(0), entryBB);

      prod_kernel.Emit(loopBB, {input, c_}, {&temp}, var);
      reduce_kernel.Emit(loopBB, {&temp}, {&sum_vec_buf}, var);

      llvm::Value *nextvar =
              builder.CreateAdd(var, builder.getInt64(n_elements_));
      llvm::Value *end_condition =
              builder.CreateICmpEQ(nextvar, builder.getInt64(chunked));
      builder.CreateCondBr(end_condition, exitBB, loopBB);
      var->addIncoming(nextvar, loopBB);

      builder.SetInsertPoint(exitBB);
      if (leftovers > 0) {
        internal::ElementWiseProduct prod_kernel_leftovers(leftovers);
        prod_kernel_leftovers.Emit(exitBB, {input, c_}, {&output},
                                   builder.getInt64(chunked));

        internal::SumReduction sum_kernel_leftovers(leftovers);
        sum_kernel_leftovers.Emit(exitBB, {&temp}, {&sum_vec_buf},
                                  builder.getInt64(chunked));
      }

      llvm::Value *vector_hsum;
      llvm::Value *loaded_output;
      llvm::Type *vec_type = llvm::VectorType::get(c_->GetType(), n_elements_);
      if (c_->GetType()->isIntegerTy()) {
        vector_hsum = builder.CreateAddReduce(builder.CreateAlignedLoad(
                builder.CreateBitCast(sum_vec_buf.GetValue(), vec_type->getPointerTo()), 32));
        builder.CreateStore(vector_hsum, output.GetValue());
      }
      if (c_->GetType()->isFloatingPointTy()) {
        vector_hsum = builder.CreateFAddReduce(
                llvm::UndefValue::get(vec_type),
                builder.CreateAlignedLoad(
                        builder.CreateBitCast(sum_vec_buf.GetValue(), vec_type->getPointerTo()), 32));
        builder.CreateStore(vector_hsum, output.GetValue());
      }

      return output;
    }

    Buffer *c_;
    uint64_t n_elements_;
  };
}

#endif // HOBBIT_FUNCTOR_HPP
