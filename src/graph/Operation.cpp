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

#include <graph/Operation.hpp>

// glog
#include <glog/logging.h>

Hobbit::graph::Operation::Operation(
    const std::string &name, llvm::ArrayRef<Hobbit::graph::Node *> inputs,
    Hobbit::ops::Operator::OperatorType op_type)
    : Node(name), m_inputs_(inputs.begin(), inputs.end()), m_op_type_(op_type),
      m_op_(nullptr) {}

Hobbit::graph::Node::NodeType Hobbit::graph::Operation::GetNodeType() const {
  return OPERATION_ID;
}

void Hobbit::graph::Operation::Print(llvm::raw_ostream &os) const {
  os << "Operator: " << m_name_ << " - Inputs:\n";
  for (auto &input : m_inputs_) {
    input->Print(os);
  }
}

const Hobbit::ops::Operator::OperatorType &
Hobbit::graph::Operation::GetOperatorType() {
  return m_op_type_;
}

Hobbit::ops::Operator *Hobbit::graph::Operation::GetOp() const { return m_op_; }

void Hobbit::graph::Operation::SetOp(Hobbit::ops::Operator *op) {
  CHECK_NOTNULL(op);
  m_op_ = std::move(op);
}

llvm::ArrayRef<Hobbit::graph::Node *> Hobbit::graph::Operation::Inputs() {
  return m_inputs_;
}

Hobbit::graph::Variable *Hobbit::graph::Operation::GetOutputVariable() const {
  return m_op_->GetOutputVariable();
}
