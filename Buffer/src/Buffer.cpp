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

Hobbit::Buffer::Buffer(llvm::IRBuilder<> &builder, llvm::Type *scalar_type,
                       const Hobbit::Shape &shape)
    : m_shape_(shape) {
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

Hobbit::Buffer *Hobbit::Buffer::GetChunk(llvm::IRBuilder<> &builder,
                                         const Hobbit::Range &k_range,
                                         const Hobbit::Range &h_range,
                                         const Hobbit::Range &w_range) {
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

llvm::ArrayRef<llvm::Value *> Hobbit::Buffer::Pack(llvm::IRBuilder<> &builder,
                                                   const uint32_t &vector_size) {
  llvm::Type *vector_type =
      llvm::VectorType::get(m_type_, vector_size); // m_type_ is a scalar type
  uint32_t leftovers = m_shape_.GetSize() % vector_size;
  uint32_t num_vectors = (m_shape_.GetSize() - leftovers) / vector_size;

  std::vector<llvm::Value *> output (num_vectors);
  for (uint32_t i = 0; i < num_vectors; i++) {
    output[i] = llvm::UndefValue::get(vector_type);
    for (uint32_t j = 0; j < vector_size; j++) {
      llvm::Value *elt = builder.CreateLoad(
          builder.CreateGEP(m_value_, builder.getInt32(i * vector_size + j)));
      output[i] = builder.CreateInsertElement(output[i], elt, j);
    }
    output[i]->setName(m_value_->getName() + ".pack" + std::to_string(i));
  }

  if (leftovers != 0) {
    llvm::Value *val = llvm::UndefValue::get(vector_type);
    for (uint32_t j = 0; j < leftovers; j++) {
      llvm::Value *elt = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt32(num_vectors * vector_size + j)));
      val = builder.CreateInsertElement(val, elt, j);
    }
    val->setName(m_value_->getName() + ".pack" + std::to_string(num_vectors));
    output.push_back(val);
  }

  return output;
}

Hobbit::Constant::Constant(llvm::IRBuilder<> &builder, llvm::Type *type,
                           Hobbit::CompileTimeBuffer &buffer)
    : Buffer(buffer.GetValue(builder, type), buffer.GetShape()) {}

Hobbit::Variable::Variable(llvm::Value *value, llvm::Type *scalar_type,
                           const Hobbit::Shape &shape)
    : Buffer(value, scalar_type, shape, nullptr) {}
