//
// Created by Aman LaChapelle on 3/4/18.
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

#include <Module.hpp>
#include <ElementWiseFunctors.hpp>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

TEST(TestModule, Init) {
  llvm::LLVMContext ctx;
  EXPECT_NO_THROW(Hobbit::Module module("test_module", ctx));
}

TEST(TestModule, CreateFunction) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatTy(ctx), llvm::Type::getHalfTy(ctx)};
  module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args);
  module.PrintModule();
}

TEST(TestModule, CreateDuplicateFunction) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatTy(ctx), llvm::Type::getHalfTy(ctx)};
  module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args);

  EXPECT_THROW(module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args), std::runtime_error);
  module.PrintModule();
}

TEST(TestModule, AllocBuffer) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatPtrTy(ctx), llvm::Type::getInt64Ty(ctx)};
  module.CreateFunction("sdot", llvm::Type::getFloatTy(ctx), args);

  std::vector<float> f;
  for (int i = 0; i < 10; i++) {
    f.push_back(i);
  }

  EXPECT_NO_THROW(module.GetFloatConstant("sdot", f.data(), Hobbit::Shape(1, 2, 5)));

  module.PrintModule();
}

TEST(TestModule, PerformProd) {
//  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeNativeTarget();

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatPtrTy(ctx)};
  module.CreateFunction("prod", llvm::Type::getFloatPtrTy(ctx), args);
  Hobbit::Buffer *input_buffer = module.GetBufferFromInputs("prod", 0, Hobbit::Shape(1, 2, 5));

  std::vector<float> f;
  for (int i = 0; i < 10; i++) {
    f.push_back(i);
  }

  Hobbit::Buffer *fconst = module.GetFloatConstant("prod", f.data(), Hobbit::Shape(1, 2, 5));

  Hobbit::ElementWiseProduct prod(fconst);
  Hobbit::Operation prod_op("prod_op");
  prod_op.PushFunctor(prod);

  Hobbit::Buffer *result = module.InsertOperation("prod", &prod_op, input_buffer); // how do I give the Operation its inputs?

  module.FinalizeFunction("prod", result);

  module.PrintModule();

  module.FinalizeModule();
  module.PrepareJIT();

  float *(*prod_fn)(float *) = (float *(*)(float *))module.GetFunctionPtr("prod"); // this is segfaulting, can't find the function I guess?

  float *float_result = prod_fn(f.data());
  for (int i = 0; i < 10; i++) {
    std::cout << float_result[i] << std::endl;
  }
}