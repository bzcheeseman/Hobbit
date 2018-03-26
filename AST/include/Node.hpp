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

#include <string>
#include <vector>

#include <llvm/ADT/SmallVector.h>

namespace llvm {
  class BasicBlock;
  class PHINode;
  class BranchInst;
  class Value;
  class Type;
  class Module;
  class Function;
}

namespace Hobbit {
  namespace ast {

    const uint8_t ALIGNMENT = 32;

    class Tensor;
    class Visitor;

    class Node {
    public:
      enum NodeType {
        TensorID,
        FunctionNodeID,
        LoopNodeID,
      };

      virtual const std::string &GetName() = 0;
      virtual Node *GetParent() = 0;

      virtual void Emit(Visitor *CG) = 0;

      virtual NodeType GetNodeType() const = 0;
    };

    class Function : public Node {
    public:
      static Function *Create(const std::string &name);

      const std::string &GetName() override;
      Node *GetParent() override { return nullptr; }

      // Add an argument to the function signature and get it so that we can
      // operate on it
      Tensor *GetNewArg(const std::string &name,
                        llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type);

      void SetArg(Tensor *t);

      void Emit(Visitor *CG) override;

      void PushNode(Node *node);

      NodeType GetNodeType() const override { return FunctionNodeID; }

      static inline bool classof(const Node *node) {
        return node->GetNodeType() == FunctionNodeID;
      }

    private:
      std::string name_;
      std::string signature_ = "";

      // For the function signature
      llvm::SmallVector<Tensor *, 4> arg_table_;
      std::vector<Node *> op_table_;

      // The last block
      llvm::BasicBlock *last_bb;
    };

    class Loop : public Node {
    public:
      // Computes the output size and creates a new Tensor that is returned when
      // the args are set.
      // Throws an error if none of the dimensions of the input tensors match
      // the range start and end.
      virtual Tensor *SetArgs(llvm::SmallVector<Tensor *, 2> args) = 0;

      NodeType GetNodeType() const override { return LoopNodeID; }

      void Emit(Visitor *CG) override = 0;

      // In case we have analytics that want to know if the loop is a reduction
      virtual bool GetIsReduction() = 0;

      static inline bool classof(const Node *node) {
        return node->GetNodeType() == LoopNodeID;
      }

    protected:
      Loop(const std::string &name, Node *parent, uint64_t range_start,
           uint64_t range_end);

      // where to put the IR for the actual loop
      // GEMM will be a bunch of nested DOTs (for example)
      // Elementwise operations can treat a Tensor as a flat buffer and iterate
      // over everything
      virtual void
      AddKernel(llvm::SmallVector<llvm::PHINode *, 4> idx_vars) = 0;

      // Add basic blocks after the branch instance that ended the previous BB
      // Adds the phi node for the loop index
      // Can create many of these blocks for nested for loops
      llvm::PHINode *AddLoopEntry_(llvm::BasicBlock *prev_bb);

      // Increments the index and adds the BBs correctly.
      // Inserts the end condition and loop metadata as well.
      llvm::BasicBlock *AddLoopExit_(llvm::PHINode *idx_var);

      // br has getContext, getParent (for BB) - convenience function for
      // sub-functions
      // do we want to add a struct for loop options (like clang?)
      void AddLoopMetadata_(llvm::BranchInst *loop_end_br);

    protected:
      std::string name_;
      std::string signature_ = "";

      // Parent node (like containing node) - loop within a loop or loop within
      // a function for example
      Node *parent_;

      // The input tensors
      llvm::SmallVector<Tensor *, 2> args_;

      // The output tensors
      llvm::SmallVector<Tensor *, 1> out_;

      // Loop characteristics
      uint64_t range_start_;
      uint64_t range_end_;
      uint64_t dim_idx_;
    };

    class HSum : public Loop {
    public:
      static HSum *Create(const std::string &name, Node *parent,
                          uint64_t range_start, uint64_t range_end);

      Tensor *SetArgs(llvm::SmallVector<Tensor *, 2> args) override;

      void Emit(Visitor *CG) override;

      const std::string &GetName() override;
      Node *GetParent() override { return parent_; }

      bool GetIsReduction() override { return true; }

    protected:
      HSum(const std::string &name, Node *parent, uint64_t range_start,
           uint64_t range_end);
      void AddKernel(llvm::SmallVector<llvm::PHINode *, 4> idx_vars) override;
    };

    class Tensor : public Node {
    public:
      static Tensor *CreateVariable(const std::string &name, Node *parent,
                                    llvm::SmallVector<uint64_t, 4> dims,
                                    llvm::Type *type);
      static Tensor *CreateConstant(const std::string &name, Node *parent,
                                    llvm::SmallVector<uint64_t, 4> dims,
                                    llvm::Type *type, void *buffer);

      NodeType GetNodeType() const override { return TensorID; }

      static inline bool classof(const Node *node) {
        return node->GetNodeType() == TensorID;
      }

      // Gets the parent/creator node for this tensor (for a function arg, it
      // would be a Function*, for example)
      Node *GetParent() override;
      llvm::Type *GetType();
      const std::string &GetName() override;
      inline void Emit(Visitor *CG) override {
        ;
      }

      void SetBuffer(llvm::Value *val);

      // Gets the number of dimensions for a tensor
      uint64_t NDim();
      // Gets a dimension of a tensor
      uint64_t Dim(uint64_t which);
      // Gets the overall size of a tensor
      uint64_t Size();

      // Get a chip from a tensor (that references the current tensor)
      Tensor *Chip(llvm::BasicBlock *BB,
                   llvm::SmallVector<uint64_t, 4> start_idx,
                   llvm::SmallVector<uint64_t, 4> dims);
      Tensor *Chip(llvm::BasicBlock *BB,
                   llvm::SmallVector<llvm::Value *, 4> start_idx,
                   llvm::SmallVector<uint64_t, 4> dims);

      // Collapse all the dimensions of this tensor into a single dimension,
      // returns pointer to this tensor
      Tensor *Flatten();

      // ----- Should these not be in an AST?
      // GEP an element of a tensor
      llvm::Value *GEP(llvm::BasicBlock *BB,
                       llvm::SmallVector<uint64_t, 4> idx);
      llvm::Value *GEP(llvm::BasicBlock *BB,
                       llvm::SmallVector<llvm::Value *, 4> idx);
      // GEP an element as if the tensor was a flat buffer
      llvm::Value *GEP(llvm::BasicBlock *BB, uint64_t raw_idx);
      llvm::Value *GEP(llvm::BasicBlock *BB, llvm::Value *raw_idx);

      // Load an element of a tensor (wraps GEP with a builder.CreateLoad call)
      llvm::Value *Load(llvm::BasicBlock *BB,
                        llvm::SmallVector<uint64_t, 4> idx);
      llvm::Value *Load(llvm::BasicBlock *BB,
                        llvm::SmallVector<llvm::Value *, 4> idx);
      // Load an element as if the tensor was a flat buffer
      llvm::Value *Load(llvm::BasicBlock *BB, uint64_t raw_idx);
      llvm::Value *Load(llvm::BasicBlock *BB, llvm::Value *raw_idx);

      // Store an element of a tensor (wraps GEP with a builder.CreateStore
      // call)
      void Store(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> idx,
                 llvm::Value *val);
      void Store(llvm::BasicBlock *BB, llvm::SmallVector<llvm::Value *, 4> idx,
                 llvm::Value *val);
      // Store an element as if the tensor was a flat buffer
      void Store(llvm::BasicBlock *BB, uint64_t raw_idx, llvm::Value *val);
      void Store(llvm::BasicBlock *BB, llvm::Value *raw_idx, llvm::Value *val);

      // elementwise operations?
    private:
      Tensor(llvm::Value *ptr, llvm::SmallVector<uint64_t, 4> dims,
             llvm::Type *type);

      // for calculating the index of an item in a shaped flat buffer
      uint64_t At_(llvm::SmallVector<uint64_t, 4> idx);
      llvm::Value *AtVal_(llvm::BasicBlock *BB,
                          llvm::SmallVector<llvm::Value *, 4> idx);

    private:
      std::string name_;
      std::string signature_ = "";

      // can store constant array here
      llvm::Value *llvm_buffer_;
      llvm::Type *llvm_type_;
      llvm::SmallVector<uint64_t, 4> dims_;

      // not sure if this is necessary?
      Node *parent_;
    };
  }
}

#endif // HOBBIT_NODE_HPP
