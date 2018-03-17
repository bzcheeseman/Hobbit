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

#include <gtest/gtest.h>

#include <Module.hpp>
#include <Function.hpp>
#include <Type.hpp>
#include <Variable.hpp>

using namespace Hobbit;

TEST(Basic, CreateModule) {
  llvm::LLVMContext ctx;

  EXPECT_NO_THROW(core::Module module ("test_module", ctx));
}

TEST(Basic, GetContext) {
  llvm::LLVMContext ctx;

  core::Module module ("test_module", ctx);

  EXPECT_EQ(module.GetContext(), &ctx);
}

TEST(Basic, CreateFunction) {
  llvm::LLVMContext ctx;

  core::Module module ("test_module", ctx);

  EXPECT_NO_THROW(std::unique_ptr<core::Function> func = core::Function::Create(&module, "test_func"));
}

TEST(Basic, AddVariable) {
  llvm::LLVMContext ctx;

  core::Module module ("test_module", ctx);

  std::unique_ptr<core::Function> func = core::Function::Create(&module, "test_func");

  core::Type<float, 16> type;
  core::Tensor *tensor;
  EXPECT_NO_THROW(tensor = core::Variable::Create(func.get(), &type, Shape(1, 2, 3)));
  EXPECT_TRUE(func->GetSymbol(tensor)->shape == tensor->GetShape());
  EXPECT_EQ(func->GetSymbol(tensor)->type, llvm::Type::getHalfTy(ctx));
}

TEST(Basic, AddConstant) {
  llvm::LLVMContext ctx;

  core::Module module ("test_module", ctx);

  std::unique_ptr<core::Function> func = core::Function::Create(&module, "test_func");

  std::vector<float> input (1*2*3);
  for (int i = 0; i < 1*2*3; i++) {
    input[i] = i;
  }

  core::Type<float, 32> type;
  core::Tensor *tensor;
  // Should not throw errors
  EXPECT_NO_THROW(tensor = core::Constant::Create(func.get(), &type, Shape(1, 2, 3), input.data()));
  // The shape of the symbol should be the same
  EXPECT_TRUE(func->GetSymbol(tensor)->shape == tensor->GetShape());
  // The symbol's buffer should just alias the input buffer
  EXPECT_EQ(func->GetSymbol(tensor)->buffer, input.data());
}

TEST(Basic, AddAlloca) {
  llvm::LLVMContext ctx;

  core::Module module ("test_module", ctx);

  std::unique_ptr<core::Function> func = core::Function::Create(&module, "test_func");

  std::vector<float> input (1*2*3);
  for (int i = 0; i < 1*2*3; i++) {
    input[i] = i;
  }

  core::Type<float, 32> type;
  core::Tensor *tensor;
  // Should not throw errors
  EXPECT_NO_THROW(tensor = core::Constant::Create(func.get(), &type, Shape(1, 2, 3), input.data()));
  // The shape of the symbol should be the same
  EXPECT_TRUE(func->GetSymbol(tensor)->shape == tensor->GetShape());
  // The symbol's buffer should just alias the input buffer
  EXPECT_EQ(func->GetSymbol(tensor)->buffer, input.data());

  core::Tensor *t;
  EXPECT_NO_THROW(t = func->AddOpNode({tensor}, core::OpCode::ALLOCA));

  // The symbol should be an alias of the function's symbol, addressed by `tensor`
  EXPECT_EQ(t->GetSymbol(), func->GetSymbol(tensor));

  // Alloca throws if you pass in too many args - it only takes in one tensor.
  EXPECT_THROW(func->AddOpNode({tensor, tensor}, core::OpCode::ALLOCA), core::TooManyArgs);
}


