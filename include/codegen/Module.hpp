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

#ifndef HOBBIT_MODULE_HPP
#define HOBBIT_MODULE_HPP

// Project
#include "Type.hpp"
// STL
#include <list>
#include <map>
#include <set>
// LLVM
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/LLVMContext.h>

namespace llvm {
class Type;
class raw_ostream;
class Module;
class Function;
} // namespace llvm

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

class Visitor;

struct Function {
  std::string name;
};

// opid wraps the operator from the registry...how to register ops?

class Module {
public:
  explicit Module(const std::string &name);

  llvm::LLVMContext &GetContext();

  Function *ParseTree(const std::string &name, Visitor &visitor);

  graph::Variable GetVariable(const std::string &name,
                              llvm::ArrayRef<uint64_t> dims, TypeID type);

  graph::Variable GetVariable(const std::string &name,
                              llvm::ArrayRef<uint64_t> dims, llvm::Type *type);

  graph::Operation GetOperation(const std::string &name,
                                llvm::ArrayRef<graph::Node *> inputs,
                                ops::Operator *op);

  void Print(llvm::raw_ostream &os);

private:
  llvm::FunctionType *ParseArgs_(const std::set<graph::Variable *> &args);
  void ParseTree_(llvm::Function *f, std::list<graph::Operation *> &tree);

  llvm::LLVMContext m_ctx_;
  std::unique_ptr<llvm::Module> m_module_;

  std::map<Function *, llvm::Function *> m_func_table_;
};

} // namespace codegen
} // namespace Hobbit

#endif // HOBBIT_MODULE_HPP
