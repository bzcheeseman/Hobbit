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

namespace Hobbit {
  struct Range {
    explicit Range(const uint32_t &start = 0, const uint32_t &end = 0)
        : start(start), end(end) {}

    uint32_t start;
    uint32_t end; // exclusive
  };

  enum Axis { K, H, W };

  class Shape {
  public:
    Shape() = default;
    Shape(const uint32_t &k, const uint32_t &h, const uint32_t &w);

    uint32_t At(const uint32_t &k, const uint32_t &h, const uint32_t &w);

    Shape GetChunkShape(const Range &k_range, const Range &h_range,
                        const Range &w_range);
    Range GetChunkIdx(const Range &k_range, const Range &h_range,
                      const Range &w_range);

    uint32_t GetSize() const;

    const uint32_t &GetAxisSize(Axis &axis) const;

    friend inline bool operator==(const Shape &lhs, const Shape &rhs) {
      return (lhs.k_ == rhs.k_ && lhs.h_ == rhs.h_ && lhs.w_ == rhs.w_);
    }

    friend inline bool operator!=(const Shape &lhs, const Shape &rhs) {
      return !(lhs == rhs);
    }

  private:
    uint32_t k_, h_, w_;
  };
}

#endif // HOBBIT_SHAPE_HPP
