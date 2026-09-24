/*
 * This file implements basic finite field operations for various fields used in SM4th.
 */

#ifndef FIELDS_H
#define FIELDS_H

#include "macros.h"
#include "endian_compat.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t bf8_t;
typedef uint64_t bf64_t;

#define BF_VALUE(v, i) ((v).values[i])

typedef struct {
  uint64_t values[2];
} bf128_t;

typedef struct {
  uint64_t values[3];
} bf160_t;

#define BF128C(x0, x1)                                                                             \
  {                                                                                                \
    { x0, x1 }                                                                                     \
  }
#define BF160C(x0, x1, x2)                                                                         \
  {                                                                                                \
    { x0, x1, x2 }                                                                                 \
  }

#define BF128_ALIGN 8
#define BF160_ALIGN 8

#define BF128_NUM_BYTES (128 / 8)
#define BF160_NUM_BYTES (160 / 8)

// SM4 instance uses SM4 GF(2^8) routines below.

static inline bf8_t bf8_load(const uint8_t* src) {
  return *src;
}

static inline void bf8_store(uint8_t* dst, bf8_t src) {
  *dst = src;
}

static inline bf8_t bf8_zero(void) {
  return 0;
}

static inline bf8_t bf8_one(void) {
  return 1;
}

// bf8_t bf8_rand(void);

inline bf8_t bf8_add(bf8_t lhs, bf8_t rhs) {
  return lhs ^ rhs;
}

static inline bf8_t bf8_from_bit(uint8_t bit) {
  return bit & 1;
}

// GF(2^64) implementation

inline bf64_t bf64_load(const uint8_t* src) {
  bf64_t ret;
  memcpy(&ret, src, sizeof(ret));
  return ret;
}

inline void bf64_store(uint8_t* dst, bf64_t src) {
  memcpy(dst, &src, sizeof(src));
}

static inline bf64_t bf64_zero(void) {
  return 0;
}

static inline bf64_t bf64_one(void) {
  return 1;
}

// bf64_t bf64_rand(void);

static inline bf64_t bf64_add(bf64_t lhs, bf64_t rhs) {
  return lhs ^ rhs;
}

bf64_t bf64_mul(bf64_t lhs, bf64_t rhs);

static inline bf64_t bf64_from_bit(uint8_t bit) {
  return bit & 1;
}

// GF(2^128) implementation

static inline bf128_t bf128_load(const uint8_t* src) {
  bf128_t ret;
  memcpy(&ret, src, BF128_NUM_BYTES);
  return ret;
}

static inline void bf128_store(uint8_t* dst, bf128_t src) {
  memcpy(dst, &src, BF128_NUM_BYTES);
}

static inline bf128_t bf128_from_bf64(bf64_t src) {
  bf128_t ret      = BF128C(0, 0);
  BF_VALUE(ret, 0) = src;
  return ret;
}

static inline bf128_t bf128_from_bf8(bf8_t src) {
  bf128_t ret      = BF128C(0, 0);
  BF_VALUE(ret, 0) = src;
  return ret;
}

static inline bf128_t bf128_from_bit(uint8_t bit) {
  return bf128_from_bf8(bit & 1);
}

static inline bf128_t bf128_zero(void) {
  const bf128_t ret = BF128C(0, 0);
  return ret;
}

static inline bf128_t bf128_one(void) {
  const bf128_t ret = BF128C(1, 0);
  return ret;
}

static inline bf128_t bf128_add(bf128_t lhs, bf128_t rhs) {
  for (unsigned int i = 0; i != ARRAY_SIZE(lhs.values); ++i) {
    lhs.values[i] ^= rhs.values[i];
  }
  return lhs;
}

bf128_t bf128_mul(bf128_t lhs, bf128_t rhs);
bf128_t bf128_mul_64(bf128_t lhs, bf64_t rhs);
bf128_t bf128_mul_bit(bf128_t lhs, uint8_t rhs);
bf128_t bf128_sum_poly(const bf128_t* xs);
bf128_t bf128_sum_poly_bits(const uint8_t* xs);

// GF(2^160) implementation

static inline bf160_t bf160_load(const uint8_t* src) {
  bf160_t ret = BF160C(0, 0, 0);
  memcpy(&ret, src, BF160_NUM_BYTES);
  BF_VALUE(ret, 2) &= UINT64_C(0xffffffff);
  return ret;
}

static inline void bf160_store(uint8_t* dst, bf160_t src) {
  BF_VALUE(src, 2) &= UINT64_C(0xffffffff);
  memcpy(dst, &src, BF160_NUM_BYTES);
}

static inline bf160_t bf160_from_bf64(bf64_t src) {
  bf160_t ret      = BF160C(0, 0, 0);
  BF_VALUE(ret, 0) = src;
  return ret;
}

static inline bf160_t bf160_from_bf8(bf8_t src) {
  bf160_t ret      = BF160C(0, 0, 0);
  BF_VALUE(ret, 0) = src;
  return ret;
}

static inline bf160_t bf160_from_bit(uint8_t bit) {
  return bf160_from_bf8(bit & 1);
}

static inline bf160_t bf160_zero(void) {
  const bf160_t ret = BF160C(0, 0, 0);
  return ret;
}

static inline bf160_t bf160_one(void) {
  const bf160_t ret = BF160C(1, 0, 0);
  return ret;
}

static inline bf160_t bf160_add(bf160_t lhs, bf160_t rhs) {
  for (unsigned int i = 0; i != ARRAY_SIZE(lhs.values); ++i) {
    lhs.values[i] ^= rhs.values[i];
  }
  BF_VALUE(lhs, 2) &= UINT64_C(0xffffffff);
  return lhs;
}

bf160_t bf160_mul(bf160_t lhs, bf160_t rhs);
void bf160_mul_out(bf160_t* out, const bf160_t* lhs, const bf160_t* rhs);
bf160_t bf160_sqr(bf160_t lhs);
bf160_t bf160_mul_64(bf160_t lhs, bf64_t rhs);
bf160_t bf160_mul_bit(bf160_t lhs, uint8_t rhs);
bf160_t bf160_sum_poly(const bf160_t* xs);
bf160_t bf160_sum_poly_bits(const uint8_t* xs);

/* ================================================================ */
/*  SM4-specific field operations                                    */
/*  GF(2^8) with SM4 polynomial: x^8 + x^7 + x^6 + x^5 + x^4 + x^2 + 1 */
/* ================================================================ */

uint8_t bits_sq_sm4(uint8_t x);
bf8_t bf8_mul_sm4(bf8_t lhs, bf8_t rhs);
bf8_t bf8_square_sm4(bf8_t lhs);
bf8_t bf8_inv_sm4(bf8_t lhs);

bf128_t bf128_byte_combine_sm4(const bf128_t *x);
bf128_t bf128_byte_combine_bits_sm4(uint8_t x);
void bf128_sq_bit_sm4(bf128_t *out_tag, const bf128_t *in_tag);
void bf128_sq_bit_inplace_sm4(bf128_t *tag);
bf128_t bf128_byte_combine_sq_sm4(const bf128_t *x);
bf128_t bf128_byte_combine_bits_sq_sm4(uint8_t x);

bf160_t bf160_byte_combine_sm4(const bf160_t *x);
bf160_t bf160_byte_combine_bits_sm4(uint8_t x);
void bf160_sq_bit_sm4(bf160_t *out_tag, const bf160_t *in_tag);
void bf160_sq_bit_inplace_sm4(bf160_t *tag);
bf160_t bf160_byte_combine_sq_sm4(const bf160_t *x);
bf160_t bf160_byte_combine_bits_sq_sm4(uint8_t x);

/* ================================================================ */
/*  Random Number Generation for Field Elements                    */
/* ================================================================ */

/**
 * Initialize PRNG seed for field element generation
 * @param seed Random seed value (0 will use default seed)
 */
void bf_random_seed(uint64_t seed);

/**
 * Generate random field elements
 */
bf8_t bf8_random(void);
bf64_t bf64_random(void);
bf128_t bf128_random(void);
bf160_t bf160_random(void);

#endif
