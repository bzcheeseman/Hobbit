//
// Created by Aman LaChapelle on 5/18/18.
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
#include <graph/Node.hpp>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <ops/Factory.hpp>

Hobbit::codegen::CGVisitor::CGVisitor(
    Hobbit::Module *module, std::set<Hobbit::graph::Variable *> &args,
    std::list<Hobbit::graph::Operation *> &ops)
    : m_module_(std::move(module)), m_args_(args), m_ops_(ops) {}

void Hobbit::codegen::CGVisitor::CodeGenTree(
    const std::string &function_name, llvm::ArrayRef<graph::Node *> outputs) {
  for (auto &op : m_ops_) {
    ResolveDependencies_(op);
  }

  // Resolve outputs into their Variables
  llvm::SmallVector<graph::Variable *, 2> output_vars;
  graph::Operation *op;
  graph::Variable *var;

  for (auto &out : outputs) {
    if ((op = llvm::dyn_cast<graph::Operation>(out))) {
      output_vars.push_back(op->GetOp()->GetOutputVariable());
    }
    if ((var = llvm::dyn_cast<graph::Variable>(out))) {
      output_vars.push_back(var);
    }
  }

  llvm::Function *f = InitFunction_(function_name, output_vars);
  llvm::BasicBlock *prev = &f->getEntryBlock();
  for (auto &op : m_ops_) {
    prev = CodeGen_(op->GetOp(), f, prev);
  }

  FinalizeFunction_(prev);
}

void Hobbit::codegen::CGVisitor::ResolveDependencies_(
    Hobbit::graph::Operation *op) {
  std::vector<graph::Variable *> in_vars;
  for (auto &in : op->Inputs()) {
    graph::Variable *in_var;
    if ((in_var = llvm::dyn_cast<graph::Variable>(in))) {
      // if it's a variable, then we're done.
      in_vars.push_back(in_var);
      continue;
    }
    // if it's an Operation, then I need to get the output variable
    in_var = llvm::cast<graph::Operation>(in)->GetOp()->GetOutputVariable();
    in_vars.push_back(in_var);
  }
  op->SetOp(ops::Create(op->GetOperatorType(), m_module_, in_vars));
}

llvm::FunctionType *Hobbit::codegen::CGVisitor::InitFunctionType_(
    llvm::ArrayRef<graph::Variable *> output_vars) {
  std::vector<llvm::Type *> arg_types;
  for (auto &arg : m_args_) {
    if (arg->GetShape().Size() == 1)
      arg_types.push_back(arg->GetType());
    else
      arg_types.push_back(arg->GetType()->getPointerTo(0));
  }

  llvm::Type *out_type;
  for (auto &out_var : output_vars) {
    out_type = out_var->GetType()->getPointerTo(0);
    arg_types.push_back(out_type);
  }

  llvm::FunctionType *ft = llvm::FunctionType::get(
      llvm::Type::getVoidTy(m_module_->GetContext()), arg_types, false);
  return ft;
}

llvm::Function *Hobbit::codegen::CGVisitor::InitFunction_(
    const std::string &name, llvm::ArrayRef<graph::Variable *> output_vars) {
  llvm::FunctionType *ft = InitFunctionType_(output_vars);
  llvm::Function *f = m_module_->GetFunction(name, ft);

  auto m_arg_iter = m_args_.begin();
  // Don't go to the last few args because they are output pointers
  for (auto f_arg_iter = f->arg_begin(),
            f_arg_end = f->arg_end() - output_vars.size();
       f_arg_iter != f_arg_end; ++f_arg_iter, ++m_arg_iter) {
    llvm::Argument *arg = &*f_arg_iter;
    arg->setName((*m_arg_iter)->GetName());
    (*m_arg_iter)->SetVal(arg);
  }

  auto m_out_iter = output_vars.begin();
  // Don't go to the last few args because they are output pointers
  for (auto f_arg_iter = f->arg_end() - output_vars.size(),
            f_arg_end = f->arg_end();
       f_arg_iter != f_arg_end; ++f_arg_iter, ++m_out_iter) {
    llvm::Argument *arg = &*f_arg_iter;
    arg->setName((*m_out_iter)->GetName());
    (*m_out_iter)->SetVal(arg);
  }

  llvm::BasicBlock::Create(m_module_->GetContext(), "hobbit." + name + ".entry",
                           f);
  return f;
}

llvm::BasicBlock *Hobbit::codegen::CGVisitor::CodeGen_(
    Hobbit::ops::Operator *op, llvm::Function *f, llvm::BasicBlock *prev) {
  return op->InsertIntoFunction(f, prev);
}

void Hobbit::codegen::CGVisitor::FinalizeFunction_(
    llvm::BasicBlock *end_block) {
  llvm::IRBuilder<> builder(end_block);
  builder.CreateRetVoid();
}