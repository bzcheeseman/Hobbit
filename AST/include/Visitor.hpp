//
// Created by Aman LaChapelle on 3/24/18.
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

#ifndef HOBBIT_VISITOR_HPP
#define HOBBIT_VISITOR_HPP

#include <map>

namespace llvm {
  class LLVMContext;
  class Module;
  class Function;
}

namespace Hobbit {
  class Function;

  class Visitor {
  public:
    static Visitor *Create(llvm::LLVMContext *ctx,
                           const std::string &module_name);

    llvm::LLVMContext *GetContext();

    llvm::Module *GetModule();

    llvm::Function *GetFunction(Function *key);

    void PushFunction(Function *key, llvm::Function *val);

    void FinalizeFunction(Function *key);

    void Finalize(unsigned int opt_level, const std::string &target_triple,
                  const std::string &cpu, const std::string &features);

  protected:
    Visitor(llvm::LLVMContext *ctx, const std::string &module_name);

  private:
    llvm::LLVMContext *ctx_;
    std::unique_ptr<llvm::Module> module_;
    std::map<Function *, llvm::Function *> func_table_;
  };
}

#endif // HOBBIT_VISITOR_HPP
