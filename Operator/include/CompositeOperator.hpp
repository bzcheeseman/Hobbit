//
// Created by Aman LaChapelle on 2/18/18.
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


#ifndef HOBBIT_COMPOSITEOPERATOR_HPP
#define HOBBIT_COMPOSITEOPERATOR_HPP

#include <deque>
#include "ElementWiseOp.hpp"
#include "ReductionOp.hpp"

namespace Hobbit {
  class CompositeOperator : public Operator {
    CompositeOperator() = default;
    explicit CompositeOperator(std::deque<ElementWiseOp *> &elemwise_table, std::deque<ReductionOp *> &reduction_table);

    void PushOperator(ElementWiseOp *op);
    void PushOperator(ReductionOp *op);

//    llvm::Value *Emit(llvm::IRBuilder<> &builder, llvm::Value *input, llvm::Type *vector_type);

  protected:
    std::deque<ElementWiseOp *> elemwise_op_table_;
    std::deque<ReductionOp *> reduction_op_table_;
  };
}


#endif //HOBBIT_COMPOSITEOPERATOR_HPP
