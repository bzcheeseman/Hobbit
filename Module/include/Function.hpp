//
// Created by Aman LaChapelle on 3/7/18.
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

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace Hobbit {
  struct Function {
    llvm::LLVMContext *ctx_;
    llvm::Function *llvm_function = nullptr;
    llvm::BasicBlock *entry_block = nullptr;
    std::vector<llvm::BasicBlock *> bb;

    llvm::BasicBlock *AddBB(const std::string &name = "") {
      bb.push_back(llvm::BasicBlock::Create(*ctx_, name, llvm_function));
      return *(bb.end() - 1); // return BB just created
    }
  };
}

#endif // HOBBIT_FUNCTION_HPP
