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

namespace codegen {
class Module;
}

namespace graph {
class Variable;
}

namespace ops {
class Operator {
public:
  enum OperatorType {
#include "OperatorTypes.def"
  };

  explicit Operator(codegen::Module *m) : m_module_(m) {}

  //  virtual void SetInputs(llvm::ArrayRef<graph::Variable *> inputs);
  virtual OperatorType GetOperatorType() const = 0;
  virtual llvm::BasicBlock *InsertIntoFunction(llvm::Function *) = 0;
  virtual graph::Variable *GetOutputVariable() const = 0;

protected:
  codegen::Module *m_module_;
};

template <Operator::OperatorType>
Operator *CreateOperator(codegen::Module *module,
                         llvm::ArrayRef<graph::Variable *> args) {
  return nullptr;
}

template<> Operator *CreateOperator<Operator::mockID>(codegen::Module *module,
                                                      llvm::ArrayRef<graph::Variable *> /* args */);
template<> Operator *CreateOperator<Operator::gemmID>(codegen::Module *module,
                                                      llvm::ArrayRef<graph::Variable *> args);
template<> Operator *CreateOperator<Operator::eltwiseAddID>(codegen::Module *module,
                                                            llvm::ArrayRef<graph::Variable *> args);

inline Operator *Create(Operator::OperatorType op_id, codegen::Module *module,
                 llvm::ArrayRef<graph::Variable *> args) {
  switch (op_id) {
  case Operator::mockID:
    return CreateOperator<Operator::mockID>(module, args);
  case Operator::gemmID:
    return CreateOperator<Operator::gemmID>(module, args);
  case Operator::eltwiseAddID:
    CreateOperator<Operator::eltwiseAddID>(module, args);
  default:
    return nullptr;
  }
}

} // namespace ops
} // namespace Hobbit

#endif // HOBBIT_OPERATOR_HPP
