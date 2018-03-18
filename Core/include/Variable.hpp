//
// Created by Aman LaChapelle on 3/17/18.
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

#ifndef HOBBIT_VARIABLE_HPP
#define HOBBIT_VARIABLE_HPP

#include "Function.hpp"
#include "Shape.hpp"
#include "Symbol.hpp"
#include "Tensor.hpp"
#include "Type.hpp"

namespace llvm {
  class Type;
}

namespace Hobbit {

  // can get a Core from the global Module - this will insert the correct
  // alloca into the entry block and add it to the function args
  class Variable : public Tensor {
  private:
    explicit Variable(core::Symbol *s) : Tensor(s){};

  public:
    template <typename T, unsigned int BITWIDTH>
    static Variable *Create(std::unique_ptr<Function> &f,
                            Hobbit::core::Type<T, BITWIDTH> *type,
                            const Shape &s) {
      core::Symbol *sym =
          new core::Symbol(f, s, type->get(f->GetContext()), false, nullptr);

      Variable *var = new Variable(sym);
      f->AddSymbol(var, sym);

      return var;
    }

    static Variable *Create(std::unique_ptr<Function> &f, llvm::Type *type,
                            const Shape &s) {
      core::Symbol *sym = new core::Symbol(f, s, type, false, nullptr);

      Variable *var = new Variable(sym);
      f->AddSymbol(var, sym);

      return var;
    }
  };

  class Constant : public Tensor {
  private:
    explicit Constant(core::Symbol *s) : Tensor(s){};

  public:
    template <typename T, unsigned int BITWIDTH>
    static Constant *Create(std::unique_ptr<Function> &f,
                            Hobbit::core::Type<T, BITWIDTH> *type,
                            const Shape &s, T *buffer) {
      core::Symbol *sym =
          new core::Symbol(f, s, type->get(f->GetContext()), false, buffer);

      Constant *c = new Constant(sym);
      f->AddSymbol(c, sym);
      return c;
    }

    static Constant *Create(std::unique_ptr<Function> &f, llvm::Type *type,
                            const Shape &s, void *buffer) {
      core::Symbol *sym = new core::Symbol(f, s, type, false, buffer);

      Constant *c = new Constant(sym);
      f->AddSymbol(c, sym);

      return c;
    }
  };
}

#endif // HOBBIT_VARIABLE_HPP
