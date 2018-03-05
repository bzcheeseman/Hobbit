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

#include <random>

#include <gtest/gtest.h>

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/OrcMCJITReplacement.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include "Buffer.hpp"

TEST(TestBuffer, CreateEmpty) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  std::unique_ptr<llvm::Module> Mod =
      llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(
      Mod->getOrInsertFunction("create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getFloatTy(ctx);

  Hobbit::Shape shape(1, 2, 4);

  Hobbit::Buffer buffer(builder, float_type, shape);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);
}

TEST(TestBuffer, Subindex) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  std::unique_ptr<llvm::Module> Mod =
      llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(
      Mod->getOrInsertFunction("create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getFloatTy(ctx);

  Hobbit::Shape shape(1, 2, 4);

  Hobbit::Buffer buffer(builder, float_type, shape);

  Hobbit::Buffer *subbuffer = buffer.GetChunk(
      entry, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(0, 4));
  EXPECT_TRUE(subbuffer->GetShape() == Hobbit::Shape(1, 1, 4));

  Hobbit::Buffer *sub_subbuffer = subbuffer->GetChunk(
      entry, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(2, 3));
  EXPECT_TRUE(sub_subbuffer->GetShape() == Hobbit::Shape(1, 1, 1));

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);
}

TEST(TestBuffer, SubindexPack) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  std::unique_ptr<llvm::Module> Mod =
      llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(
      Mod->getOrInsertFunction("create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getFloatTy(ctx);

  Hobbit::Shape shape(1, 2, 8);

  Hobbit::Buffer buffer(builder, float_type, shape);

  Hobbit::Buffer *subbuffer = buffer.GetChunk(
      entry, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(0, 8));
  EXPECT_TRUE(subbuffer->GetShape() == Hobbit::Shape(1, 1, 8));

  llvm::ArrayRef<llvm::Value *> vals = subbuffer->Pack(entry, 4);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);
}

TEST(TestConstant, Create) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  std::unique_ptr<llvm::Module> Mod =
      llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(
      Mod->getOrInsertFunction("create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getDoubleTy(ctx);

  Hobbit::Shape shape(1, 2, 4);

  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(1, 6);

  double random[shape.GetSize()];

  for (int i = 0; i < shape.GetSize(); i++) {
    random[i] = dis(gen);
  }

  Hobbit::CompileTimeFPBuffer cp_buffer(random, shape);

  Hobbit::Constant constant(builder, float_type, cp_buffer);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);
}

TEST(TestConstant, Subindex) { // this isn't working for some reason...
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  std::unique_ptr<llvm::Module> Mod =
      llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction(
      "create", llvm::Type::getDoubleTy(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::BasicBlock *chunk0 = llvm::BasicBlock::Create(ctx, "chunk0", f);
  llvm::BasicBlock *chunk1 = llvm::BasicBlock::Create(ctx, "chunk1", f);
  llvm::BasicBlock *end = llvm::BasicBlock::Create(ctx, "end", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getDoubleTy(ctx);

  Hobbit::Shape shape(1, 2, 4);

  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(1, 6);

  double random[shape.GetSize()];

  for (int i = 0; i < shape.GetSize(); i++) {
    random[i] = dis(gen);
  }

  Hobbit::CompileTimeFPBuffer cp_buffer(random, shape);

  Hobbit::Constant constant(builder, float_type, cp_buffer);

  builder.CreateBr(chunk0);
  Hobbit::Buffer *h_is_0 = constant.GetChunk(
      chunk0, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(0, 4));
  EXPECT_TRUE(h_is_0->GetShape() == Hobbit::Shape(1, 1, 4));

  builder.SetInsertPoint(chunk0);
  builder.CreateBr(chunk1);
  Hobbit::Buffer *h_is_1 = constant.GetChunk(
      chunk1, Hobbit::Range(0, 1), Hobbit::Range(1, 2), Hobbit::Range(0, 4));
  EXPECT_TRUE(h_is_1->GetShape() == Hobbit::Shape(1, 1, 4));

  builder.SetInsertPoint(chunk1);
  builder.CreateBr(end);

  builder.SetInsertPoint(end);
  llvm::Value *ret = builder.getInt64(0);
  for (uint64_t i = 0; i < 4; i++) {
    llvm::Value *lhs_elt = builder.CreateLoad(
        builder.CreateGEP(h_is_0->GetValue(), builder.getInt64(i)));
    llvm::Value *rhs_elt = builder.CreateLoad(
        builder.CreateGEP(h_is_1->GetValue(), builder.getInt64(i)));
    ret = builder.CreateFAdd(builder.CreateBitCast(ret, builder.getDoubleTy()),
                             builder.CreateFMul(lhs_elt, rhs_elt));
  }

  builder.CreateRet(ret);

  h_is_0->GetValue()->print(llvm::errs(), true);
  h_is_1->GetValue()->print(llvm::errs(), true);

  Mod->print(llvm::outs(), nullptr);
  llvm::verifyFunction(*f);
}