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

#include <llvm/ADT/SmallVector.h>
#include <map>

namespace llvm {
  class LLVMContext;
  class Module;
  class Function;
  class Type;
} // namespace llvm

// TODO (Aman): refactor the visitors to use the new Operators
namespace Hobbit {
  namespace ast {
    class Tensor;
    class Node;
  } // namespace ast

  class Function {
  public:
    static Function *Create(const std::string &name);

    // Add an argument to the function signature and get it so that we can
    // operate on it
    ast::Tensor *GetNewArg(const std::string &name,
                           llvm::SmallVector<uint64_t, 4> dims,
                           llvm::Type *type);
    ast::Tensor *GetNewAlloca(const std::string &name,
                              llvm::SmallVector<uint64_t, 4> dims,
                              llvm::Type *type);

    void SetArg(ast::Tensor *t);

    void PushNode(ast::Node *node);

  private:
    friend class FunctionCG;

    std::string name_;

    // For the function signature
    llvm::SmallVector<ast::Tensor *, 4> arg_table_;
    llvm::SmallVector<ast::Tensor *, 8> alloca_table_;

    // Holds the graph in memory
    ast::Node *child_;
    ast::Node *last_node_;
  };

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
} // namespace Hobbit

#endif // HOBBIT_VISITOR_HPP
