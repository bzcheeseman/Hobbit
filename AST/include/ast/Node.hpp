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

#ifndef HOBBIT_NODE_HPP
#define HOBBIT_NODE_HPP

#include <string>
#include <vector>

namespace Hobbit {
class Visitor;

namespace ast {

class Node {
public:
  enum NodeType {
#include "NodeTypes.def"
  };

  virtual const std::string &GetName() const = 0;
  virtual NodeType GetNodeType() const = 0;

  //  virtual llvm::BasicBlock *InsertIntoFunction(llvm::Function *) = 0;

  virtual const std::vector<Node *> &Consumers() { return mb_consumers_; }
  virtual const std::vector<Node *> &Producers() { return mb_producers_; }

  virtual std::vector<Node *> &MutableConsumers() { return mb_consumers_; }
  virtual std::vector<Node *> &MutableProducers() { return mb_producers_; }

protected:
  std::vector<Node *> mb_consumers_; // these use this node
  std::vector<Node *> mb_producers_; // this node uses these
};

} // namespace ast
} // namespace Hobbit

#endif // HOBBIT_NODE_HPP
