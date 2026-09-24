/*
 * Minimal field set for vistrutith_d3_512s:
 * - GF(2^8)  (AES polynomial) for byte-level helpers
 * - GF(2^64) used by VOLE/zk hash scalar lane
 * - GF(2^512) used by vole_hash_512 and zk_hash_512
 */

#ifndef FIELDS_H
#define FIELDS_H

#include "macros.h"
#include <stdint.h>
#include <string.h>

typedef uint8_t bf8_t;
typedef uint64_t bf64_t;

typedef struct {
  uint64_t values[8];
} bf512_t;

#define BF_VALUE(v, i) ((v).values[i])
#define BF512C(x0, x1, x2, x3, x4, x5, x6, x7)                                                     \
  {                                                                                                  \
    { x0, x1, x2, x3, x4, x5, x6, x7 }                                                               \
  }

#define BF64_NUM_BYTES (64 / 8)
#define BF512_NUM_BYTES (512 / 8)

uint8_t bits_sq(uint8_t x);

// GF(2^8), AES polynomial x^8 + x^4 + x^3 + x + 1
static inline bf8_t bf8_load(const uint8_t* src) { return *src; }
static inline void bf8_store(uint8_t* dst, bf8_t src) { *dst = src; }
static inline bf8_t bf8_zero(void) { return 0; }
static inline bf8_t bf8_one(void) { return 1; }
static inline bf8_t bf8_add(bf8_t lhs, bf8_t rhs) { return lhs ^ rhs; }
static inline bf8_t bf8_from_bit(uint8_t bit) { return bit & 1; }
bf8_t bf8_mul(bf8_t lhs, bf8_t rhs);
bf8_t bf8_square(bf8_t lhs);
bf8_t bf8_inv(bf8_t lhs);

// GF(2^64)
static inline bf64_t bf64_load(const uint8_t* src) {
  bf64_t ret;
  memcpy(&ret, src, sizeof(ret));
  return ret;
}
static inline void bf64_store(uint8_t* dst, bf64_t src) { memcpy(dst, &src, sizeof(src)); }
static inline bf64_t bf64_zero(void) { return 0; }
static inline bf64_t bf64_one(void) { return 1; }
static inline bf64_t bf64_add(bf64_t lhs, bf64_t rhs) { return lhs ^ rhs; }
static inline bf64_t bf64_from_bit(uint8_t bit) { return bit & 1; }
bf64_t bf64_mul(bf64_t lhs, bf64_t rhs);

// GF(2^512)
static inline bf512_t bf512_load(const uint8_t* src) {
  bf512_t ret;
  memcpy(&ret, src, BF512_NUM_BYTES);
  return ret;
}
static inline void bf512_store(uint8_t* dst, bf512_t src) { memcpy(dst, &src, BF512_NUM_BYTES); }
static inline bf512_t bf512_from_bf64(bf64_t src) {
  bf512_t ret      = BF512C(0, 0, 0, 0, 0, 0, 0, 0);
  BF_VALUE(ret, 0) = src;
  return ret;
}
static inline bf512_t bf512_from_bit(uint8_t bit) { return bf512_from_bf64(bit & 1); }
static inline bf512_t bf512_zero(void) {
  const bf512_t ret = BF512C(0, 0, 0, 0, 0, 0, 0, 0);
  return ret;
}
static inline bf512_t bf512_add(bf512_t lhs, bf512_t rhs) {
  for (unsigned int i = 0; i < ARRAY_SIZE(lhs.values); ++i) {
    lhs.values[i] ^= rhs.values[i];
  }
  return lhs;
}
bf512_t bf512_mul(bf512_t lhs, bf512_t rhs);
bf512_t bf512_mul_64(bf512_t lhs, bf64_t rhs);
bf512_t bf512_mul_bit(bf512_t lhs, uint8_t rhs);
bf512_t bf512_byte_combine(const bf512_t* x);
bf512_t bf512_byte_combine_bits(uint8_t x);
void bf512_sq_bit(bf512_t* out_tag, const bf512_t* in_tag);
void bf512_sq_bit_inplace(bf512_t* tag);
bf512_t bf512_byte_combine_sq(const bf512_t* x);
bf512_t bf512_byte_combine_bits_sq(uint8_t x);
bf512_t bf512_sum_poly(const bf512_t* xs);
bf512_t bf512_sum_poly_bits(const uint8_t* xs);

#endif
