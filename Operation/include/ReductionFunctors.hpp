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

#ifndef HOBBIT_REDUCTIONFUNCTORS_HPP
#define HOBBIT_REDUCTIONFUNCTORS_HPP

#include <cstdint>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "Buffer.hpp"
#include "Functor.hpp"

namespace Hobbit {

  template <uint32_t VectorWidth = 1>
  class AllSumReduction : public Functor<VectorWidth> {
  public:
    Variable Emit(llvm::IRBuilder<> &builder, Variable &input) override {

      llvm::ArrayRef<llvm::Value *> input_vectors =
          input.Flatten()->Pack(builder, VectorWidth);

      uint64_t size = input_vectors.size();
      llvm::Type *type = input.GetType();

      llvm::Value *output_buffer =
          builder.CreateAlloca(type, builder.getInt64(1));
      builder.CreateStore(builder.CreateBitCast(builder.getInt64(0), type),
                          output_buffer);

      if (type->isIntegerTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateAdd(output_buffer, input_vectors[i]);
        }
      } else if (type->isFloatingPointTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateFAdd(output_buffer, input_vectors[i]);
        }
      }

      return Variable(output_buffer, type, Shape(1, 1, 1));
    }
  };

  template <uint32_t AXIS = 2, uint32_t VECTOR_WIDTH = 1>
  class AxisSumReduction : public Functor<VECTOR_WIDTH> {
  public:
    Variable Emit(llvm::IRBuilder<> &builder, Variable &input) override {

      uint32_t k_axis_size = input.GetShape().GetAxisSize(Axis::K);
      uint32_t h_axis_size = input.GetShape().GetAxisSize(Axis::H);
      uint32_t w_axis_size = input.GetShape().GetAxisSize(Axis::W);

      Shape output_shape;
      switch (AXIS) {
      case 0:
        output_shape = Shape(1, h_axis_size, w_axis_size);
        break;
      case 1:
        output_shape = Shape(k_axis_size, 1, w_axis_size);
        break;
      case 2:
        output_shape = Shape(k_axis_size, h_axis_size, 1);
        break;
      default:
        throw std::runtime_error("Invalid reduction shape specified");
      }

      // get sizes of axes *not* along reduction axis, to iterate over
      // get chunk of size of reduction axis and flatten
      // hsum over that axis

      llvm::ArrayRef<llvm::Value *> input_vectors =
          input.Pack(builder, VECTOR_WIDTH);

      uint64_t size = input_vectors.size();
      llvm::Type *type = input.GetType();

      llvm::Value *output_buffer =
          builder.CreateAlloca(type, builder.getInt64(1));
      builder.CreateStore(builder.CreateBitCast(builder.getInt64(0), type),
                          output_buffer);

      if (type->isIntegerTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateAdd(output_buffer, input_vectors[i]);
        }
      } else if (type->isFloatingPointTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateFAdd(output_buffer, input_vectors[i]);
        }
      }

      return Variable(output_buffer, type, Shape(1, 1, 1));
    }
  };
}

#endif // HOBBIT_REDUCTIONFUNCTORS_HPP
