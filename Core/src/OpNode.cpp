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


#include "OpNode.hpp"

#include <utility>

Hobbit::core::OpNode::OpNode(const std::initializer_list<Symbol *> &args, const std::string &node_name)
        : args_(args), name_(node_name) {
  if (args_.size() != 1) throw TooManyArgs(node_name);
}

Hobbit::core::OpNode::OpNode(std::vector<Symbol *> args, const std::string &node_name)
        : args_(std::move(args)), name_(node_name) {
  if (args_.size() != 1) throw TooManyArgs(node_name);
}

Hobbit::core::Symbol *Hobbit::core::Alloca::GetOutput() {
  return args_[0];
}

llvm::Value *Hobbit::core::Alloca::Emit(llvm::Function *func) {
  llvm::BasicBlock *BB = &func->getEntryBlock();

  llvm::IRBuilder<> builder(BB);

  llvm::Type *arr_type = llvm::ArrayType::get(args_[0]->type, args_[0]->shape.GetSize());

  return builder.CreateAlloca(arr_type, builder.getInt64(1), "hobbit.alloca");
}
