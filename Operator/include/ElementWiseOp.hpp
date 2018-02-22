//
// Created by Aman LaChapelle on 2/17/18.
//
// c_science
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

#ifndef HOBBIT_ELEMENTWISEOP_HPP
#define HOBBIT_ELEMENTWISEOP_HPP

#include <list>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>

#include "Buffer.hpp"

namespace Hobbit {

  template<uint32_t VectorWidth>
  class Functor {
  public:
    virtual Variable Emit(llvm::IRBuilder<> &builder, Variable &input) = 0;
  };

  class Operation {
  public:
    void PushFunctor(Functor &f);
    llvm::Value *Emit(llvm::IRBuilder<> &builder, Variable &input);

  private:
    std::list<Functor *> op_table_;
  };

  template<uint32_t VectorWidth=4>
  class ElementWiseProduct : public Functor<VectorWidth> {
  public:
    explicit ElementWiseProduct(Constant &c) : c_(c) {}

    Variable Emit(llvm::IRBuilder<> &builder, Variable &input) override {
      if (c_.GetType() != input.GetType()) throw std::runtime_error("Both Variable and Constant must be the same type!");
      if (c_.GetShape() != input.GetShape()) throw std::runtime_error("Both Variable and Constant must be the same shape!");

      llvm::ArrayRef<llvm::Value *> constant_vectors = c_.Pack(builder, VectorWidth);
      llvm::ArrayRef<llvm::Value *> input_vectors = input.Pack(builder, VectorWidth);

      uint64_t size = constant_vectors.size();
      llvm::Type *ctype = c_.GetType();

      llvm::Value *output_buffer = builder.CreateAlloca(c_.GetType(), builder.getInt64(size*VectorWidth));
      std::vector<llvm::Value *> results (size);

      if (ctype->isIntegerTy()) {
        for (uint64_t i = 0; i < size; i++) {
          results[i] = builder.CreateMul(constant_vectors[i], input_vectors[i]);
        }
      }
      else if (ctype->isFloatingPointTy()) {
        for (uint64_t i = 0; i < size; i++) {
          results[i] = builder.CreateFMul(constant_vectors[i], input_vectors[i]);
        }
      }

      for (uint64_t i = 0; i < size; i++) {
        for (uint64_t j = 0; j < VectorWidth; j++) {
          llvm::Value *elt = builder.CreateGEP(output_buffer, builder.getInt64(i*VectorWidth + j));
          builder.CreateStore(builder.CreateExtractElement(results[i], j), elt);
        }
      }

      return Variable(output_buffer, ctype, c_.GetShape());
    }

  private:
    Constant c_;
  };

  template<uint32_t VectorWidth=1>
  class AllSumReduction : public Functor<VectorWidth> {
  public:
    Variable Emit(llvm::IRBuilder<> &builder, Variable &input) override {

      llvm::ArrayRef<llvm::Value *> input_vectors = input.Flatten()->Pack(builder, VectorWidth);

      uint64_t size = input_vectors.size();
      llvm::Type *type = input.GetType();

      llvm::Value *output_buffer = builder.CreateAlloca(type, builder.getInt64(1));
      builder.CreateStore(builder.CreateBitCast(builder.getInt64(0), type), output_buffer);

      if (type->isIntegerTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateAdd(output_buffer, input_vectors[i]);
        }
      }
      else if (type->isFloatingPointTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateFAdd(output_buffer, input_vectors[i]);
        }
      }

      return Variable(output_buffer, type, Shape(1, 1, 1));
    }
  };

  template<uint32_t AXIS=2, uint32_t VECTOR_WIDTH=1>
  class AxisSumReduction : public Functor<VECTOR_WIDTH> {
  public:
    Variable Emit(llvm::IRBuilder<> &builder, Variable &input) override {

      uint32_t k_axis_size = input.GetShape().GetAxisSize(Axis::K);
      uint32_t h_axis_size = input.GetShape().GetAxisSize(Axis::H);
      uint32_t w_axis_size = input.GetShape().GetAxisSize(Axis::W);

      Shape output_shape;
      switch (AXIS) {
        case 0: output_shape = Shape(1, h_axis_size, w_axis_size); break;
        case 1: output_shape = Shape(k_axis_size, 1, w_axis_size); break;
        case 2: output_shape = Shape(k_axis_size, h_axis_size, 1); break;
        default: throw std::runtime_error("Invalid reduction shape specified");
      }

      // get sizes of axes *not* along reduction axis, to iterate over
      // get chunk of size of reduction axis and flatten
      // hsum over that axis

      llvm::ArrayRef<llvm::Value *> input_vectors = input.Pack(builder, VECTOR_WIDTH);

      uint64_t size = input_vectors.size();
      llvm::Type *type = input.GetType();

      llvm::Value *output_buffer = builder.CreateAlloca(type, builder.getInt64(1));
      builder.CreateStore(builder.CreateBitCast(builder.getInt64(0), type), output_buffer);

      if (type->isIntegerTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateAdd(output_buffer, input_vectors[i]);
        }
      }
      else if (type->isFloatingPointTy()) {
        for (uint64_t i = 0; i < size; i++) {
          output_buffer = builder.CreateFAdd(output_buffer, input_vectors[i]);
        }
      }

      return Variable(output_buffer, type, Shape(1, 1, 1));
    }
  };
}

#endif // HOBBIT_ELEMENTWISEOP_HPP
