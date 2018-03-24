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
#include <llvm/Support/raw_ostream.h>

#include "Node.hpp"

namespace {
  namespace Ha = Hobbit::ast;

  TEST(Basic, CreateFunction) {
    llvm::LLVMContext ctx;

    Ha::Function *func = Ha::Function::Create("TestFunction");
    std::cout << "\n" << func->GetSignature() << std::endl;

    Ha::Tensor *lhs = func->GetNewArg("lhs", {153}, llvm::Type::getFloatTy(ctx));
    std::cout << "\n" << func->GetSignature() << std::endl;

    Ha::Node *hsum = Ha::HSum::Create("hsum", func, 0, 153);
    llvm::dyn_cast<Ha::HSum>(hsum)->SetArgs({lhs});
    func->PushNode(hsum);
    std::cout << "\n" << func->GetSignature() << std::endl;
  }

  TEST(Basic, CreateLLVMFunction) {
    llvm::LLVMContext ctx;

    std::unique_ptr<llvm::Module> module = llvm::make_unique<llvm::Module>("test_module", ctx);

    Ha::Function *func = Ha::Function::Create("TestFunction");

    Ha::Tensor *lhs = func->GetNewArg("lhs", {153}, llvm::Type::getFloatPtrTy(ctx));

    Ha::Node *hsum = Ha::HSum::Create("hsum", func, 0, 153);

    func->PushNode(hsum);
    llvm::dyn_cast<Ha::HSum>(hsum)->SetArgs({lhs});
    llvm::Function *f = func->EmitFunction(module.get());
    module->print(llvm::outs(), nullptr);
  }
}
