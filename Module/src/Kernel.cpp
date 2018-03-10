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
    const llvm::ArrayRef<Buffer *> &outputs, llvm::Value *idx) {

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

void Hobbit::internal::SumReduction::Emit(
    llvm::BasicBlock *BB, const llvm::ArrayRef<Buffer *> &inputs,
    const llvm::ArrayRef<Buffer *> &outputs, llvm::Value *idx) { // Works inline but not in loop...

  llvm::IRBuilder<> builder(BB);

  llvm::Value *output = outputs[0]->GetValue();

  llvm::Value *input_gep = builder.CreateGEP(inputs[0]->GetValue(), idx);

  llvm::Value *sum;
  if (outputs[0]->GetType()->isIntegerTy()) {
    sum = builder.CreateAdd(
            builder.CreateAlignedLoad(input_gep, 4),
            builder.CreateAlignedLoad(output, 4)
    );

  }
  else if (outputs[0]->GetType()->isFloatingPointTy()) {
    sum = builder.CreateFAdd(
            builder.CreateAlignedLoad(input_gep, 4),
            builder.CreateAlignedLoad(output, 4)
    );
  }

  builder.CreateAlignedStore(sum, output, 4);

  // outputs is sdata in the nvidia example, eventually will return the first
  // element
  // One ring to rule them all
  // One ring to find them
  // One ring to bring them all and in the darkness bind them
  // In the land of Morrrrdorrrrrr where the shadows lie
}
