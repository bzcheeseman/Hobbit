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

#include <codegen/Module.hpp>
#include <codegen/TreeBuilder.hpp>
#include <codegen/TreeCodeGen.hpp>
#include <compile/Optimize.hpp>
#include <graph/Node.hpp>
#include <graph/Operation.hpp>
#include <graph/Variable.hpp>
#include <ops/eltwise_add.hpp>
#include <ops/mock.hpp>

namespace {
using namespace Hobbit;

TEST(Basic, CreateGraph) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::SmallVector<uint64_t, 4> tensor_dims = {4, 4};

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
  //  module.RegisterOutput(eltwise_add);

  module.CodeGen("test_func", gemm);

  const std::string target_triple = llvm::sys::getDefaultTargetTriple();
  const std::string cpu = llvm::sys::getHostCPUName();
  llvm::StringMap<bool> feats_map;
  llvm::sys::getHostCPUFeatures(feats_map);

  std::string features = "";
  for (auto &entry : feats_map) {
    if (entry.second)
      features += "+" + std::string(entry.first()) + ",";
  }

  llvm::errs() << features;

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
      target->createTargetMachine(target_triple, cpu, features, options, RM);
  module.SetTarget(target_triple, target_machine->createDataLayout());

  //  std::error_code EC;
  //  llvm::raw_fd_ostream OS("/Users/Aman/Desktop/test_module.ll", EC,
  //  llvm::sys::fs::F_None); module.Print(OS); OS.flush();
}

} // namespace
