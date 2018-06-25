//
// Created by Aman LaChapelle on 6/10/18.
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

#ifndef HOBBIT_OPERATION_HPP
#define HOBBIT_OPERATION_HPP

#include "Node.hpp"
#include <ops/Operator.hpp>

namespace Hobbit {
namespace graph {
class Operation : public Node { // How do I use another Operation as an input?
public:
  Operation(const std::string &name, llvm::ArrayRef<Node *> inputs,
            ops::Operator::OperatorType op_type);

  // LLVM-style RTTI
  NodeType GetNodeType() const override;
  static inline bool classof(const Node *node) {
    return node->GetNodeType() == OPERATION_ID;
  }

  void Print(llvm::raw_ostream &os) const override;

  const ops::Operator::OperatorType &GetOperatorType();

  ops::Operator *GetOp() const;
  void SetOp(ops::Operator *op);

  llvm::ArrayRef<Node *> Inputs();

  Variable *GetOutputVariable() const;

private:
  llvm::SmallVector<Node *, 5> m_inputs_;
  const ops::Operator::OperatorType m_op_type_;
  ops::Operator *m_op_;
};
} // namespace graph
} // namespace Hobbit

#endif // HOBBIT_OPERATION_HPP
