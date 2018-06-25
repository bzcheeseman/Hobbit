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

#ifndef HOBBIT_VARIABLE_HPP
#define HOBBIT_VARIABLE_HPP

#include "Node.hpp"
#include "Operation.hpp"
#include "Shape.hpp"

namespace llvm {
class Value;
}

namespace Hobbit {
namespace graph {
class Variable : public Node {
public:
  explicit Variable(const std::string &name, llvm::Type *type = nullptr,
                    Node *creator = nullptr);
  Variable(const std::string &name, Shape *shape, llvm::Type *type = nullptr,
           Node *creator = nullptr);
  Variable(const std::string &name, std::unique_ptr<Shape> &&shape,
           llvm::Type *type = nullptr, Node *creator = nullptr);

  // LLVM-style RTTI (llvm::dyn_cast/llvm::cast)
  NodeType GetNodeType() const override;
  static inline bool classof(const Node *node) {
    return node->GetNodeType() == VARIABLE_ID;
  }

  void Print(llvm::raw_ostream &os) const override;

  llvm::Value *GetVal() const;
  void SetVal(llvm::Value *val);

  llvm::Type *GetType() const;
  void SetType(llvm::Type *type);

  Node *Creator();

  Shape &GetShape() const;

private:
  std::unique_ptr<Shape> m_shape_; // Variable owns its shape
  llvm::Type *m_type_;
  llvm::Value *m_val_;
  Node *m_creator_;
};
} // namespace graph
} // namespace Hobbit

#endif // HOBBIT_VARIABLE_HPP
