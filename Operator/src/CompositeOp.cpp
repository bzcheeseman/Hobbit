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

#include "CompositeOp.hpp"

Hobbit::CompositeOp::CompositeOp() { composite_op_table_.push_back(this); }

Hobbit::CompositeOp::CompositeOp(std::deque<ElementWiseOp *> &elemwise_table,
                                 std::deque<ReductionOp *> &reduction_table) {
  composite_op_table_.push_back(this);

  elemwise_op_table_ = std::move(elemwise_table);
  reduction_op_table_ = std::move(reduction_table);
}

Hobbit::CompositeOp::CompositeOp(std::deque<ElementWiseOp *> &elemwise_table,
                                 std::deque<ReductionOp *> &reduction_table,
                                 std::deque<CompositeOp *> &composite_table) {
  composite_op_table_ = std::move(composite_table);
  elemwise_op_table_ = std::move(elemwise_table);
  reduction_op_table_ = std::move(reduction_table);
}

void Hobbit::CompositeOp::PushOperator(Hobbit::ElementWiseOp *op) {
  elemwise_op_table_.push_back(std::move(op));
}

void Hobbit::CompositeOp::PushOperator(Hobbit::ReductionOp *op) {
  reduction_op_table_.push_back(std::move(op));
}

void Hobbit::CompositeOp::PushOperator(Hobbit::CompositeOp *op) {
  composite_op_table_.push_back(std::move(op));
}

llvm::Value *Hobbit::CompositeOp::Emit(llvm::IRBuilder<> &builder,
                                       llvm::Value *input,
                                       llvm::Type *vector_type) {
  llvm::Value *output = input;

  for (auto &composite_op : composite_op_table_) {
    output = composite_op->EmitElemwise_(builder, output, vector_type);
    output = composite_op->EmitReduction_(builder, output, vector_type);
  }

  return output;
}
