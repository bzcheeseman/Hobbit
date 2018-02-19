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

#include "ReductionOp.hpp"

Hobbit::ReductionOp::ReductionOp(llvm::LLVMContext &ctx) {
  this->constant = nullptr;
}

Hobbit::HorizontalSumReduction::HorizontalSumReduction(llvm::LLVMContext &ctx)
    : ReductionOp(ctx) {}

llvm::Value *
Hobbit::HorizontalSumReduction::Emit(llvm::IRBuilder<> &builder, llvm::Value *input, llvm::Type *vector_type) {
  return nullptr;
}

llvm::Value *Hobbit::HorizontalSumReduction::SequenceHFAdd(llvm::IRBuilder<> &builder, llvm::Value *input) {

  llvm::Type *output_type = input->getType()->getArrayElementType();
  llvm::Value *output = builder.getInt64(0);
  output = builder.CreateBitCast(output, output_type);

  uint64_t array_num_elements = input->getType()->getArrayNumElements();

  for (uint64_t i = 0; i < array_num_elements; i++) {
    output = builder.CreateFAdd(output, builder.CreateExtractValue(input, i)); // add the next element of the input
  }

  return output;

}

llvm::Value *Hobbit::HorizontalSumReduction::SequenceHAdd(llvm::IRBuilder<> &builder, llvm::Value *input) {
  return nullptr;
}

llvm::Value *Hobbit::HorizontalSumReduction::VectorHFAdd(llvm::IRBuilder<> &builder, llvm::Value *input) {

  uint64_t vector_width = input->getType()->getVectorNumElements();

  uint64_t step = 1;
  while (step < vector_width) {
    for (uint64_t i = 0; i < vector_width; i += (step << 1)) {
      llvm::Value *result_chunk_sum =
              builder.CreateFAdd(builder.CreateExtractElement(input, i),
                                 builder.CreateExtractElement(input, i + step),
                                 "result_chunk_sum");
      builder.CreateInsertElement(input, result_chunk_sum, i);
    }
    step <<= 1;
  }

  return builder.CreateExtractElement(input, (uint64_t)0, "hsum_reduction.vector.hfadd");
}

llvm::Value *Hobbit::HorizontalSumReduction::VectorHAdd(llvm::IRBuilder<> &builder, llvm::Value *input) {

  uint64_t vector_width = input->getType()->getVectorNumElements();

  uint64_t step = 1;
  while (step < vector_width) {
    for (uint64_t i = 0; i < vector_width; i += (step << 1)) {
      llvm::Value *result_chunk_sum =
              builder.CreateAdd(builder.CreateExtractElement(input, i),
                                 builder.CreateExtractElement(input, i + step),
                                 "result_chunk_sum");
      builder.CreateInsertElement(input, result_chunk_sum, i);
    }
    step <<= 1;
  }

  return builder.CreateExtractElement(input, (uint64_t)0, "hsum_reduction.vector.hadd");
}
