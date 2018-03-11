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

void Hobbit::internal::ElementWiseProduct::Emit(
    llvm::BasicBlock *BB, const llvm::ArrayRef<Buffer *> &inputs,
    const llvm::ArrayRef<Buffer *> &outputs,
    llvm::Value *idx) {

  llvm::IRBuilder<> builder(BB);

  llvm::Value *output_gep = builder.CreateGEP(outputs[0]->GetValue(), idx);

  llvm::Value *first_input_gep = builder.CreateGEP(inputs[0]->GetValue(), idx);
  builder.CreateStore(builder.CreateLoad(first_input_gep),
                      output_gep); // store the first item

  if (outputs[0]->GetType()->isIntegerTy()) {
    for (uint64_t i = 1; i < inputs.size(); i++) {
      llvm::Value *input_gep = builder.CreateGEP(inputs[i]->GetValue(), idx);
      llvm::Value *loaded_input = builder.CreateAlignedLoad(input_gep, 4);

      llvm::Value *loaded_output = builder.CreateAlignedLoad(output_gep, 4);

      builder.CreateStore(builder.CreateMul(loaded_input, loaded_output),
                          output_gep);
    }
  } else if (outputs[0]->GetType()->isFloatingPointTy()) {
    for (uint64_t i = 1; i < inputs.size(); i++) {
      llvm::Value *input_gep = builder.CreateGEP(inputs[i]->GetValue(), idx);
      llvm::Value *loaded_input = builder.CreateAlignedLoad(input_gep, 4);

      llvm::Value *loaded_output = builder.CreateAlignedLoad(output_gep, 4);

      builder.CreateStore(builder.CreateFMul(loaded_input, loaded_output),
                          output_gep);
    }
  }
}

Hobbit::internal::SumReduction::SumReduction(const uint64_t &elts_per_call)
    : elts_per_call_(elts_per_call), stride_(1) {}

void Hobbit::internal::SumReduction::Emit(
    llvm::BasicBlock *BB, const llvm::ArrayRef<Buffer *> &inputs,
    const llvm::ArrayRef<Buffer *> &outputs,
    llvm::Value *idx) {

  llvm::IRBuilder<> builder(BB);

  llvm::Value *output = outputs[0]->GetValue();
  llvm::Value *sum = builder.CreateAlignedLoad(output, 4);

  std::vector<llvm::Value *> loaded_inputs(elts_per_call_);

  for (uint64_t i = 0; i < elts_per_call_; i++) {
    llvm::Value *index = builder.CreateAdd(idx, builder.getInt64(i)); // idx = idx + i
    loaded_inputs[i] = builder.CreateAlignedLoad(builder.CreateGEP(inputs[0]->GetValue(), index), 4);
  }

  for (auto &loaded_input : loaded_inputs) {
    if (outputs[0]->GetType()->isIntegerTy()) {
      sum = builder.CreateAdd(loaded_input, sum);
    } else if (outputs[0]->GetType()->isFloatingPointTy()) {
      sum = builder.CreateFAdd(loaded_input, sum);
    }
  }

  builder.CreateAlignedStore(sum, output, 4);
}
