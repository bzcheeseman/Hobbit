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

    inline void Emit(llvm::BasicBlock *BB, Buffer *input, Buffer *output) override {
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

      llvm::IRBuilder<> builder(BB);

      const Shape &shape = c_->GetShape();
      llvm::Type *type = c_->GetType();
      llvm::Value *c_value = c_->GetValue();
      llvm::Value *input_value = input->GetValue();

      llvm::Value *ret = output->GetValue();

      if (type->isIntegerTy()) {
        for (uint64_t i = 0; i < c_->GetShape().GetSize(); i++) {
          llvm::Value *c_gep = builder.CreateGEP(c_value, builder.getInt64(i));
          llvm::Value *c_elt = builder.CreateLoad(c_gep);

          llvm::Value *i_gep = builder.CreateGEP(input_value, builder.getInt64(i));
          llvm::Value *i_elt = builder.CreateLoad(i_gep);

          builder.CreateStore(builder.CreateMul(c_elt, i_elt), ret);
        }
      } else if (type->isFloatingPointTy()) {
        for (uint64_t i = 0; i < c_->GetShape().GetSize(); i++) {
          llvm::Value *c_gep = builder.CreateGEP(c_value, builder.getInt64(i));
          llvm::Value *c_elt = builder.CreateLoad(c_gep);

          llvm::Value *i_gep = builder.CreateGEP(input_value, builder.getInt64(i));
          llvm::Value *i_elt = builder.CreateLoad(i_gep);

          builder.CreateStore(builder.CreateFMul(c_elt, i_elt), ret);
        }
      }
    }

  private:
    Buffer *c_;
  };
}

#endif // HOBBIT_ELEMENTWISEFUNCTORS_HPP
