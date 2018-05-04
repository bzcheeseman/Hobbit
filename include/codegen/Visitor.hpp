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


#ifndef HOBBIT_VISITOR_HPP
#define HOBBIT_VISITOR_HPP

#include <graph/Node.hpp>
#include <llvm/Support/Casting.h>
#include <set>
#include <list>
#include "Module.hpp"

namespace Hobbit {

namespace graph {
  class Node;
  class Variable;
  class Operation;
}

namespace codegen {

bool OperationRHSDependsOnLHS(graph::Operation *lhs, graph::Operation *rhs) {
  if (lhs == rhs) return false;

  bool depends = false;
  for (auto &input : rhs->Inputs()) { // if LHS is in the inputs for RHS then RHS depends on LHS
    depends |= (input == lhs);
  }

  return depends;
}

class Visitor {
public:

  void BuildTree(graph::Node *root);
  Function GetWrapperFunction(const std::string &name, unsigned int addrspace=0);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os, Visitor &v);

private:
  void BuildTree_(graph::Node *root);
  void SortTree_();

private:
  std::list<graph::Operation *> m_ops_; // ops can only depend on ops that come before them
  std::set<graph::Variable *> m_args_;
};

}
}

#endif //HOBBIT_VISITOR_HPP
