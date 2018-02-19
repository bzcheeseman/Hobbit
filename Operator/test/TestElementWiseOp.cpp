//
// Created by Aman LaChapelle on 2/17/18.
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

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/OrcMCJITReplacement.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>

#include "ElementWiseOp.hpp"

TEST(TestElementWiseProduct, VectorCodegen) {

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  Hobbit::ElementWiseProduct prod(ctx);

  std::unique_ptr<llvm::Module> Mod = llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction("vec_codegen", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder (entry);

  llvm::Value *one = builder.getInt32(1);
  llvm::Value *two = builder.getInt32(2);
  llvm::Value *three = builder.getInt32(3);
  llvm::Value *four = builder.getInt32(4);

  llvm::Value *one_array = builder.CreateVectorSplat(4, one);
  llvm::Value *two_array = builder.CreateVectorSplat(4, two);

  EXPECT_TRUE(prod.SetConstant(two_array));

  llvm::Value *ret = prod.Emit(builder, one_array, prod.Geti32VectorType(4));
  ret = builder.CreateExtractElement(ret, (uint64_t)0);

  builder.CreateRet(ret);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);

  llvm::EngineBuilder engineBuilder(std::move(Mod));
  llvm::ExecutionEngine *engine = engineBuilder.create();

  int32_t (*run)() = (int32_t (*)())engine->getFunctionAddress("vec_codegen");
  llvm::errs() << "Module run result: " << run() << "\n";

}

TEST(TestElementWiseProduct, ArrayCodegen) {

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  Hobbit::ElementWiseProduct prod(ctx);

  std::unique_ptr<llvm::Module> Mod = llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction("arr_codegen", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder (entry);

  llvm::Value *one_array = llvm::UndefValue::get(llvm::ArrayType::get(llvm::Type::getInt32Ty(ctx), 4));
  llvm::Value *two_array = llvm::UndefValue::get(llvm::ArrayType::get(llvm::Type::getInt32Ty(ctx), 4));
  for (int i = 0; i < 4; i++) {
    one_array = builder.CreateInsertValue(one_array, builder.getInt32(1), i);
    two_array = builder.CreateInsertValue(two_array, builder.getInt32(2), i);
  }

  prod.SetConstant(two_array);
  llvm::Value *ret = prod.Emit(builder, one_array, nullptr);
//  ret = builder.CreateLoad(builder.CreateGEP(ret, builder.getInt32(0)));

  builder.CreateRet(ret);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);

  llvm::EngineBuilder engineBuilder(std::move(Mod));
  llvm::ExecutionEngine *engine = engineBuilder.create();

  int32_t (*run)() = (int32_t (*)())engine->getFunctionAddress("arr_codegen");
  llvm::errs() << "Module run result: " << run() << "\n";

}

TEST(TestElementWiseProduct, MixedCodegen) {

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::LLVMContext ctx;

  Hobbit::ElementWiseProduct prod(ctx);

  std::unique_ptr<llvm::Module> Mod = llvm::make_unique<llvm::Module>("test", ctx);
  llvm::Function *f = llvm::cast<llvm::Function>(Mod->getOrInsertFunction("mixed_codegen", llvm::Type::getInt32Ty(ctx), nullptr));

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", f);
  llvm::IRBuilder<> builder (entry);

  llvm::Value *one_array = builder.CreateVectorSplat(4, builder.getInt32(1));

  llvm::Value *two_array = llvm::UndefValue::get(llvm::ArrayType::get(llvm::Type::getInt32Ty(ctx), 4));
  for (int i = 0; i < 4; i++) {
    two_array = builder.CreateInsertValue(two_array, builder.getInt32(2), i);
  }

  prod.SetConstant(two_array);
  llvm::Value *ret = prod.Emit(builder, one_array, prod.Geti32VectorType(4));
  ret = builder.CreateExtractElement(ret, (uint64_t)0);

  builder.CreateRet(ret);

  Mod->print(llvm::errs(), nullptr);
  llvm::verifyFunction(*f);

  llvm::EngineBuilder engineBuilder(std::move(Mod));
  llvm::ExecutionEngine *engine = engineBuilder.create();

  int32_t (*run)() = (int32_t (*)())engine->getFunctionAddress("mixed_codegen");
  llvm::errs() << "Module run result: " << run() << "\n";

}

