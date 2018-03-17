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


#ifndef HOBBIT_MODULE_HPP
#define HOBBIT_MODULE_HPP

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace Hobbit { namespace core {
  class Module {
  public:
    Module(const std::string &name, llvm::LLVMContext &ctx);

    llvm::LLVMContext *GetContext();

  private:
    llvm::LLVMContext *ctx_;
    std::string name_;
    std::unique_ptr<llvm::Module> module_;
  };
}}


#endif //HOBBIT_MODULE_HPP
