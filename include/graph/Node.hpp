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

// STL
#include <string>
// LLVM
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm {
class raw_ostream;
}

namespace Hobbit {
namespace codegen {
class TreeBuilder;
}
namespace graph {

class Node {
public:
  enum NodeType {
    VARIABLE_ID,
    OPERATION_ID,
  };

  explicit Node(std::string name) : m_name_(std::move(name)) {}

  const std::string &GetName() const { return m_name_; }
  virtual NodeType GetNodeType() const = 0;

  virtual void Print(llvm::raw_ostream &os) const = 0;

protected:
  std::string m_name_;
};

} // namespace graph
} // namespace Hobbit

#endif // HOBBIT_NODE_HPP
