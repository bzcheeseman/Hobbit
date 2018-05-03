//
// Created by Aman LaChapelle on 4/29/18.
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

#ifndef HOBBIT_AST_OPERATOR_HPP
#define HOBBIT_AST_OPERATOR_HPP

// Project
#include <ast/DataStorage.hpp>
#include <ast/Node.hpp>

// STL
#include <string>

// LLVM
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

namespace Hobbit {

namespace ops {
class Operator;
}

namespace ast {
class Operator : public Node {
public:
  static Operator *Create(const std::string &name, ops::Operator *op,
                          llvm::ArrayRef<Node *> inputs) {
    Operator *out = new ast::Operator(name, std::move(op), inputs);
    return out;
  }

  const std::string &GetName() const override { return m_name_; }
  NodeType GetNodeType() const override { return OperatorID; };

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Operator &op) {
    os << "Operator: " << op.m_name_ << " - Inputs:\n";
    for (auto &input : op.m_inputs_) {
      os << *llvm::dyn_cast<Tensor>(input);
    }
    return os;
  }

  static inline bool classof(const Node *node) {
    return node->GetNodeType() == OperatorID;
  }

  // TODO: codegen

protected:
  Operator(std::string name, ops::Operator *op, llvm::ArrayRef<Node *> inputs)
      : m_name_(std::move(name)), m_op_(std::move(op)),
        m_inputs_(inputs.begin(), inputs.end()) {}

private:
  std::string m_name_;
  ops::Operator *m_op_;
  llvm::SmallVector<Node *, 2> m_inputs_;
  llvm::SmallVector<Node *, 2> m_consumers_;
};
} // namespace ast
} // namespace Hobbit

#endif // HOBBIT_AST_OPERATOR_HPP
