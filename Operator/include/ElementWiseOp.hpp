//
// Created by Aman LaChapelle on 2/17/18.
//
// c_science
// Copyright (c) 2018 Aman LaChapelle
// Full license at c_science/LICENSE.txt
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

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>

namespace Hobbit {
  class ElementWiseOp {
  public:
    explicit ElementWiseOp(llvm::LLVMContext &ctx);

    virtual llvm::Value *Emit(llvm::IRBuilder<> &builder, llvm::Value *lhs,
                              llvm::Value *rhs, llvm::Type *vector_type) = 0;

  protected:
    llvm::Value *ArrayVectorPack_(llvm::IRBuilder<> &builder,
                                  llvm::Value *array, llvm::Type *vector_type);

    llvm::Value *PtrVectorPack_(llvm::IRBuilder<> &builder, llvm::Value *ptr,
                                llvm::Type *vector_type);

    llvm::Value *constant = nullptr;
  };

  class ElementWiseProduct : public ElementWiseOp {
  public:
    explicit ElementWiseProduct(llvm::LLVMContext &ctx);

    llvm::Value *Emit(llvm::IRBuilder<> &builder, llvm::Value *lhs,
                      llvm::Value *rhs, llvm::Type *vector_type) override;

  private:
    // Returns an array of size sequence_len
    llvm::Value *SequenceFMul_(llvm::IRBuilder<> &builder, llvm::Value *lhs,
                               llvm::Value *rhs);
    llvm::Value *SequenceMul_(llvm::IRBuilder<> &builder, llvm::Value *lhs,
                              llvm::Value *rhs);

    // Returns a vector that's the same size as the inputs
    llvm::Value *VectorFMul_(llvm::IRBuilder<> &builder, llvm::Value *lhs,
                             llvm::Value *rhs);
    llvm::Value *VectorMul_(llvm::IRBuilder<> &builder, llvm::Value *lhs,
                            llvm::Value *rhs);
  };
}

#endif // HOBBIT_ELEMENTWISEOP_HPP
