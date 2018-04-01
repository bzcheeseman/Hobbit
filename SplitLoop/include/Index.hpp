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


#ifndef HOBBIT_INDEX_HPP
#define HOBBIT_INDEX_HPP

#include <cstdint>

namespace Hobbit {
  class Index {
  public:
    explicit Index(uint64_t i, uint64_t chunk=1) : i_(i), chunk_(chunk) {}

    Index &operator++(int);

    Index &operator=(uint64_t i);
    Index &operator=(int i);

    uint64_t Get();

    inline friend bool operator<(Index &lhs, Index &rhs) {
      return lhs.i_ < rhs.i_;
    }

    inline friend bool operator<(Index &lhs, const uint64_t &rhs) {
      return lhs.i_ < rhs;
    }

  private:
    uint64_t i_;
    uint64_t chunk_;
  };
}


#endif //HOBBIT_INDEX_HPP
