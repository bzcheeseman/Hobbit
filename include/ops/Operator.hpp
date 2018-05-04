//
// Created by Aman LaChapelle on 4/8/18.
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

#ifndef HOBBIT_OPERATOR_HPP
#define HOBBIT_OPERATOR_HPP

namespace llvm {
class Function;
class BasicBlock;
class Type;
}

namespace Hobbit {
namespace ops {
class Operator { // TODO: only create through Module
public:
  enum OperatorType {
#include "OperatorTypes.def"
  };

  virtual OperatorType GetOperatorType() const = 0;
  virtual llvm::BasicBlock *InsertIntoFunction(llvm::Function *) = 0;
  virtual llvm::Type *GetOutputType() const = 0;
  virtual llvm::ArrayRef<uint64_t> GetOutputShape() const = 0;
};

//class NoOp : public Operator {
//public:
//  OperatorType GetOperatorType() const override { return noopID; }
//
//  static inline bool classof(const Operator *op) {
//    return op->GetOperatorType() == noopID;
//  }
//
//  virtual llvm::BasicBlock *InsertIntoFunction(llvm::Function *f) override {
//    return &*--f->end();
//  }
//};
} // namespace ops
} // namespace Hobbit

#endif // HOBBIT_OPERATOR_HPP
