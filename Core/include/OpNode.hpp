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


#ifndef HOBBIT_OPNODE_HPP
#define HOBBIT_OPNODE_HPP

#include <llvm/IR/IRBuilder.h>

#include "Shape.hpp"
#include "Symbol.hpp"

namespace Hobbit { namespace core {

  enum OpCode {
    ALLOCA = 0,
  };

  class TooManyArgs : public std::runtime_error {
  public:
    explicit TooManyArgs(const std::string &node_name) :
            std::runtime_error("Node "+node_name+" is initialized with too many args!") {};
  };

  class OpNode {
  public:
    OpNode(const std::initializer_list<Symbol *> &args, const std::string &node_name);
    OpNode(std::vector<Symbol *> args, const std::string &node_name);

    virtual Symbol *GetOutput() = 0;
    virtual llvm::Value *Emit(llvm::Function *func) = 0;

  protected:
    const std::string name_;
    std::vector<Symbol *> args_;
  };

  class Alloca : public OpNode {
  public:
    Alloca(const std::initializer_list<Symbol *> &args) : OpNode(args, "Alloca") {};
    Alloca(std::vector<Symbol *> args) : OpNode(args, "Alloca") {};

    Symbol *GetOutput() override;
    llvm::Value *Emit(llvm::Function *func) override;
  };
}}


#endif //HOBBIT_OPNODE_HPP
