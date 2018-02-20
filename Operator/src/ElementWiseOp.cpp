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

Hobbit::ElementWiseOp::ElementWiseOp(llvm::LLVMContext &ctx) {
  this->constant = nullptr;
}

llvm::Value *Hobbit::ElementWiseOp::ArrayVectorPack_(llvm::IRBuilder<> &builder,
                                                     llvm::Value *array,
                                                     llvm::Type *vector_type) {

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

llvm::Value *Hobbit::ElementWiseOp::PtrVectorPack_(llvm::IRBuilder<> &builder,
                                                   llvm::Value *ptr,
                                                   llvm::Type *vector_type) {
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

// EWiseProduct /////////////////

Hobbit::ElementWiseProduct::ElementWiseProduct(llvm::LLVMContext &ctx)
    : ElementWiseOp(ctx) {}

llvm::Value *Hobbit::ElementWiseProduct::Emit(llvm::IRBuilder<> &builder,
                                              llvm::Value *lhs,
                                              llvm::Value *rhs,
                                              llvm::Type *vector_type) {
  llvm::Type *lhs_type = lhs->getType();
  llvm::Type *rhs_type = rhs->getType();

  if (lhs_type->isVectorTy() || rhs_type->isVectorTy()) {
    if (!lhs_type->isVectorTy() && lhs_type->isArrayTy()) {
      lhs = ArrayVectorPack_(builder, lhs, rhs_type);
      lhs_type = rhs_type;
    } else if (!lhs_type->isVectorTy() && lhs_type->isPointerTy()) {
      lhs = PtrVectorPack_(builder, lhs, rhs_type);
      lhs_type = rhs_type;
    } else if (!rhs_type->isVectorTy() && rhs_type->isArrayTy()) {
      rhs = ArrayVectorPack_(builder, rhs, lhs_type);
      rhs_type = lhs_type;
    } else if (!rhs_type->isVectorTy() && rhs_type->isPointerTy()) {
      rhs = PtrVectorPack_(builder, rhs, lhs_type);
      rhs_type = lhs_type;
    }

    // now they're both vectors
    if (lhs_type->isFPOrFPVectorTy()) {
      return VectorFMul_(builder, lhs, rhs);
    }
    if (lhs_type->isIntOrIntVectorTy()) {
      return VectorMul_(builder, lhs, rhs);
    }
  }

  if (lhs_type->isArrayTy() && rhs_type->isArrayTy()) {
    llvm::Type *lhs_elt_type = lhs_type->getArrayElementType();
    llvm::Type *rhs_elt_type = rhs_type->getArrayElementType();

    if (lhs_elt_type->isFPOrFPVectorTy() && rhs_elt_type->isFPOrFPVectorTy()) {
      return SequenceFMul_(builder, lhs, rhs);
    }
    if (lhs_elt_type->isIntOrIntVectorTy() &&
        rhs_elt_type->isIntOrIntVectorTy()) {
      return SequenceMul_(builder, lhs, rhs);
    }
  }

  return nullptr;
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
