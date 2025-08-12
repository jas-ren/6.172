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

// Implements the ADT specified in bitarray.h as a packed array of bits; a bit
// array containing bit_sz bits will consume roughly bit_sz/8 bytes of
// memory.


#include "./bitarray.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>


// ********************************* Types **********************************

// Concrete data type representing an array of bits.
struct bitarray {
  // The number of bits represented by this bit array.
  // Need not be divisible by 8.
  size_t bit_sz;

  // The underlying memory buffer that stores the bits in
  // packed form (8 per byte).
  char* buf;
};


// ******************** Prototypes for static functions *********************

// Rotates a subarray left by an arbitrary number of bits.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
// bit_left_amount is the number of places to rotate the
//                    subarray left
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
static void bitarray_rotate_left(bitarray_t* const bitarray,
                                 const size_t bit_offset,
                                 const size_t bit_length,
                                 const size_t bit_left_amount);

// Rotates a subarray left by one bit.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
// static void bitarray_rotate_left_one(bitarray_t* const bitarray,
//                                      const size_t bit_offset,
//                                      const size_t bit_length);

// Portable modulo operation that supports negative dividends.
//
// Many programming languages define modulo in a manner incompatible with its
// widely-accepted mathematical definition.
// http://stackoverflow.com/questions/1907565/c-python-different-behaviour-of-the-modulo-operation
// provides details; in particular, C's modulo
// operator (which the standard calls a "remainder" operator) yields a result
// signed identically to the dividend e.g., -1 % 10 yields -1.
// This is obviously unacceptable for a function which returns size_t, so we
// define our own.
//
// n is the dividend and m is the divisor
//
// Returns a positive integer r = n (mod m), in the range
// 0 <= r < m.
static size_t modulo(const ssize_t n, const size_t m);

// Produces a mask which, when ANDed with a byte, retains only the
// bit_index th byte.
//
// Example: bitmask(5) produces the byte 0b00100000.
//
// (Note that here the index is counted from right
// to left, which is different from how we represent bitarrays in the
// tests.  This function is only used by bitarray_get and bitarray_set,
// however, so as long as you always use bitarray_get and bitarray_set
// to access bits in your bitarray, this reverse representation should
// not matter.
static char bitmask(const size_t bit_index);

static void bitarray_reverse(bitarray_t* bitarray, const size_t start, const size_t end);


// ******************************* Functions ********************************

bitarray_t* bitarray_new(const size_t bit_sz) {
  // Allocate an underlying buffer of ceil(bit_sz/8) bytes.
  char* const buf = calloc(1, (bit_sz+7) / 8);
  if (buf == NULL) {
    return NULL;
  }

  // Allocate space for the struct.
  bitarray_t* const bitarray = malloc(sizeof(struct bitarray));
  if (bitarray == NULL) {
    free(buf);
    return NULL;
  }

  bitarray->buf = buf;
  bitarray->bit_sz = bit_sz;
  return bitarray;
}

void bitarray_free(bitarray_t* const bitarray) {
  if (bitarray == NULL) {
    return;
  }
  free(bitarray->buf);
  bitarray->buf = NULL;
  free(bitarray);
}

size_t bitarray_get_bit_sz(const bitarray_t* const bitarray) {
  return bitarray->bit_sz;
}

bool bitarray_get(const bitarray_t* const bitarray, const size_t bit_index) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to get the nth
  // bit, we want to look at the (n mod 8)th bit of the (floor(n/8)th)
  // byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to produce either a zero byte (if the bit was 0) or a nonzero byte
  // (if it wasn't).  Finally, we convert that to a boolean.
  return (bitarray->buf[bit_index / 8] & bitmask(bit_index)) ?
         true : false;
}

void bitarray_set(bitarray_t* const bitarray,
                  const size_t bit_index,
                  const bool value) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to set the nth
  // bit, we want to set the (n mod 8)th bit of the (floor(n/8)th) byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to clear out the bit we're about to set.  We bitwise-or the result
  // with a byte that has either a 1 or a 0 in the correct place.
  bitarray->buf[bit_index / 8] =
    (bitarray->buf[bit_index / 8] & ~bitmask(bit_index)) |
    (value ? bitmask(bit_index) : 0);
}

void bitarray_randfill(bitarray_t* const bitarray){
  int32_t *ptr = (int32_t *)bitarray->buf;
  for (int64_t i=0; i<bitarray->bit_sz/32 + 1; i++){
    ptr[i] = rand();
  }
}

void bitarray_rotate(bitarray_t* const bitarray,
                     const size_t bit_offset,
                     const size_t bit_length,
                     const ssize_t bit_right_amount) {
  assert(bit_offset + bit_length <= bitarray->bit_sz);

  if (bit_length == 0) {
    return;
  }

  // Convert a rotate left or right to a left rotate only, and eliminate
  // multiple full rotations.
  bitarray_rotate_left(bitarray, bit_offset, bit_length,
                       modulo(-bit_right_amount, bit_length));
}

static void bitarray_rotate_left(bitarray_t* const bitarray,
                                 const size_t bit_offset,
                                 const size_t bit_length,
                                 const size_t bit_left_amount) {
  // for (size_t i = 0; i < bit_left_amount; i++) {
  //   bitarray_rotate_left_one(bitarray, bit_offset, bit_length);
  // }
  size_t midpoint = bit_offset + bit_left_amount;
  bitarray_reverse(bitarray, bit_offset, midpoint);
  bitarray_reverse(bitarray, midpoint, bit_offset + bit_length);
  bitarray_reverse(bitarray, bit_offset, bit_offset + bit_length);
}

static size_t modulo(const ssize_t n, const size_t m) {
  const ssize_t signed_m = (ssize_t)m;
  assert(signed_m > 0);
  const ssize_t result = ((n % signed_m) + signed_m) % signed_m;
  assert(result >= 0);
  return (size_t)result;
}

static char bitmask(const size_t bit_index) {
  return 1 << (bit_index % 8);
}

static void bitarray_reverse(bitarray_t* bitarray, const size_t start, const size_t end) {
  // [start, end)
  size_t left = start;
  size_t right = end - 1;
  while (left < right && right <= end) {
    bool left_bit = bitarray_get(bitarray, left);
    bool right_bit = bitarray_get(bitarray, right);
    bitarray_set(bitarray, left, right_bit);
    bitarray_set(bitarray, right, left_bit);
    left++;
    right--;
  }
}

u_int64_t word_size_bitarray_rotate(const u_int64_t bitarray,
                                      const size_t offset,
                                      const size_t length,
                                      const size_t right_rotation) {
  // Handle edge case: avoid undefined behavior when length == 64
  u_int64_t mask;
  if (length == 64 && offset == 0) {
    mask = ~0ULL;  // All bits set
  } else if (length == 64) {
    // This shouldn't happen in a properly called function, but handle it
    mask = ~0ULL >> offset;
  } else {
    mask = ((1ULL << length) - 1) << offset;
  }
  
  u_int64_t subarray_to_rotate = bitarray & mask;
  u_int64_t left = subarray_to_rotate << right_rotation;
  u_int64_t right = subarray_to_rotate >> (length - right_rotation);
  u_int64_t rotated_subarray = (left | right) & mask;
  return rotated_subarray | (~mask & bitarray);
}

void multi_word_rotate(u_int64_t* bitarray,
                              const size_t offset, // bits
                              const size_t length,
                              const size_t right_rotation) {
  if (right_rotation % length == 0) {
    return;
  }
  const size_t WORDSIZE = 64; // in bits
  
  // Optimization: if the rotation fits in a single word, use the simpler function
  if (length <= WORDSIZE && (offset % WORDSIZE) + length <= WORDSIZE) {
    size_t word_idx = offset / WORDSIZE;
    bitarray[word_idx] = word_size_bitarray_rotate(bitarray[word_idx], offset % WORDSIZE, length, right_rotation);
    return;
  }
  
  u_int64_t* subarray_to_rotate = calloc((length + WORDSIZE - 1) / WORDSIZE, sizeof(u_int64_t)); // round to nearest word

  // Step 1: Isolate the subarray to be rotated
  u_int64_t len_copy = length;
  u_int64_t offset_within_word = offset % WORDSIZE;
  u_int64_t offset_within_word_copy = offset_within_word;
  size_t source_ind = offset / WORDSIZE;
  size_t dest_ind = 0;
  while (len_copy > 0)  {
    u_int64_t mask;
    if ((len_copy + offset_within_word) < WORDSIZE) { // the remaining bits fit within the word
      mask = ((1ULL << len_copy) - 1) << offset_within_word;
    } else {
      mask = -1ULL << offset_within_word;
    }
    subarray_to_rotate[dest_ind] = bitarray[source_ind] & mask;
    len_copy -= __builtin_popcountll(mask);
    offset_within_word = 0;
    dest_ind++;
    source_ind++;
  }
  // Results in the bits that need to be rotated in the new subarray_to_rotate, but with
  // padding in the front by offset_within_word_copy.

  // Step 2: Align the subarray to be rotated
  offset_within_word = offset_within_word_copy;
  for (size_t i = 0; i < dest_ind - 1; ++i) {
    u_int64_t current_aligned = subarray_to_rotate[i] << offset_within_word;
    u_int64_t next_partial = (subarray_to_rotate[i+1] >> (WORDSIZE - offset_within_word));
    subarray_to_rotate[i] = current_aligned | next_partial;
  }


  // Step 3: Perform the rotation
  // First we need to clarify how many words in subarray_to_rotate contain valid bits
  // so that we don't accidentally mix padding into the rotation
  size_t num_words = (length + WORDSIZE - 1) / WORDSIZE;
  size_t last_word_idx = num_words - 1;  // Convert count to index
  size_t trailing_bits_count = length % WORDSIZE;

  // Substep 1: move the bits within words
  u_int64_t n_bits = right_rotation % WORDSIZE;
  if (n_bits < trailing_bits_count) {
    u_int64_t first_word = subarray_to_rotate[0];
    for (size_t j = 0; j < last_word_idx - 1; ++j) {
      u_int64_t current_aligned = subarray_to_rotate[j] << n_bits;
      u_int64_t next_partial = (subarray_to_rotate[j + 1] >> (WORDSIZE - n_bits)); 
      subarray_to_rotate[j] = current_aligned | next_partial;
    }
    u_int64_t last_word = subarray_to_rotate[last_word_idx] << n_bits;
    u_int64_t wrap_around_partial = first_word >> (WORDSIZE - n_bits);
    subarray_to_rotate[last_word_idx] = last_word | wrap_around_partial;
  } else {
    u_int64_t first_word = subarray_to_rotate[0];
    
    // Handle the general case: shift all words except the last two
    if (last_word_idx >= 2) {
      for (size_t j = 0; j < last_word_idx - 1; ++j) {
        u_int64_t current_aligned = subarray_to_rotate[j] << n_bits;
        u_int64_t next_partial = (subarray_to_rotate[j + 1] >> (WORDSIZE - n_bits)); 
        subarray_to_rotate[j] = current_aligned | next_partial;
      }
      
      // Handle second-to-last word
      u_int64_t second_last_word = subarray_to_rotate[last_word_idx - 1] << n_bits;
      u_int64_t trailing_bits_shifted = subarray_to_rotate[last_word_idx] >> (WORDSIZE - trailing_bits_count - n_bits);
      u_int64_t bits_from_first_word_that_fall_into_second_last_word = first_word >> (WORDSIZE - (n_bits - trailing_bits_count));
      second_last_word = second_last_word | trailing_bits_shifted | bits_from_first_word_that_fall_into_second_last_word;
      subarray_to_rotate[last_word_idx - 1] = second_last_word;
    }

    // Handle last word
    subarray_to_rotate[last_word_idx] = first_word >> (WORDSIZE - trailing_bits_count);
  }

  // Substep 2: move whole words by n_words
  u_int64_t n_words = right_rotation / WORDSIZE;
  u_int64_t* temp_buf = malloc(n_words * sizeof(u_int64_t));
  memcpy(temp_buf, subarray_to_rotate, n_words * sizeof(u_int64_t));
  memmove(subarray_to_rotate, subarray_to_rotate + n_words, (dest_ind - n_words) * sizeof(u_int64_t));
  memcpy(subarray_to_rotate + (dest_ind - n_words), temp_buf, n_words * sizeof(u_int64_t));
  free(temp_buf);


  // Step 4: shift the rotated words back by n_bits
  size_t bits_offset = offset % WORDSIZE;
  if (trailing_bits_count + bits_offset <= WORDSIZE) {
    for (size_t k = 1; k <= last_word_idx; ++k) {
      u_int64_t current_shifted = subarray_to_rotate[k] >> bits_offset;
      u_int64_t overflow_from_previous = subarray_to_rotate[k-1] << (WORDSIZE - bits_offset);
      subarray_to_rotate[k] = current_shifted | overflow_from_previous;
    }
    subarray_to_rotate[0] = subarray_to_rotate[0] >> bits_offset;
  } else {
    u_int64_t last_word = subarray_to_rotate[last_word_idx];
    for (size_t k = 1; k < last_word_idx; ++k) {
      u_int64_t current_shifted = subarray_to_rotate[k] >> bits_offset;
      u_int64_t overflow_from_previous = subarray_to_rotate[k-1] << (WORDSIZE - bits_offset);
      subarray_to_rotate[k] = current_shifted | overflow_from_previous;
    }
    subarray_to_rotate[last_word_idx] = last_word >> bits_offset;
    subarray_to_rotate[last_word_idx + 1] = last_word << (WORDSIZE - bits_offset);
    subarray_to_rotate[0] = subarray_to_rotate[0] >> bits_offset;
    last_word_idx++;
  }

  // Step 5: Put the bits back into the original array
  size_t original_array_idx = offset / WORDSIZE;
  size_t rotation_array_idx = 0;
  while (rotation_array_idx <= last_word_idx) {
    if (rotation_array_idx == 0) {
      u_int64_t first_word_mask = -1ULL << (WORDSIZE - offset_within_word_copy);
      u_int64_t first_word = (bitarray[original_array_idx] & first_word_mask) | (subarray_to_rotate[rotation_array_idx] & ~first_word_mask);
      bitarray[original_array_idx] = first_word;
    } else if (rotation_array_idx == last_word_idx) {
      u_int64_t last_word_mask = -1ULL >> (offset_within_word_copy + trailing_bits_count);
      u_int64_t last_word = (bitarray[original_array_idx] & last_word_mask) | subarray_to_rotate[rotation_array_idx];
      bitarray[original_array_idx] = last_word;
    } else {
      bitarray[original_array_idx] = subarray_to_rotate[rotation_array_idx];
    }
    original_array_idx++;
    rotation_array_idx++;
  }
  free(subarray_to_rotate);
}