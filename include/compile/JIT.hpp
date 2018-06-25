//
// Created by Aman LaChapelle on 5/19/18.
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

#ifndef HOBBIT_JIT_HPP
#define HOBBIT_JIT_HPP

#include <string>

namespace llvm {
class TargetMachine;
class Target;
class ExecutionEngine;
} // namespace llvm

namespace Hobbit {
namespace compile {
class JIT { // takes in a Hobbit::codegen::Module
public:
  void InitializeLLVM();

  void BuildEngine(const std::string &target_triple,
                   const std::string &cpu = "",
                   const std::string &features = "");
  // TODO: thin wrapper around llvm::ExecutionEngine

private:
  const llvm::Target *InitTarget_(const std::string &target_triple);

private:
  llvm::TargetMachine *m_target_machine_;
  llvm::ExecutionEngine *m_engine_;
};
} // namespace compile
} // namespace Hobbit

#endif // HOBBIT_JIT_HPP
