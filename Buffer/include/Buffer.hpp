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

#include "Shape.hpp"

namespace Hobbit {

  // Maybe we just get rid of this and think about how the user will use the
  // functions...
  // If you give me a float* and a shape for everything, then I can use/re-use
  // that memory as I
  // see fit...maybe the function definition is more important than I thought,
  // and each
  // operation should define its own IR function and push it into a vector,
  // which then
  // lives in a Module that can get compiled to llvm IR, that way we have
  // composable chunks in IR
  // too?

  class Buffer {
  public:
    // Takes in the shape of the buffer to allocate, allows us to do things like
    // Pack and Split
    Buffer(llvm::BasicBlock *BB, llvm::Type *scalar_type, const Shape &shape);

    Buffer(llvm::Value *value, llvm::Type *scalar_type, const Shape &shape,
           Buffer *parent);

    Buffer(llvm::Value *value, const Shape &shape);

    virtual ~Buffer();

    const Shape &GetShape();
    llvm::Value *GetValue();
    llvm::Type *GetType();

    // Should be used to split into arbitrary chunks - will allow you to further
    // split if desired.
    Buffer *GetChunk(llvm::BasicBlock *BB, const Range &k_range,
                     const Range &h_range, const Range &w_range);
    Buffer *Flatten();

    llvm::Value *GetElement(llvm::BasicBlock *BB, const uint64_t &idx);
    llvm::Value *GetElement(llvm::BasicBlock *BB, const uint64_t &k,
                            const uint64_t &h, const uint64_t &w);

    void ClearChild(Buffer *child);
    void ClearChildren();

  protected:
    llvm::Value *m_value_;
    llvm::Type *m_type_;
    Shape m_shape_;

    Buffer *m_parent_ = nullptr;
    std::list<Buffer *> m_children_;
  };
}

#endif // HOBBIT_VARIABLE_HPP
