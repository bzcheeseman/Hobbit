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

    inline void Emit(Function &f, Buffer *input,
                     Buffer *output) override {
      if (output_type_ != input->GetType())
        throw std::runtime_error("Input and output must be the same type!");
      if (output->GetShape() != Shape(1, 1, 1))
        throw std::runtime_error("Output shape must match specified shape!");

      llvm::BasicBlock *BB = f.AddBB("SumReduction");
      llvm::IRBuilder<> builder(BB);

      llvm::Value *input_value = input->GetValue();
      llvm::Value *output_value = output->GetValue();

      builder.CreateStore(
          builder.CreateBitCast(builder.getInt64(0), output_type_),
          output_value);

      if (output_type_->isIntegerTy()) {
        for (uint64_t i = 0; i < input->GetShape().GetSize(); i++) {
          llvm::Value *i_gep =
              builder.CreateGEP(input_value, builder.getInt64(i));
          llvm::Value *i_elt = builder.CreateAlignedLoad(i_gep, 4);

          llvm::Value *out_elt = builder.CreateLoad(output_value);

          builder.CreateAlignedStore(builder.CreateAdd(i_elt, out_elt),
                                     output_value, 4);
        }
      } else if (output_type_->isFloatingPointTy()) {
        for (uint64_t i = 0; i < input->GetShape().GetSize(); i++) {
          llvm::Value *i_gep =
              builder.CreateGEP(input_value, builder.getInt64(i));
          llvm::Value *i_elt = builder.CreateAlignedLoad(i_gep, 4);

          llvm::Value *out_elt = builder.CreateLoad(output_value);

          builder.CreateAlignedStore(builder.CreateFAdd(i_elt, out_elt),
                                     output_value, 4);
        }
      }
    }

  private:
    llvm::Type *output_type_;
  };

  class LargeSumReduction : public Functor {
  public:
    explicit LargeSumReduction(llvm::Type *output_type)
            : output_type_(output_type) {}

    inline Buffer AllocOutput(llvm::BasicBlock *BB) override {
      return Buffer(BB, output_type_, Shape(1, 1, 1));
    }

    inline void Emit(Function &f, Buffer *input,
                     Buffer *output) override {
      if (output_type_ != input->GetType())
        throw std::runtime_error("Input and output must be the same type!");
      if (output->GetShape() != Shape(1, 1, 1))
        throw std::runtime_error("Output shape must match specified shape!");

      llvm::BasicBlock *entryBB = f.AddBB("LargeSumRedEntry");
      llvm::BasicBlock *loopBB = f.AddBB("LargeSumRedProduct");
      llvm::BasicBlock *exitBB = f.AddBB("LargeSumRedExit");

      llvm::IRBuilder<> builder(entryBB);
      builder.CreateBr(loopBB);

      llvm::Value *input_value = input->GetValue();
      llvm::Value *output_value = output->GetValue();

      builder.SetInsertPoint(loopBB);
      llvm::PHINode *idx_var = builder.CreatePHI(builder.getInt64Ty(), 2, "LargeSumRedIndex");
      llvm::PHINode *var = builder.CreatePHI(output_type_, 2, "LargeSumRedResult");
      idx_var->addIncoming(builder.getInt64(0), entryBB);
      var->addIncoming(builder.CreateBitCast(builder.getInt64(0), output_type_), entryBB);

      llvm::Value *i_gep = builder.CreateGEP(input_value, idx_var);
      llvm::Value *i_elt = builder.CreateAlignedLoad(i_gep, 4);

      llvm::Value *nextvar;
      if (output_type_->isIntegerTy()) {
        nextvar = builder.CreateAdd(var, i_elt);
      }
      else if (output_type_->isFloatingPointTy()) {
        nextvar = builder.CreateFAdd(var, i_elt);
      }

      llvm::Value *next_idx_var = builder.CreateAdd(idx_var, builder.getInt64(1));
      llvm::Value *end_condition = builder.CreateICmpEQ(next_idx_var, builder.getInt64(input->GetShape().GetSize()));

      builder.CreateCondBr(end_condition, exitBB, loopBB);
      idx_var->addIncoming(next_idx_var, loopBB);
      var->addIncoming(nextvar, loopBB);

      builder.SetInsertPoint(exitBB);
      builder.CreateAlignedStore(nextvar, output_value, 4);

    }

  private:
    llvm::Type *output_type_;
  };
}

#endif // HOBBIT_REDUCTIONFUNCTORS_HPP
