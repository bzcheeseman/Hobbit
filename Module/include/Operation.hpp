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

#ifndef HOBBIT_OPERATION_HPP
#define HOBBIT_OPERATION_HPP

#include <list>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "Buffer.hpp"
#include "Functor.hpp"

namespace Hobbit {
  class Operation { // TODO: operations are composed of Kernels now, can reorder
                    // kernels that are not reductions
  public:
    explicit Operation(const std::string &name = "");
    explicit Operation(const std::initializer_list<Functor *> &f,
                       const std::string &name = "");
    explicit Operation(const std::list<Functor *> &f,
                       const std::string &name = "");

    // kernel -> functor -> operation (multiple functors)
    // functor allows many kernels to be emitted
    // functors apply kernels over a given range of indices, emit inline code
    // operations give functors their ranges and handle chunking/setup of the problem in nice ways

    // this way we can have an sdot functor, operation can loop over chunks of the input for sdot, and loop
    // over that for gemm/conv (already have buffer subindexing)
    // also no matter what will have to be able to loop over functors (gemm, conv == repeated sdot)

    void PushFunctor(Functor &f);
    void Emit(Function *f, Buffer *input, bool emit_inline);

    const std::string &GetName();

  private:
    std::list<Functor *> op_table_;
    const std::string name_;
  };
}

#endif // HOBBIT_OPERATION_HPP
