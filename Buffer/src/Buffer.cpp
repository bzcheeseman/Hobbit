//
// Created by Aman LaChapelle on 2/22/18.
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

#include "Buffer.hpp"

Hobbit::Buffer::Buffer(llvm::BasicBlock *BB, llvm::Type *scalar_type,
                       const Hobbit::Shape &shape)
    : m_shape_(shape) {

  llvm::IRBuilder<> builder(BB);

  m_type_ = scalar_type;
  m_value_ =
      builder.CreateAlloca(m_type_, builder.getInt64(m_shape_.GetSize()));
  m_value_->setName("hobbit.buffer");
}

Hobbit::Buffer::Buffer(llvm::Value *value, llvm::Type *type,
                       const Hobbit::Shape &shape, Buffer *parent)
    : m_shape_(shape), m_value_(value), m_type_(type), m_parent_(parent) {
  ;
}

Hobbit::Buffer::Buffer(llvm::Value *value, const Hobbit::Shape &shape)
    : m_shape_(shape), m_value_(value) {
  llvm::Type *value_type = m_value_->getType();

  if (value_type->isPointerTy())
    m_type_ = value_type->getPointerElementType();
}

Hobbit::Buffer::~Buffer() {
  // If I have a parent, remove myself from their list
  if (m_parent_ != nullptr) {
    m_parent_->ClearChild(this); // remove myself from their list
  }

  // If I have children, delete them
  ClearChildren();
}

const Hobbit::Shape &Hobbit::Buffer::GetShape() { return m_shape_; }

llvm::Value *Hobbit::Buffer::GetValue() { return m_value_; }

llvm::Type *Hobbit::Buffer::GetType() { return m_type_; }

Hobbit::Buffer *Hobbit::Buffer::GetChunk(llvm::BasicBlock *BB,
                                         const Hobbit::Range &k_range,
                                         const Hobbit::Range &h_range,
                                         const Hobbit::Range &w_range) {
  llvm::IRBuilder<> builder(BB);

  Range idx_range = m_shape_.GetChunkIdx(k_range, h_range, w_range);
  Shape chunk_shape = m_shape_.GetChunkShape(k_range, h_range, w_range);

  llvm::Value *chunk_start_ptr =
      builder.CreateGEP(m_value_, builder.getInt32(idx_range.start));
  chunk_start_ptr->setName(m_value_->getName() + ".chunk");

  Buffer *output = new Buffer(chunk_start_ptr, m_type_, chunk_shape, this);

  this->m_children_.push_back(output);

  return output;
}

Hobbit::Buffer *Hobbit::Buffer::Flatten() {
  Shape flat_shape(1, 1, m_shape_.GetSize());

  Buffer *output = new Buffer(m_value_, m_type_, flat_shape, this);

  this->m_children_.push_back(output);

  return output;
}

void Hobbit::Buffer::ClearChild(Hobbit::Buffer *child) {
  auto iter = std::find(m_children_.begin(), m_children_.end(), child);
  if (iter != m_children_.end()) {
    m_children_.erase(iter);
  }
}

void Hobbit::Buffer::ClearChildren() { this->m_children_.clear(); }

llvm::Value *Hobbit::Buffer::GetElement(llvm::BasicBlock *BB,
                                        const uint64_t &idx) {
  llvm::IRBuilder<> builder(BB);

  return builder.CreateLoad(builder.CreateGEP(m_value_, builder.getInt64(idx)),
                            m_value_->getName() + ".element." +
                                std::to_string(idx));
}

llvm::Value *Hobbit::Buffer::GetElement(llvm::BasicBlock *BB, const uint64_t &k,
                                        const uint64_t &h, const uint64_t &w) {
  uint64_t idx = m_shape_.At(k, h, w);

  return this->GetElement(BB, idx);
}
