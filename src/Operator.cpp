//
// Created by Aman LaChapelle on 5/13/18.
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
#include <graph/Node.hpp>
#include <ops/Operator.hpp>

Hobbit::ops::MockOperator::MockOperator(codegen::Module *m)
    : Operator(m), m_ctx_(m->GetContext()) {}

Hobbit::ops::Operator::OperatorType
Hobbit::ops::MockOperator::GetOperatorType() const {
  return mockID;
}

llvm::BasicBlock *
Hobbit::ops::MockOperator::InsertIntoFunction(llvm::Function *f) {
  llvm::BasicBlock *BB =
      llvm::BasicBlock::Create(m_ctx_, "hobbit.mock_operator", f);
  llvm::BasicBlock *BB_predecessor;
  llvm::IRBuilder<> builder(m_ctx_);
  if ((BB_predecessor = BB->getSinglePredecessor())) {
    builder.SetInsertPoint(BB_predecessor);
    builder.CreateBr(BB);
  }

  builder.SetInsertPoint(BB);

  llvm::ConstantInt *one = builder.getInt64(1), *two = builder.getInt64(2);

  outvar->SetVal(
      builder.CreateAdd(one, two, "hobbit.mock_operator.add", true, true));

  return BB;
}

Hobbit::graph::Variable *Hobbit::ops::MockOperator::GetOutputVariable() const {
  *outvar = m_module_->GetVariable("mock.output", {1},
                                   llvm::Type::getInt64Ty(m_ctx_));
  return outvar;
}
