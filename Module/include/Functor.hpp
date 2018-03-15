//
// Created by Aman LaChapelle on 2/22/18.
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

#ifndef HOBBIT_FUNCTOR_HPP
#define HOBBIT_FUNCTOR_HPP

#include <llvm/IR/IRBuilder.h>

#include "Buffer.hpp"
#include "Function.hpp"
#include "Kernel.hpp"

namespace Hobbit {
  class Workspace {
  public:
    inline void PushBuffer(Buffer buffer) {
      buffer_table_.push_back(buffer);
    }

    inline Buffer *GetBuffer(const uint64_t &i) {
      return &(buffer_table_[i]);
    }

  protected:
    std::vector<Buffer> buffer_table_;
  };

  class Functor {
  public:
    virtual void AllocOutput(llvm::BasicBlock *BB, Workspace &workspace) = 0;
    virtual Buffer *Emit(Function *f, Buffer *input, Workspace &workspace,
                      bool emit_inline) = 0;
  };

  class Dot : public Functor {
  public:
    explicit Dot(Buffer *c, const uint64_t &n_elements) : c_(c), n_elements_(n_elements) {}

//    llvm::ArrayRef<Buffer *> AllocOutput(llvm::BasicBlock *BB);
//    void Emit(Function *f, Buffer *input, llvm::ArrayRef<Buffer *> output,
//                      bool emit_inline);

  private:
    Buffer *c_;
    uint64_t n_elements_;
  };
}

#endif // HOBBIT_FUNCTOR_HPP
