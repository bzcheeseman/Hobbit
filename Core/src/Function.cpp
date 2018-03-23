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

#include <set>

#include "Module.hpp"
#include "Node.hpp"

namespace Hobbit {
  std::unique_ptr<Function> Function::Create(Module *m,
                                             const std::string &name) {
    std::unique_ptr<Function> f = llvm::make_unique<Function>();
    f->name_ = name;
    f->module_ = m;

    return f;
  }

  llvm::LLVMContext *Function::GetContext() { return module_->GetContext(); }

  void Function::AddSymbol(void *sym_addr, core::Symbol *arg) {
    if (symbol_table_.find(sym_addr) != symbol_table_.end())
      throw std::runtime_error("Attempting to overwrite an existing argument!");

    symbol_table_[sym_addr] = arg;
  }

  void Function::MarkSymbolAsArg(void *sym_addr) {
    symbol_table_.at(sym_addr)->is_arg = true;
  }

  void Function::AddNode(core::Node *node) {
    op_table_.push_back(node);
  }

//  Tensor *Function::AddOpNode(std::initializer_list<void *> sym_addrs,
//                              const OpCode &opcode) {
//
//    std::vector<core::Symbol *> symbols;
//    for (auto &addr : sym_addrs) {
//      symbols.push_back(symbol_table_.at(addr));
//    }
//
//    core::Node *op;
//    Tensor *output;
//    switch (opcode) {
//    case ALLOCA: {
//      op = new core::Alloca(symbols);
//      break;
//    }
//    case SDOT: {
//      op = new core::Sdot(symbols);
//      break;
//    }
//    }
//
//    output = op->GetOutput();
//    op_table_.emplace_back(op);
//    symbol_table_[output] = output->GetSymbol();
//
//    return output;
//  }

  core::Symbol *Function::GetSymbol(void *sym_addr) {
    return symbol_table_.at(sym_addr);
  }

  void Function::Emit(llvm::Function *func) {
    for (auto &op : op_table_) {
      op->Emit(func);
    }
  }

  std::vector<Tensor *>
  Function::GetSignatureArgs(std::initializer_list<void *> output_addrs) {
    std::vector<Tensor *> output_types;
    std::vector<Tensor *> arg_types;

    std::set<void *> visited_addrs;
    for (auto &addr : output_addrs) {
      visited_addrs.insert(addr);
      output_types.push_back((Tensor *)addr);
    }
    for (auto &sym : symbol_table_) {
      if (visited_addrs.find(sym.first) == visited_addrs.end() &&
          sym.second->is_arg)
        arg_types.push_back((Tensor *)sym.first);
    }

    std::vector<Tensor *> out(arg_types.begin(), arg_types.end());
    out.reserve(output_types.size());
    out.insert(out.end(), output_types.begin(), output_types.end());

    for (auto &o : out) {
      this->MarkSymbolAsArg(o);
    }

    return out;
  }

  const std::string &Function::GetName() { return name_; }

  llvm::Function *Function::GetFunction(std::initializer_list<void *> output_addrs) {
    std::vector<Tensor *> signature_args = this->GetSignatureArgs(std::move(output_addrs));
    return module_->GetFunction(name_, signature_args);
  }
}
