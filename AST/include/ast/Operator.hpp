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

#ifndef HOBBIT_OPERATOR_HPP
#define HOBBIT_OPERATOR_HPP

// Project
#include <ast/Node.hpp>

// STL
#include <string>

namespace Hobbit {

namespace ops {
  class Operator;
}

namespace ast {
class Operator : public Node {
public:
  static ast::Operator *Create(const std::string &name, ops::Operator *op) {
    ast::Operator *out = new ast::Operator(name, std::move(op));
    return out;
  }

  const std::string &GetName() const override { return m_name_; }
  NodeType GetNodeType() const override { return OperatorID; };

  static inline bool classof(const Node *node) {
    return node->GetNodeType() == OperatorID;
  }

protected:
  Operator(std::string name, ops::Operator *op)
      : m_name_(std::move(name)), m_op_(std::move(op)) {}

private:
  std::string m_name_;
  ops::Operator *m_op_;
};
} // namespace ast
} // namespace Hobbit

#endif // HOBBIT_OPERATOR_HPP
