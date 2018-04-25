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
#include <utility>
#include <vector>

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/TypeBuilder.h>
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

  virtual const std::string &GetName() const = 0;
  virtual NodeType GetNodeType() const = 0;

  //  virtual llvm::BasicBlock *InsertIntoFunction(llvm::Function *) = 0;

  virtual const std::vector<Node *> &Consumers() { return mb_consumers_; }
  virtual const std::vector<Node *> &Producers() { return mb_producers_; }

  virtual std::vector<Node *> &MutableConsumers() { return mb_consumers_; }
  virtual std::vector<Node *> &MutableProducers() { return mb_producers_; }

protected:
  std::vector<Node *> mb_consumers_; // these use this node
  std::vector<Node *> mb_producers_; // this node uses these
};

// TODO (Aman): class Constant : public Node
class Variable : public Node {
public:

  static Variable *Create(const std::string &name, llvm::ArrayRef<uint64_t> dims, bool is_arg) {
    Variable *var = new Variable(name, dims, is_arg);
//    llvm::Type *type = llvm::TypeBuilder<CxxType, xcompile>::get(ctx);

    return var;
  }

  const std::string &GetName() const override { return m_name_; }
  NodeType GetNodeType() const override { return VariableID; };

  static inline bool classof(const Node *node) {
    return node->GetNodeType() == VariableID;
  }

  // TODO: codegen

protected:
  Variable(std::string name, llvm::ArrayRef<uint64_t> dims, bool is_arg) :
          m_name_(std::move(name)), m_dims_(dims.begin(), dims.end()), m_is_arg_(is_arg) {}

private:
  std::string m_name_;
  llvm::SmallVector<uint64_t, 4> m_dims_;
  bool m_is_arg_;
};

class Operator : public Node {
public:

  static ast::Operator *Create(const std::string &name, Operator &&op) {
    ast::Operator *out = new ast::Operator(name, std::move(op));
    return out;
  }

  const std::string &GetName() const override { return m_name_; }
  NodeType GetNodeType() const override { return OperatorID; };

  static inline bool classof(const Node *node) {
    return node->GetNodeType() == OperatorID;
  }

protected:
  Operator(std::string name, Operator &&op) : m_name_(std::move(name)), m_op_(std::move(op)) {}

private:
  std::string m_name_;
  Operator m_op_;
};

class Tensor : public Node {
public:
  static Tensor *Create(const std::string &name, Tensor *parent,
                        llvm::ArrayRef<uint64_t> dims, llvm::Type *type);
  // CreateArg
  // CreateConst

  const std::string &GetName() override { return name_; }

  NodeType GetNodeType() const override { return VariableID; }

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

  // Collapse all the dimensions of this tensor into a single dimension,
  // returns pointer to this tensor
  Tensor *Flatten();

private:
  Tensor(llvm::Type *type, llvm::ArrayRef<uint64_t> dims,
         llvm::ArrayRef<uint64_t> start_idx = {0});
  virtual ~Tensor();

private:
  std::string name_;

  llvm::Value *llvm_value_;
  llvm::Type *llvm_type_;
  llvm::SmallVector<uint64_t, 4> dims_;
  llvm::SmallVector<llvm::ConstantInt *, 4> value_dims_;
  llvm::SmallVector<uint64_t, 4> start_idx_;

  std::vector<Tensor *> children_;

  Tensor *parent_;
};

} // namespace ast
} // namespace Hobbit

#endif // HOBBIT_NODE_HPP
