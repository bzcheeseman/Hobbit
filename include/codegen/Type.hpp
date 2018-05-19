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

#ifndef HOBBIT_TYPE_HPP
#define HOBBIT_TYPE_HPP

// LLVM
#include <llvm/IR/Module.h>

enum TypeID { // Datatypes are signed (so all ints are signed)
  INT1,
  INT8,
  INT16,
  INT32,
  INT64,
  FLOAT16,
  FLOAT32,
  FLOAT64,
};

inline llvm::Type *GetType(TypeID type, llvm::LLVMContext &ctx) {
  switch (type) {
  case INT1:
    return llvm::Type::getInt1Ty(ctx);
  case INT8:
    return llvm::Type::getInt8Ty(ctx);
  case INT16:
    return llvm::Type::getInt16Ty(ctx);
  case INT32:
    return llvm::Type::getInt32Ty(ctx);
  case INT64:
    return llvm::Type::getInt64Ty(ctx);
  case FLOAT16:
    return llvm::Type::getHalfTy(ctx);
  case FLOAT32:
    return llvm::Type::getFloatTy(ctx);
  case FLOAT64:
    return llvm::Type::getDoubleTy(ctx);
  default:
    return nullptr;
  }
}

#endif // HOBBIT_TYPE_HPP
