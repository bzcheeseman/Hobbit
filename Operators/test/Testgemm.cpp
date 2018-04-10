//
// Created by Aman LaChapelle on 4/8/18.
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

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include "../src/gemm.cpp"

// TODO (Aman): Add actual asserts here
TEST(OpTest_Create, gemm) {
  llvm::LLVMContext ctx;

  llvm::Type *float_ty = llvm::Type::getFloatTy(ctx);
  llvm::Type *float_ptr_ty = float_ty->getPointerTo(0);

  std::unique_ptr<llvm::Module> Mod =
          llvm::make_unique<llvm::Module>("gemm_test", ctx);

  llvm::FunctionType *ft =
          llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {float_ptr_ty, float_ptr_ty, float_ptr_ty}, false);

  llvm::Function *f = llvm::cast<llvm::Function>(
          Mod->getOrInsertFunction("gemm_test", ft));

  const size_t N = 1024;
  const size_t M = 1024;
  const size_t K = 1024;

  llvm::Function::arg_iterator iter = f->arg_begin();

  Hobbit::ast::Tensor *A = Hobbit::ast::Tensor::Create("A", nullptr, {N, K}, float_ty);
  llvm::Argument *arg_A = &(*iter);
  A->SetValue(arg_A);

  Hobbit::ast::Tensor *B = Hobbit::ast::Tensor::Create("B", nullptr, {K, M}, float_ty);
  llvm::Argument *arg_B = &(*++iter);
  B->SetValue(arg_B);

  Hobbit::ast::Tensor *C = Hobbit::ast::Tensor::Create("C", nullptr, {N, M}, float_ty);
  llvm::Argument *arg_C = &(*++iter);
  C->SetValue(arg_C);

  llvm::Value *alpha = llvm::ConstantFP::get(float_ty, 1.0);
  llvm::Value *beta = llvm::ConstantFP::get(float_ty, 0.0);

  Hobbit::Operator *gemm_op = new Hobbit::gemm(N, M, K, A, B, C, alpha, beta);

  llvm::BasicBlock *ret_block = gemm_op->InsertIntoFunction(f);

  llvm::IRBuilder<> builder(ret_block);
  builder.CreateRetVoid();

  Mod->print(outs(), nullptr);
  llvm::verifyModule(*Mod, &llvm::errs());

  EXPECT_TRUE(llvm::dyn_cast<Hobbit::gemm>(gemm_op) != nullptr);
}

TEST(OpTest_Verify, gemm) {
  llvm::LLVMContext ctx;

  llvm::Type *float_ty = llvm::Type::getFloatTy(ctx);
  llvm::Type *float_ptr_ty = float_ty->getPointerTo(0);

  std::unique_ptr<llvm::Module> Mod =
          llvm::make_unique<llvm::Module>("gemm_test", ctx);

  llvm::FunctionType *ft =
          llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {float_ptr_ty, float_ptr_ty, float_ptr_ty}, false);

  llvm::Function *f = llvm::cast<llvm::Function>(
          Mod->getOrInsertFunction("gemm_test", ft));

  const size_t N = 1024;
  const size_t M = 1024;
  const size_t K = 1024;

  llvm::Function::arg_iterator iter = f->arg_begin();

  Hobbit::ast::Tensor *A = Hobbit::ast::Tensor::Create("A", nullptr, {N, K}, float_ty);
  llvm::Argument *arg_A = &(*iter);
  A->SetValue(arg_A);

  Hobbit::ast::Tensor *B = Hobbit::ast::Tensor::Create("B", nullptr, {K, M}, float_ty);
  llvm::Argument *arg_B = &(*++iter);
  B->SetValue(arg_B);

  Hobbit::ast::Tensor *C = Hobbit::ast::Tensor::Create("C", nullptr, {N, M}, float_ty);
  llvm::Argument *arg_C = &(*++iter);
  C->SetValue(arg_C);

  llvm::Value *alpha = llvm::ConstantFP::get(float_ty, 1.0);
  llvm::Value *beta = llvm::ConstantFP::get(float_ty, 0.0);

  Hobbit::Operator *gemm_op = new Hobbit::gemm(N, M, K, A, B, C, alpha, beta);

  llvm::BasicBlock *ret_block = gemm_op->InsertIntoFunction(f);

  llvm::IRBuilder<> builder(ret_block);
  builder.CreateRetVoid();

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  Mod->setTargetTriple(llvm::sys::getDefaultTargetTriple());

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(llvm::sys::getDefaultTargetTriple(), error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
          target->createTargetMachine(llvm::sys::getDefaultTargetTriple(), "generic", "", options, RM);

  Mod->setDataLayout(target_machine->createDataLayout());
  Mod->setTargetTriple(llvm::sys::getDefaultTargetTriple());

  llvm::verifyModule(*Mod, &llvm::errs());

  llvm::legacy::PassManager pm;
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = 3;

  PMBuilder.populateModulePassManager(pm);
  target_machine->adjustPassManager(PMBuilder);

  pm.run(*Mod);

  llvm::verifyModule(*Mod, &llvm::errs());

  std::string error_str; // JIT has not been linked in

  llvm::EngineBuilder engineBuilder(std::move(Mod));
  engineBuilder.setErrorStr(&error_str);
  engineBuilder.setEngineKind(llvm::EngineKind::JIT);
  llvm::ExecutionEngine *engine = engineBuilder.create();

  void (*hobbit_gemm)(float *, float *, float *) = (void (*)(float *, float *, float *))engine->getFunctionAddress("gemm_test");
}
