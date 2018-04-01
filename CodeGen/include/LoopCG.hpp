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


#ifndef HOBBIT_PHICODEGEN_HPP
#define HOBBIT_PHICODEGEN_HPP

#include <glog/logging.h>

namespace llvm {
  class BasicBlock;
  class Value;
  class BranchInst;
}

namespace Hobbit {
  class Index;

  struct LoopMD {
    uint32_t vector_width = 4;
    uint32_t unroll_count = 1;
  };

  class LoopCG {
  public:
    explicit LoopCG(Index *idx, const LoopMD &loop_md);

    llvm::BasicBlock *EmitHead(const std::string &name, llvm::BasicBlock *phi_bb);

    llvm::Value *GetIDX(llvm::BasicBlock *BB);

    llvm::BasicBlock *EmitTail(llvm::BasicBlock *exit_bb);

  protected:

    void AddLoopMetadata(llvm::BranchInst *loop_end_br);

  private:
    Index *idx_;

    LoopMD loopmd_;

    bool head_called_;
    bool tail_called_;
  };
}


#endif //HOBBIT_PHICODEGEN_HPP
