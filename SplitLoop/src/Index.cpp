//
// Created by Aman LaChapelle on 4/1/18.
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


#include "Index.hpp"

Hobbit::Index &Hobbit::Index::operator++(int) {
  i_++;
  return *this;
}

Hobbit::Index &Hobbit::Index::operator=(uint64_t i) {
  i_ = i;
  return *this;
}

Hobbit::Index &Hobbit::Index::operator=(int i) {
  i_ = (uint64_t)i;
  return *this;
}

Hobbit::Index::operator uint64_t() const {
  return 0;
}
