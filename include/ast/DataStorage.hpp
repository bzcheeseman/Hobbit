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

#ifndef HOBBIT_DATASTORAGE_HPP
#define HOBBIT_DATASTORAGE_HPP

// Project
#include <ast/Node.hpp>

// glog
#include <glog/logging.h>
// STL
#include <string>
#include <utility>
// LLVM
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm {
class LLVMContext;
class Value;
class BasicBlock;
class Type;
} // namespace llvm

namespace Hobbit {
namespace ast {

class Shape {
public:
  Shape(llvm::LLVMContext &ctx, llvm::ArrayRef<uint64_t> dims);
  explicit Shape(llvm::ArrayRef<uint64_t> dims);
  explicit Shape(llvm::ArrayRef<llvm::Value *> dims);

  void InitLLVM(llvm::LLVMContext &ctx);

  // Gets the number of dimensions for a tensor
  uint64_t NDim() const;

  // Gets a dimension of a tensor
  uint64_t Dim(uint64_t which) const;

  llvm::Type *DimType() const;

  // Gets a dimension of a tensor
  llvm::Value *Dim(llvm::Value *which) const;

  uint64_t Size() const;

  // Gets the overall size of a tensor
  llvm::Value *Size(llvm::BasicBlock *BB) const;

  uint64_t At(llvm::ArrayRef<uint64_t> idx) const;

  llvm::Value *At(llvm::ArrayRef<llvm::Value *> idx,
                  llvm::BasicBlock *BB) const;

  // BB can be nullptr, in which case the output is just a shape with llvm not
  // initialized
  Shape Flatten(llvm::BasicBlock *BB) const;

private:
  llvm::SmallVector<uint64_t, 4> m_dims_;
  bool m_has_llvm_;
  llvm::SmallVector<llvm::Value *, 4> m_v_dims_;
};

class Tensor;

class TensorChip {
public:
  TensorChip(Tensor &t, llvm::ArrayRef<uint64_t> start_idx, Shape shape)
      : m_parent_tensor_(t), m_start_idx_(start_idx.begin(), start_idx.end()),
        m_shape_(std::move(shape)) {
    CHECK_EQ(m_start_idx_.size(), m_shape_.NDim());
  }

  // Gets the number of dimensions for a tensor
  uint64_t NDim() const;

  // Gets a dimension of a tensor
  uint64_t Dim(uint64_t which) const;

  // Gets a dimension of a tensor
  llvm::Value *Dim(llvm::Value *which) const;

  uint64_t Size() const;

  // Gets the overall size of a tensor
  llvm::Value *Size(llvm::BasicBlock *BB) const;

  uint64_t At(llvm::ArrayRef<uint64_t> idx) const;

  llvm::Value *At(llvm::ArrayRef<llvm::Value *> idx,
                  llvm::BasicBlock *BB) const;

  // BB can be nullptr, in which case the output is just a shape with llvm not
  // initialized. Returns a reference to itself, now flattened.
  TensorChip &Flatten(llvm::BasicBlock *BB);

private:
  llvm::SmallVector<uint64_t, 4> m_start_idx_;
  Shape m_shape_;
  Tensor &m_parent_tensor_;
};

class Tensor : public Node { // constant or variable
public:
  Tensor(std::string name, llvm::ArrayRef<uint64_t> dims)
      : m_name_(std::move(name)), m_shape_(dims) {}
  Tensor(std::string name, llvm::ArrayRef<uint64_t> dims,
         llvm::LLVMContext &ctx)
      : m_name_(std::move(name)), m_shape_(ctx, dims) {}

  const std::string &GetName() const override { return m_name_; }
  NodeType GetNodeType() const override { return VariableID; };

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Tensor &t) {
    os << t.m_name_ << ": Shape = {";
    for (uint64_t i = 0; i < t.m_shape_.NDim(); i++) {
      os << t.m_shape_.Dim(i) << ", ";
    }
    os << "}\n";
    return os;
  }

  static inline bool classof(const Node *node) {
    return node->GetNodeType() == VariableID;
  }

  void InitLLVM(llvm::LLVMContext &ctx) { m_shape_.InitLLVM(ctx); }

  const Shape &GetShape() { return m_shape_; }

  void Reshape(const Shape &shape) { m_shape_ = shape; }

  llvm::Type *Type() {
    CHECK_NOTNULL(m_type_);
    return m_type_;
  }

  llvm::Value *Value() {
    CHECK_NOTNULL(m_data_);
    return m_data_;
  }

  void SetType(llvm::Type *type) {
    CHECK_NOTNULL(type);
    m_type_ = type;
  }

  void SetValue(llvm::Value *val) {
    CHECK_NOTNULL(val);
    m_data_ = val;
  }

  TensorChip Chip(llvm::ArrayRef<uint64_t> start_idx,
                  llvm::ArrayRef<uint64_t> dims) {
    return TensorChip(*this, start_idx, Shape(dims));
  }

  TensorChip Chip(llvm::ArrayRef<uint64_t> start_idx, Shape shape) {
    return TensorChip(*this, start_idx, std::move(shape));
  }

  TensorChip Chip(llvm::ArrayRef<uint64_t> start_idx,
                  llvm::ArrayRef<llvm::Value *> dims) {
    return TensorChip(*this, start_idx, Shape(dims));
  }

  // TODO: codegen

private:
  std::string m_name_;
  llvm::Type *m_type_;
  llvm::Value *m_data_ = nullptr; // can be nothing until we're ready to init
  Shape m_shape_;
};

} // namespace ast
} // namespace Hobbit

#endif // HOBBIT_DATASTORAGE_HPP
