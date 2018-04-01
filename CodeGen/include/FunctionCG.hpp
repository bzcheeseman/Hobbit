//
// Created by Aman LaChapelle on 3/31/18.
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


#ifndef HOBBIT_FUNCTIONCG_HPP
#define HOBBIT_FUNCTIONCG_HPP

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <Node.hpp>
#include <llvm/Support/raw_ostream.h>

namespace Hobbit {
  class FunctionCG {
  public:
    explicit FunctionCG(Function *func) : func_(func) {}

    void Emit(llvm::LLVMContext &ctx, llvm::Module *module) {

      llvm::SmallVector<llvm::Type *, 4> arg_types;
      for (auto &arg : func_->arg_table_) {
        arg_types.push_back(arg->GetType());
      }

      llvm::FunctionType *ft =
              llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), arg_types, false);

      llvm_func_ =
              llvm::cast<llvm::Function>(module->getOrInsertFunction(func_->name_, ft));
      // Create entry block, do nothing with it
      llvm::BasicBlock *entry_bb =
              llvm::BasicBlock::Create(ctx, func_->name_ + ".entry", llvm_func_);
      llvm::IRBuilder<> builder(entry_bb);

      llvm::Function::arg_iterator iter = llvm_func_->arg_begin();
      for (auto &arg : func_->arg_table_) {
        arg->SetBuffer(&(*iter++));
      }

      for (auto &alloca : func_->alloca_table_) {
        llvm::Type *alloca_type = alloca->GetType();
        if (alloca_type->isPointerTy())
          alloca_type = alloca_type->getPointerElementType();
        alloca_type = llvm::ArrayType::get(alloca_type, alloca->Size());
        llvm::Value *a = builder.CreateAlloca(alloca_type);
        builder.CreateStore(
                llvm::ConstantAggregateZero::get(alloca_type), a
        );
        llvm::Value *ptr = builder.CreateInBoundsGEP(a, {builder.getInt64(0), builder.getInt64(0)});
        alloca->SetBuffer(ptr);
      }

      ast::Node *n = func_->child_;
      while (n != nullptr) {
//        n->Emit(CG); // TODO: need to revamp this bit
        n = n->GetChild();
      }

      // Create a return for the exit BB
      llvm::BasicBlock *exit_bb = &*(--llvm_func_->end());
      builder.SetInsertPoint(exit_bb);
      builder.CreateRetVoid();

      // Finalize the various BB in the function

      for (auto BB = llvm_func_->begin(); BB != llvm_func_->end(); BB++) {

        if (BB->empty()) {
          llvm::outs() << "BB empty\n";
          builder.SetInsertPoint(&*BB);
          builder.CreateBr(&*(++BB));
        }

        llvm::TerminatorInst *br = llvm::dyn_cast<llvm::TerminatorInst>(&(*(--(*BB).end())));
        if (!br) {
          llvm::outs() << "No terminator inst found, inserting at end\n";
          builder.SetInsertPoint(&*BB);
          builder.CreateBr(&*(++BB));
        }
        llvm::outs() << "Terminator inst found\n";
      }

    }

  private:
    Function *func_;
    llvm::Function *llvm_func_;
  };
}


#endif //HOBBIT_FUNCTIONCG_HPP
