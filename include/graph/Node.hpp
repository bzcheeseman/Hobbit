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

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>

#include <ops/Operator.hpp>

#include "DataStorage.hpp"

namespace llvm {
class raw_ostream;
}

namespace Hobbit {
namespace codegen {
  class Visitor;
}
namespace graph {

class Node {
public:
  enum NodeType {
#include "NodeTypes.def"
  };

  explicit Node(const std::string &name) : m_name_(name) {}

  const std::string &GetName() const { return m_name_; }
  virtual NodeType GetNodeType() const = 0;

//  virtual void Accept(Visitor &v) = 0;

  virtual void Print(llvm::raw_ostream &os) const = 0;

protected:
  std::string m_name_;
};

class Operation;

class Variable : public Node { // TODO: only create through Module
public:
  Variable(const std::string &name, llvm::Type *type=nullptr, Node *creator=nullptr)
          : Node(name), m_shape_(nullptr), m_val_(nullptr), m_type_(type), m_creator_(creator) {}
  Variable(const std::string &name, Shape *shape, llvm::Type *type=nullptr, Node *creator=nullptr)
          : Node(name), m_shape_(std::move(shape)), m_val_(nullptr), m_type_(type), m_creator_(creator) {}
  Variable(const std::string &name, std::unique_ptr<Shape> shape, llvm::Type *type=nullptr, Node *creator=nullptr)
          : Node(name), m_shape_(std::move(shape)), m_val_(nullptr), m_type_(type), m_creator_(creator) {}

  // LLVM-style RTTI
  NodeType GetNodeType() const override { return VariableID; }
  static inline bool classof(const Node *node) {
    return node->GetNodeType() == VariableID;
  }

  void Print(llvm::raw_ostream &os) const override {
    os << "Variable: " << m_name_;
    if (m_shape_ != nullptr) {
      os << " Shape: {";
      for (uint64_t i = 0; i < m_shape_->NDim(); i++) {
        os << m_shape_->Dim(i) << ", ";
      }
      os << "}";
    }
    os << "\n";
  }

  llvm::Value *GetVal() const { return m_val_; }
  void SetVal(llvm::Value *val) { CHECK_NOTNULL(val); m_val_ = val; }

  llvm::Type *GetType() const { return m_type_; }
  void SetType(llvm::Type *type) { CHECK_NOTNULL(type); m_type_ = type; }

  Node *Creator() { return m_creator_; }

private:
  std::unique_ptr<Shape> m_shape_; // Variable owns its shape
  llvm::Type *m_type_;
  llvm::Value *m_val_;
  Node *m_creator_;
};

class Operation : public Node { // How do I use another Operation as an input?
public:
  Operation(const std::string &name, llvm::ArrayRef<Node *> inputs)
      : Node(name), m_inputs_(inputs.begin(), inputs.end()), m_op_(nullptr) {}

  // LLVM-style RTTI
  NodeType GetNodeType() const override { return OperatorID; }
  static inline bool classof(const Node *node) {
    return node->GetNodeType() == OperatorID;
  }

  void Print(llvm::raw_ostream &os) const override {
    os << "Operator: " << m_name_ << " - Inputs:\n";
    for (auto &input : m_inputs_) {
      input->Print(os);
    }
  }

  const ops::Operator *GetOp() const { return m_op_; }
  void SetOp(ops::Operator *op) { CHECK_NOTNULL(op); m_op_ = std::move(op); }

  llvm::ArrayRef<Node *> Inputs() { return m_inputs_; }

private:
  llvm::SmallVector<Node *, 5> m_inputs_;
  ops::Operator *m_op_;
};

} // namespace graph
} // namespace Hobbit

#endif // HOBBIT_NODE_HPP
