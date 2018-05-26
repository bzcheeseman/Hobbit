//
// Created by Aman LaChapelle on 5/26/18.
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

#ifndef HOBBIT_OPTIMIZE_HPP
#define HOBBIT_OPTIMIZE_HPP

#include <llvm/IR/LegacyPassManager.h>
#include <string>

namespace Hobbit {
class Module;

namespace compile {
class Optimize {
public:
  void Initialize(unsigned int opt_level);

  void Run(Module *m, const std::string &target_triple, const std::string &cpu,
           const std::string &features);

private:
  llvm::legacy::PassManager m_pass_manager_;
};
} // namespace compile
} // namespace Hobbit

#endif // HOBBIT_OPTIMIZE_HPP
