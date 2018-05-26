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

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "include/Index.hpp"

const size_t SIZE = 34000000;
const size_t CHUNK = 32000;
const size_t STRIDE = 8;

//float dot(float *lhs, float *rhs) {
//  float out[STRIDE];
//
//  memset(out, 0, STRIDE*sizeof(float));
//
//  size_t leftovers_start = SIZE - (SIZE % CHUNK);
//
//  for (size_t i = 0; i < leftovers_start; i += CHUNK) {
//    for (size_t j = 0; j < CHUNK; j += 1) {
//      out[j % STRIDE] += lhs[i + j] * rhs[i + j];
//    }
//  }
//
//  float result = 0.f;
//  for (size_t i = 0; i < STRIDE; i++) {
//    result += out[i];
//  }
//
//  return result;
//}

//float simpledot(float *lhs, float *rhs) {
//  float out  = 0.f;
//
//  for (size_t i = 0; i < SIZE; i++) {
//    out += lhs[i] * rhs[i];
//  }
//
//  return out;
//}

const size_t M = 1024;
const size_t N = 512;
const size_t M_TILE = 32;
//const size_t STRIDE = 8;

void simplegemv(float **lhs, float *rhs, float *out) {
  for (size_t i = 0; i < M; i++) {
    for (size_t j = 0; j < N; j++) {
      out[i] += lhs[i][j] * rhs[j];
    }
  }
}

//float indexdot(float *lhs, float *rhs) {
//  float out = 0.f;
//
//  Hobbit::Index i (0);
//  for (; i < SIZE; i++) {
//    out += lhs[i.Get()] * rhs[i.Get()];
//  }
//
//  return out;
//}
