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

#include <glog/logging.h>

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
        IndexID,
      };

      virtual const std::string &GetName() { return name_; }
      virtual Node *GetParent() { return parent_; }
      virtual Node *GetChild() { return child_; }
      virtual void SetChild(Node *node) {
        if (child_) node->SetChild(child_);
        child_ = node;
        node->parent_ = this;
      }

      virtual NodeType GetNodeType() const = 0;

    protected:
      explicit Node(Node *parent = nullptr, Node *child = nullptr) : parent_(parent), child_(child) {}

      std::string name_;

      Node *parent_;
      Node *child_;
    };
  }

  class Function {
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

    void PushNode(ast::Node *node);

  private:
    friend class FunctionCG;

    std::string name_;

    // For the function signature
    llvm::SmallVector<Tensor *, 4> arg_table_;
    llvm::SmallVector<Tensor *, 8> alloca_table_;

    // Holds the graph in memory
    ast::Node *child_;
    ast::Node *last_node_;
  };

  class Index : public ast::Node {
  public:
    Index(uint64_t start, uint64_t end, uint64_t stride=1, bool redux=false)
            : start_(start), end_(end), stride_(stride), is_redux_(false) {}

    uint64_t GetRange() {
      return end_ - start_;
    }

    Index *Split(uint64_t chunk_size) {
      if (is_redux_) LOG(FATAL) << "Attempting to split along a reduction dimension!";

      Index *child_idx = new Index(0, chunk_size, 1);

      this->stride_ = chunk_size;

      this->SetChild(child_idx);

      return child_idx;
    }

    NodeType GetNodeType() const override { return IndexID; }

    static bool classof(Node *n) {
      return n->GetNodeType() == IndexID;
    }

  private:
    friend class LoopCG;

    uint64_t start_ = 0;
    uint64_t end_ = 0;
    uint64_t stride_ = 1;
    bool is_redux_;

    llvm::PHINode *phi_ = nullptr;
    Index *parent_ = nullptr; // you can accumulate an index by adding together the phi's of the parents
    Index *child_ = nullptr; // you can traverse in either direction
  };

  class Kernel : public ast::Node {
  public:
    explicit Kernel(llvm::SmallVector<Tensor *, 2> args) : args_(std::move(args)) {}

    // maybe we use regular C to express the kernel...

    // How to represent kernels...elementwise is fine, just store Tensor1 + Tensor2
    // I guess we can just have special sumreduce/prodreduce?

  private:
    friend class KernelCG;

    llvm::SmallVector<Tensor *, 2> args_;
  };

  class Tensor : public ast::Node {
  public:
    static Tensor CreateVariable(const std::string &name, Node *parent,
                                  llvm::SmallVector<uint64_t, 4> dims,
                                  llvm::Type *type);
    static Tensor CreateConstant(const std::string &name, Node *parent,
                                  llvm::SmallVector<uint64_t, 4> dims,
                                  llvm::Type *type, void *buffer);

    NodeType GetNodeType() const override { return TensorID; }

    static inline bool classof(const Node *node) {
      return node->GetNodeType() == TensorID;
    }

    llvm::Type *GetType();

    void SetBuffer(llvm::Value *val);
    llvm::Value *GetBuffer();

    // Gets the number of dimensions for a tensor
    uint64_t NDim();
    // Gets a dimension of a tensor
    uint64_t Dim(uint64_t which);
    // Gets the overall size of a tensor
    uint64_t Size();

    // Get a chip from a tensor (that references the current tensor)
    Tensor *Chip(llvm::SmallVector<uint64_t, 4> start_idx,
                 llvm::SmallVector<uint64_t, 4> dims);

    // Collapse all the dimensions of this tensor into a single dimension,
    // returns pointer to this tensor
    Tensor *Flatten();

//    Tensor &operator=(Tensor &other);
//    Tensor &operator+=(Tensor &other);
//    Tensor &operator-=(Tensor &other);
//    Tensor &operator*=(Tensor &other);
//    Tensor &operator/=(Tensor &other);
  private:
    friend class TensorCG;

    Tensor(llvm::Value *ptr, llvm::SmallVector<uint64_t, 4> dims,
           llvm::Type *type, llvm::SmallVector<uint64_t, 4> start_idx={0});

    // for calculating the index of an item in a shaped flat buffer
    uint64_t At_(llvm::SmallVector<uint64_t, 4> idx);

  private:
    std::string name_;

    // can store constant array here
    llvm::Value *llvm_buffer_;
    llvm::Type *llvm_type_;
    llvm::SmallVector<uint64_t, 4> dims_;
    llvm::SmallVector<uint64_t, 4> start_idx_;
  };
}

#endif // HOBBIT_NODE_HPP
