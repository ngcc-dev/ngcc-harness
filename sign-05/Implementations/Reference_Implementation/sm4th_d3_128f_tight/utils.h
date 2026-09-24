/*
 *  This file implements some utility functions, such as bit manipulation and array XOR.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "compat.h"
#include "macros.h"
#include "params.h"

/* XOR two byte arrays: out[i] = a[i] ^ b[i]. */
static inline void xor_u8_array(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t len) {
  size_t i = 0;
  for (; i < len; i++) {
    out[i] = a[i] ^ b[i];
  }
}

/* Conditionally XOR: out[i] = a[i] ^ (b[i] & mask), where mask = 0xFF if
 * mask_bit is 1, 0x00 if mask_bit is 0. */
static inline void masked_xor_u8_array(const uint8_t* a, const uint8_t* b, uint8_t* out,
                                       uint8_t mask_bit, size_t len) {
  uint8_t mask = -(mask_bit & 1);
  size_t i = 0;
  for (; i < len; i++) {
    out[i] = a[i] ^ (b[i] & mask);
  }
}

#define get_bit(value, index) (((value) >> (index)) & 1)
#define set_bit(value, index) ((value) << (index))
#define ptr_get_bit(value, index) (((value)[(index) / 8] >> ((index) % 8)) & 1)
#define ptr_set_bit(dst, index, value)                                                             \
  do {                                                                                             \
    const unsigned int ptr_set_bit_index_mod_8 = (index) % 8;                                      \
    (dst)[(index) / 8] = ((dst)[(index) / 8] & ~(1 << ptr_set_bit_index_mod_8)) |                  \
                         ((value) << ptr_set_bit_index_mod_8);                                     \
  } while (0)

/* Shared 32-bit rotate-left helper */
uint32_t rotl32_u(uint32_t x, uint8_t n);

/* Load/store a 32-bit value in big-endian byte order; n is the word index (byte offset = 4*n) */
static inline uint32_t load_u32_be(const uint8_t* b, uint32_t n)
{
    return ((uint32_t)b[4 * n]     << 24)
         | ((uint32_t)b[4 * n + 1] << 16)
         | ((uint32_t)b[4 * n + 2] <<  8)
         |  (uint32_t)b[4 * n + 3];
}

static inline void store_u32_be(uint32_t v, uint8_t* b)
{
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)(v);
}

#endif
