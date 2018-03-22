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
#include <llvm/Support/raw_ostream.h>

namespace Hobbit {
  class Tensor;

  class Module {
  public:
    Module(const std::string &name, llvm::LLVMContext &ctx);

    llvm::LLVMContext *GetContext();
    llvm::Function *GetFunction(const std::string &name,
                                const std::vector<Tensor *> &args);
    void FinalizeFunction(llvm::Function *f);
    void FinalizeModule(unsigned int opt_level,
                        const std::string &target_triple,
                        const std::string &cpu = "corei7-avx",
                        const std::string &features = "+avx2");
    void Print();

    void *GetFunctionPtr(const std::string &name);

  private:
    llvm::LLVMContext *ctx_;
    std::string name_;
    std::unique_ptr<llvm::Module> module_;
  };
}

#endif // HOBBIT_MODULE_HPP
