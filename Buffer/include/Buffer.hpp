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

#ifndef HOBBIT_VARIABLE_HPP
#define HOBBIT_VARIABLE_HPP

#include <cstdint>
#include <list>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "CompileTimeBuffer.hpp"
#include "Shape.hpp"

namespace Hobbit {

  class Buffer {
  public:
    // Takes in the shape of the buffer to allocate, allows us to do things like
    // Pack and Split
    Buffer(llvm::IRBuilder<> &builder, llvm::Type *scalar_type,
           const Shape &shape);

    Buffer(llvm::Value *value, llvm::Type *scalar_type, const Shape &shape,
           Buffer *parent);

    Buffer(llvm::Value *value, const Shape &shape);

    virtual ~Buffer();

    const Shape &GetShape();
    llvm::Value *GetValue();
    llvm::Type *GetType();

    // Should be used to split into arbitrary chunks - will allow you to further
    // split if desired.
    Buffer *GetChunk(llvm::IRBuilder<> &builder, const Range &k_range,
                     const Range &h_range, const Range &w_range);
    Buffer *Flatten();

    void ClearChild(Buffer *child);
    void ClearChildren();

    // This function treats the whole array as a flat buffer, and is therefore
    // recommended to only be used
    // when trying to schedule a computation using SIMD instructions, and
    // therefore used on a very small
    // piece of a variable.
    llvm::ArrayRef<llvm::Value *> Pack(llvm::IRBuilder<> &builder,
                                       uint32_t &vector_size);

  protected:
    llvm::Value *m_value_;
    llvm::Type *m_type_;
    Shape m_shape_;

    Buffer *m_parent_ = nullptr;
    std::list<Buffer *> m_children_;
  };

  class Constant : public Buffer {
  public:
    Constant(llvm::IRBuilder<> &builder, llvm::Type *scalar_type,
             CompileTimeBuffer &buffer);
  };

  class Variable : public Buffer {
  public:
    Variable(llvm::Value *value, llvm::Type *scalar_type, const Shape &shape);
  };
}

#endif // HOBBIT_VARIABLE_HPP
