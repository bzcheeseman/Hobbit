//
// Created by Aman LaChapelle on 2/17/18.
//
// c_science
// Copyright (c) 2018 Aman LaChapelle
// Full license at c_science/LICENSE.txt
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

#ifndef C_SCIENCE_OPERATOR_HPP
#define C_SCIENCE_OPERATOR_HPP

#include <llvm/IR/IRBuilder.h>

namespace Hobbit {

  class Operator {
  public:
    virtual bool SetConstant(llvm::Value *constant) = 0; // move down one level?


  protected:
    llvm::Type *void_t;
    llvm::Type *i1_t, *i8_t, *i16_t, *i32_t, *i64_t;
    llvm::Type *f16_t, *f32_t, *f64_t;

    llvm::Type *i1x64, *i1x128, *i1x256;
    llvm::Type *i8x8, *i8x16, *i8x32;
    llvm::Type *i16x4, *i16x8, *i16x16;
    llvm::Type *i32x2, *i32x4, *i32x8;
    llvm::Type *i64x2, *i64x4;

    llvm::Type *f16x4, *f16x8, *f16x16;
    llvm::Type *f32x2, *f32x4, *f32x8;
    llvm::Type *f64x2, *f64x4;
  };
}

#endif // C_SCIENCE_OPERATOR_HPP
