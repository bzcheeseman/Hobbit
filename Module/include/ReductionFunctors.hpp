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
    explicit SumReduction(llvm::Type *output_type)
        : output_type_(output_type) {}

    inline Buffer AllocOutput(llvm::BasicBlock *BB) override {
      return Buffer(BB, output_type_, Shape(1, 1, 1));
    }

    inline void Emit(Function *f, Buffer *input, Buffer *output,
                     bool emit_inline) override {
      emit_inline ? EmitInline_(f, input, output) : EmitPHI_(f, input, output);
    }

  private:
    void EmitInline_(Function *f, Buffer *input, Buffer *output) {
      if (output_type_ != input->GetType())
        throw std::runtime_error("Input and output must be the same type!");
      if (output->GetShape() != Shape(1, 1, 1))
        throw std::runtime_error("Output shape must match specified shape!");

      llvm::BasicBlock *BB = f->AddBB("SumReduction");
      llvm::IRBuilder<> builder(BB);
      internal::SumReduction sum_reduce_kernel;

      llvm::Value *input_value = input->GetValue();
      llvm::Value *output_value = output->GetValue();

      std::vector<Buffer *> inputs = {input};
      std::vector<Buffer *> outputs = {output};

      builder.CreateStore(
          builder.CreateBitCast(builder.getInt64(0), output_type_),
          output_value);

      for (uint64_t i = 0; i < input->GetShape().GetSize(); i++) {
        sum_reduce_kernel.Emit(BB, inputs, outputs, builder.getInt64(i));
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

      internal::SumReduction sum_reduce_kernel;

      llvm::IRBuilder<> builder(entryBB);
      llvm::Value *output_value = output->GetValue();
      builder.CreateAlignedStore(builder.CreateBitCast(builder.getInt64(0), output_type_), output_value, 4);
      builder.CreateBr(loopBB);

      std::vector<Buffer *> inputs = {input};
      std::vector<Buffer *> outputs = {output};

      builder.SetInsertPoint(loopBB);
      llvm::PHINode *idx_var =
          builder.CreatePHI(builder.getInt64Ty(), 2, "LargeSumRedIndex");
      idx_var->addIncoming(builder.getInt64(0), entryBB);

      sum_reduce_kernel.Emit(loopBB, inputs, outputs, idx_var);

      llvm::Value *next_idx_var =
          builder.CreateAdd(idx_var, builder.getInt64(1));
      llvm::Value *end_condition = builder.CreateICmpEQ(
          next_idx_var, builder.getInt64(input->GetShape().GetSize()));

      builder.CreateCondBr(end_condition, exitBB, loopBB);
      idx_var->addIncoming(next_idx_var, loopBB);

      builder.SetInsertPoint(exitBB);
    }

  private:
    llvm::Type *output_type_;
  };
}

#endif // HOBBIT_REDUCTIONFUNCTORS_HPP
