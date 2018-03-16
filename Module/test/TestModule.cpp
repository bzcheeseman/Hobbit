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

#include <random>

#include <gtest/gtest.h>

#include <ElementWiseFunctors.hpp>
#include <Module.hpp>
#include <ReductionFunctors.hpp>

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

TEST(TestModule, Init) {
  llvm::LLVMContext ctx;
  EXPECT_NO_THROW(Hobbit::Module module("test_module", ctx));
}

TEST(TestModule, CreateFunction) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatTy(ctx),
                                    llvm::Type::getHalfTy(ctx)};
  EXPECT_NO_THROW(
      module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args));
  // module.PrintModule(llvm::outs());
}

TEST(TestModule, CreateDuplicateFunction) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatTy(ctx),
                                    llvm::Type::getHalfTy(ctx)};
  module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args);

  EXPECT_THROW(module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args),
               std::runtime_error);
  // module.PrintModule(llvm::outs());
}

TEST(TestModule, AllocBuffer) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatPtrTy(ctx),
                                    llvm::Type::getInt64Ty(ctx)};
  module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args);

  std::vector<float> f;
  for (int i = 0; i < 10; i++) {
    f.push_back(i);
  }

  EXPECT_NO_THROW(
      module.GetFloatConstant("Test", f.data(), Hobbit::Shape(1, 2, 5)));

  // module.PrintModule(llvm::outs());
}

TEST(TestModule, PerformProd) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  int num_elements = 100;

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx) // input
  };
  module.CreateFunction("prod", llvm::Type::getFloatPtrTy(ctx), args);
  Hobbit::Buffer *input_buffer =
      module.GetBufferFromInputs("prod", 0, Hobbit::Shape(1, 1, num_elements));

  std::vector<float> f;
  for (int i = 0; i < num_elements; i++) {
    f.push_back(i);
  }

  Hobbit::Buffer *fconst = module.GetFloatConstant(
      "prod", f.data(), Hobbit::Shape(1, 1, num_elements));

  Hobbit::ElementWiseProduct prod(fconst, 4);
  Hobbit::Operation prod_op("prod_op");
  prod_op.PushFunctor(prod);

  Hobbit::Buffer *result =
      module.InsertOperation("prod", &prod_op, input_buffer, true);

  module.FinalizeFunction("prod", result);

  module.FinalizeModule(3);

   module.PrintModule(llvm::outs());
  module.PrepareJIT();

  float *(*prod_fn)(float *) =
      (float *(*)(float *))module.GetFunctionPtr("prod");
  float *float_result = prod_fn(f.data());

  for (int i = 0; i < num_elements; i++) {
    EXPECT_FLOAT_EQ(float_result[i], (float)i * i);
  }
}

TEST(TestModule, PerformSDOT) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  const int n_elts = 100;

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx) // input
  };
  module.CreateFunction("sdot", llvm::Type::getFloatTy(ctx), args);
  Hobbit::Buffer *input_buffer =
      module.GetBufferFromInputs("sdot", 0, Hobbit::Shape(1, 1, n_elts));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(0.0, 1.0);

  std::vector<float> f1, f2;
  for (int i = 0; i < n_elts; i++) {
    f1.push_back(dis(gen));
    f2.push_back(dis(gen));
  }

  Hobbit::Buffer *fconst =
      module.GetFloatConstant("sdot", f2.data(), Hobbit::Shape(1, 1, n_elts));

  Hobbit::Dot dot(fconst, 4);
  Hobbit::Operation sdot_op("sdot_op");
  sdot_op.PushFunctor(dot);

  Hobbit::Buffer *result =
      module.InsertOperation("sdot", &sdot_op, input_buffer, true);

  module.FinalizeFunction("sdot", result);

  module.FinalizeModule(0);

   module.PrintModule(llvm::outs());
  module.PrepareJIT();

  float (*prod_fn)(float *) = (float (*)(float *))module.GetFunctionPtr("sdot");

  auto start = std::chrono::high_resolution_clock::now();
  float float_result = prod_fn(f1.data());
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_FLOAT_EQ(float_result, ref_sdot<n_elts>(f1.data(), f2.data())); // apparently the sdot is squaring the input
}

TEST(TestModule, PerformLargeSDOT) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  const int n_elts = 4000;

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx) // input
  };
  module.CreateFunction("sdot", llvm::Type::getFloatTy(ctx), args);
  Hobbit::Buffer *input_buffer =
      module.GetBufferFromInputs("sdot", 0, Hobbit::Shape(1, 1, n_elts));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(0.0, 1.0);

  std::vector<float> f1, f2;
  for (int i = 0; i < n_elts; i++) {
    f1.push_back(dis(gen));
    f2.push_back(dis(gen));
  }

  Hobbit::Buffer *fconst =
      module.GetFloatConstant("sdot", f2.data(), Hobbit::Shape(1, 1, n_elts));

  Hobbit::Dot dot(fconst, 4);
  Hobbit::Operation sdot_op("sdot_op");
  sdot_op.PushFunctor(dot);

  Hobbit::Buffer *result =
      module.InsertOperation("sdot", &sdot_op, input_buffer, false);

  module.FinalizeFunction("sdot", result);

  module.FinalizeModule(3);

   module.PrintModule(llvm::outs());
  module.PrepareJIT();

  float (*prod_fn)(float *) = (float (*)(float *))module.GetFunctionPtr("sdot");

  auto start = std::chrono::high_resolution_clock::now();
  float float_result = prod_fn(f1.data());
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_FLOAT_EQ(float_result, ref_sdot<n_elts>(f1.data(), f2.data()));
}

TEST(TestModule, PerformNoConstSDOT) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  const int n_elts = 33142000; // 30 ms, vector width = 4 - 25 ms, vector width = 8, 23 ms, vector width = 16

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx), // input vec 1
      llvm::Type::getFloatPtrTy(ctx)  // input vec 2
  };
  module.CreateFunction("sdot", llvm::Type::getFloatTy(ctx), args);
  Hobbit::Buffer *vec1 =
      module.GetBufferFromInputs("sdot", 0, Hobbit::Shape(1, 1, n_elts));
  Hobbit::Buffer *vec2 =
      module.GetBufferFromInputs("sdot", 1, Hobbit::Shape(1, 1, n_elts));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(0.0, 1.0);

  std::vector<float> f1, f2;
  for (int i = 0; i < n_elts; i++) {
    f1.push_back(dis(gen));
    f2.push_back(dis(gen));
  }

  Hobbit::Dot dot(vec2, 4);
  Hobbit::Operation sdot_op("sdot_op");
  sdot_op.PushFunctor(dot);

  Hobbit::Buffer *result =
      module.InsertOperation("sdot", &sdot_op, vec1, false);

  module.FinalizeFunction("sdot", result);

  module.FinalizeModule(3);

  module.PrintModule(llvm::outs());
  module.PrepareJIT();

  float (*prod_fn)(float *, float *) =
      (float (*)(float *, float *))module.GetFunctionPtr("sdot");

  auto start = std::chrono::high_resolution_clock::now();
  float float_result = prod_fn(f1.data(), f2.data());
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_FLOAT_EQ(float_result, ref_sdot<n_elts>(f1.data(), f2.data()));
}

TEST(TestModule, PerformNoConstProd) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  int n_elts = 10000;

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx), // input1
      llvm::Type::getFloatPtrTy(ctx), // input2
      llvm::Type::getFloatPtrTy(ctx)  // output
  };
  module.CreateFunction("prod", llvm::Type::getVoidTy(ctx), args,
                        true); // last_is_output
  Hobbit::Buffer *input_buffer =
      module.GetBufferFromInputs("prod", 0, Hobbit::Shape(1, 1, n_elts));
  Hobbit::Buffer *input_buffer2 =
      module.GetBufferFromInputs("prod", 1, Hobbit::Shape(1, 1, n_elts));
  Hobbit::Buffer *output =
      module.GetBufferFromInputs("prod", 2, Hobbit::Shape(1, 1, n_elts));

  float *f;
  posix_memalign((void **)&f, 32, n_elts * sizeof(float));
  for (int i = 0; i < n_elts; i++) {
    f[i] = i;
  }

  Hobbit::ElementWiseProduct prod(input_buffer2, 4);
  Hobbit::Operation prod_op("prod_op");
  prod_op.PushFunctor(prod);

  Hobbit::Buffer *result =
      module.InsertOperation("prod", &prod_op, input_buffer, false);

  module.FinalizeFunction("prod", result, output);

  module.FinalizeModule(3);

  // module.PrintModule(llvm::outs());
  module.PrepareJIT();

  void (*prod_fn)(float *, float *, float *) =
      (void (*)(float *, float *, float *))module.GetFunctionPtr("prod");

  float *float_result;
  posix_memalign((void **)&float_result, 32, n_elts * sizeof(float));

  auto start = std::chrono::high_resolution_clock::now();
  prod_fn(f, f, float_result);
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  for (int i = 0; i < n_elts; i++) {
    EXPECT_FLOAT_EQ(float_result[i], (float)i * i);
  }
}

TEST(TestModule, CompileSDOT) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  int n_elts = 100;

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx) // input
  };
  module.CreateFunction("sdot", llvm::Type::getFloatTy(ctx), args);
  Hobbit::Buffer *input_buffer =
      module.GetBufferFromInputs("sdot", 0, Hobbit::Shape(1, 1, n_elts));

  std::vector<float> f;
  for (int i = 0; i < n_elts; i++) {
    f.push_back(i);
  }

  Hobbit::Buffer *fconst =
      module.GetFloatConstant("sdot", f.data(), Hobbit::Shape(1, 1, n_elts));

  Hobbit::ElementWiseProduct prod(fconst, 4);
  Hobbit::SumReduction hsum(llvm::Type::getFloatTy(ctx), 4);
  Hobbit::Operation sdot_op("sdot_op");
  sdot_op.PushFunctor(prod);
  sdot_op.PushFunctor(hsum);

  Hobbit::Buffer *result =
      module.InsertOperation("sdot", &sdot_op, input_buffer, true);

  EXPECT_NO_THROW(module.FinalizeFunction("sdot", result));

  EXPECT_NO_THROW(module.FinalizeModule(3));

  std::string filename = "sdot";
  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  EXPECT_NO_THROW(module.PrintModule(dest));
}

TEST(TestModule, CompileLargeSDOT) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  int n_elts = 100;

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx) // input
  };
  module.CreateFunction("sdot", llvm::Type::getFloatTy(ctx), args);
  Hobbit::Buffer *input_buffer =
      module.GetBufferFromInputs("sdot", 0, Hobbit::Shape(1, 1, n_elts));

  std::vector<float> f;
  for (int i = 0; i < n_elts; i++) {
    f.push_back(i);
  }

  Hobbit::Buffer *fconst =
      module.GetFloatConstant("sdot", f.data(), Hobbit::Shape(1, 1, n_elts));

  Hobbit::ElementWiseProduct prod(fconst, 4);
  Hobbit::SumReduction hsum(llvm::Type::getFloatTy(ctx), 4);
  Hobbit::Operation sdot_op("sdot_op");
  sdot_op.PushFunctor(prod);
  sdot_op.PushFunctor(hsum);

  Hobbit::Buffer *result =
      module.InsertOperation("sdot", &sdot_op, input_buffer, false);

  EXPECT_NO_THROW(module.FinalizeFunction("sdot", result));

  EXPECT_NO_THROW(module.FinalizeModule(3));

  std::string filename = "sdot_large";
  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  EXPECT_NO_THROW(module.PrintModule(dest));
}