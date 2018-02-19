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

#include "ElementWiseOp.hpp"
#include "ReductionOp.hpp"
#include <deque>

namespace Hobbit {
  class CompositeOp {
  public:
    CompositeOp();
    CompositeOp(std::deque<ElementWiseOp *> &elemwise_table,
                std::deque<ReductionOp *> &reduction_table);
    CompositeOp(std::deque<ElementWiseOp *> &elemwise_table,
                std::deque<ReductionOp *> &reduction_table,
                std::deque<CompositeOp *> &composite_table);

    void PushOperator(ElementWiseOp *op);
    void PushOperator(ReductionOp *op);

    // This allows us to trivially fuse the generation of operators
    void PushOperator(CompositeOp *op);

    llvm::Value *Emit(llvm::IRBuilder<> &builder, llvm::Value *input,
                      llvm::Type *vector_type);

  protected:
    // Either of these two methods is fully allowed to be a no-op (just return
    // input)
    virtual llvm::Value *EmitElemwise_(llvm::IRBuilder<> &builder,
                                       llvm::Value *input,
                                       llvm::Type *vector_type) = 0;
    virtual llvm::Value *EmitReduction_(llvm::IRBuilder<> &builder,
                                        llvm::Value *input,
                                        llvm::Type *vector_type) = 0;

    std::deque<ElementWiseOp *> elemwise_op_table_;
    std::deque<ReductionOp *> reduction_op_table_;

    std::deque<CompositeOp *> composite_op_table_;
  };
}

#endif // HOBBIT_COMPOSITEOPERATOR_HPP
