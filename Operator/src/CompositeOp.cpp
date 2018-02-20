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

Hobbit::CompositeOp::CompositeOp(
    std::map<std::string, ElementWiseOp *> &elemwise_table,
    std::map<std::string, ReductionOp *> &reduction_table) {
  elemwise_op_table_ = std::move(elemwise_table);
  reduction_op_table_ = std::move(reduction_table);
}

void Hobbit::CompositeOp::PushOperator(const std::string &name,
                                       Hobbit::ElementWiseOp *op) {
  if (elemwise_op_table_.find(name) != elemwise_op_table_.end())
    throw std::runtime_error("Operator exists in the table");
  elemwise_op_table_[name] = std::move(op);
}

void Hobbit::CompositeOp::PushOperator(const std::string &name,
                                       Hobbit::ReductionOp *op) {
  if (reduction_op_table_.find(name) != reduction_op_table_.end())
    throw std::runtime_error("Operator exists in the table");
  reduction_op_table_[name] = std::move(op);
}

void Hobbit::CompositeOp::PushOperator(Hobbit::CompositeOp *op) {
  // TODO: how to insert other operator's values into the table?
  return;
}

llvm::Value *Hobbit::CompositeOp::Emit(llvm::IRBuilder<> &builder,
                                       llvm::ArrayRef<llvm::Value *> operands) {
  if (this->strategy_.chunking_strategy == FINISH_ELEMENTWISE_FIRST)
    // TODO: size checking on the operands based on op name sizes
    return EmitFinishElemwise_(builder, operands);
  if (this->strategy_.chunking_strategy == PIECEWISE_ELEMENTWISE_REDUCE) {
    if (this->strategy_.elemwise_op_names.size() != this->strategy_.reduction_op_names.size() &&
            this->strategy_.elemwise_op_names.size() != this->strategy_.reduction_op_names.size()+1) {
      throw std::runtime_error("Invalid number of operations for a piecewise composite operation");
    }
    return EmitPiecewise_(builder, operands);
  }

  return nullptr;
}

void Hobbit::CompositeOp::SetComputeStrategy(
    Hobbit::ComputeStrategy &strategy) {
  strategy_ = std::move(strategy);
}

const Hobbit::ComputeStrategy &Hobbit::CompositeOp::GetComputeStrategy() {
  return strategy_;
}

std::vector<llvm::Value *>
Hobbit::CompositeOp::ChunkArrayOperand_(llvm::IRBuilder<> &builder,
                                        llvm::Value *operand) {
  uint64_t &chunk_size = this->strategy_.chunk_size;
  llvm::Type *operand_type = operand->getType();

  std::vector<llvm::Value *> output;

  uint64_t operand_size = operand_type->getArrayNumElements();
  uint64_t leftovers = operand_size % chunk_size;
  uint64_t chunks = (operand_size - leftovers) / chunk_size;

  llvm::Type *vector_type =
      llvm::VectorType::get(operand_type->getArrayElementType(), chunk_size);

  for (uint64_t i = 0; i < chunks; i++) {
    llvm::Value *vector = llvm::UndefValue::get(vector_type);
    for (uint64_t j = 0; j < chunk_size; j++) {
      llvm::Value *operand_elt =
          builder.CreateExtractValue(operand, i * chunk_size + j);
      vector = builder.CreateInsertElement(vector, operand_elt, j);
    }
    output.push_back(vector);
  }

  llvm::Value *vector = llvm::UndefValue::get(vector_type);
  for (uint64_t j = 0; j < leftovers; j++) {
    llvm::Value *operand_elt =
        builder.CreateExtractValue(operand, chunks * chunk_size + j);
    vector = builder.CreateInsertElement(vector, operand_elt, j);
  }
  output.push_back(vector);

  return output;
}

std::vector<llvm::Value *>
Hobbit::CompositeOp::ChunkVectorOperand_(llvm::IRBuilder<> &builder,
                                         llvm::Value *operand) {
  uint64_t &chunk_size = this->strategy_.chunk_size;
  llvm::Type *operand_type = operand->getType();

  std::vector<llvm::Value *> output;

  uint64_t operand_size = operand_type->getVectorNumElements();
  uint64_t leftovers = operand_size % chunk_size;
  uint64_t chunks = (operand_size - leftovers) / chunk_size;

  llvm::Type *vector_type =
      llvm::VectorType::get(operand_type->getVectorElementType(), chunk_size);

  for (uint64_t i = 0; i < chunks; i++) {
    llvm::Value *vector = llvm::UndefValue::get(vector_type);
    for (uint64_t j = 0; j < chunk_size; j++) {
      llvm::Value *operand_elt =
          builder.CreateExtractElement(operand, i * chunk_size + j);
      vector = builder.CreateInsertElement(vector, operand_elt, j);
    }
    output.push_back(vector);
  }

  if (leftovers > 0) {
    llvm::Value *vector = llvm::UndefValue::get(vector_type);
    for (uint64_t j = 0; j < leftovers; j++) {
      llvm::Value *operand_elt =
              builder.CreateExtractElement(operand, chunks * chunk_size + j);
      vector = builder.CreateInsertElement(vector, operand_elt, j);
    }
    output.push_back(vector);
  }

  return output;
}

llvm::Value *
Hobbit::CompositeOp::MergeOperandsArray_(llvm::IRBuilder<> &builder,
                                         std::vector<llvm::Value *> operands) {
  llvm::Type *element_type = operands[0]->getType();
  llvm::Type *aggregate_type =
      llvm::ArrayType::get(element_type, operands.size());

  llvm::Value *output = llvm::UndefValue::get(aggregate_type);

  for (uint64_t i = 0; i < operands.size(); i++) {
    output = builder.CreateInsertValue(output, operands[i], i);
  }

  return output;
}

llvm::Value *
Hobbit::CompositeOp::MergeOperandsVector_(llvm::IRBuilder<> &builder,
                                          std::vector<llvm::Value *> operands) {
  llvm::Type *element_type = operands[0]->getType();
  llvm::Type *aggregate_type =
      llvm::VectorType::get(element_type, operands.size());

  llvm::Value *output = llvm::UndefValue::get(aggregate_type);

  for (uint64_t i = 0; i < operands.size(); i++) {
    output = builder.CreateInsertElement(output, operands[i], i);
  }

  return output;
}

llvm::Value *Hobbit::CompositeOp::EmitFinishElemwise_(
    llvm::IRBuilder<> &builder, llvm::ArrayRef<llvm::Value *> &operands) {

  llvm::ArrayRef<llvm::Value *> first_operand = operands.take_front(1);
  operands = operands.drop_front(1);

  std::vector<llvm::Value *> chunked_lhs, chunked_rhs;

  if (first_operand[0]->getType()->isVectorTy()) {
    chunked_lhs = ChunkVectorOperand_(builder, first_operand[0]);
  } else if (first_operand[0]->getType()->isArrayTy()) {
    chunked_lhs = ChunkArrayOperand_(builder, first_operand[0]);
  }

  std::vector<llvm::Value *> chunked_result = chunked_lhs;
  uint64_t num_chunks;
  for (auto &ewise_op_name : this->strategy_.elemwise_op_names) {
    ElementWiseOp *ewise_op = elemwise_op_table_[ewise_op_name];

    llvm::Value *next_rhs = *operands.begin();
    operands = operands.drop_front(1);

    if (next_rhs->getType()->isVectorTy()) {
      chunked_rhs = ChunkVectorOperand_(builder, next_rhs);
    } else if (next_rhs->getType()->isArrayTy()) {
      chunked_rhs = ChunkArrayOperand_(builder, next_rhs);
    }

    num_chunks = chunked_rhs.size(); // rhs and previous result need to have the same number of chunks

    for (uint64_t i = 0; i < num_chunks; i++) {
      chunked_result[i] =
          ewise_op->Emit(builder, chunked_result[i], chunked_rhs[i], nullptr);
    }
  }

  num_chunks = chunked_result.size();
  for (auto &reduction_op_name : this->strategy_.reduction_op_names) {
    ReductionOp *reduction_op = reduction_op_table_[reduction_op_name];

    for (uint64_t i = 0; i < num_chunks; i++) {
      chunked_result[i] =
          reduction_op->Emit(builder, chunked_result[i], nullptr);
    }
    // now we have a bunch of single elements
    chunked_result = ChunkVectorOperand_(builder, MergeOperandsVector_(builder, chunked_result));
    num_chunks = chunked_result.size();
  }

  return chunked_result[0]; // the only piece we care about is the first element
                            // at this point
}

llvm::Value *
Hobbit::CompositeOp::EmitPiecewise_(llvm::IRBuilder<> &builder,
                                    llvm::ArrayRef<llvm::Value *> &operands) {
  llvm::ArrayRef<llvm::Value *> first_operand = operands.take_front(1);
  operands = operands.drop_front(1);
  std::vector<llvm::Value *> chunked_lhs, chunked_rhs;

  if (first_operand[0]->getType()->isVectorTy()) {
    chunked_lhs = ChunkVectorOperand_(builder, first_operand[0]);
  } else if (first_operand[0]->getType()->isArrayTy()) {
    chunked_lhs = ChunkArrayOperand_(builder, first_operand[0]);
  }

  uint64_t num_chunks = chunked_lhs.size();
  std::vector<llvm::Value *> chunked_result = chunked_lhs;
  uint64_t num_ops = this->strategy_.reduction_op_names.size(); // at least this many elementwise ops

  for (uint64_t i = 0; i < num_ops; i++) {
    std::string &ewise_op_name = this->strategy_.elemwise_op_names[i];
    std::string &reduction_op_name = this->strategy_.reduction_op_names[i];
    ElementWiseOp *ewise_op = elemwise_op_table_[ewise_op_name];
    ReductionOp *reduction_op = reduction_op_table_[reduction_op_name];

    llvm::Value *next_rhs = *operands.begin();
    operands = operands.drop_front(1);

    if (next_rhs->getType()->isVectorTy()) {
      chunked_rhs = ChunkVectorOperand_(builder, next_rhs);
    } else if (next_rhs->getType()->isArrayTy()) {
      chunked_rhs = ChunkArrayOperand_(builder, next_rhs);
    }

    for (uint64_t j = 0; j < num_chunks; j++) {
      chunked_result[j] = ewise_op->Emit(builder, chunked_result[j], chunked_rhs[j], nullptr);
      chunked_result[j] = reduction_op->Emit(builder, chunked_result[j], nullptr);
    }

    // Update the result and the rhs for the next round
    chunked_result = ChunkVectorOperand_(builder, MergeOperandsVector_(builder, chunked_result));
    num_chunks = chunked_result.size();
  }

  if (num_ops == this->strategy_.elemwise_op_names.size()) {
    if (chunked_result.size() > 1) {
      return MergeOperandsVector_(builder, chunked_result);
    }

    return chunked_result[0];
  }

  // At this point we only have one last operand in the list, so load it up and operate on it
  std::string &ewise_op_name = this->strategy_.elemwise_op_names.back();
  ElementWiseOp *ewise_op = elemwise_op_table_[ewise_op_name];

  llvm::Value *next_rhs = *operands.begin();
  operands = operands.drop_front(1);

  if (next_rhs->getType()->isVectorTy()) {
    chunked_rhs = ChunkVectorOperand_(builder, next_rhs);
  } else if (next_rhs->getType()->isArrayTy()) {
    chunked_rhs = ChunkArrayOperand_(builder, next_rhs);
  }

  for (uint64_t j = 0; j < num_chunks; j++) {
    chunked_result[j] = ewise_op->Emit(builder, chunked_result[j], chunked_rhs[j], nullptr);
  }

  llvm::Value *result = MergeOperandsVector_(builder, chunked_result);
  return result;
}
