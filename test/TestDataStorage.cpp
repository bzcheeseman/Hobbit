//
// Created by Aman LaChapelle on 4/30/18.
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

#include <gtest/gtest.h>

#include <graph/DataStorage.hpp>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_ostream.h>

#include "TestHelper.hpp"

namespace { // Maybe wait until we use the tensors a little bit to test them...
TEST(Shape, CreateShapeNoCtx) {
  llvm::SmallVector<uint64_t, 3> dims = {
      32, 44, 0}; // should be able to handle a zero dimension
  for (int i = 0; i < 15; i++) {
    EXPECT_NO_THROW(Hobbit::graph::Shape shape(dims));
    ++dims[2];
  }
}

TEST(Shape, AtNoCtx) {
  llvm::SmallVector<uint64_t, 3> dims = {32, 44, 2348};
  llvm::SmallVector<uint64_t, 3> idx = {3, 5, 1234};
  uint64_t correct_idx = ((idx[0] * dims[1]) + idx[1]) * dims[2] + idx[2];
  Hobbit::graph::Shape shape(dims);
  EXPECT_EQ(shape.At(idx), correct_idx);
}

TEST(Shape, FlattenNoCtx) {
  llvm::SmallVector<uint64_t, 3> dims = {32, 44, 2348};
  Hobbit::graph::Shape shape(dims);
  Hobbit::graph::Shape flat = shape.Flatten(nullptr);
  EXPECT_EQ(flat.NDim(), 1);
  EXPECT_EQ(shape.Size(), flat.Dim((uint64_t)0));
}

TEST(Shape, CreateShapeCtx) {
  llvm::SmallVector<uint64_t, 3> dims = {
      32, 44, 0}; // should be able to handle a zero dimension
  llvm::LLVMContext ctx;
  for (int i = 0; i < 15; i++) {
    EXPECT_NO_THROW(Hobbit::graph::Shape shape(ctx, dims));
    ++dims[2];
  }
}

TEST(Shape, AtCtx) {
  llvm::LLVMContext ctx;

  llvm::SmallVector<uint64_t, 3> dims = {32, 44, 2348};
  llvm::SmallVector<uint64_t, 3> idx = {3, 5, 1234};

  llvm::Type *uint64_type = llvm::Type::getInt64Ty(ctx);

  llvm::SmallVector<llvm::Value *, 3> idx_v = {
      llvm::ConstantInt::get(uint64_type, 3),
      llvm::ConstantInt::get(uint64_type, 5),
      llvm::ConstantInt::get(uint64_type, 1234)};
  uint64_t correct_idx = ((idx[0] * dims[1]) + idx[1]) * dims[2] + idx[2];

  auto module = CreateLLVMModule(ctx, "TestModule");
  llvm::Function *func = CreateLLVMFunction(module, "AtCtx", uint64_type);
  EXPECT_TRUE(func != nullptr);
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(ctx, "AtBB", func);

  Hobbit::graph::Shape shape(ctx, dims);
  EXPECT_EQ(shape.At(idx), correct_idx);
  llvm::Value *at_val = shape.At(idx_v, BB);

  llvm::IRBuilder<> builder(BB);
  builder.CreateRet(at_val);

  FinalizeModule(module, 3, llvm::sys::getDefaultTargetTriple(), "corei7-avx",
                 "+avx,+sse,+x87,+cx16");

  uint64_t retval = 0;
  for (auto iter = BB->begin(), end = BB->end(); iter != end; ++iter) {
    llvm::ReturnInst *ret = llvm::dyn_cast<llvm::ReturnInst>(&*iter);
    if (ret != nullptr) {
      retval =
          llvm::dyn_cast<llvm::ConstantInt>(ret->getOperand(0))->getZExtValue();
    }
  }

  EXPECT_EQ(retval, correct_idx);
}

TEST(Shape, FlattenCtx) {
  llvm::LLVMContext ctx;
  llvm::SmallVector<uint64_t, 3> dims = {32, 44, 2348};
  Hobbit::graph::Shape shape(ctx, dims);

  llvm::Type *uint64_type = llvm::Type::getInt64Ty(ctx);

  llvm::Value *zero = llvm::ConstantInt::get(uint64_type, 0);

  auto module = CreateLLVMModule(ctx, "TestModule");
  llvm::Function *func = CreateLLVMFunction(module, "FlattenCtx", uint64_type);
  EXPECT_TRUE(func != nullptr);
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(ctx, "flatBB", func);

  Hobbit::graph::Shape flat = shape.Flatten(BB);

  llvm::IRBuilder<> builder(BB);
  builder.CreateRet(flat.Dim(zero));

  FinalizeModule(module, 3, llvm::sys::getDefaultTargetTriple(), "corei7-avx",
                 "+avx,+sse,+x87,+cx16");

  module->print(llvm::outs(), nullptr);

  EXPECT_EQ(flat.NDim(), 1);

  uint64_t retval = 0;
  for (auto iter = BB->begin(), end = BB->end(); iter != end; ++iter) {
    llvm::ReturnInst *ret = llvm::dyn_cast<llvm::ReturnInst>(&*iter);
    if (ret != nullptr) {
      retval =
          llvm::dyn_cast<llvm::ConstantInt>(ret->getOperand(0))->getZExtValue();
    }
  }

  EXPECT_EQ(retval, shape.Size());
}; // TODO: add tests for Tensor (some small ones for now, not a lot of codegen
   // to be done?)
;  // TODO: add tests for TensorChip
} // namespace
