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
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction(
          "create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getFloatTy(ctx);

  Hobbit::Shape shape (1, 2, 4);

  Hobbit::Buffer buffer (builder, float_type, shape);

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
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction(
          "create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getFloatTy(ctx);

  Hobbit::Shape shape (1, 2, 4);

  Hobbit::Buffer buffer (builder, float_type, shape);

  Hobbit::Buffer *subbuffer = buffer.GetChunk(builder, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(0, 4));
  EXPECT_TRUE(subbuffer->GetShape() == Hobbit::Shape(1, 1, 4));

  Hobbit::Buffer *sub_subbuffer = subbuffer->GetChunk(builder, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(2, 3));
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
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction(
          "create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getFloatTy(ctx);

  Hobbit::Shape shape (1, 2, 8);

  Hobbit::Buffer buffer (builder, float_type, shape);

  Hobbit::Buffer *subbuffer = buffer.GetChunk(builder, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(0, 8));
  EXPECT_TRUE(subbuffer->GetShape() == Hobbit::Shape(1, 1, 8));

  llvm::ArrayRef<llvm::Value *> vals = subbuffer->Pack(builder, 4);

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
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction(
          "create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getDoubleTy(ctx);

  Hobbit::Shape shape (1, 2, 4);

  std::random_device rd;  //Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(1, 6);

  double random[shape.GetSize()];

  for (int i = 0; i < shape.GetSize(); i++) {
    random[i] = dis(gen);
  }

  Hobbit::CompileTimeFPBuffer cp_buffer (random, shape);

  Hobbit::Constant constant (builder, float_type, cp_buffer);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);
}

TEST(TestConstant, Subindex) {  // this isn't working for some reason...
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  std::unique_ptr<llvm::Module> Mod =
          llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction(
          "create", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder(entry);

  llvm::Type *float_type = llvm::Type::getDoubleTy(ctx);

  Hobbit::Shape shape (1, 2, 4);

  std::random_device rd;  //Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(1, 6);

  double random[shape.GetSize()];

  for (int i = 0; i < shape.GetSize(); i++) {
    random[i] = dis(gen);
  }

  Hobbit::CompileTimeFPBuffer cp_buffer (random, shape);

  Hobbit::Constant constant (builder, float_type, cp_buffer);

  Hobbit::Buffer *h_is_1 = constant.GetChunk(builder, Hobbit::Range(), Hobbit::Range(1, 2), Hobbit::Range());
  Hobbit::Buffer *h_is_0 = constant.GetChunk(builder, Hobbit::Range(), Hobbit::Range(0, 1), Hobbit::Range());

//  std::cout << h_is_0->GetShape() << std::endl;
//  std::cout << h_is_1->GetShape() << std::endl;

  EXPECT_TRUE(h_is_0->GetShape() == Hobbit::Shape(1, 1, 4));
  EXPECT_TRUE(h_is_1->GetShape() == Hobbit::Shape(1, 1, 4));

  llvm::ArrayRef<llvm::Value *> h0 = h_is_0->Pack(builder, 4);
  EXPECT_EQ(h0.size(), 1);
  llvm::ArrayRef<llvm::Value *> h1 = h_is_1->Pack(builder, 4);
  EXPECT_EQ(h1.size(), 1);

  h0[0]->print(llvm::errs(), true);
  h1[0]->print(llvm::errs(), true);

  llvm::Value *mult = builder.CreateFMul(h0[0], h1[0]);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);
}