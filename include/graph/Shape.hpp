//
// Created by Aman LaChapelle on 5/3/18.
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


#ifndef HOBBIT_SHAPE_HPP
#define HOBBIT_SHAPE_HPP

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ArrayRef.h>

namespace llvm {
  class LLVMContext;
  class Value;
  class Type;
  class BasicBlock;
}

namespace Hobbit {
  namespace graph {

    class Shape {
    public:
      Shape(llvm::LLVMContext &ctx, llvm::ArrayRef <uint64_t> dims);

      explicit Shape(llvm::ArrayRef <uint64_t> dims);

      explicit Shape(llvm::ArrayRef<llvm::Value *> dims);

      void InitLLVM(llvm::LLVMContext &ctx);

      // Gets the number of dimensions for a tensor
      uint64_t NDim() const;

      // Gets a dimension of a tensor
      uint64_t Dim(uint64_t which) const;

      llvm::Type *DimType() const;

      // Gets a dimension of a tensor
      llvm::Value *Dim(llvm::Value *which) const;

      uint64_t Size() const;

      // Gets the overall size of a tensor
      llvm::Value *Size(llvm::BasicBlock *BB) const;

      uint64_t At(llvm::ArrayRef <uint64_t> idx) const;

      llvm::Value *At(llvm::ArrayRef<llvm::Value *> idx,
                      llvm::BasicBlock *BB) const;

      // BB can be nullptr, in which case the output is just a shape with llvm not
      // initialized
      Shape Flatten(llvm::BasicBlock *BB) const;

    private:
      llvm::SmallVector<uint64_t, 4> m_dims_;
      bool m_has_llvm_;
      llvm::SmallVector<llvm::Value *, 4> m_v_dims_;
    };

  }
}

#endif //HOBBIT_SHAPE_HPP
