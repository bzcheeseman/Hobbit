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

#include <llvm/IR/Module.h>

#include <set>
#include <graph/Node.hpp>

#include "Type.hpp"
#include "Visitor.hpp"

namespace Hobbit {
namespace graph {
  class Variable;
}

namespace codegen {

struct Function {
  std::string name;
  llvm::ArrayRef<llvm::Type *> arg_types;
};

// opid wraps the operator from the registry...how to register ops?

class Module {
public:
  Module(const std::string &name) : m_ctx_(), m_module_(llvm::make_unique<llvm::Module>(name, m_ctx_)) {}

  llvm::LLVMContext &GetContext() { return m_ctx_; }

  void InsertFunction(Function &f) {
    llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getVoidTy(m_ctx_), f.arg_types, false);
    llvm::Value *func = m_module_->getOrInsertFunction(f.name, ft);
    m_func_table_[&f] = llvm::cast<llvm::Function>(func);
  }

  graph::Variable GetVariable(const std::string &name, llvm::ArrayRef<uint64_t> dims, TypeID type) {
    std::unique_ptr<graph::Shape> shape = llvm::make_unique<graph::Shape>(m_ctx_, dims);
    return graph::Variable(name, std::move(shape), GetType(type, m_ctx_));
  }

  graph::Operation GetOperation(const std::string &name, llvm::ArrayRef<graph::Node *> inputs, ops::Operator *op) {
    return graph::Operation(name, inputs, op);
  }

  void Print(llvm::raw_ostream &os) {
    m_module_->print(os, nullptr);
  }

private:
  llvm::LLVMContext m_ctx_;
  std::unique_ptr<llvm::Module> m_module_;

  std::map<Function *, llvm::Function *> m_func_table_;
};

}
}

#endif //HOBBIT_MODULE_HPP
