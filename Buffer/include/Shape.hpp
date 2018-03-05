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

#ifndef HOBBIT_SHAPE_HPP
#define HOBBIT_SHAPE_HPP

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace Hobbit {
  struct Range {
    explicit Range(const uint64_t &start = 0, const uint64_t &end = 0)
        : start(start), end(end) {}

    uint64_t start;
    uint64_t end; // exclusive
  };

  class Shape {
  public:
    Shape() = default;
    Shape(const uint64_t &k, const uint64_t &h, const uint64_t &w);

    uint64_t At(const uint64_t &k, const uint64_t &h, const uint64_t &w);

    Shape GetChunkShape(const Range &k_range, const Range &h_range,
                        const Range &w_range);
    Range GetChunkIdx(const Range &k_range, const Range &h_range,
                      const Range &w_range);

    uint64_t GetSize() const;

    friend inline bool operator==(const Shape &lhs, const Shape &rhs) {
      return (lhs.k_ == rhs.k_ && lhs.h_ == rhs.h_ && lhs.w_ == rhs.w_);
    }

    friend inline bool operator!=(const Shape &lhs, const Shape &rhs) {
      return !(lhs == rhs);
    }

    friend inline std::ostream &operator<<(std::ostream &out, const Shape &s) {
      out << "{"
          << "K: " << s.k_ << " H: " << s.h_ << " W: " << s.w_ << "}";

      return out;
    }

  private:
    uint64_t k_, h_, w_;
  };
}

#endif // HOBBIT_SHAPE_HPP
