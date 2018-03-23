//
// Created by Aman LaChapelle on 3/17/18.
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

#ifndef HOBBIT_FUNCTION_HPP
#define HOBBIT_FUNCTION_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace llvm {
  class LLVMContext;
  class Type;
  class Function;
  class Value;
  class Argument;
}

namespace Hobbit {
  class Module;
  class Tensor;

  namespace core {
    class Symbol;
    class Node;
  }

  enum OpCode {
    ALLOCA = 0,
    SDOT = 1,
  };

  class Function {
  public:
    static std::unique_ptr<Function> Create(Module *m, const std::string &name);

    void AddSymbol(void *sym_addr, core::Symbol *arg);
    core::Symbol *GetSymbol(void *sym_addr);
    void MarkSymbolAsArg(void *sym_addr);

    void AddNode(core::Node *node);

    llvm::LLVMContext *GetContext();

    const std::string &GetName();

    llvm::Function *GetFunction(std::initializer_list<void *> output_addrs);

    void Emit(llvm::Function *func);

  private:
    std::vector<Tensor *>
    GetSignatureArgs(std::initializer_list<void *> output_addrs);

  private:
    std::string name_;
    Module *module_;

    // everything in this function comes from these
    std::map<void *, core::Symbol *> symbol_table_;
    std::vector<core::Node *> op_table_;
  };
}

#endif // HOBBIT_FUNCTION_HPP
