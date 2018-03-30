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
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IRBuilder.h>

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

  class Tensor;
  class Visitor;
  const uint8_t ALIGNMENT = 32;

  namespace ast {

    class Node {
    public:
      enum NodeType {
        TensorID,
        FunctionNodeID,
        LoopNodeID,
      };

      virtual const std::string &GetName() { return name_; }
      virtual Node *GetParent() { return parent_; }
      virtual Node *GetChild() { return child_; }
      virtual void SetChild(Node *node) {
        if (child_) node->SetChild(child_);
        child_ = node;
      }

      virtual void Emit(Visitor *CG) = 0;

      virtual NodeType GetNodeType() const = 0;

    protected:
      explicit Node(Node *parent = nullptr, Node *child = nullptr) : parent_(parent), child_(child) {}

      std::string name_;

      Node *parent_;
      Node *child_;
    };
  }

  class Function : public ast::Node {
  public:
    static Function *Create(const std::string &name);

    // Add an argument to the function signature and get it so that we can
    // operate on it
    Tensor *GetNewArg(const std::string &name,
                      llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type);
    Tensor *GetNewAlloca(const std::string &name,
                         llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type);

    Tensor *GetNewArg(const std::string &name,
                      llvm::SmallVector<uint64_t, 4> dims, llvm::Type
                      *type, void *buffer);

    void SetArg(Tensor *t);

    void Emit(Visitor *CG) override;

    void PushNode(Node *node);

    void SetChild(Node *node) override {
      this->PushNode(node);
    }

    NodeType GetNodeType() const override { return FunctionNodeID; }

    static inline bool classof(const Node *node) {
      return node->GetNodeType() == FunctionNodeID;
    }

  private:
    std::string name_;

    // For the function signature
    llvm::SmallVector<Tensor *, 4> arg_table_;
    llvm::SmallVector<Tensor *, 8> alloca_table_;

    Node *last_node_;
  };


  class Loop : public ast::Node {
  public:

    NodeType GetNodeType() const override { return LoopNodeID; }

    // Computes the output size and creates a new Tensor that is returned when
    // the args are set.
    // Throws an error if none of the dimensions of the input tensors match
    // the range start and end.
    virtual Tensor *SetArgs(llvm::SmallVector<Tensor *, 2> args) = 0;
    void Emit(Visitor *CG) override;

    // In case we have analytics that want to know if the loop is a reduction
    virtual bool GetIsReduction() = 0;

    static inline bool classof(const Node *node) {
      return node->GetNodeType() == LoopNodeID;
    }

  protected:
    Loop(const std::string &name, Node *parent, uint64_t range_start,
         uint64_t range_end);

    // Splits the loop according to the chunk size (like this)
    /*
     * GEMM:
     * for (int m = 0; m < M_SIZE; m += M_TILE) {
     *  for (int n = 0; n < N_SIZE; n += N_TILE) {
     *   for (int k = 0; k < K_SIZE; k++) {
     *    for (int M = 0; M < M_TILE; M++) {
     *     for (int N = 0; N < N_TILE; N++) {
     *      C[M + m][N + n] = A[M + m][k] * B[k][N + n];
     *     }
     *    }
     *   }
     *  }
     * }
     */
//    void ChunkLoop_(uint64_t chunk_size);

    // TODO: still need to nest independent loops

    // Add basic blocks after the branch instance that ended the previous BB
    // Adds the phi node for the loop index
    // Can create many of these blocks for nested for loops
    llvm::BasicBlock * AddLoopEntry_(llvm::BasicBlock *phi_bb);

    virtual void AddKernel_(llvm::BasicBlock *body_bb) = 0;

    // Increments the index and adds the BBs correctly.
    // Inserts the end condition and loop metadata as well.
    llvm::BasicBlock *AddLoopExit_(llvm::BasicBlock *body_bb);

    // br has getContext, getParent (for BB) - convenience function for
    // sub-functions
    // do we want to add a struct for loop options (like clang?)
    void AddLoopMetadata_(llvm::BranchInst *loop_end_br);

  protected:
    struct LoopInfo {
      uint64_t start = 0;
      uint64_t end = 0;

      llvm::PHINode *phi = nullptr;
      llvm::Value *increment = nullptr;
      llvm::Value *cumulative_idx = nullptr;
    };

    std::string name_;

    // The input tensors
    llvm::SmallVector<Tensor *, 2> args_;

    // The output tensors
    llvm::SmallVector<Tensor *, 1> out_;

    Loop *dependent_ = nullptr;

    // Loop characteristics
    LoopInfo loop_info_;
  };

  class HSum : public Loop {
  public:
    static HSum *Create(const std::string &name, Node *parent,
                        uint64_t range_start, uint64_t range_end);

    Tensor *SetArgs(llvm::SmallVector<Tensor *, 2> args) override;

    bool GetIsReduction() override { return true; }

  protected:
    HSum(const std::string &name, Node *parent, uint64_t range_start,
         uint64_t range_end) : Loop(name, parent, range_start, range_end) {}
    void AddKernel_(llvm::BasicBlock *body_bb) override;
  };

  class Tensor : public ast::Node {
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

    llvm::Type *GetType();

    inline void Emit(Visitor *CG) override { ; }

    void SetBuffer(llvm::Value *val);
    llvm::Value *GetBuffer();

    // Gets the number of dimensions for a tensor
    uint64_t NDim();
    // Gets a dimension of a tensor
    uint64_t Dim(uint64_t which);
    // Gets the overall size of a tensor
    uint64_t Size();

    // Get a chip from a tensor (that references the current tensor)
    Tensor *Chip(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> start_idx,
                 llvm::SmallVector<uint64_t, 4> dims);
    Tensor *Chip(llvm::BasicBlock *BB,
                 llvm::SmallVector<llvm::Value *, 4> start_idx,
                 llvm::SmallVector<uint64_t, 4> dims);

    // Collapse all the dimensions of this tensor into a single dimension,
    // returns pointer to this tensor
    Tensor *Flatten();

    // ----- Should these not be in an AST?
    // GEP an element of a tensor
    llvm::Value *GEP(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> idx);
    llvm::Value *GEP(llvm::BasicBlock *BB,
                     llvm::SmallVector<llvm::Value *, 4> idx);
    // GEP an element as if the tensor was a flat buffer
    llvm::Value *GEP(llvm::BasicBlock *BB, uint64_t raw_idx);
    llvm::Value *GEP(llvm::BasicBlock *BB, llvm::Value *raw_idx);

    // Load an element of a tensor (wraps GEP with a builder.CreateLoad call)
    llvm::Value *Load(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> idx);
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

    // can store constant array here
    llvm::Value *llvm_buffer_;
    llvm::Type *llvm_type_;
    llvm::SmallVector<uint64_t, 4> dims_;
  };
}

#endif // HOBBIT_NODE_HPP
