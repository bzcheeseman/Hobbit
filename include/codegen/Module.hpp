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

namespace Hobbit {
namespace graph {
  class Variable;
}

namespace codegen {

struct Function {
  std::string name;
  llvm::ArrayRef<llvm::Type *> arg_types;
};

class Module {
public:
  Module(const std::string &name) : m_ctx_(), m_module_(llvm::make_unique<llvm::Module>(name, m_ctx_)) {}
  virtual ~Module() {
    for (auto &func : m_func_table_) {
      delete func.first;
    }
  }

  llvm::LLVMContext &GetContext() { return m_ctx_; }

  void InsertFunction(Function &f) {
    llvm::Value *func = m_module_->getOrInsertFunction(f.name, llvm::Type::getVoidTy(m_ctx_), f.arg_types);
    m_func_table_[&f] = llvm::cast<llvm::Function>(func);
  }

private:
  llvm::LLVMContext m_ctx_;
  std::unique_ptr<llvm::Module> m_module_;

  std::map<Function *, llvm::Function *> m_func_table_;
};

}
}

#endif //HOBBIT_MODULE_HPP
