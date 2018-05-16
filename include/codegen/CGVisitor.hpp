//
// Created by Aman LaChapelle on 5/14/18.
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

#ifndef HOBBIT_CGVISITOR_HPP
#define HOBBIT_CGVISITOR_HPP

// STL
#include <list>
#include <set>

#include <graph/Node.hpp>
#include <llvm/Support/Casting.h>

namespace llvm {
class raw_ostream;
}

namespace Hobbit {

namespace graph {
class Node;
class Variable;
class Operation;
} // namespace graph

namespace ops {
class Operator;
}

namespace codegen {

class CGVisitor {
public:
  CGVisitor(codegen::Module *module, std::set<graph::Variable *> &args,
            std::list<graph::Operation *> &ops)
      : m_module_(std::move(module)), m_args_(args), m_ops_(ops) {}

  void VisitTree() {
    for (auto &op : m_ops_) {
      ResolveDependencies_(op);
    }
  }

private:
  void ResolveDependencies_(graph::Operation *op) {
    std::vector<graph::Variable *> in_vars;
    for (auto &in : op->Inputs()) {
      graph::Variable *in_var;
      if ((in_var = llvm::dyn_cast<graph::Variable>(in))) {
        LOG(INFO) << "Variable: " << in_var->GetName();
        // if it's a variable, then we're done.
        in_vars.push_back(in_var);
        continue;
      }
      // if it's an Operation, then I need to get the output variable
      LOG(INFO) << "Operation: " << in->GetName();
      in_var = llvm::cast<graph::Operation>(in)->GetOp()->GetOutputVariable();
      in_vars.push_back(in_var);
    }
    op->SetOp(ops::Create(op->GetOperatorType(), m_module_, in_vars));
  }

private:
  codegen::Module *m_module_;
  std::list<graph::Operation *> m_ops_;
  std::set<graph::Variable *> m_args_;
};

} // namespace codegen
} // namespace Hobbit

#endif // HOBBIT_CGVISITOR_HPP
