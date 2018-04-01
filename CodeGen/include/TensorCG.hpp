//
// Created by Aman LaChapelle on 3/31/18.
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


#ifndef HOBBIT_TENSORCG_HPP
#define HOBBIT_TENSORCG_HPP

#include <cstdint>
#include <Node.hpp>

namespace llvm {
  class BasicBlock;
  class Value;
}

namespace Hobbit {
  class TensorCG {
  public:
    explicit TensorCG(Tensor *tensor) : tensor_(tensor) {}

    Tensor *Chip(llvm::BasicBlock *BB,
                 llvm::SmallVector<llvm::Value *, 4> start_idx,
                 llvm::SmallVector<uint64_t, 4> dims) {
      // create a new tensor that aliases this
      Tensor *chip = new Tensor(
              this->GEP(BB, start_idx), // llvm_buffer_
              std::move(dims), // dims_
              tensor_->llvm_type_ // llvm_type_
      );

      tensor_->SetChild(chip);

      return chip;
    }

    llvm::Value *GEP(llvm::BasicBlock *BB,
                                     llvm::SmallVector<uint64_t, 4> idx) {
      llvm::IRBuilder<> builder(BB);
      llvm::Value *idx_val = builder.getInt64(tensor_->At_(std::move(idx)));

      return builder.CreateInBoundsGEP(tensor_->llvm_buffer_, idx_val);
    }

    llvm::Value *GEP(llvm::BasicBlock *BB,
                                     llvm::SmallVector<llvm::Value *, 4> idx) {
      llvm::IRBuilder<> builder(BB);
      llvm::Value *idx_val = this->AtVal_(BB, idx);

      return builder.CreateInBoundsGEP(tensor_->llvm_buffer_, idx_val);
    }

    llvm::Value *GEP(llvm::BasicBlock *BB, uint64_t raw_idx) {
      llvm::IRBuilder<> builder(BB);
      llvm::Value *idx_val = builder.getInt64(raw_idx);

      return builder.CreateInBoundsGEP(tensor_->llvm_buffer_, idx_val);
    }

    llvm::Value *GEP(llvm::BasicBlock *BB, llvm::Value *raw_idx) {
      llvm::IRBuilder<> builder(BB);

      return builder.CreateInBoundsGEP(tensor_->llvm_buffer_, raw_idx);
    }

    llvm::Value *Load(llvm::BasicBlock *BB,
                                      llvm::SmallVector<uint64_t, 4> idx) {
      llvm::Value *gep = this->GEP(BB, std::move(idx));
      llvm::IRBuilder<> builder(BB);

      return builder.CreateAlignedLoad(gep, ALIGNMENT);
    }

    llvm::Value *Load(llvm::BasicBlock *BB,
                                      llvm::SmallVector<llvm::Value *, 4> idx) {
      llvm::Value *gep = this->GEP(BB, idx);
      llvm::IRBuilder<> builder(BB);

      return builder.CreateAlignedLoad(gep, ALIGNMENT);
    }

    llvm::Value *Load(llvm::BasicBlock *BB, uint64_t raw_idx) {
      llvm::Value *gep = this->GEP(BB, raw_idx);
      llvm::IRBuilder<> builder(BB);

      return builder.CreateAlignedLoad(gep, ALIGNMENT);
    }

    llvm::Value *Load(llvm::BasicBlock *BB, llvm::Value *raw_idx) {
      llvm::Value *gep = this->GEP(BB, raw_idx);
      llvm::IRBuilder<> builder(BB);

      return builder.CreateAlignedLoad(gep, ALIGNMENT);
    }

    void Store(llvm::BasicBlock *BB,
                               llvm::SmallVector<uint64_t, 4> idx,
                               llvm::Value *val) {
      llvm::Value *gep = this->GEP(BB, std::move(idx));
      llvm::IRBuilder<> builder(BB);

      builder.CreateAlignedStore(val, gep, ALIGNMENT);
    }

    void Store(llvm::BasicBlock *BB,
                               llvm::SmallVector<llvm::Value *, 4> idx,
                               llvm::Value *val) {
      llvm::Value *gep = this->GEP(BB, std::move(idx));
      llvm::IRBuilder<> builder(BB);

      builder.CreateAlignedStore(val, gep, ALIGNMENT);
    }

    void Store(llvm::BasicBlock *BB, uint64_t raw_idx,
                               llvm::Value *val) {
      llvm::Value *gep = this->GEP(BB, std::move(raw_idx));
      llvm::IRBuilder<> builder(BB);

      builder.CreateAlignedStore(val, gep, ALIGNMENT);
    }

    void Store(llvm::BasicBlock *BB, llvm::Value *raw_idx,
                               llvm::Value *val) {
      llvm::Value *gep = this->GEP(BB, std::move(raw_idx));
      llvm::IRBuilder<> builder(BB);

      builder.CreateAlignedStore(val, gep, ALIGNMENT);
    }

    llvm::Value *AtVal_(llvm::BasicBlock *BB,
                        llvm::SmallVector<llvm::Value *, 4> idx) {
      llvm::IRBuilder<> builder(BB);
      llvm::Value *out = builder.getInt64(0);

      for (uint64_t i = 0; i < tensor_->dims_.size() - 1; i++) {
        out = builder.CreateAdd(out, idx[i]);
        out = builder.CreateMul(out, builder.getInt64(tensor_->dims_[i + 1]));
      }
      out = builder.CreateAdd(out, *idx.end());

      return out;
    }

  private:
    Tensor *tensor_;
  };
}


#endif //HOBBIT_TENSORCG_HPP
