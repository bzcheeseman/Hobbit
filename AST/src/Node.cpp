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

Hobbit::Function *Hobbit::Function::Create(const std::string &name) {
  Function *f = new Function;
  f->name_ = "hobbit." + name;
  f->child_ = nullptr;
  f->parent_ = nullptr;

  return f;
}

Hobbit::Tensor *Hobbit::Function::GetNewArg(const std::string &name,
                                            llvm::SmallVector<uint64_t, 4> dims,
                                            llvm::Type *type) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateVariable(name, this, std::move(dims), type);
  // Add the tensor to the arg table
  arg_table_.push_back(arg);
  return arg;
}

Hobbit::Tensor *
Hobbit::Function::GetNewAlloca(const std::string &name,
                               llvm::SmallVector<uint64_t, 4> dims,
                               llvm::Type *type) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateVariable(name, this, std::move(dims), type);
  // Add the tensor to the arg table
  alloca_table_.push_back(arg);
  return arg;
}

 Hobbit::Tensor *
 Hobbit::Function::GetNewArg(const std::string &name,
 llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type,
                                 void *buffer) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateConstant(name, this, std::move(dims), type,
  buffer);
  // Add the tensor to the arg table
  arg_table_.push_back(arg);
  return arg;
}

void Hobbit::Function::SetArg(Hobbit::Tensor *t) {
  // If we've already added this arg to the alloca table then erase it - it
  // should be an arg
  for (auto &alloca : alloca_table_) {
    if (t == alloca)
      alloca_table_.erase(&alloca);
  }
  arg_table_.push_back(t);
}

void Hobbit::Function::PushNode(Hobbit::ast::Node *node) {
  if (child_ == nullptr) {
    child_ = std::move(node);
    last_node_ = child_;
    return;
  }

  // Update the child of the last node
  last_node_->SetChild(node);
  // Update the last node
  last_node_ = last_node_->GetChild();
}

Hobbit::Tensor Hobbit::Tensor::CreateVariable(
    const std::string &name, Hobbit::ast::Node *parent,
    llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type) {
  Tensor t(nullptr, std::move(dims), type);
  t.name_ = name;
  t.parent_ = parent;

  return std::move(t);
}

Hobbit::Tensor
Hobbit::Tensor::CreateConstant(const std::string &name, Node *parent,
                                  llvm::SmallVector<uint64_t, 4> dims,
                                  llvm::Type *type, void *buffer) {
  Tensor t = std::move(CreateVariable(name, parent, std::move(dims), type));
  t.name_ = name;

  uint64_t total_size = t.Size();

  std::vector<llvm::Constant *> buffer_constants;
  if (type->isPointerTy()) {
    type = type->getPointerElementType();
  }

  if (type->isFloatingPointTy()) {
    if (type->isFloatTy()) {
      float *buf = (float *)buffer;
      for (uint64_t i = 0; i < total_size; i++) {
        buffer_constants.push_back(llvm::ConstantFP::get(type,
        (double)buf[i]));
      }
    }
    if (type->isDoubleTy()) {
      double *buf = (double *)buffer;
      for (uint64_t i = 0; i < total_size; i++) {
        buffer_constants.push_back(llvm::ConstantFP::get(type, buf[i]));
      }
    }
  } else if (type->isIntegerTy(64)) {
    uint64_t *buf = (uint64_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(llvm::ConstantInt::get(type, buf[i], true));
    }
  } else if (type->isIntegerTy(32)) {
    uint32_t *buf = (uint32_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
          llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  } else if (type->isIntegerTy(16)) {
    uint16_t *buf = (uint16_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
          llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  } else if (type->isIntegerTy(8)) {
    uint8_t *buf = (uint8_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
          llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  } else if (type->isIntegerTy(1)) {
    bool *buf = (bool *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
          llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  } else {
    LOG(FATAL) << "Unsupported type";
  }

  llvm::ArrayType *arr_type =
      llvm::ArrayType::get(type, buffer_constants.size());
  t.llvm_buffer_ = llvm::ConstantArray::get(arr_type, buffer_constants);
  t.llvm_type_ = t.llvm_buffer_->getType();
  buffer_constants.clear();

  return t;
}

llvm::Type *Hobbit::Tensor::GetType() { return llvm_type_; }

void Hobbit::Tensor::SetBuffer(llvm::Value *val) {
  this->llvm_buffer_ = val;
  this->llvm_type_ = val->getType();

  // set the name of the buffer
  this->llvm_buffer_->setName(this->name_);
}

llvm::Value *Hobbit::Tensor::GetBuffer() { return llvm_buffer_; }

uint64_t Hobbit::Tensor::NDim() { return dims_.size(); }

uint64_t Hobbit::Tensor::Dim(uint64_t which) { return dims_[which]; }

uint64_t Hobbit::Tensor::Size() {
  uint64_t total_size = 1;
  for (auto &dim : dims_) {
    total_size *= dim;
  }
  return total_size;
}

Hobbit::Tensor *Hobbit::Tensor::Chip(llvm::SmallVector<uint64_t, 4> start_idx,
                                     llvm::SmallVector<uint64_t, 4> dims) {
  // create a new tensor that aliases this
  Tensor *chip = new Tensor(nullptr, std::move(dims), this->llvm_type_, start_idx);
  chip->parent_ = this;
  this->child_ = chip;

  return chip;
}

Hobbit::Tensor *Hobbit::Tensor::Flatten() {
  this->dims_ = {this->Size()};
  return this;
}

Hobbit::Tensor::Tensor(llvm::Value *ptr, llvm::SmallVector<uint64_t, 4> dims,
                       llvm::Type *type, llvm::SmallVector<uint64_t, 4> start_idx)
    : llvm_buffer_(ptr), dims_(std::move(dims)), llvm_type_(type),
      Node(nullptr, nullptr) {
  if (start_idx == {0}) {
    for (uint64_t i = 0; i < dims.size(); i++) {
      start_idx_.push_back(0);
    }
  }
  else {
    start_idx_ = std::move(start_idx);
  }
}

uint64_t Hobbit::Tensor::At_(llvm::SmallVector<uint64_t, 4> idx) {
  if (idx.size() != dims_.size())
    LOG(FATAL) << "Incorrect number of indices";

  uint64_t out = 0;
  for (uint64_t i = 0; i < dims_.size() - 1; i++) {
    out += idx[i];
    out *= dims_[i + 1];
  }
  out += *idx.end();

  return out;
}
