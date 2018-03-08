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

// TODO: test the chunk methods

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

  Hobbit::Buffer buffer(entry, float_type, shape);

  Mod->print(llvm::outs(), nullptr);
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

  Hobbit::Buffer buffer(entry, float_type, shape);

  Hobbit::Buffer *subbuffer = buffer.GetChunk(
      entry, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(0, 4));
  EXPECT_TRUE(subbuffer->GetShape() == Hobbit::Shape(1, 1, 4));

  Hobbit::Buffer *sub_subbuffer = subbuffer->GetChunk(
      entry, Hobbit::Range(0, 1), Hobbit::Range(0, 1), Hobbit::Range(2, 3));
  EXPECT_TRUE(sub_subbuffer->GetShape() == Hobbit::Shape(1, 1, 1));

  Mod->print(llvm::outs(), nullptr);
  llvm::verifyFunction(*f);
}