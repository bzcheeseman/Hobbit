//
// Created by Aman LaChapelle on 5/14/18.
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

#ifndef HOBBIT_MOCK_HPP
#define HOBBIT_MOCK_HPP

#include "Operator.hpp"

namespace llvm {
class Function;
class BasicBlock;
class Type;
class LLVMContext;
} // namespace llvm

namespace Hobbit {

class Module;

namespace graph {
class Variable;
}

namespace ops {
class MockOperator : public Operator {
public:
  explicit MockOperator(Module *m);

  OperatorType GetOperatorType() const override;
  static inline bool classof(const Operator *op) {
    return op->GetOperatorType() == mockID;
  }

  bool PreservesShape() const override {
    return true;
  }

  bool IsOrderInvariant() const override {
    return true;
  }

  llvm::BasicBlock *InsertIntoFunction(llvm::Function *f,
                                       llvm::BasicBlock *previous) override;

  graph::Variable *GetOutputVariable() const override;

private:
  graph::Variable *outvar;
  llvm::LLVMContext &m_ctx_;
};

} // namespace ops
} // namespace Hobbit

#endif // HOBBIT_MOCK_HPP
