//
// Created by Aman LaChapelle on 3/22/18.
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


#ifndef HOBBIT_NODE_HPP
#define HOBBIT_NODE_HPP

namespace Hobbit {
  namespace ast {
    class Node {
      virtual const std::string &GetName() = 0;
      virtual const std::string &GetSignature() = 0; // for printing
    };

    class Function : public Node {
    public:
      void PushNode(Node *node);

    private:
      std::vector<Node *> op_table_;
    };

    class Tensor : public Node { // what methods does a Tensor need?
    public:
      llvm::Value *At(llvm::SmallVector<uint64_t, 4> idx);

    private:

    private:
      llvm::Value *llvm_buffer_; // can store constant array here
      llvm::SmallVector<uint64_t, 4> dims_;
    };

    class Loop : public Node {
    public:
      static Loop *Create(Node *parent, uint64_t range_start, uint64_t range_end, bool is_reduction);
      void AddArgs(llvm::SmallVector<Tensor *, 2> args);

    protected:
      virtual void AddKernel(llvm::BasicBlock *BB) = 0; // where to put the IR
      void AddLoopMetadata(llvm::BranchInst *loop_end_br); // br has getContext, getParent (for BB)

    protected:
      // Parent node (like containing node) - loop within a loop or loop within a function for example
      Node *parent_;

      // The input tensors
      llvm::SmallVector<Tensor *, 2> args_;

      // Loop characteristics
      uint64_t range_start_;
      uint64_t range_end_;
      bool redux_;
    };
  }
}


#endif //HOBBIT_NODE_HPP
