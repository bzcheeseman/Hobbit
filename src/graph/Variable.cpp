//
// Created by Aman LaChapelle on 6/10/18.
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

#include <graph/Variable.hpp>

// glog
#include <glog/logging.h>


Hobbit::graph::Variable::Variable(const std::string &name, llvm::Type *type, Hobbit::graph::Node *creator)
        : Node(name), m_shape_(nullptr), m_val_(nullptr), m_type_(type),
          m_creator_(creator) {}

Hobbit::graph::Variable::Variable(const std::string &name, Hobbit::graph::Shape *shape, llvm::Type *type,
                                  Hobbit::graph::Node *creator)
        : Node(name), m_shape_(std::move(shape)), m_val_(nullptr), m_type_(type),
          m_creator_(creator) {}

Hobbit::graph::Variable::Variable(const std::string &name, std::unique_ptr<Hobbit::graph::Shape> &&shape,
                                  llvm::Type *type, Hobbit::graph::Node *creator)
        : Node(name), m_shape_(std::move(shape)), m_val_(nullptr), m_type_(type),
          m_creator_(creator) {}

Hobbit::graph::Node::NodeType Hobbit::graph::Variable::GetNodeType() const { return VariableID; }

void Hobbit::graph::Variable::Print(llvm::raw_ostream &os) const {
  os << "Variable: " << m_name_;
  if (m_shape_ != nullptr) {
    os << " Shape: {";
    for (uint64_t i = 0; i < m_shape_->NDim(); i++) {
      os << m_shape_->Dim(i) << ", ";
    }
    os << "}";
  }
  os << "\n";
}

llvm::Value *Hobbit::graph::Variable::GetVal() const { return m_val_; }

void Hobbit::graph::Variable::SetVal(llvm::Value *val) {
  CHECK_NOTNULL(val);
  m_val_ = val;
}

llvm::Type *Hobbit::graph::Variable::GetType() const { return m_type_; }

void Hobbit::graph::Variable::SetType(llvm::Type *type) {
  CHECK_NOTNULL(type);
  m_type_ = type;
}

Hobbit::graph::Node *Hobbit::graph::Variable::Creator() { return m_creator_; }

Hobbit::graph::Shape &Hobbit::graph::Variable::GetShape() const { return *m_shape_; }
