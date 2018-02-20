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

llvm::Value *Hobbit::HorizontalSumReduction::Emit(llvm::IRBuilder<> &builder,
                                                  llvm::Value *input,
                                                  llvm::Type *vector_type) {
  llvm::Type *input_type = input->getType();

  if (input_type->isArrayTy() && vector_type != nullptr) {
    input = ArrayVectorPack_(builder, input, vector_type);
    input_type = vector_type;
  } else if (input_type->isArrayTy() && vector_type == nullptr) {
    input_type = input_type->getArrayElementType();
  } else if (input_type->isPointerTy() && vector_type != nullptr) {
    input = PtrVectorPack_(builder, input, vector_type);
    input_type = vector_type;
  } else if (input_type->isPointerTy() && vector_type == nullptr) {
    return nullptr;
  }

  if (input_type->isFPOrFPVectorTy()) {
    if (input_type->isVectorTy()) {
      return VectorHFAdd_(builder, input);
    }
    return SequenceHFAdd_(builder, input);
  }

  if (input_type->isIntOrIntVectorTy()) {
    if (input_type->isVectorTy()) {
      return VectorHAdd_(builder, input);
    }
    return SequenceHAdd_(builder, input);
  }

  return nullptr;
}

llvm::Value *Hobbit::HorizontalSumReduction::ArrayVectorPack_(
    llvm::IRBuilder<> &builder, llvm::Value *array, llvm::Type *vector_type) {

  uint64_t vector_elements = vector_type->getVectorNumElements();

  llvm::Value *output = llvm::UndefValue::get(vector_type);

  llvm::Type *vector_element_type = vector_type->getVectorElementType();
  llvm::Type *array_element_type = array->getType()->getArrayElementType();

  // make sure the if the vector type is float then everyone's a
  // float...otherwise break
  if (vector_element_type->isFPOrFPVectorTy() &&
      !array_element_type->isFPOrFPVectorTy()) {
    for (uint64_t i = 0; i < vector_elements; i++) {
      llvm::Value *array_element = builder.CreateFPCast(
          builder.CreateExtractValue(array, i), vector_element_type);
      output = builder.CreateInsertElement(output, array_element, i,
                                           array->getName() + ".pack");
    }

    return output;
  }

  if (vector_element_type->isIntOrIntVectorTy() &&
      array_element_type->isFPOrFPVectorTy()) {
    throw std::runtime_error(
        "Attempting to cast float to int, not yet supported");
  }

  for (uint64_t i = 0; i < vector_elements; i++) {
    llvm::Value *array_element = builder.CreateExtractValue(array, i);
    output = builder.CreateInsertElement(output, array_element, i,
                                         array->getName() + ".pack");
  }

  return output;
}

llvm::Value *Hobbit::HorizontalSumReduction::PtrVectorPack_(
    llvm::IRBuilder<> &builder, llvm::Value *ptr, llvm::Type *vector_type) {
  uint64_t vector_elements = vector_type->getVectorNumElements();

  llvm::Value *output = llvm::UndefValue::get(vector_type);

  llvm::Type *vector_element_type = vector_type->getVectorElementType();

  for (uint64_t i = 0; i < vector_elements; i++) {
    llvm::Value *ptr_element =
        builder.CreateFPCast( // force the pointer values to the right type
            builder.CreateLoad(builder.CreateGEP(ptr, builder.getInt64(i))),
            vector_element_type);
    output = builder.CreateInsertElement(output, ptr_element, i,
                                         ptr->getName() + ".pack");
  }

  return output;
}

llvm::Value *
Hobbit::HorizontalSumReduction::SequenceHFAdd_(llvm::IRBuilder<> &builder,
                                               llvm::Value *input) {

  llvm::Type *output_type = input->getType()->getArrayElementType();
  llvm::Value *output = builder.getInt64(0);
  output = builder.CreateBitCast(output, output_type);

  uint64_t array_num_elements = input->getType()->getArrayNumElements();

  for (uint64_t i = 0; i < array_num_elements; i++) {
    output = builder.CreateFAdd(output, builder.CreateExtractValue(input, i));
    // add the next element of the input
  }

  return output;
}

llvm::Value *
Hobbit::HorizontalSumReduction::SequenceHAdd_(llvm::IRBuilder<> &builder,
                                              llvm::Value *input) {
  llvm::Type *output_type = input->getType()->getArrayElementType();
  llvm::Value *output = builder.getInt64(0);
  output = builder.CreateBitCast(output, output_type);

  uint64_t array_num_elements = input->getType()->getArrayNumElements();

  for (uint64_t i = 0; i < array_num_elements; i++) {
    output = builder.CreateAdd(output, builder.CreateExtractValue(input, i));
    // add the next element of the input
  }

  return output;
}

llvm::Value *
Hobbit::HorizontalSumReduction::VectorHFAdd_(llvm::IRBuilder<> &builder,
                                             llvm::Value *input) {

  uint64_t vector_width = input->getType()->getVectorNumElements();

  llvm::Type *output_type = input->getType()->getVectorElementType();
  llvm::Value *output = builder.getInt64(0);
  output = builder.CreateBitCast(output, output_type);

  for (uint64_t i = 0; i < vector_width; i += 2) {
    output = builder.CreateFAdd(
        output,
        builder.CreateFAdd(builder.CreateExtractElement(input, i),
                           builder.CreateExtractElement(input, i + 1)),
        "hsum_reduction.vector.hfadd");
  }

  return output;
}

llvm::Value *
Hobbit::HorizontalSumReduction::VectorHAdd_(llvm::IRBuilder<> &builder,
                                            llvm::Value *input) {

  uint64_t vector_width = input->getType()->getVectorNumElements();

  llvm::Type *output_type = input->getType()->getVectorElementType();
  llvm::Value *output = builder.getInt64(0);
  output = builder.CreateBitCast(output, output_type);

  for (uint64_t i = 0; i < vector_width; i += 2) {
    output = builder.CreateAdd(
        output,
        builder.CreateAdd(builder.CreateExtractElement(input, i),
                          builder.CreateExtractElement(input, i + 1)),
        "hsum_reduction.vector.hadd");
  }

  return output;
}
