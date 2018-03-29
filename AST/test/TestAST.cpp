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

#include "Node.hpp"
#include "Visitor.hpp"

namespace {
  namespace Ha = Hobbit::ast;

//  TEST(Basic, CreateFunction) {
//    llvm::LLVMContext ctx;
//
//    Ha::Function *func = Ha::Function::Create("TestFunction");
//
//    Ha::Tensor *lhs =
//        func->GetNewArg("lhs", {153}, llvm::Type::getFloatTy(ctx));
//
//    Ha::Node *hsum = Ha::HSum::Create("hsum", func, 0, 153);
//    llvm::dyn_cast<Ha::HSum>(hsum)->SetArgs({lhs});
//    func->PushNode(hsum);
//    std::cout << "\n" << func->GetSignature() << std::endl;
//  }

  TEST(Basic, CreateLLVMFunction) {
    llvm::LLVMContext ctx;

    Ha::Visitor *cgvisitor = Ha::Visitor::Create(&ctx, "test_module");

    Ha::Function *func = Ha::Function::Create("TestFunction");

    const int n_elts = 153;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0, 1.0);

    std::vector<float> f1, f2; // why are there a bunch of zeros in the middle...
    for (int i = 0; i < n_elts; i++) {
      f1.push_back(dis(gen));
      f2.push_back(dis(gen));
    }

    Ha::Tensor *lhs =
        func->GetNewArg("lhs", {n_elts}, llvm::Type::getFloatPtrTy(ctx));

    Ha::Node *hsum = Ha::HSum::Create("hsum", func, 0, 153);
    Ha::Node *noop = Ha::HSum::Create("hsum", func, 0, 1);

    func->PushNode(hsum);
    Ha::Tensor *out = llvm::dyn_cast<Ha::HSum>(hsum)->SetArgs({lhs});
    func->PushNode(noop);
    Ha::Tensor *noop_out = llvm::dyn_cast<Ha::HSum>(noop)->SetArgs({out});
    func->SetArg(noop_out);

    func->Emit(cgvisitor);

    cgvisitor->FinalizeFunction(func);
    cgvisitor->GetModule()->print(llvm::outs(), nullptr);
    cgvisitor->Finalize(3, llvm::sys::getDefaultTargetTriple(), "corei7-avx", "+avx,+sse,+x87,+cx16");
    cgvisitor->GetModule()->print(llvm::outs(), nullptr);
  }
}
