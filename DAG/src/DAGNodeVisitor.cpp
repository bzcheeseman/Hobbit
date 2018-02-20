//
// Created by Aman LaChapelle on 2/18/18.
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

#include "DAGNodeVisitor.hpp"

void Hobbit::FusionVisitor::Visit(Hobbit::DAGNode *node) {

  // Move the operators into this node
  node->op->PushOperator(node->sink->op);

  // Link the child's sinks to this node
  node->sink = std::move(node->sink->sink);

  // And finally delete the child
  delete node->sink;
}

Hobbit::ReplacementVisitor::ReplacementVisitor(Hobbit::CompositeOp *op) {
  this->op = std::move(op);
}

void Hobbit::ReplacementVisitor::Visit(Hobbit::DAGNode *node) {
  node->op = this->op; // replace the operation with a new one do nothing else
}
