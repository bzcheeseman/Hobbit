//
// Created by Aman LaChapelle on 4/27/18.
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

// Project
#include <graph/DataStorage.hpp>

// STL
#include <numeric>

// glog
#include <glog/logging.h>

// LLVM
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>

Hobbit::ast::Shape::Shape(llvm::LLVMContext &ctx,
                               llvm::ArrayRef<uint64_t> dims)
    : m_dims_(dims.begin(), dims.end()) {
  for (auto &dim : dims) {
    m_v_dims_.push_back(
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), dim, false));
  }
  m_has_llvm_ = true;
}

Hobbit::ast::Shape::Shape(llvm::ArrayRef<uint64_t> dims)
    : m_dims_(dims.begin(), dims.end()), m_has_llvm_(false) {}

Hobbit::ast::Shape::Shape(llvm::ArrayRef<llvm::Value *> dims)
    : m_v_dims_(dims.begin(), dims.end()), m_has_llvm_(true) {
  llvm::ConstantInt *d;
  for (auto &dim : dims) {
    d = llvm::dyn_cast<llvm::ConstantInt>(dim);
    if (d != nullptr) m_dims_.push_back(d->getZExtValue());
  }
}

void Hobbit::ast::Shape::InitLLVM(llvm::LLVMContext &ctx) {
  for (auto &dim : m_dims_) {
    m_v_dims_.push_back(
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), dim, false));
  }
  m_has_llvm_ = true;
}

uint64_t Hobbit::ast::Shape::NDim() const { return m_has_llvm_ ? m_v_dims_.size() : m_dims_.size(); }

uint64_t Hobbit::ast::Shape::Dim(uint64_t which) const {
  CHECK_LT(which, m_dims_.size());
  return m_dims_[which];
}

llvm::Type *Hobbit::ast::Shape::DimType() const {
  CHECK(m_has_llvm_) << "LLVM Value dims not initialized";
  return m_v_dims_[0]->getType();
}

llvm::Value *Hobbit::ast::Shape::Dim(llvm::Value *which) const {
  CHECK(m_has_llvm_) << "LLVM Value dims not initialized";

  llvm::ConstantInt *which_dim = llvm::dyn_cast<llvm::ConstantInt>(which);
  CHECK_NOTNULL(which_dim);

  CHECK_LT(which_dim->getZExtValue(), m_v_dims_.size());
  return m_v_dims_[which_dim->getZExtValue()];
}

uint64_t Hobbit::ast::Shape::Size() const {
  return std::accumulate(m_dims_.begin(), m_dims_.end(), (uint64_t)1,
                         std::multiplies<uint64_t>());
}

llvm::Value *Hobbit::ast::Shape::Size(llvm::BasicBlock *BB) const {
  CHECK(m_has_llvm_) << "LLVM Value dims not initialized";
  llvm::Value *out = llvm::ConstantInt::get(m_v_dims_[0]->getType(), 1);
  for (auto &dim : m_v_dims_) {
    out = llvm::BinaryOperator::CreateMul(out, dim, "", BB);
  }
  return out;
}

uint64_t Hobbit::ast::Shape::At(llvm::ArrayRef<uint64_t> idx) const {
  CHECK_EQ(idx.size(), m_dims_.size());

  uint64_t out = 0;
  for (uint64_t i = 0; i < m_dims_.size() - 1; i++) {
    out += idx[i];
    out *= m_dims_[i + 1];
  }
  out += *(idx.end() - 1);

  return out;
}

llvm::Value *Hobbit::ast::Shape::At(llvm::ArrayRef<llvm::Value *> idx,
                                         llvm::BasicBlock *BB) const {
  CHECK(m_has_llvm_) << "LLVM Value dims not initialized";
  CHECK_EQ(idx.size(), m_dims_.size());

  llvm::Value *out = llvm::ConstantInt::get(m_v_dims_[0]->getType(), 0);
  for (uint64_t i = 0; i < m_dims_.size() - 1; i++) {
    out = llvm::BinaryOperator::CreateAdd(out, idx[i], "", BB);
    out = llvm::BinaryOperator::CreateMul(out, m_v_dims_[i + 1], "", BB);
  }
  out = llvm::BinaryOperator::CreateAdd(out, *(idx.end() - 1), "", BB);

  return out;
}

Hobbit::ast::Shape Hobbit::ast::Shape::Flatten(llvm::BasicBlock *BB) const {
  if (!m_has_llvm_ || BB == nullptr) {
    llvm::ArrayRef<uint64_t> flattened_dims = {this->Size()};
    return Hobbit::ast::Shape(flattened_dims);
  }
  CHECK_NOTNULL(BB);
  llvm::ArrayRef<llvm::Value *> flattened_dims = {this->Size(BB)};
  return Hobbit::ast::Shape(flattened_dims);
}
