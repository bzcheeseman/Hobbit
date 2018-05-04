//
// Created by Aman LaChapelle on 5/3/18.
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

#include <codegen/Visitor.hpp>

void Hobbit::codegen::Visitor::BuildTree(Hobbit::graph::Node *root) { // last node

  if (root == nullptr) {
    return;
  }

  graph::Variable *node_var;
  if ((node_var = llvm::dyn_cast<graph::Variable>(root))) {
    m_args_.insert(node_var);
    BuildTree(node_var->Creator());
  }

  graph::Operation *node_op;
  if ((node_op = llvm::dyn_cast<graph::Operation>(root))) {
    m_ops_.push_front(node_op);
    for (auto &input : node_op->Inputs()) {
      BuildTree(input);
    }
  }
}

Hobbit::codegen::Function Hobbit::codegen::Visitor::GetWrapperFunction(const std::string &name, unsigned int addrspace) {
  llvm::Type *ret_type = (*--m_ops_.end())->GetOp()->GetOutputType(); //->getPointerTo(addrspace); // TODO: do I want double ptr?
  std::vector<llvm::Type *> arg_types;
  for (auto &arg : m_args_) {
    arg_types.push_back(arg->GetType()->getPointerTo(addrspace));
  }
  arg_types.push_back(ret_type->getPointerTo(addrspace));

  Function f = {name, arg_types};

  return f;
}

llvm::raw_ostream &Hobbit::codegen::operator<<(llvm::raw_ostream &os, Hobbit::codegen::Visitor &v) {

  for (auto &node : v.m_ops_) {
    os << "Operation: " << node->GetName() << "\n";
    os << "\tOpArgs:\n";
    for (auto &arg : node->Inputs()) {
      os << "\t\t" << arg->GetName() << "\n";
    }
  }

  return os;
}
