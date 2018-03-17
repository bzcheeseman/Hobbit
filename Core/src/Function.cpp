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


#include "Function.hpp"

namespace Hobbit { namespace core {
  std::unique_ptr<Function> Function::Create(Module *m, const std::string &name) {
    std::unique_ptr<Function> f = llvm::make_unique<Function>();
    f->name_ = name;
    f->module_ = m;

    return f;
  }

  llvm::LLVMContext *Function::GetContext() {
    return module_->GetContext();
  }

  void Function::AddBlock(const std::string &name) {
    if (function_blocks_.find(name) != function_blocks_.end())
      throw std::runtime_error("Attempting to overwrite an existing basic block!");

    function_blocks_[name] = std::vector<llvm::Value *>();
  }

  void Function::AddToBlock(const std::string &name, llvm::Value *v) {
    function_blocks_[name].push_back(v);
  }

  void Function::AddSymbol(void *sym_addr, Symbol *arg) {
    if (symbol_table_.find(sym_addr) != symbol_table_.end())
      throw std::runtime_error("Attempting to overwrite an existing argument!");

    symbol_table_[sym_addr] = arg;
  }

  void Function::MarkSymbolAsArg(void *sym_addr) {
    symbol_table_.at(sym_addr)->is_arg = true;
  }

  Tensor *Function::AddOpNode(std::initializer_list<void *> sym_addrs, const OpCode &opcode) {

    std::vector<Symbol *> symbols;
    for (auto &addr : sym_addrs) {
      symbols.push_back(symbol_table_.at(addr));
    }

    switch (opcode) {
      case ALLOCA : {
        Alloca *op = new Alloca(symbols);
        op_table_.emplace_back(op);
        return new Tensor(op->GetOutput()); // return a new variable?
      }
    }

    return nullptr;
  }

  Symbol *Function::GetSymbol(void *sym_addr) {
    return symbol_table_.at(sym_addr);
  }

    // Private functions
//  std::unique_ptr<Tensor> Function::CreateVariable(void *addr) {
//    return Variable::Create(this, symbol_table_.at(addr)->type, symbol_table_.at(addr)->shape);
//  }
//
//  std::unique_ptr<Tensor> Function::CreateConstant(void *addr) {
//    return Constant::Create(this, symbol_table_.at(addr)->type, symbol_table_.at(addr)->shape);
//  }

}}
