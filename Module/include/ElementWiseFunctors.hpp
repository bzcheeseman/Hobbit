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

namespace Hobbit {
  class ElementWiseProduct : public Functor {
  public:
    explicit ElementWiseProduct(Buffer *c) : c_(c) {}

    inline Buffer AllocOutput(llvm::BasicBlock *BB) override {
      return Buffer(BB, c_->GetType(), c_->GetShape());
    }

    inline void Emit(internal::Function &f, Buffer *input,
                     Buffer *output) override {
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

      llvm::BasicBlock *BB = f.AddBB("EWiseProduct");
      llvm::IRBuilder<> builder(BB);

      const Shape &shape = c_->GetShape();
      llvm::Type *type = c_->GetType();
      llvm::Value *c_value = c_->GetValue();
      llvm::Value *input_value = input->GetValue();

      llvm::Value *ret = output->GetValue();

      if (type->isIntegerTy()) {
        for (uint64_t i = 0; i < c_->GetShape().GetSize(); i++) {
          llvm::Value *c_gep = builder.CreateGEP(c_value, builder.getInt64(i));
          llvm::Value *c_elt = builder.CreateAlignedLoad(c_gep, 4);

          llvm::Value *i_gep =
              builder.CreateGEP(input_value, builder.getInt64(i));
          llvm::Value *i_elt = builder.CreateAlignedLoad(i_gep, 4);

          llvm::Value *ret_elt = builder.CreateGEP(ret, builder.getInt64(i));

          builder.CreateAlignedStore(builder.CreateMul(c_elt, i_elt), ret_elt,
                                     4);
        }
      } else if (type->isFloatingPointTy()) {
        for (uint64_t i = 0; i < c_->GetShape().GetSize(); i++) {
          llvm::Value *c_gep = builder.CreateGEP(c_value, builder.getInt64(i));
          llvm::Value *c_elt = builder.CreateAlignedLoad(c_gep, 4);

          llvm::Value *i_gep =
              builder.CreateGEP(input_value, builder.getInt64(i));
          llvm::Value *i_elt = builder.CreateAlignedLoad(i_gep, 4);

          llvm::Value *ret_elt = builder.CreateGEP(ret, builder.getInt64(i));

          builder.CreateAlignedStore(builder.CreateFMul(c_elt, i_elt), ret_elt,
                                     4);
        }
      }
    }

  private:
    Buffer *c_;
  };

  class LargeElementWiseProduct : public Functor {
  public:
    explicit LargeElementWiseProduct(Buffer *c) : c_(c) {}

    inline Buffer AllocOutput(llvm::BasicBlock *BB) override {
      return Buffer(BB, c_->GetType(), c_->GetShape());
    }

    inline void Emit(internal::Function &f, Buffer *input,
                     Buffer *output) override {
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

      llvm::BasicBlock *entryBB = f.AddBB("LargeEwiseEntry");
      llvm::BasicBlock *loopBB = f.AddBB("LargeEwiseProduct");
      llvm::BasicBlock *exitBB = f.AddBB("LargeEwiseExit");

      llvm::IRBuilder<> builder(entryBB);
      builder.CreateBr(loopBB);

      const Shape &shape = c_->GetShape();
      llvm::Type *type = c_->GetType();
      llvm::Value *c_value = c_->GetValue();
      llvm::Value *input_value = input->GetValue();

      llvm::Value *ret = output->GetValue();

      builder.SetInsertPoint(loopBB);
      llvm::PHINode *var = builder.CreatePHI(builder.getInt64Ty(), 2, "LargeEwiseProductIndex");
      var->addIncoming(builder.getInt64(0), entryBB);

      llvm::Value *c_gep = builder.CreateGEP(c_value, var);
      llvm::Value *c_elt = builder.CreateAlignedLoad(c_gep, 4);

      llvm::Value *i_gep = builder.CreateGEP(input_value, var);
      llvm::Value *i_elt = builder.CreateAlignedLoad(i_gep, 4);

      llvm::Value *ret_elt = builder.CreateGEP(ret, var);

      if (type->isIntegerTy()) {
        builder.CreateAlignedStore(builder.CreateMul(c_elt, i_elt), ret_elt, 4);
      }
      else if (type->isFloatingPointTy()) {
        builder.CreateAlignedStore(builder.CreateFMul(c_elt, i_elt), ret_elt, 4);
      }

      llvm::Value *nextvar = builder.CreateAdd(var, builder.getInt64(1));
      llvm::Value *end_condition = builder.CreateICmpEQ(nextvar, builder.getInt64(shape.GetSize()));
      builder.CreateCondBr(end_condition, exitBB, loopBB);
      var->addIncoming(nextvar, loopBB);

      builder.SetInsertPoint(exitBB);
    }

  private:
    Buffer *c_;
  };
}

#endif // HOBBIT_ELEMENTWISEFUNCTORS_HPP
