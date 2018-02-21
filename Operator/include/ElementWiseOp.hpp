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
  // Use functors to generalize what we emit
  // the functor operates on chunk(s) of data which are llvm::Vector's. It will
  // be called once for each chunk in
  // the operator input data
  struct ElementWiseBinaryFunctor {
    virtual llvm::Value *operator()(llvm::Value *lhs, llvm::Value *rhs) = 0;
  };

  struct ElementWiseUnaryFunctor {
    virtual llvm::Value *operator()(llvm::Value *input) = 0;
  };

  // ElementWiseOp has PushUnaryFunctor(ElementWiseUnaryFunctor *f, llvm::Value
  // *constant) (constant can be null)
  // ElementWiseOp has PushBinaryFunctor(ElementWiseBinaryFunctor *f)
  // now the only question is how to keep track of inputs as they come in...tag
  // them maybe?

  class EWiseOp {
    // trivial chunking
  };

  class ROp {
    // chunking needs to be done carefully to not lose generality
  };

  class ElementWiseOp {
  public:
    // single value (contains either an entire array or one llvm::Vector
    virtual llvm::ArrayRef<llvm::Value *>
    Emit(llvm::IRBuilder<> &builder, llvm::Value *lhs, uint64_t &lhs_size,
         llvm::Value *rhs, uint64_t &rhs_size, llvm::Type *vector_type) = 0;

    // Chunked values (multiple values or one llvm::Vector)
    virtual llvm::ArrayRef<llvm::Value *>
    Emit(llvm::IRBuilder<> &builder, llvm::ArrayRef<llvm::Value *> lhs,
         llvm::ArrayRef<llvm::Value *> rhs, llvm::Type *vector_type) = 0;

  protected:
    llvm::ArrayRef<llvm::Value *> ArrayVectorPack_(llvm::IRBuilder<> &builder,
                                                   llvm::Value *array,
                                                   llvm::Type *vector_type);

    llvm::ArrayRef<llvm::Value *> PtrVectorPack_(llvm::IRBuilder<> &builder,
                                                 llvm::Value *ptr,
                                                 uint64_t &array_num_elements,
                                                 llvm::Type *vector_type);
  };

  class ElementWiseProduct : public ElementWiseOp {
  public:
    explicit ElementWiseProduct() = default;

    // Returns an array of chunks
    llvm::ArrayRef<llvm::Value *> Emit(llvm::IRBuilder<> &builder,
                                       llvm::Value *lhs, uint64_t &lhs_size,
                                       llvm::Value *rhs, uint64_t &rhs_size,
                                       llvm::Type *vector_type) override;

    llvm::ArrayRef<llvm::Value *> Emit(llvm::IRBuilder<> &builder,
                                       llvm::ArrayRef<llvm::Value *> lhs,
                                       llvm::ArrayRef<llvm::Value *> rhs,
                                       llvm::Type *vector_type) override;

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
