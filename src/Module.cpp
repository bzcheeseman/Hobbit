//
// Created by Aman LaChapelle on 5/3/18.
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

#include <codegen/Module.hpp>
#include <codegen/TreeVisitor.hpp>
#include <graph/Node.hpp>

#include <llvm/IR/Module.h>

Hobbit::codegen::Module::Module(const std::string &name)
    : m_ctx_(), m_module_(llvm::make_unique<llvm::Module>(name, m_ctx_)) {}

llvm::LLVMContext &Hobbit::codegen::Module::GetContext() { return m_ctx_; }

Hobbit::graph::Variable *Hobbit::codegen::Module::GetVariable(
    const std::string &name, llvm::ArrayRef<uint64_t> dims, TypeID type) {
  std::unique_ptr<graph::Shape> shape =
      llvm::make_unique<graph::Shape>(m_ctx_, dims);
  return new graph::Variable(name, std::move(shape), GetType(type, m_ctx_));
}

Hobbit::graph::Variable *Hobbit::codegen::Module::GetVariable(
    const std::string &name, llvm::ArrayRef<uint64_t> dims, llvm::Type *type) {
  std::unique_ptr<graph::Shape> shape =
      llvm::make_unique<graph::Shape>(m_ctx_, dims);
  return new graph::Variable(name, std::move(shape), type);
}

Hobbit::graph::Variable *
Hobbit::codegen::Module::GetVariable(const std::string &name,
                                     graph::Shape *shape, llvm::Type *type) {
  shape->InitLLVM(m_ctx_);
  return new graph::Variable(name, std::move(shape), type);
}

Hobbit::graph::Operation *Hobbit::codegen::Module::GetOperation(
    const std::string &name, llvm::ArrayRef<Hobbit::graph::Node *> inputs,
    Hobbit::ops::Operator::OperatorType op_type) {
  return new graph::Operation(name, inputs, op_type);
}

llvm::Function *Hobbit::codegen::Module::GetFunction(const std::string &name,
                                                     llvm::FunctionType *ft) {
  return llvm::cast<llvm::Function>(m_module_->getOrInsertFunction(name, ft));
}

void Hobbit::codegen::Module::Print(llvm::raw_ostream &os) {
  m_module_->print(os, nullptr);
}
