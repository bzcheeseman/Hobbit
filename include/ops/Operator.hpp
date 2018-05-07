//
// Created by Aman LaChapelle on 4/8/18.
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

#ifndef HOBBIT_OPERATOR_HPP
#define HOBBIT_OPERATOR_HPP

// LLVM
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

namespace llvm {
class Function;
class BasicBlock;
class Type;
class LLVMContext;
} // namespace llvm

namespace Hobbit {
namespace ops {
class Operator {
public:
  enum OperatorType {
#include "OperatorTypes.def"
  };

  virtual OperatorType GetOperatorType() const = 0;
  virtual llvm::BasicBlock *InsertIntoFunction(llvm::Function *) = 0;
  virtual llvm::Type *GetOutputType() const = 0;
  virtual llvm::ArrayRef<uint64_t> GetOutputShape() const = 0;
};

template <class OP, class... Args> OP CreateOperator(Args... args) {
  return OP(std::forward<Args...>(args...));
}

class MockOperator : public Operator {
public:
  explicit MockOperator(llvm::LLVMContext &ctx) : m_ctx_(ctx) {}

  OperatorType GetOperatorType() const override { return mockID; }
  static inline bool classof(const Operator *op) {
    return op->GetOperatorType() == mockID;
  }

  llvm::BasicBlock *InsertIntoFunction(llvm::Function *f) override {
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

    builder.CreateAdd(one, two, "hobbit.mock_operator.add", true, true);

    return BB;
  }

  llvm::Type *GetOutputType() const override {
    return llvm::Type::getFloatTy(m_ctx_);
  }

  llvm::ArrayRef<uint64_t> GetOutputShape() const override { return {1}; }

private:
  llvm::LLVMContext &m_ctx_;
};
} // namespace ops
} // namespace Hobbit

#endif // HOBBIT_OPERATOR_HPP
