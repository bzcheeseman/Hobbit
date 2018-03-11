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

#ifndef HOBBIT_KERNEL_HPP
#define HOBBIT_KERNEL_HPP

#include "Function.hpp"
#include <Buffer.hpp>

namespace Hobbit {
  namespace internal {
    class Kernel {
    public:
      virtual void Emit(llvm::BasicBlock *BB,
                        llvm::ArrayRef<Buffer *> inputs,
                        llvm::ArrayRef<Buffer *> outputs,
                        llvm::Value *idx) = 0;
    };

    class ElementWiseProduct : public Kernel {
    public:
      explicit ElementWiseProduct(const uint64_t &elts_per_call);

      void Emit(llvm::BasicBlock *BB, llvm::ArrayRef<Buffer *> inputs,
                llvm::ArrayRef<Buffer *> outputs,
                llvm::Value *idx) override;
    private:
      uint64_t elts_per_call_;
    };

    class SumReduction : public Kernel {
    public:
      explicit SumReduction(const uint64_t &elts_per_call);

      void Emit(llvm::BasicBlock *BB, llvm::ArrayRef<Buffer *> inputs,
                llvm::ArrayRef<Buffer *> outputs,
                llvm::Value *idx) override;

    private:
      uint64_t elts_per_call_;
    };
  }
}

#endif // HOBBIT_KERNEL_HPP
