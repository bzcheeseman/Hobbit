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


#ifndef HOBBIT_ELTWISE_ADD_HPP
#define HOBBIT_ELTWISE_ADD_HPP

#include "Operator.hpp"

namespace llvm {
  class Value;
  class Function;
  class BasicBlock;
} // namespace llvm

namespace Hobbit {
  namespace graph {
    class Variable;
  }
  namespace codegen {
    class Module;
  }
  namespace ops {
    class eltwise_add : public Operator {
    public:
      eltwise_add(codegen::Module *m, graph::Variable *A, graph::Variable *B);

      OperatorType GetOperatorType() const override { return eltwiseAddID; }

      static inline bool classof(const Operator *op) {
        return op->GetOperatorType() == eltwiseAddID;
      }

      graph::Variable *GetOutputVariable() const override;

      llvm::BasicBlock *InsertIntoFunction(llvm::Function *func) override;

    private:
      graph::Variable *A_, *B_, *C_;
    };
  }
}

#endif //HOBBIT_ELTWISE_ADD_HPP
