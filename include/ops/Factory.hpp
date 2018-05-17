//
// Created by Aman LaChapelle on 5/16/18.
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


#ifndef HOBBIT_FACTORY_HPP
#define HOBBIT_FACTORY_HPP

#include "Operator.hpp"
#include "eltwise_add.hpp"
#include "gemm.hpp"
#include "mock.hpp"

namespace Hobbit {
  namespace ops {
    template <Operator::OperatorType>
    Operator *CreateOperator(codegen::Module *module,
                             llvm::ArrayRef<graph::Variable *> args) {
      return nullptr;
    }

    template <>
    Operator *
    CreateOperator<Operator::mockID>(codegen::Module *module,
                                     llvm::ArrayRef<graph::Variable *> /* args */) {
      return new MockOperator(module);
    }

    template <>
    Operator *
    CreateOperator<Operator::gemmID>(codegen::Module *module,
                                     llvm::ArrayRef<graph::Variable *> args) {
      return new gemm(module, args[0], args[1], args[2], args[3]);
    }

    template <>
    Operator *
    CreateOperator<Operator::eltwiseAddID>(codegen::Module *module,
                                           llvm::ArrayRef<graph::Variable *> args) {
      return new eltwise_add(module, args[0], args[1]);
    }

    inline Operator *Create(Operator::OperatorType op_id, codegen::Module *module,
                            llvm::ArrayRef<graph::Variable *> args) {
      switch (op_id) {
        case Operator::mockID:
          return CreateOperator<Operator::mockID>(module, args);
        case Operator::gemmID:
          return CreateOperator<Operator::gemmID>(module, args);
        case Operator::eltwiseAddID:
          return CreateOperator<Operator::eltwiseAddID>(module, args);
        default:
          return nullptr;
      }
    }
  }
}

#endif //HOBBIT_FACTORY_HPP
