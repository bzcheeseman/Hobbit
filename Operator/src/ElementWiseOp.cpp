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

#include "ElementWiseOp.hpp"

llvm::ArrayRef<llvm::Value *> Hobbit::ElementWiseOp::ArrayVectorPack_(
    llvm::IRBuilder<> &builder, llvm::Value *array, llvm::Type *vector_type) {

  uint64_t vector_elements = vector_type->getVectorNumElements();
  uint64_t array_elements = array->getType()->getArrayNumElements();

  uint64_t leftovers = array_elements % vector_elements;
  uint64_t chunks = (array_elements - leftovers) / vector_elements;
  if (leftovers != 0)
    chunks += 1;

  std::vector<llvm::Value *> output(chunks);

  llvm::Type *vector_element_type = vector_type->getVectorElementType();
  llvm::Type *array_element_type = array->getType()->getArrayElementType();

  if (vector_element_type != array_element_type) { // the types are not equal
    if (vector_element_type
            ->isFloatingPointTy()) { // we can totally cast int to float
      for (uint64_t i = 0; i < chunks; i++) {
        for (uint64_t j = 0; j < vector_elements; j++) {
          llvm::Value *array_element = builder.CreateFPCast(
              builder.CreateExtractValue(array, i * vector_elements + j),
              vector_element_type);
          output[i] = builder.CreateInsertElement(output[i], array_element, j,
                                                  array->getName() + ".pack." +
                                                      std::to_string(j));
        }
      }

      return output;
    }

    if (vector_element_type
            ->isIntegerTy()) { // Don't want to cast float to int (yet)
      throw std::runtime_error(
          "Attempting to cast float to int, not yet supported");
    }
  }

  for (uint64_t i = 0; i < chunks; i++) {
    for (uint64_t j = 0; j < vector_elements; j++) {
      llvm::Value *array_element =
          builder.CreateExtractValue(array, i * vector_elements + j);
      output[i] = builder.CreateInsertElement(output[i], array_element, j,
                                              array->getName() + ".pack." +
                                                  std::to_string(j));
    }
  }

  return output;
}

llvm::ArrayRef<llvm::Value *> Hobbit::ElementWiseOp::PtrVectorPack_(
    llvm::IRBuilder<> &builder, llvm::Value *ptr, uint64_t &array_num_elements,
    llvm::Type *vector_type) {

  uint64_t vector_elements = vector_type->getVectorNumElements();

  uint64_t leftovers = array_num_elements % vector_elements;
  uint64_t chunks = (array_num_elements - leftovers) / vector_elements;
  if (leftovers != 0)
    chunks += 1;

  std::vector<llvm::Value *> output(chunks);

  llvm::Type *vector_element_type = vector_type->getVectorElementType();
  llvm::Type *array_element_type = ptr->getType()->getPointerElementType();

  if (vector_element_type != array_element_type) { // the types are not equal
    if (vector_element_type
            ->isFloatingPointTy()) { // we can totally cast int to float
      for (uint64_t i = 0; i < chunks; i++) {
        for (uint64_t j = 0; j < vector_elements; j++) {
          llvm::Value *array_element = builder.CreateFPCast(
              builder.CreateLoad(builder.CreateGEP(
                  ptr, builder.getInt64(i * vector_elements + j))),
              vector_element_type);
          output[i] = builder.CreateInsertElement(output[i], array_element, j,
                                                  ptr->getName() + ".pack." +
                                                      std::to_string(j));
        }
      }

      return output;
    }

    if (vector_element_type
            ->isIntegerTy()) { // Don't want to cast float to int (yet)
      throw std::runtime_error(
          "Attempting to cast float to int, not yet supported");
    }
  }

  for (uint64_t i = 0; i < chunks; i++) {
    for (uint64_t j = 0; j < vector_elements; j++) {
      llvm::Value *array_element = builder.CreateLoad(
          builder.CreateGEP(ptr, builder.getInt64(i * vector_elements + j)));
      output[i] = builder.CreateInsertElement(output[i], array_element, j,
                                              ptr->getName() + ".pack." +
                                                  std::to_string(j));
    }
  }

  return output;
}

// EWiseProduct /////////////////

llvm::ArrayRef<llvm::Value *>
Hobbit::ElementWiseProduct::Emit(llvm::IRBuilder<> &builder, llvm::Value *lhs,
                                 uint64_t &lhs_size, llvm::Value *rhs,
                                 uint64_t &rhs_size, llvm::Type *vector_type) {

  llvm::Type *lhs_type = lhs->getType();
  llvm::Type *rhs_type = rhs->getType();

  llvm::ArrayRef<llvm::Value *> lhs_chunked, rhs_chunked;
  if (lhs_type->isPointerTy()) {
    lhs_chunked = PtrVectorPack_(builder, lhs, lhs_size, vector_type);
  } else if (lhs_type->isArrayTy()) {
    lhs_chunked = ArrayVectorPack_(builder, lhs, vector_type);
  }

  if (rhs_type->isPointerTy()) {
    rhs_chunked = PtrVectorPack_(builder, rhs, rhs_size, vector_type);
  } else if (rhs_type->isArrayTy()) {
    rhs_chunked = ArrayVectorPack_(builder, rhs, vector_type);
  }
}

llvm::Value *
Hobbit::ElementWiseProduct::SequenceFMul_(llvm::IRBuilder<> &builder,
                                          llvm::Value *lhs, llvm::Value *rhs) {

  uint64_t sequence_len = lhs->getType()->getArrayNumElements();

  llvm::Value *output = llvm::UndefValue::get(lhs->getType());

  for (uint64_t i = 0; i < sequence_len; i++) {
    llvm::Value *lhs_elt = builder.CreateExtractValue(lhs, i);
    llvm::Value *rhs_elt = builder.CreateExtractValue(rhs, i);
    llvm::Value *result = builder.CreateFMul(lhs_elt, rhs_elt);
    output = builder.CreateInsertValue(output, result, i);
  }

  return output;
}

llvm::Value *
Hobbit::ElementWiseProduct::SequenceMul_(llvm::IRBuilder<> &builder,
                                         llvm::Value *lhs, llvm::Value *rhs) {

  uint64_t sequence_len = lhs->getType()->getArrayNumElements();

  llvm::Value *output = llvm::UndefValue::get(lhs->getType());

  for (uint64_t i = 0; i < sequence_len; i++) {
    llvm::Value *lhs_elt = builder.CreateExtractValue(lhs, i);
    llvm::Value *rhs_elt = builder.CreateExtractValue(rhs, i);
    llvm::Value *result = builder.CreateMul(lhs_elt, rhs_elt);
    output = builder.CreateInsertValue(output, result, i);
  }

  return output;
}

llvm::Value *Hobbit::ElementWiseProduct::VectorFMul_(llvm::IRBuilder<> &builder,
                                                     llvm::Value *lhs,
                                                     llvm::Value *rhs) {
  return builder.CreateFMul(lhs, rhs, "ewise_product.vector.fmul");
}

llvm::Value *Hobbit::ElementWiseProduct::VectorMul_(llvm::IRBuilder<> &builder,
                                                    llvm::Value *lhs,
                                                    llvm::Value *rhs) {
  return builder.CreateMul(lhs, rhs, "ewise_product.vector.mul");
}
