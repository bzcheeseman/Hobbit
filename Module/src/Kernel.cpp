//
// Created by Aman LaChapelle on 3/8/18.
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

#include "Kernel.hpp"

Hobbit::internal::ElementWiseProduct::ElementWiseProduct(
    const uint64_t &elts_per_call)
    : elts_per_call_(elts_per_call) {}

void Hobbit::internal::ElementWiseProduct::Emit(
    llvm::BasicBlock *BB, llvm::ArrayRef<Buffer *> inputs,
    llvm::ArrayRef<Buffer *> outputs, llvm::Value *idx) {

  llvm::IRBuilder<> builder(BB);

  llvm::Type *type = outputs[0]->GetType();
  llvm::Type *vec_type = llvm::VectorType::get(type, elts_per_call_);

  llvm::Value *output_gep = builder.CreateBitCast(
      builder.CreateGEP(outputs[0]->GetValue(), idx), vec_type->getPointerTo());

  llvm::Value *lhs_vec = builder.CreateAlignedLoad(
      builder.CreateBitCast(builder.CreateGEP(inputs[0]->GetValue(), idx),
                            vec_type->getPointerTo()),
      32);
  llvm::Value *rhs_vec = builder.CreateAlignedLoad(
      builder.CreateBitCast(builder.CreateGEP(inputs[1]->GetValue(), idx),
                            vec_type->getPointerTo()),
      32);

  llvm::Value *result;
  if (outputs[0]->GetType()->isIntegerTy()) {
    result = builder.CreateMul(lhs_vec, rhs_vec);
  } else if (outputs[0]->GetType()->isFloatingPointTy()) {
    result = builder.CreateFMul(lhs_vec, rhs_vec);
  }

  builder.CreateAlignedStore(result, output_gep, 8);
}

Hobbit::internal::SumReduction::SumReduction(const uint64_t &elts_per_call)
    : elts_per_call_(elts_per_call) {}

void Hobbit::internal::SumReduction::Emit(llvm::BasicBlock *BB,
                                          llvm::ArrayRef<Buffer *> inputs,
                                          llvm::ArrayRef<Buffer *> outputs,
                                          llvm::Value *idx) {

  llvm::IRBuilder<> builder(BB);

  llvm::Value *input = inputs[0]->GetValue();
  llvm::Value *output = outputs[0]->GetValue();
  llvm::Type *type = outputs[0]->GetType();
  llvm::Type *vec_type = llvm::VectorType::get(type, elts_per_call_);

  llvm::Value *input_gep = builder.CreateGEP(input, idx);

  llvm::Value *loaded_vector = builder.CreateAlignedLoad(
      builder.CreateBitCast(input_gep, vec_type->getPointerTo()), 32);

  llvm::Value *sum = builder.CreateAlignedLoad(builder.CreateBitCast(output, vec_type->getPointerTo()), 32);

  if (type->isIntegerTy()) {
    sum = builder.CreateAdd(loaded_vector, sum);
  } else if (type->isFloatingPointTy()) {
    sum = builder.CreateFAdd(loaded_vector, sum);
  }

  builder.CreateAlignedStore(sum, builder.CreateBitCast(output, vec_type->getPointerTo()), 32);
}
