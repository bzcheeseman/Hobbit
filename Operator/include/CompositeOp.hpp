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
  enum ChunkStrategy {
    FINISH_ELEMENTWISE_FIRST,
    PIECEWISE_ELEMENTWISE_REDUCE,
  };

  struct ComputeStrategy {
    uint64_t chunk_size;
    std::deque<std::string> elemwise_op_names;
    std::deque<std::string> reduction_op_names;
    ChunkStrategy chunking_strategy;
  };

  class CompositeOp {
  public:
    CompositeOp() = default;
    CompositeOp(std::map<std::string, ElementWiseOp *> &elemwise_table,
                std::map<std::string, ReductionOp *> &reduction_table);

    void PushOperator(const std::string &name, ElementWiseOp *op);
    void PushOperator(const std::string &name, ReductionOp *op);

    void PushOperator(CompositeOp *op);

    void SetComputeStrategy(ComputeStrategy &strategy);
    const ComputeStrategy &GetComputeStrategy();

    llvm::Value *Emit(llvm::IRBuilder<> &builder,
                      llvm::ArrayRef<llvm::Value *> operands);

  private:
    std::vector<llvm::Value *> ChunkArrayOperand_(llvm::IRBuilder<> &builder,
                                                  llvm::Value *operand);
    std::vector<llvm::Value *> ChunkVectorOperand_(llvm::IRBuilder<> &builder,
                                                   llvm::Value *operand);
    llvm::Value *MergeOperandsArray_(llvm::IRBuilder<> &builder,
                                     std::vector<llvm::Value *> operands);
    llvm::Value *MergeOperandsVector_(llvm::IRBuilder<> &builder,
                                      std::vector<llvm::Value *> operands);

    // Applies the result of each elementwise operation as the lhs of the next
    // elementwise operation - ordering is:
    // second_reduction_op(
    //  first_reduction_op(
    //    third_ewise_op(second_ewise_op(first_ewise_op(_1, _2), _3), _4)...
    //  )
    // ), etc.
    llvm::Value *EmitFinishElemwise_(llvm::IRBuilder<> &builder,
                                     llvm::ArrayRef<llvm::Value *> &operands);

    // Applies the result of each reduction operation as the lhs of the next
    // elementwise operation - ordering is:
    // second_reduction_op(
    //  second_ewise_op(first_reduction_op(first_ewise_op(_1, _2)), _3)
    // ), etc.
    llvm::Value *EmitPiecewise_(llvm::IRBuilder<> &builder,
                                llvm::ArrayRef<llvm::Value *> &operands);

    std::map<std::string, ElementWiseOp *> elemwise_op_table_;
    std::map<std::string, ReductionOp *> reduction_op_table_;

    ComputeStrategy strategy_;
  };
}

#endif // HOBBIT_COMPOSITEOPERATOR_HPP
