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

#ifndef HOBBIT_DAGNODE_HPP
#define HOBBIT_DAGNODE_HPP

#include <CompositeOp.hpp>

namespace Hobbit {

  struct DAGNode {   // corresponds to one 'kernel'
    DAGNode *source; // one source

    DAGNode *sink; // one sink

    // can have multiple operators by creating a new composite operator
    CompositeOp *op;
  };
}

#endif // HOBBIT_DAGNODE_HPP
