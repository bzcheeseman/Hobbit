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

#include <stdint.h>
#include <stdbool.h>

#ifndef HOBBIT_INDEX_TYPE
#define HOBBIT_INDEX_TYPE uint64_t
#endif

void sgemm(float **C, float **A, float **B,
           HOBBIT_INDEX_TYPE M, HOBBIT_INDEX_TYPE N, HOBBIT_INDEX_TYPE K,
           bool transa, bool transb) {
  for (HOBBIT_INDEX_TYPE k = 0; k < K; k++) {
    for (HOBBIT_INDEX_TYPE i = 0; i < M; i++) {
      for (HOBBIT_INDEX_TYPE j = 0; j < N; j++) {
        float a_elt = transa ? A[i][k] : A[k][i];
        float b_elt = transb ? B[k][j] : B[j][k];
        C[i][j] += a_elt * b_elt;
      }
    }
  }
}

void dgemm(double **C, double **A, double **B,
           HOBBIT_INDEX_TYPE M, HOBBIT_INDEX_TYPE N, HOBBIT_INDEX_TYPE K,
           bool transa, bool transb) {
  for (HOBBIT_INDEX_TYPE k = 0; k < K; k++) {
    for (HOBBIT_INDEX_TYPE i = 0; i < M; i++) {
      for (HOBBIT_INDEX_TYPE j = 0; j < N; j++) {
        double a_elt = transa ? A[i][k] : A[k][i];
        double b_elt = transb ? B[k][j] : B[j][k];
        C[i][j] += a_elt * b_elt;
      }
    }
  }
}
