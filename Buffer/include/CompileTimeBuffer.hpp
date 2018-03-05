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

#ifndef HOBBIT_COMPILETIMEBUFFER_HPP
#define HOBBIT_COMPILETIMEBUFFER_HPP

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "Shape.hpp"

namespace Hobbit {
  class CompileTimeBuffer {
  public:
    virtual const Shape &GetShape() = 0;
    virtual llvm::Value *GetValue(llvm::IRBuilder<> &builder, llvm::Type *type,
                                  const llvm::Twine &name = "") = 0;
  };

  class CompileTimeIntBuffer : public CompileTimeBuffer {
  public:
    CompileTimeIntBuffer(uint64_t *ptr, const Shape &shape)
        : ptr_(ptr), shape_(shape) {}

    const Shape &GetShape() override { return shape_; }

    llvm::Value *GetValue(llvm::IRBuilder<> &builder, llvm::Type *int_type,
                          const llvm::Twine &name = "") override {
      llvm::Type *type = int_type;

      if (int_type == nullptr)
        type = builder.getInt16Ty();

      uint32_t size = shape_.GetSize();

      llvm::Value *alloca_size = builder.getInt32(size);
      uint32_t bit_width = type->getIntegerBitWidth();

      llvm::Value *out = builder.CreateAlloca(type, alloca_size);

      for (uint32_t i = 0; i < size; i++) {
        llvm::Value *elt = builder.getInt(llvm::APInt(bit_width, ptr_[i]));
        builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt32(i)));
      }

      if (name.isTriviallyEmpty())
        out->setName("hobbit.compiletimebuffer.INT");

      return out;
    }

  private:
    uint64_t *ptr_;
    const Shape shape_;
  };

  class CompileTimeFPBuffer : public CompileTimeBuffer {
  public:
    CompileTimeFPBuffer(double *ptr, const Shape &shape)
        : ptr_(ptr), shape_(shape) {}

    const Shape &GetShape() override { return shape_; }

    llvm::Value *GetValue(llvm::IRBuilder<> &builder, llvm::Type *fp_type,
                          const llvm::Twine &name = "") override {
      llvm::Type *type = fp_type;

      if (fp_type == nullptr)
        type = builder.getFloatTy();

      uint32_t size = shape_.GetSize();

      llvm::Value *alloca_size = builder.getInt32(size);

      llvm::Value *out = builder.CreateAlloca(type, alloca_size);

      for (uint32_t i = 0; i < size; i++) {
        llvm::Value *elt = llvm::ConstantFP::get(type, ptr_[i]);
        builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt64(i)));
      }

      if (name.isTriviallyEmpty())
        out->setName("hobbit.compiletimebuffer.FP");

      return out;
    }

  private:
    double *ptr_;
    const Shape shape_;
  };
}

#endif // HOBBIT_COMPILETIMEBUFFER_HPP
