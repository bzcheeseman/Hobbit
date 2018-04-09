//
// Created by Aman LaChapelle on 3/22/18.
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

#include "Node.hpp"
#include "Visitor.hpp"

#include <sstream>

#include <glog/logging.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

Hobbit::ast::Tensor *Hobbit::ast::Tensor::Create(const std::string &name,
                                                 Hobbit::ast::Tensor *parent,
                                                 llvm::ArrayRef<uint64_t> dims,
                                                 llvm::Type *type) {
  Tensor *t = new Tensor(type, std::move(dims));
  t->name_ = name;
  t->parent_ = parent;

  return std::move(t);
}

llvm::Type *Hobbit::ast::Tensor::GetType() { return llvm_type_; }

llvm::Value *Hobbit::ast::Tensor::GetValue() { return llvm_value_; }

void Hobbit::ast::Tensor::SetValue(llvm::Value *value) {
  llvm_value_ = std::move(value);
  llvm_value_->setName(name_);
}

uint64_t Hobbit::ast::Tensor::NDim() { return dims_.size(); }

uint64_t Hobbit::ast::Tensor::Dim(uint64_t which) { return dims_[which]; }

uint64_t Hobbit::ast::Tensor::Size() {
  uint64_t total_size = 1;
  for (auto &dim : dims_) {
    total_size *= dim;
  }
  return total_size;
}

Hobbit::ast::Tensor *Hobbit::ast::Tensor::Flatten() {
  this->dims_ = {this->Size()};
  return this;
}

Hobbit::ast::Tensor::Tensor(llvm::Type *type, llvm::ArrayRef<uint64_t> dims,
                            llvm::ArrayRef<uint64_t> start_idx)
    : dims_(dims.begin(), dims.end()), llvm_type_(type) {
  llvm::IntegerType *int64ty = llvm::Type::getInt64Ty(llvm_type_->getContext());

  for (auto &dim : dims_) {
    value_dims_.push_back(llvm::ConstantInt::get(int64ty, dim));
  }

  if (start_idx.empty() || (start_idx.size() == 1 && *start_idx.begin() == 0)) {
    for (uint64_t i = 0; i < dims.size(); i++) {
      start_idx_.push_back(0);
    }
  } else {
    start_idx_ = std::move(
        llvm::SmallVector<uint64_t, 4>(start_idx.begin(), start_idx.end()));
  }
}

Hobbit::ast::Tensor::~Tensor() {
  // clean up children
  std::for_each(children_.begin(), children_.end(),
                [](Tensor *t) { delete t; });
}

uint64_t Hobbit::ast::Tensor::At(llvm::ArrayRef<uint64_t> idx) {
  CHECK_EQ(idx.size(), dims_.size());

  uint64_t out = 0;
  for (uint64_t i = 0; i < dims_.size() - 1; i++) {
    out += idx[i];
    out *= dims_[i + 1];
  }
  out += *(idx.end() - 1);

  return out;
}

llvm::Value *Hobbit::ast::Tensor::At(llvm::ArrayRef<llvm::Value *> idx,
                                     llvm::BasicBlock *BB) {
  CHECK_EQ(idx.size(), dims_.size());

  llvm::Value *out = llvm::ConstantInt::get(value_dims_[0]->getType(), 0);
  for (uint64_t i = 0; i < dims_.size() - 1; i++) {
    out = llvm::BinaryOperator::CreateAdd(out, idx[i], "", BB);
    out = llvm::BinaryOperator::CreateMul(out, value_dims_[i + 1], "", BB);
  }
  out = llvm::BinaryOperator::CreateAdd(out, *(idx.end() - 1), "", BB);

  return out;
}
