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

#ifndef HOBBIT_MODULE_HPP
#define HOBBIT_MODULE_HPP

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/IR/Verifier.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include <Function.hpp>
#include <Buffer.hpp>
#include <Operation.hpp>

namespace Hobbit {

  class Module {
  public:
    explicit Module(const std::string &name, llvm::LLVMContext &ctx);

    // TODO: Do I want to make the functions return void? that way only the host
    // calls malloc...
    void CreateFunction(const std::string &name, llvm::Type *return_type,
                        llvm::ArrayRef<llvm::Type *> args_types);

    Buffer *GetBufferFromInputs(const std::string &function_name,
                                const uint32_t &ptr_idx, const Shape &shape);

    // TODO: add GetFunction (by name - returns a Hobbit::Function pointer)

    // Getters
    Buffer *GetVariable(const std::string &function_name,
                        llvm::Type *scalar_type, const Hobbit::Shape &shape);
    Buffer *GetIntConstant(const std::string &function_name, uint64_t *ptr,
                           const uint8_t &int_width, const Shape &shape);
    Buffer *GetHalfConstant(const std::string &function_name, float *ptr,
                            const Shape &shape);
    Buffer *GetFloatConstant(const std::string &function_name, float *ptr,
                             const Shape &shape);
    Buffer *GetDoubleConstant(const std::string &function_name, double *ptr,
                              const Shape &shape);

    Buffer *InsertOperation(const std::string &function_name, Operation *op,
                            Buffer *input);

    void PrintModule(llvm::raw_ostream &out_stream);
    void PrintModule(llvm::raw_fd_ostream &out_stream);

    void FinalizeFunction(const std::string &function_name,
                          Buffer *return_value);

    void FinalizeModule(unsigned opt_level);

    void PrepareJIT();

    uint64_t GetFunctionPtr(const std::string &name);

  private:
    llvm::LLVMContext *ctx_;
    std::unique_ptr<llvm::Module> module_;
    std::map<std::string, Function> function_table_;
    llvm::Function *malloc_;

    bool prepare_called_;
    llvm::ExecutionEngine *engine_;
  };
}

#endif // HOBBIT_MODULE_HPP
