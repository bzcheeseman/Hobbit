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

#ifndef C_SCIENCE_REDUCTIONOP_HPP
#define C_SCIENCE_REDUCTIONOP_HPP

#include "Operator.hpp"

namespace Hobbit {
class ReductionOp {
public:
  explicit ReductionOp(llvm::LLVMContext &ctx);

  virtual llvm::Value *Emit(llvm::IRBuilder<> &builder, llvm::Value *input, llvm::Type *vector_type) = 0;

protected:
  llvm::Value *constant;
};

class HorizontalSumReduction : public ReductionOp {
public:
  explicit HorizontalSumReduction(llvm::LLVMContext &ctx);

  llvm::Value *Emit(llvm::IRBuilder<> &builder, llvm::Value *input, llvm::Type *vector_type) override;

private:
  llvm::Value *SequenceHFAdd(llvm::IRBuilder<> &builder, llvm::Value *input);
  llvm::Value *SequenceHAdd(llvm::IRBuilder<> &builder, llvm::Value *input);

  llvm::Value *VectorHFAdd(llvm::IRBuilder<> &builder, llvm::Value *input);
  llvm::Value *VectorHAdd(llvm::IRBuilder<> &builder, llvm::Value *input);
};
}

#endif // C_SCIENCE_REDUCTIONOP_HPP
