//
// Created by Aman LaChapelle on 2/22/18.
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

#include "Operation.hpp"

Hobbit::Operation::Operation(const std::string &name) : name_(name) {}

Hobbit::Operation::Operation(const std::initializer_list<Functor *> &f,
                             const std::string &name)
    : op_table_(f), name_(name) {}

Hobbit::Operation::Operation(const std::list<Functor *> &f,
                             const std::string &name)
    : op_table_(f), name_(name) {}

void Hobbit::Operation::PushFunctor(Hobbit::Functor &f) {
  op_table_.push_back(&f);
}

Hobbit::Buffer *Hobbit::Operation::Emit(Function *f, Buffer *input, bool emit_inline) {
  Buffer *temp, *functor_input = input;
  Workspace workspace;
  for (auto &op : op_table_) {
    op->AllocOutput(f->entry_block, workspace);
    temp = op->Emit(f, functor_input, workspace, emit_inline);
    functor_input = temp;
  }
  return functor_input;
}

const std::string &Hobbit::Operation::GetName() { return name_; }
