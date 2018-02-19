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

bool Hobbit::ElementWiseOp::SetConstant(llvm::Value *constant) {
  if (this->constant != nullptr) return false;

  this->constant = std::move(constant);
  return true;
}

llvm::Value *Hobbit::ElementWiseOp::ArrayVectorPack_(
        llvm::IRBuilder<> &builder, llvm::Value *array, llvm::Type *vector_type) {

  uint64_t vector_elements = vector_type->getVectorNumElements();

  llvm::Value *output = llvm::UndefValue::get(vector_type);

  llvm::Type *vector_element_type = vector_type->getVectorElementType();
  llvm::Type *array_element_type = array->getType()->getArrayElementType();

  // make sure the if the vector type is float then everyone's a float...otherwise break
  if (vector_element_type->isFPOrFPVectorTy() && !array_element_type->isFPOrFPVectorTy()) {
    for (uint64_t i = 0; i < vector_elements; i++) {
      llvm::Value *array_element = builder.CreateFPCast(
              builder.CreateExtractValue(array, i),
              vector_element_type
      );
      output = builder.CreateInsertElement(output, array_element, i,
                                           array->getName() + ".pack");
    }

    return output;
  }

  if (vector_element_type->isIntOrIntVectorTy() && array_element_type->isFPOrFPVectorTy()) {
    throw std::runtime_error("Attempting to cast float to int, not yet supported");
  }

  for (uint64_t i = 0; i < vector_elements; i++) {
    llvm::Value *array_element = builder.CreateExtractValue(array, i);
    output = builder.CreateInsertElement(output, array_element, i,
                                         array->getName() + ".pack");
  }

  return output;
}

llvm::Value *
Hobbit::ElementWiseOp::PtrVectorPack_(llvm::IRBuilder<> &builder, llvm::Value *ptr, llvm::Type *vector_type) {
  uint64_t vector_elements = vector_type->getVectorNumElements();

  llvm::Value *output = llvm::UndefValue::get(vector_type);

  llvm::Type *vector_element_type = vector_type->getVectorElementType();

  for (uint64_t i = 0; i < vector_elements; i++) {
    llvm::Value *ptr_element = builder.CreateFPCast( // force the pointer values to the right type
            builder.CreateLoad(builder.CreateGEP(ptr, builder.getInt64(i))),
            vector_element_type
    );
    output = builder.CreateInsertElement(output, ptr_element, i,
                                         ptr->getName() + ".pack");
  }

  return output;
}

llvm::Type *Hobbit::ElementWiseOp::Geti1VectorType(int width) {
  switch (width) {
    case 64: return i1x64;
    case 128: return i1x128;
    case 256: return i1x256;
    default: throw std::runtime_error("Only i1x64, i1x128, i1x256 allowed");
  }
}

llvm::Type *Hobbit::ElementWiseOp::Geti8VectorType(int width) {
  switch (width) {
    case 8: return i8x8;
    case 16: return i8x16;
    case 32: return i8x32;
    default: throw std::runtime_error("Only i8x8, i8x16, i8x32 allowed");
  }
}

llvm::Type *Hobbit::ElementWiseOp::Geti16VectorType(int width) {
  switch (width) {
    case 4: return i16x4;
    case 8: return i16x8;
    case 16: return i16x16;
    default: throw std::runtime_error("Only i16x4, i16x8, i16x16 allowed");
  }
}

llvm::Type *Hobbit::ElementWiseOp::Geti32VectorType(int width) {
  switch (width) {
    case 2: return i32x2;
    case 4: return i32x4;
    case 8: return i32x8;
    default: throw std::runtime_error("Only i32x2, i32x4, i32x8 allowed");
  }
}

llvm::Type *Hobbit::ElementWiseOp::Geti64VectorType(int width) {
  switch (width) {
    case 2: return i64x2;
    case 4: return i64x4;
    default: throw std::runtime_error("Only i64x2, i64x4 allowed");
  }
}

llvm::Type *Hobbit::ElementWiseOp::Getf16VectorType(int width) {
  switch (width) {
    case 4: return f16x4;
    case 8: return f16x8;
    case 16: return f16x16;
    default: throw std::runtime_error("Only f16x4, f16x8, f16x16 allowed");
  }
}

llvm::Type *Hobbit::ElementWiseOp::Getf32VectorType(int width) {
  switch (width) {
    case 2: return f32x2;
    case 4: return f32x4;
    case 8: return f32x8;
    default: throw std::runtime_error("Only f32x2, f32x4, f32x8 allowed");
  }
}

llvm::Type *Hobbit::ElementWiseOp::Getf64VectorType(int width) {
  switch (width) {
    case 2: return f64x2;
    case 4: return f64x4;
    default: throw std::runtime_error("Only f64x2, f64x4 allowed");
  }
}

// EWiseProduct /////////////////

Hobbit::ElementWiseProduct::ElementWiseProduct(llvm::LLVMContext &ctx)
    : ElementWiseOp(ctx) {}

llvm::Value *Hobbit::ElementWiseProduct::Emit(llvm::IRBuilder<> &builder, llvm::Value *input, llvm::Type *vector_type) {
  llvm::Type *constant_type = this->constant->getType();

  llvm::Type *input_type = input->getType();

  if (vector_type != nullptr || constant_type->isVectorTy()) {
    // Check the constant
    if (!constant_type->isVectorTy() && constant_type->isArrayTy()) {
      this->constant = ArrayVectorPack_(builder, this->constant, vector_type);
    }
    else if (!constant_type->isVectorTy() && constant_type->isPointerTy()) {
      this->constant = PtrVectorPack_(builder, this->constant, vector_type);
    }

    // Check the input
    if (!input_type->isVectorTy() && input_type->isArrayTy()) {
      input = ArrayVectorPack_(builder, input, vector_type);
    }
    else if (!input_type->isVectorTy() && input_type->isPointerTy()) {
      input = PtrVectorPack_(builder, input, vector_type);
    }

    if (constant_type->isFPOrFPVectorTy() && input_type->isFPOrFPVectorTy()) {
      return this->VectorFMul_(builder, this->constant, input);
    }
    return this->VectorMul_(builder, this->constant, input);
  }

  if (constant_type->isArrayTy() && input_type->isArrayTy()) {

    if (constant_type->getArrayElementType() != input_type->getArrayElementType()) throw std::runtime_error("Unequal array types");

    if (constant_type->getArrayElementType()->isFPOrFPVectorTy() && input_type->getArrayElementType()->isFPOrFPVectorTy()) {
      return this->SequenceFMul_(builder, this->constant, input);
    }

    return this->SequenceMul_(builder, this->constant, input);
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

llvm::Value *
Hobbit::ElementWiseProduct::VectorFMul_(llvm::IRBuilder<> &builder,
                                          llvm::Value *lhs, llvm::Value *rhs) {
  return builder.CreateFMul(lhs, rhs, "ewise_product.vector.fmul");
}

llvm::Value *
Hobbit::ElementWiseProduct::VectorMul_(llvm::IRBuilder<> &builder,
                                         llvm::Value *lhs, llvm::Value *rhs) {
  return builder.CreateMul(lhs, rhs, "ewise_product.vector.mul");
}
