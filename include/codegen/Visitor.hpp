//
// Created by Aman LaChapelle on 5/3/18.
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

#ifndef HOBBIT_VISITOR_HPP
#define HOBBIT_VISITOR_HPP

// STL
#include <list>
#include <set>

namespace llvm {
class raw_ostream;
}

namespace Hobbit {

namespace graph {
class Node;
class Variable;
class Operation;
} // namespace graph

namespace codegen {

class Function;

class Visitor {
public:
  void BuildTree(graph::Node *root);

  // TODO: should these be mutable?
  std::list<graph::Operation *> &Tree();
  std::set<graph::Variable *> &Args();

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os, Visitor &v);
  friend std::ostream &operator<<(std::ostream &os, Visitor &v);

private:
  void BuildTree_(graph::Node *root);
  void SortTree_();

private:
  std::list<graph::Operation *> m_ops_;
  std::set<graph::Variable *> m_args_;
};

} // namespace codegen
} // namespace Hobbit

#endif // HOBBIT_VISITOR_HPP
