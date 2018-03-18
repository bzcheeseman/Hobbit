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

#ifndef HOBBIT_TYPE_HPP
#define HOBBIT_TYPE_HPP

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

namespace Hobbit {
  namespace core {

    template <typename T, unsigned int BITWIDTH> class Type { ; };

    template <> class Type<float, 16> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getHalfTy(*ctx);
      }
    };

    template <> class Type<float *, 16> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getHalfPtrTy(*ctx);
      }
    };

    template <> class Type<float, 32> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getFloatTy(*ctx);
      }
    };

    template <> class Type<float *, 32> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getFloatPtrTy(*ctx);
      }
    };

    template <> class Type<double, 64> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getDoubleTy(*ctx);
      }
    };

    template <> class Type<double *, 64> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getDoublePtrTy(*ctx);
      }
    };

    template <unsigned int BITWIDTH> class Type<int, BITWIDTH> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getIntNTy(*ctx, BITWIDTH);
      }
    };

    template <unsigned int BITWIDTH> class Type<int *, BITWIDTH> {
    public:
      static llvm::Type *get(llvm::LLVMContext *ctx) {
        return llvm::Type::getIntNPtrTy(*ctx, BITWIDTH);
      }
    };
  }
}

#endif // HOBBIT_TYPE_HPP
