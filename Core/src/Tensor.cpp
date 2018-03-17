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


#include "Tensor.hpp"

Hobbit::core::Tensor::Tensor(Hobbit::core::Symbol *s): s_(s) {

}

llvm::Type *Hobbit::core::Tensor::GetType() {
  return s_->type;
}

const Hobbit::Shape &Hobbit::core::Tensor::GetShape() {
  return s_->shape;
}

const Hobbit::core::Symbol *Hobbit::core::Tensor::GetSymbol() {
  return s_;
}
