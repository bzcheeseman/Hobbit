//
// Created by Aman LaChapelle on 3/23/18.
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

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <random>

#include <codegen/CGVisitor.hpp>
#include <codegen/Module.hpp>
#include <codegen/TreeVisitor.hpp>
#include <graph/Node.hpp>
#include <ops/eltwise_add.hpp>
#include <ops/mock.hpp>
#include <compile/Optimize.hpp>

namespace {
using namespace Hobbit;

TEST(Basic, CreateGraph) {
  llvm::SmallVector<uint64_t, 4> tensor_dims = {28, 28};

  Module module("TestModule");

  graph::Variable *argA = module.GetVariable("argA", tensor_dims, FLOAT32);
  graph::Variable *argB = module.GetVariable("argB", tensor_dims, FLOAT32);
  graph::Variable *argC = module.GetVariable("argC", tensor_dims, FLOAT32);
  graph::Variable *alpha = module.GetVariable("alpha", {1}, FLOAT32);
  graph::Variable *beta = module.GetVariable("beta", {1}, FLOAT32);

  graph::Operation *eltwise_add =
      module.GetOperation("add", {argA, argB}, ops::Operator::eltwiseAddID);
  graph::Operation *gemm = module.GetOperation(
      "gemm", {eltwise_add, argC, alpha, beta}, ops::Operator::gemmID);

  module.RegisterOutput(gemm);
  module.RegisterOutput(eltwise_add);

  module.CodeGen("test_func", gemm);

  module.Finalize(llvm::sys::getDefaultTargetTriple(),
                  llvm::sys::getHostCPUName(), "");
  module.Print(llvm::errs());

//  compile::Optimize opt;
//  opt.Initialize(3);
//  opt.Run(&module, llvm::sys::getDefaultTargetTriple(), llvm::sys::getHostCPUName(), "");
}

} // namespace
