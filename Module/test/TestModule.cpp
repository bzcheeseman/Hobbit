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

#include <ElementWiseFunctors.hpp>
#include <Module.hpp>
#include <ReductionFunctors.hpp>

TEST(TestModule, Init) {
  llvm::LLVMContext ctx;
  EXPECT_NO_THROW(Hobbit::Module module("test_module", ctx));
}

TEST(TestModule, CreateFunction) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatTy(ctx),
                                    llvm::Type::getHalfTy(ctx)};
  module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args);
  module.PrintModule(llvm::outs());
}

TEST(TestModule, CreateDuplicateFunction) {
  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  std::vector<llvm::Type *> args = {llvm::Type::getFloatTy(ctx),
                                    llvm::Type::getHalfTy(ctx)};
  module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args);

  EXPECT_THROW(module.CreateFunction("Test", llvm::Type::getFloatTy(ctx), args),
               std::runtime_error);
  module.PrintModule(llvm::outs());
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

  module.PrintModule(llvm::outs());
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

  module.FinalizeFunction("sdot", result);

  module.FinalizeModule(3);

  module.PrintModule(llvm::outs());
  module.PrepareJIT();

  float (*prod_fn)(float *) = (float (*)(float *))module.GetFunctionPtr("sdot");

  auto start = std::chrono::high_resolution_clock::now();
  float float_result = prod_fn(f.data());
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_FLOAT_EQ(float_result,
                  (float)(n_elts - 1) / 3.f * ((float)(n_elts - 1) + 1.f) *
                      ((float)(n_elts - 1) + 0.5f));
}

TEST(TestModule, PerformLargeSDOT) {

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

  module.FinalizeFunction("sdot", result);

  module.FinalizeModule(3);

  // module.PrintModule(llvm::outs());
  module.PrepareJIT();

  float (*prod_fn)(float *) = (float (*)(float *))module.GetFunctionPtr("sdot");

  auto start = std::chrono::high_resolution_clock::now();
  float float_result = prod_fn(f.data());
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_FLOAT_EQ(float_result,
                  (float)(n_elts - 1) / 3.f * ((float)(n_elts - 1) + 1.f) *
                      ((float)(n_elts - 1) + 0.5f));
}

TEST(TestModule, PerformNoConstSDOT) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  int n_elts = 10000;

  std::vector<llvm::Type *> args = {
      llvm::Type::getFloatPtrTy(ctx), // input vec 1
      llvm::Type::getFloatPtrTy(ctx)  // input vec 2
  };
  module.CreateFunction("sdot", llvm::Type::getFloatTy(ctx), args);
  Hobbit::Buffer *vec1 =
      module.GetBufferFromInputs("sdot", 0, Hobbit::Shape(1, 1, n_elts));
  Hobbit::Buffer *vec2 =
      module.GetBufferFromInputs("sdot", 1, Hobbit::Shape(1, 1, n_elts));

  std::vector<float> f;
  for (int i = 0; i < n_elts; i++) {
    f.push_back(i);
  }

  Hobbit::ElementWiseProduct prod(vec2, 4);
  Hobbit::SumReduction hsum(llvm::Type::getFloatTy(ctx), 4);
  Hobbit::Operation sdot_op("sdot_op");
  sdot_op.PushFunctor(prod);
  sdot_op.PushFunctor(hsum);

  Hobbit::Buffer *result =
      module.InsertOperation("sdot", &sdot_op, vec1, false);

  module.FinalizeFunction("sdot", result);

  module.FinalizeModule(0);

  module.PrintModule(llvm::outs());
  module.PrepareJIT();

  float (*prod_fn)(float *, float *) =
      (float (*)(float *, float *))module.GetFunctionPtr("sdot");

  auto start = std::chrono::high_resolution_clock::now();
  float float_result = prod_fn(f.data(), f.data());
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << n_elts
            << " elements" << std::endl;

  EXPECT_FLOAT_EQ(float_result,
                  (float)(n_elts - 1) / 3.f * ((float)(n_elts - 1) + 1.f) *
                      ((float)(n_elts - 1) + 0.5f));
}

TEST(TestModule, PerformNoConstProd) {

  llvm::LLVMContext ctx;
  Hobbit::Module module("test_module", ctx);

  uint64_t num_elements = 10000;

  std::vector<llvm::Type *> args = {
          llvm::Type::getFloatPtrTy(ctx), // input1
          llvm::Type::getFloatPtrTy(ctx), // input2
          llvm::Type::getFloatPtrTy(ctx)  // output
  };
  module.CreateFunction("prod", llvm::Type::getVoidTy(ctx), args, true); // last_is_output
  Hobbit::Buffer *input_buffer =
          module.GetBufferFromInputs("prod", 0, Hobbit::Shape(1, 1, num_elements));
  Hobbit::Buffer *input_buffer2 =
          module.GetBufferFromInputs("prod", 1, Hobbit::Shape(1, 1, num_elements));
  Hobbit::Buffer *output =
          module.GetBufferFromInputs("prod", 2, Hobbit::Shape(1, 1, num_elements));

  std::vector<float> f;
  for (int i = 0; i < num_elements; i++) {
    f.push_back(i);
  }

  Hobbit::ElementWiseProduct prod(input_buffer2, 4);
  Hobbit::Operation prod_op("prod_op");
  prod_op.PushFunctor(prod);

  Hobbit::Buffer *result =
          module.InsertOperation("prod", &prod_op, input_buffer, false);

  module.FinalizeFunction("prod", result, output);

  module.FinalizeModule(0);

  module.PrintModule(llvm::outs());
  module.PrepareJIT();

  void (*prod_fn)(float *, float *, float *) =
  (void (*)(float *, float *, float *))module.GetFunctionPtr("prod");

  float *float_result = (float *)calloc(num_elements, sizeof(float));

  auto start = std::chrono::high_resolution_clock::now();
  prod_fn(f.data(), f.data(), float_result);
  auto finish = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Elapsed time: " << elapsed.count() << " s for " << num_elements
            << " elements" << std::endl;

  for (int i = 0; i < num_elements; i++) {
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