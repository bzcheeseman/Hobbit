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
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>

#include <glog/logging.h>

namespace llvm {
class BasicBlock;
class PHINode;
class BranchInst;
class Value;
class Type;
class Module;
class Function;
} // namespace llvm

namespace Hobbit {
class Visitor;

const uint8_t ALIGNMENT = 32;

namespace ast {
class Tensor;

class Node {
public:
  enum NodeType {
#include "NodeTypes.def"
  };

  virtual const std::string &GetName() { return name_; }

  virtual Node *AddInput(Node *n) {
    inputs_.push_back(n);
    return this;
  }

  virtual Node *GetInput(int which) { return inputs_[which]; }

  virtual Node *SetInputs(llvm::ArrayRef<Node *> nodes) {
    inputs_ = llvm::SmallVector<Node *, 3>(nodes.begin(), nodes.end());
    return this;
  }

  virtual llvm::ArrayRef<Node *> GetInputs() { return inputs_; }

  virtual NodeType GetNodeType() const = 0;

  virtual void
  AcceptVisitor(Visitor *) = 0; // provides codegen function/info for this node

protected:
  std::string name_;

  llvm::SmallVector<Node *, 3> inputs_; // Tensors are nodes
};

class Tensor : public Node {
public:
  static Tensor *Create(const std::string &name, Tensor *parent,
                        llvm::ArrayRef<uint64_t> dims, llvm::Type *type);

  NodeType GetNodeType() const override { return VariableID; }

  static inline bool classof(const Node *node) {
    return node->GetNodeType() == VariableID;
  }

  llvm::Type *GetType();

  llvm::Value *GetValue();
  void SetValue(llvm::Value *value);

  // Gets the number of dimensions for a tensor
  uint64_t NDim();

  // Gets a dimension of a tensor
  uint64_t Dim(uint64_t which);

  // Gets the overall size of a tensor
  uint64_t Size();

  // for calculating the index of an item in a shaped flat buffer
  uint64_t At(llvm::ArrayRef<uint64_t> idx);

  llvm::Value *At(llvm::ArrayRef<llvm::Value *> idx, llvm::BasicBlock *BB);

  void AcceptVisitor(Visitor *) override { ; } // codegen visitors do nothing

  // Collapse all the dimensions of this tensor into a single dimension,
  // returns pointer to this tensor
  Tensor *Flatten();

private:
  Tensor(llvm::Type *type, llvm::ArrayRef<uint64_t> dims,
         llvm::ArrayRef<uint64_t> start_idx = {0});
  virtual ~Tensor();

private:
  llvm::Value *llvm_value_;
  llvm::Type *llvm_type_;
  llvm::SmallVector<uint64_t, 4> dims_;
  llvm::SmallVector<llvm::ConstantInt *, 4> value_dims_;
  llvm::SmallVector<uint64_t, 4> start_idx_;

  std::vector<Tensor *> children_;

  Tensor *parent_;
};

// TODO (Aman): class Constant : public Tensor
} // namespace ast
} // namespace Hobbit

#endif // HOBBIT_NODE_HPP
