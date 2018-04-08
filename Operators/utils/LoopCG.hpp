//
// Created by Aman LaChapelle on 4/8/18.
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

#ifndef HOBBIT_LOOPCG_HPP
#define HOBBIT_LOOPCG_HPP

#include <cstdint>
#include <string>

namespace llvm {
  class PHINode;
  class BasicBlock;
  class Value;
} // namespace llvm

namespace Hobbit {
  namespace util {
    struct LoopMD {
      uint32_t vector_width = 4;
      uint32_t unroll_count = 32;
    };

    struct LoopInfo {
      llvm::PHINode *ind_var;
      llvm::BasicBlock *head_bb;
      llvm::BasicBlock *body_bb;
      llvm::BasicBlock *tail_bb;
      llvm::BranchInst *cond;
    };

    LoopInfo EmitLoop(const std::string &name, llvm::BasicBlock *loop_prehead,
                      llvm::BasicBlock *loop_posttail, llvm::Value *loop_start,
                      llvm::Value *loop_end, llvm::Value *loop_increment);

    void AddLoopMetadata(llvm::BranchInst *loop_end_br, LoopMD loopMD);
  } // namespace util
} // namespace Hobbit

#endif // HOBBIT_LOOPCG_HPP
