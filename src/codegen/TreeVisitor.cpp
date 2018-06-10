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

#include <codegen/TreeVisitor.hpp>

#include <graph/Node.hpp>
#include <graph/Variable.hpp>
#include <graph/Operation.hpp>

// glog
#include <glog/logging.h>

#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

namespace {
using namespace Hobbit;
bool OperationRHSDependsOnLHS(graph::Operation *lhs, graph::Operation *rhs) {
  if (lhs == rhs)
    return false;

  bool depends = false;
  for (auto &input : rhs->Inputs()) { // if LHS is in the inputs for RHS then
                                      // RHS depends on LHS
    depends |= (input == lhs);
  }

  return depends;
}
} // namespace

void Hobbit::codegen::TreeVisitor::BuildTree(Hobbit::graph::Node *root) {
  BuildTree_(root);
  SortTree_();
  m_tree_built_ = true;
}

std::list<graph::Operation *> &codegen::TreeVisitor::Tree() {
  if (!m_tree_built_) {
    LOG(FATAL)
        << "Dependency scheduling step not executed, tree not built or sorted";
  }
  return m_ops_;
}

std::set<graph::Variable *> &codegen::TreeVisitor::Args() {
  if (!m_tree_built_) {
    LOG(FATAL)
        << "Dependency scheduling step not executed, tree not built or sorted";
  }
  return m_args_;
}

void Hobbit::codegen::TreeVisitor::BuildTree_(Hobbit::graph::Node *root) {

  if (root == nullptr) {
    return;
  }

  graph::Variable *node_var;
  if ((node_var = llvm::dyn_cast<graph::Variable>(root))) {
    m_args_.insert(node_var);
    BuildTree_(node_var->Creator());
  }

  graph::Operation *node_op;
  if ((node_op = llvm::dyn_cast<graph::Operation>(root))) {
    m_ops_.push_back(node_op);
    for (auto &input : node_op->Inputs()) {
      BuildTree_(input);
    }
  }
}

void Hobbit::codegen::TreeVisitor::SortTree_() {
  m_ops_.sort(OperationRHSDependsOnLHS);
}

/* ------------------- Friend functions ------------------- */

llvm::raw_ostream &Hobbit::codegen::
operator<<(llvm::raw_ostream &os, Hobbit::codegen::TreeVisitor &v) {
  for (auto &node : v.m_ops_) {
    os << "Operation: " << node->GetName() << "\n";
    os << "\tOpArgs:\n";
    for (auto &arg : node->Inputs()) {
      os << "\t\t" << arg->GetName() << "\n";
    }
  }

  return os;
}

std::ostream &Hobbit::codegen::operator<<(std::ostream &os,
                                          Hobbit::codegen::TreeVisitor &v) {
  for (auto &node : v.m_ops_) {
    os << "Operation: " << node->GetName() << "\n";
    os << "\tOpArgs:\n";
    for (auto &arg : node->Inputs()) {
      os << "\t\t" << arg->GetName() << "\n";
    }
  }

  return os;
}
