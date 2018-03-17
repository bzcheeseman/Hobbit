//
// Created by Aman LaChapelle on 3/17/18.
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


#ifndef HOBBIT_TENSOR_HPP
#define HOBBIT_TENSOR_HPP

#include <llvm/IR/Type.h>

#include "Symbol.hpp"
#include "Shape.hpp"

namespace Hobbit { namespace core {
  class Function;

  class Tensor {
  public:
    explicit Tensor(Symbol *s);

    const Symbol *GetSymbol();
    llvm::Type *GetType();
    const Shape &GetShape();

  protected:
    Symbol *s_;
  };
}}


#endif //HOBBIT_TENSOR_HPP