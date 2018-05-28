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

#include <codegen/CGVisitor.hpp>
#include <codegen/Module.hpp>
#include <codegen/TreeVisitor.hpp>
#include <graph/Node.hpp>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

Hobbit::Module::Module(const std::string &name)
    : m_ctx_(), m_module_(llvm::make_unique<llvm::Module>(name, m_ctx_)) {}

llvm::LLVMContext &Hobbit::Module::GetContext() { return m_ctx_; }

Hobbit::graph::Variable *
Hobbit::Module::GetVariable(const std::string &name,
                            llvm::ArrayRef<uint64_t> dims, TypeID type) {
  std::unique_ptr<graph::Shape> shape =
      llvm::make_unique<graph::Shape>(m_ctx_, dims);
  return new graph::Variable(name, std::move(shape), GetType(type, m_ctx_));
}

Hobbit::graph::Variable *
Hobbit::Module::GetVariable(const std::string &name,
                            llvm::ArrayRef<uint64_t> dims, llvm::Type *type) {
  std::unique_ptr<graph::Shape> shape =
      llvm::make_unique<graph::Shape>(m_ctx_, dims);
  return new graph::Variable(name, std::move(shape), type);
}

Hobbit::graph::Variable *Hobbit::Module::GetVariable(const std::string &name,
                                                     graph::Shape *shape,
                                                     llvm::Type *type) {
  shape->InitLLVM(m_ctx_);
  return new graph::Variable(name, std::move(shape), type);
}

Hobbit::graph::Operation *
Hobbit::Module::GetOperation(const std::string &name,
                             llvm::ArrayRef<Hobbit::graph::Node *> inputs,
                             Hobbit::ops::Operator::OperatorType op_type) {
  return new graph::Operation(name, inputs, op_type);
}

void Hobbit::Module::RegisterOutput(Hobbit::graph::Node *parent_ref) {
  m_outputs_.push_back(parent_ref);
}

llvm::Function *Hobbit::Module::GetFunction(const std::string &name,
                                            llvm::FunctionType *ft) {
  return llvm::cast<llvm::Function>(m_module_->getOrInsertFunction(name, ft));
}

void Hobbit::Module::CodeGen(const std::string &name, graph::Node *final_node) {
  codegen::TreeVisitor visitor;
  visitor.BuildTree(final_node);
  codegen::CGVisitor cgvisitor(this, visitor.Args(), visitor.Tree());
  cgvisitor.CodeGenTree(name, m_outputs_);
}

llvm::Module &Hobbit::Module::SetTarget(const std::string &target_triple, const llvm::DataLayout &data_layout) {
  m_module_->setDataLayout(data_layout);
  m_module_->setTargetTriple(target_triple);

  llvm::verifyModule(*m_module_);
  return *m_module_;
}

void Hobbit::Module::Print(llvm::raw_ostream &os) {
  m_module_->print(os, nullptr);
}

void Hobbit::Module::WriteToFile(llvm::raw_fd_ostream &os) {
  llvm::WriteBitcodeToFile(m_module_.get(), os);
}
