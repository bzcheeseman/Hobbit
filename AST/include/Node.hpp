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

    class Tensor;

    class Node {
    public:
      enum NodeType {
        FunctionNodeID,
        LoopNodeID,
      };

      virtual const std::string &GetName() = 0;
      virtual const std::string &GetSignature() = 0; // for printing?

      virtual NodeType GetNodeType() const = 0;
    };

    class Function : public Node {
    public:
      static Function *Create();

      void PushArg(Tensor *arg);
      void PushNode(Node *node);

      NodeType GetNodeType() const override { return FunctionNodeID; }

      static inline bool classof(const Node *node) {
        return node->GetNodeType() == FunctionNodeID;
      }

    private:
      // For the function signature
      llvm::SmallVector<Tensor *, 4> arg_table_;
      std::vector<Node *> op_table_;
    };

    class Loop : public Node {
    public:
      static Loop *Create(Node *parent, uint64_t range_start, uint64_t range_end, bool is_reduction);

      // Computes the output size and creates a new Tensor that is returned when the args are set.
      // Throws an error if none of the dimensions of the input tensors match the range start and end.
      virtual Tensor *SetArgs(llvm::SmallVector<Tensor *, 2> args) = 0;

      NodeType GetNodeType() const { return LoopNodeID; }

      static inline bool classof(const Node *node) {
        return node->GetNodeType() == LoopNodeID;
      }

    protected:
      // where to put the IR for the actual loop
      virtual void AddKernel(llvm::PHINode *idx_var) = 0; // for gemm will end up with nested loops

      // Add basic blocks after the branch instance that ended the previous BB
      // Adds the phi node for the loop index
      llvm::PHINode *AddLoopEntry_(llvm::BranchInst *prev_bb_br);

      // Increments the index and adds the BBs correctly.
      // Inserts the end condition and loop metadata as well.
      void AddLoopExit_(llvm::PHINode *idx_var);

      // br has getContext, getParent (for BB) - convenience function for sub-functions
      void AddLoopMetadata_(llvm::BranchInst *loop_end_br);


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

    class Tensor {
    public:

      static Tensor *CreateVariable(Node *parent, llvm::SmallVector<uint64_t, 4> dims);
      static Tensor *CreateConstant(Node *parent, llvm::SmallVector<uint64_t, 4> dims, void *buffer);

      // Gets the parent/creator node for this tensor (for a function arg, it would be a Function*, for example)
      Node *GetParent();

      // Gets the number of dimensions for a tensor
      uint64_t NDim();
      // Gets a dimension of a tensor
      uint64_t Dim(uint64_t which);

      // Get a chip from a tensor (that references the current tensor)
      Tensor *Chip(llvm::SmallVector<uint64_t, 4> start_idx, llvm::SmallVector<uint64_t, 4> dims);

      // Collapse all the dimensions of this tensor into a single dimension, returns pointer to this tensor
      Tensor *Flatten();

      // ----- Should these not be in an AST?
      // GEP an element of a tensor
      llvm::Value *GEP(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> idx);
      llvm::Value *GEP(llvm::SmallVector<llvm::Value *, 4> idx);

      // Load an element of a tensor (wraps GEP with a builder.CreateLoad call)
      llvm::Value *Load(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> idx);
      llvm::Value *Load(llvm::SmallVector<llvm::Value *, 4> idx);

      // elementwise operations?
    private:
      uint64_t At_(llvm::SmallVector<int64_t, 4> idx); // for calculating the index of a shape

    private:
      // can store constant array here
      llvm::Value *llvm_buffer_;
      llvm::SmallVector<uint64_t, 4> dims_;

      // not sure if this is necessary?
      Node *parent_;
    };
  }
}


#endif //HOBBIT_NODE_HPP
