//
// Created by Aman LaChapelle on 2/23/18.
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
#include <gtest/gtest.h>

TEST(TestShape, At) {
  Hobbit::Shape shape(32, 92, 92);

  EXPECT_EQ(shape.At(0, 1, 34), 92 + 34);
}

TEST(TestShape, Chunk) {
  Hobbit::Shape shape(32, 92, 92);

  Hobbit::Shape chunk_shape = shape.GetChunkShape(
      Hobbit::Range(0, 2), Hobbit::Range(0, 46), Hobbit::Range(0, 46));
  EXPECT_TRUE(chunk_shape == Hobbit::Shape(2, 46, 46));

  Hobbit::Range chunk_idx = shape.GetChunkIdx(
      Hobbit::Range(0, 2), Hobbit::Range(0, 46), Hobbit::Range(0, 46));

  EXPECT_EQ(chunk_idx.start, 0);
  EXPECT_EQ(chunk_idx.end, (1 * 92 + 45) * 92 + 45);
}
