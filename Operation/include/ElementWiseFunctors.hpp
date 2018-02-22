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
  template <uint32_t VectorWidth = 4>
  class ElementWiseProduct : public Functor<VectorWidth> {
  public:
    explicit ElementWiseProduct(Constant &c) : c_(c) {}

    inline Variable Emit(llvm::IRBuilder<> &builder, Variable &input) override {
      if (c_.GetType() != input.GetType())
        throw std::runtime_error(
            "Both Variable and Constant must be the same type!");
      if (c_.GetShape() != input.GetShape())
        throw std::runtime_error(
            "Both Variable and Constant must be the same shape!");

      llvm::ArrayRef<llvm::Value *> constant_vectors =
          c_.Pack(builder, VectorWidth);
      llvm::ArrayRef<llvm::Value *> input_vectors =
          input.Pack(builder, VectorWidth);

      uint64_t size = constant_vectors.size();
      llvm::Type *ctype = c_.GetType();

      llvm::Value *output_buffer = builder.CreateAlloca(
          c_.GetType(), builder.getInt64(size * VectorWidth));
      std::vector<llvm::Value *> results(size);

      if (ctype->isIntegerTy()) {
        for (uint64_t i = 0; i < size; i++) {
          results[i] = builder.CreateMul(constant_vectors[i], input_vectors[i]);
        }
      } else if (ctype->isFloatingPointTy()) {
        for (uint64_t i = 0; i < size; i++) {
          results[i] =
              builder.CreateFMul(constant_vectors[i], input_vectors[i]);
        }
      }

      for (uint64_t i = 0; i < size; i++) {
        for (uint64_t j = 0; j < VectorWidth; j++) {
          llvm::Value *elt = builder.CreateGEP(
              output_buffer, builder.getInt64(i * VectorWidth + j));
          builder.CreateStore(builder.CreateExtractElement(results[i], j), elt);
        }
      }

      return Variable(output_buffer, ctype, c_.GetShape());
    }

  private:
    Constant c_;
  };
}

#endif // HOBBIT_ELEMENTWISEFUNCTORS_HPP
