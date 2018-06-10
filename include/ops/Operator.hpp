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

#include <llvm/ADT/ArrayRef.h>

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
class Operator {
public:
  enum OperatorType {
#include "OperatorTypes.def"
  };

  explicit Operator(Module *m) : m_module_(m) {}

  virtual OperatorType GetOperatorType() const = 0;
  virtual bool PreservesShape() const = 0;
  virtual bool IsOrderInvariant() const = 0;
  virtual llvm::BasicBlock *InsertIntoFunction(llvm::Function *,
                                               llvm::BasicBlock *) = 0;
  virtual graph::Variable *GetOutputVariable() const = 0;

protected:
  Module *m_module_;
};

} // namespace ops
} // namespace Hobbit

#endif // HOBBIT_OPERATOR_HPP
