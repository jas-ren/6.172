/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/


#include "./util.h"


// Function prototypes
static void merge_m(data_t* A, int p, int q, int r);
static void copy_p(data_t* source, data_t* dest, int n);
void sort_m(data_t* A, int p, int r);
void isort(data_t* left, data_t* right);

// A basic merge sort routine that sorts the subarray A[p..r]
inline void sort_m(data_t* A, int p, int r) {
  assert(A);
  if (r - p < 10) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_m(A, p, q);
    sort_m(A, q + 1, r);
    merge_m(A, p, q, r);
  }
}

// A merge routine. Merges the sub-arrays A [p..q] and A [q + 1..r].
// Uses two arrays 'left' and 'right' in the merge operation.
inline static void merge_m(data_t* A, int p, int q, int r) {
  assert(A);
  assert(p <= q);
  assert((q + 1) <= r);
  int n1 = q - p + 1;

  data_t* left = 0;
  mem_alloc(&left, n1 + 1);
  if (left == NULL) {
    mem_free(&left);
    return;
  }

  copy_p(&(A[p]), left, n1);
  left[n1] = UINT_MAX;

  data_t* i = left;
  data_t* A_p = A + p;
  data_t* A_q = A + q + 1;
  data_t* const A_end = A + r + 1;

  // Then our loop
  while (A_p < A_end && A_q < A_end) {
      if (*A_q <= *i) {
          *A_p++ = *A_q++;
      } else {
          *A_p++ = *i++;
      }
  }
  while (*i < UINT_MAX) {
    *A_p++ = *i++;
  }

  mem_free(&left);
}

inline static void copy_p(data_t* source, data_t* dest, int n) {
  assert(dest);
  assert(source);

  const data_t* end = dest + n;
  while (dest < end) {
    *dest++ = *source++;
  }
}
