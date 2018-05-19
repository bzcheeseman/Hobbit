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

namespace llvm {
class raw_ostream;
class Function;
class FunctionType;
class BasicBlock;
} // namespace llvm

namespace Hobbit {

namespace graph {
class Node;
class Variable;
class Operation;
} // namespace graph

namespace ops {
class Operator;
} // namespace ops

namespace codegen {
class Module;

class CGVisitor {
public:
  CGVisitor(codegen::Module *module, std::set<graph::Variable *> &args,
            std::list<graph::Operation *> &ops);

  void CodeGenTree(const std::string &function_name);

private:
  void ResolveDependencies_(graph::Operation *op);

  llvm::FunctionType *InitFunctionType_();

  llvm::Function *InitFunction_(const std::string &name);

  llvm::BasicBlock *CodeGen_(ops::Operator *op, llvm::Function *f,
                             llvm::BasicBlock *prev);

  void FinalizeFunction_(llvm::BasicBlock *end_block);

private:
  codegen::Module *m_module_;
  std::list<graph::Operation *> m_ops_;
  std::set<graph::Variable *> m_args_;
};

} // namespace codegen
} // namespace Hobbit

#endif // HOBBIT_CGVISITOR_HPP
