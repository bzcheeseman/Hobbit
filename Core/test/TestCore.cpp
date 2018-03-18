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

#include <random>

#include <gtest/gtest.h>

#include <Function.hpp>
#include <Module.hpp>
#include <OpNode.hpp>
#include <Type.hpp>
#include <Variable.hpp>

template <size_t N> float ref_sdot(float *lhs, float *rhs) {
  float sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;
  for (size_t i = 0; i < N; i += 4) {
    sum1 += lhs[i] * rhs[i];
    sum2 += lhs[i + 1] * rhs[i + 1];
    sum2 += lhs[i + 2] * rhs[i + 2];
    sum2 += lhs[i + 3] * rhs[i + 3];
  }
  return sum1 + sum2 + sum3 + sum4;
}

using namespace Hobbit;

TEST(Basic, CreateModule) {
  llvm::LLVMContext ctx;

  EXPECT_NO_THROW(Module module("test_module", ctx));
}

TEST(Basic, GetContext) {
  llvm::LLVMContext ctx;

  Module module("test_module", ctx);

  EXPECT_EQ(module.GetContext(), &ctx);
}

TEST(Basic, CreateFunction) {
  llvm::LLVMContext ctx;

  Module module("test_module", ctx);

  EXPECT_NO_THROW(std::unique_ptr<Function> func =
                      Function::Create(&module, "test_func"));
}

TEST(Basic, AddVariable) {
  llvm::LLVMContext ctx;

  Module module("test_module", ctx);

  std::unique_ptr<Function> func = Function::Create(&module, "test_func");

  core::Type<float, 16> type;
  Tensor *tensor;
  EXPECT_NO_THROW(tensor = Variable::Create(func, &type, Shape(1, 2, 3)));
  EXPECT_TRUE(func->GetSymbol(tensor)->shape == tensor->GetShape());
  EXPECT_EQ(func->GetSymbol(tensor)->type, llvm::Type::getHalfTy(ctx));
}

TEST(Basic, AddConstant) {
  llvm::LLVMContext ctx;

  Module module("test_module", ctx);

  std::unique_ptr<Function> func = Function::Create(&module, "test_func");

  std::vector<float> input(1 * 2 * 3);
  for (int i = 0; i < 1 * 2 * 3; i++) {
    input[i] = i;
  }

  core::Type<float *, 32> type;
  Tensor *tensor;
  // Should not throw errors
  EXPECT_NO_THROW(
      tensor = Constant::Create(func, &type, Shape(1, 2, 3), input.data()));
  // The shape of the symbol should be the same
  EXPECT_TRUE(func->GetSymbol(tensor)->shape == tensor->GetShape());
  // The symbol's buffer should just alias the input buffer
  EXPECT_EQ(func->GetSymbol(tensor)->buffer, input.data());
}

TEST(Basic, AddAlloca) {
  llvm::LLVMContext ctx;

  Module module("test_module", ctx);

  std::unique_ptr<Function> func = Function::Create(&module, "test_func");

  std::vector<float> input(1 * 2 * 3);
  for (int i = 0; i < 1 * 2 * 3; i++) {
    input[i] = i;
  }

  core::Type<float *, 32> type;
  Tensor *tensor;
  // Should not throw errors
  EXPECT_NO_THROW(
      tensor = Constant::Create(func, &type, Shape(1, 2, 3), input.data()));
  // The shape of the symbol should be the same
  EXPECT_TRUE(func->GetSymbol(tensor)->shape == tensor->GetShape());
  // The symbol's buffer should just alias the input buffer
  EXPECT_EQ(func->GetSymbol(tensor)->buffer, input.data());

  Tensor *t;
  EXPECT_NO_THROW(t = func->AddOpNode({tensor}, OpCode::ALLOCA));

  // The symbol should be an alias of the function's symbol, addressed by
  // `tensor`
  EXPECT_EQ(t->GetSymbol(), func->GetSymbol(tensor));

  // Alloca throws if you pass in too many args - it only takes in one tensor.
  EXPECT_THROW(func->AddOpNode({tensor, tensor}, OpCode::ALLOCA),
               core::IncorrectNumArgs);
}

TEST(Basic, AddArg) {
  llvm::LLVMContext ctx;

  Module module("test_module", ctx);

  std::unique_ptr<Function> func = Function::Create(&module, "test_func");

  core::Type<float, 16> type1;
  Tensor *tensor, *output;
  EXPECT_NO_THROW(tensor = Variable::Create(func, &type1, Shape(1, 2, 3)));
  EXPECT_NO_THROW(output = func->AddOpNode({tensor}, ALLOCA));
  EXPECT_TRUE(func->GetSymbol(tensor)->shape == tensor->GetShape());
  EXPECT_EQ(func->GetSymbol(tensor)->type, llvm::Type::getHalfTy(ctx));
  EXPECT_NO_THROW(func->MarkSymbolAsArg(tensor));

  std::vector<Tensor *> args = func->GetSignatureArgs({output});
  EXPECT_EQ(args[0]->GetType()->getTypeID(), tensor->GetType()->getTypeID());
}

TEST(Basic, EmitFunction) {
  llvm::LLVMContext ctx;

  const int n_elts = 3400;

  Module module("test_module", ctx);

  std::unique_ptr<Function> func = Function::Create(&module, "test_func");

  core::Type<float *, 32> type;
  Tensor *lhs, *rhs, *output;
  EXPECT_NO_THROW(lhs = Variable::Create(func, &type, Shape(1, 1, n_elts)));
  EXPECT_NO_THROW(rhs = Variable::Create(func, &type, Shape(1, 1, n_elts)));
  EXPECT_NO_THROW(output = func->AddOpNode({lhs, rhs}, SDOT));

  EXPECT_EQ(func->GetSymbol(lhs)->type, llvm::Type::getFloatPtrTy(ctx));
  EXPECT_NO_THROW(func->MarkSymbolAsArg(lhs));
  EXPECT_NO_THROW(func->MarkSymbolAsArg(rhs));

  EXPECT_EQ(func->GetSymbol(rhs)->type, llvm::Type::getFloatPtrTy(ctx));
  EXPECT_NO_THROW(func->MarkSymbolAsArg(rhs));

  EXPECT_TRUE(output->GetShape() == Shape(1, 1, 1));

  std::vector<Tensor *> args = func->GetSignatureArgs({output});
  EXPECT_EQ(args.size(), 3);
  EXPECT_EQ(args[0], lhs);
  EXPECT_EQ(args[1], rhs);
  EXPECT_EQ(args[2], output);

  llvm::Function *f;
  EXPECT_NO_THROW(f = module.GetFunction(func->GetName(), args));

  EXPECT_NO_THROW(func->Emit(f));
  EXPECT_NO_THROW(module.FinalizeFunction(f));

  EXPECT_NO_THROW(module.FinalizeModule(3, llvm::sys::getDefaultTargetTriple()));
  module.Print();


  void (*sdot)(float *, float *, float *) =
      (void (*)(float *, float *, float *))module.GetFunctionPtr("test_func");

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(0.0, 1.0);

  std::vector<float> f1, f2;
  for (int i = 0; i < n_elts; i++) {
    f1.push_back(dis(gen));
    f2.push_back(dis(gen));
  }

  float float_out;

  auto start = std::chrono::high_resolution_clock::now();
  sdot(f1.data(), f2.data(), &float_out);
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = (finish - start);
  std::cout << "\nElapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_NEAR(float_out, ref_sdot<n_elts>(f1.data(), f2.data()),
              float_out * 5e-6);
}

TEST(Basic, EmitConstFunction) { // this is still iffy
  llvm::LLVMContext ctx;

  const int n_elts = 3;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(0.0, 1.0);

  std::vector<float> f1, f2; // why are there a bunch of zeros in the middle...
  for (int i = 0; i < n_elts; i++) {
    f1.push_back(dis(gen));
    f2.push_back(dis(gen));
  }

  f1.shrink_to_fit();
  f2.shrink_to_fit();

  Module module("test_module", ctx);

  std::unique_ptr<Function> func = Function::Create(&module, "test_func");

  core::Type<float *, 32> type;
  Tensor *lhs, *rhs, *output;
  EXPECT_NO_THROW(lhs = Variable::Create(func, &type, Shape(1, 1, n_elts)));
  EXPECT_NO_THROW(rhs = Constant::Create(func, &type, Shape(1, 1, n_elts), f2.data()));
  EXPECT_NO_THROW(output = func->AddOpNode({lhs, rhs}, SDOT));

  EXPECT_EQ(func->GetSymbol(lhs)->type, llvm::Type::getFloatPtrTy(ctx));
  EXPECT_NO_THROW(func->MarkSymbolAsArg(lhs));

  EXPECT_EQ(func->GetSymbol(rhs)->type, llvm::Type::getFloatPtrTy(ctx));
  EXPECT_NO_THROW(func->MarkSymbolAsArg(rhs));

  EXPECT_TRUE(output->GetShape() == Shape(1, 1, 1));

  std::vector<Tensor *> args = func->GetSignatureArgs({output});
  EXPECT_EQ(args.size(), 3);
  EXPECT_EQ(args[0], lhs);
  EXPECT_EQ(args[1], rhs);
  EXPECT_EQ(args[2], output);

  llvm::Function *f;
  EXPECT_NO_THROW(f = module.GetFunction(func->GetName(), args));
  EXPECT_EQ(func->GetSymbol(rhs)->type, llvm::Type::getFloatTy(ctx));

  EXPECT_NO_THROW(func->Emit(f));
  EXPECT_NO_THROW(module.FinalizeFunction(f));

  EXPECT_NO_THROW(module.FinalizeModule(3, llvm::sys::getDefaultTargetTriple()));
  module.Print();

  void (*sdot)(float *, float *) =
  (void (*)(float *, float *))module.GetFunctionPtr("test_func");

  float float_out;

  auto start = std::chrono::high_resolution_clock::now();
  sdot(f1.data(), &float_out);
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = (finish - start);
  std::cout << "\nElapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_NEAR(float_out, ref_sdot<n_elts>(f1.data(), f2.data()),
              float_out * 5e-6);
}
