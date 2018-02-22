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

#include "Shape.hpp"

Hobbit::Shape::Shape(const uint32_t &k, const uint32_t &h, const uint32_t &w)
    : k_(k), h_(h), w_(w) {}

uint32_t Hobbit::Shape::At(const uint32_t &k, const uint32_t &h,
                           const uint32_t &w) {
  if (k >= k_)
    throw std::runtime_error("Invalid index, k = " + std::to_string(k) +
                             "geq k_ = " + std::to_string(k_));
  if (h >= h_)
    throw std::runtime_error("Invalid index, h = " + std::to_string(h) +
                             "geq h_ = " + std::to_string(h_));
  if (w >= w_)
    throw std::runtime_error("Invalid index, w = " + std::to_string(w) +
                             "geq w_ = " + std::to_string(w_));

  return (k * h_ + h) * w_ + w;
}

Hobbit::Shape Hobbit::Shape::GetChunkShape(const Hobbit::Range &k_range,
                                           const Hobbit::Range &h_range,
                                           const Hobbit::Range &w_range) {
  uint32_t k_start, k_end, h_start, h_end, w_start, w_end;

  if (k_range.start == 0 && k_range.end == 0) {
    k_start = 0;
    k_end = k_ - 1;
  } else {
    k_start = k_range.start;
    k_end = k_range.end -
            1; // so if the range is {0, 1} we just have a single k index
  }

  if (h_range.start == 0 && h_range.end == 0) {
    h_start = 0;
    h_end = h_ - 1;
  } else {
    h_start = h_range.start;
    h_end = h_range.end - 1;
  }

  if (w_range.start == 0 && w_range.end == 0) {
    w_start = 0;
    w_end = w_ - 1;
  } else {
    w_start = w_range.start;
    w_end = w_range.end - 1;
  }

  return {k_end - k_start + 1, h_end - h_start + 1,
          w_end - w_start + 1};
}

Hobbit::Range Hobbit::Shape::GetChunkIdx(const Hobbit::Range &k_range,
                                         const Hobbit::Range &h_range,
                                         const Hobbit::Range &w_range) {
  Range idx_range(0, k_ * h_ * w_);
  uint32_t k_start, k_end, h_start, h_end, w_start, w_end;

  if (k_range.start == 0 && k_range.end == 0) {
    k_start = 0;
    k_end = k_ - 1;
  } else {
    k_start = k_range.start;
    k_end = k_range.end -
            1; // so if the range is {0, 1} we just have a single k index
  }

  if (h_range.start == 0 && h_range.end == 0) {
    h_start = 0;
    h_end = h_ - 1;
  } else {
    h_start = h_range.start;
    h_end = h_range.end - 1;
  }

  if (w_range.start == 0 && w_range.end == 0) {
    w_start = 0;
    w_end = w_ - 1;
  } else {
    w_start = w_range.start;
    w_end = w_range.end - 1;
  }

  idx_range.start = At(k_start, h_start, w_start);
  idx_range.end = At(k_end, h_end, w_end);

  return idx_range;
}

uint32_t Hobbit::Shape::GetSize() const { return k_ * h_ * w_; }

const uint32_t &Hobbit::Shape::GetAxisSize(Hobbit::Axis &axis) const {
  switch (axis) {
  case K:
    return k_;
  case H:
    return h_;
  case W:
    return w_;
  }

  return 0;
}
