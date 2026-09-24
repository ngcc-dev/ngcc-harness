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

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
#include <immintrin.h>
#endif

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

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
#if defined(__GNUC__) || defined(__clang__)
#define BF160_ALWAYS_INLINE static inline __attribute__((always_inline))
#else
#define BF160_ALWAYS_INLINE static inline
#endif

#define SM4TH_HAVE_BF160_INLINE_PCLMUL 1

typedef struct {
  __m128i v0;
  __m128i v1;
  __m128i v2;
} bf160_product_t;

#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__VPCLMULQDQ__)
#define SM4TH_HAVE_BF160_VPCLMUL 1

typedef struct {
  __m512i v0;
  __m512i v1;
  __m512i v2;
} bf160_product4_t;
#endif

BF160_ALWAYS_INLINE void bf160_inline_clmul64(uint64_t lhs, uint64_t rhs,
                                              uint64_t* lo, uint64_t* hi) {
  const __m128i a = _mm_set_epi64x(0, (long long)lhs);
  const __m128i b = _mm_set_epi64x(0, (long long)rhs);
  uint64_t out[2];
  _mm_storeu_si128((__m128i*)out, _mm_clmulepi64_si128(a, b, 0x00));
  *lo = out[0];
  *hi = out[1];
}

BF160_ALWAYS_INLINE __m128i bf160_inline_mix64(__m128i low_from_rhs,
                                               __m128i high_from_lhs) {
  return _mm_alignr_epi8(low_from_rhs, high_from_lhs, 8);
}

#if defined(SM4TH_HAVE_BF160_VPCLMUL)
BF160_ALWAYS_INLINE __m512i bf160_inline_mix64_512(__m512i low_from_rhs,
                                                   __m512i high_from_lhs) {
  return _mm512_alignr_epi8(low_from_rhs, high_from_lhs, 8);
}
#endif

BF160_ALWAYS_INLINE bf160_t bf160_inline_reduce_words(uint64_t t0, uint64_t t1,
                                                      uint64_t t2, uint64_t t3,
                                                      uint64_t t4, uint64_t t5) {
  const uint64_t h0 = (t2 >> 32) | (t3 << 32);
  const uint64_t h1 = (t3 >> 32) | (t4 << 32);
  const uint64_t h2 = ((t4 >> 32) | (t5 << 32)) & UINT64_C(0xffffffff);

  uint64_t r0 = t0 ^ h0 ^ (h0 << 2) ^ (h0 << 3) ^ (h0 << 5);
  uint64_t r1 = t1 ^ h1 ^
                ((h1 << 2) | (h0 >> 62)) ^
                ((h1 << 3) | (h0 >> 61)) ^
                ((h1 << 5) | (h0 >> 59));
  uint64_t r2 = (t2 & UINT64_C(0xffffffff)) ^ h2 ^
                ((h2 << 2) | (h1 >> 62)) ^
                ((h2 << 3) | (h1 >> 61)) ^
                ((h2 << 5) | (h1 >> 59));

  const uint64_t carry = r2 >> 32;
  r2 &= UINT64_C(0xffffffff);
  r0 ^= carry ^ (carry << 2) ^ (carry << 3) ^ (carry << 5);

  const bf160_t out = BF160C(r0, r1, r2);
  return out;
}

BF160_ALWAYS_INLINE __m128i bf160_inline_shift_left_128_2(__m128i value) {
  return _mm_xor_si128(_mm_slli_epi64(value, 2),
                       _mm_slli_si128(_mm_srli_epi64(value, 62), 8));
}

BF160_ALWAYS_INLINE __m128i bf160_inline_shift_left_128_3(__m128i value) {
  return _mm_xor_si128(_mm_slli_epi64(value, 3),
                       _mm_slli_si128(_mm_srli_epi64(value, 61), 8));
}

BF160_ALWAYS_INLINE __m128i bf160_inline_shift_left_128_5(__m128i value) {
  return _mm_xor_si128(_mm_slli_epi64(value, 5),
                       _mm_slli_si128(_mm_srli_epi64(value, 59), 8));
}

BF160_ALWAYS_INLINE bf160_t bf160_inline_reduce_product(__m128i prod0,
                                                        __m128i prod1,
                                                        __m128i prod2) {
  const __m128i high01 = _mm_alignr_epi8(prod2, prod1, 4);
  __m128i low01 = _mm_xor_si128(prod0, high01);
  low01 = _mm_xor_si128(low01, bf160_inline_shift_left_128_2(high01));
  low01 = _mm_xor_si128(low01, bf160_inline_shift_left_128_3(high01));
  low01 = _mm_xor_si128(low01, bf160_inline_shift_left_128_5(high01));

  const uint64_t high1 = (uint64_t)_mm_extract_epi64(high01, 1);
  const uint64_t high2 =
      (uint64_t)(uint32_t)_mm_cvtsi128_si64(_mm_srli_si128(prod2, 4));
  uint64_t top32 = (uint64_t)(uint32_t)_mm_cvtsi128_si64(prod1);
  top32 ^= high2;
  top32 ^= (high2 << 2) | (high1 >> 62);
  top32 ^= (high2 << 3) | (high1 >> 61);
  top32 ^= (high2 << 5) | (high1 >> 59);

  const uint64_t carry = top32 >> 32;
  top32 &= UINT64_C(0xffffffff);
  const uint64_t folded_carry = carry ^ (carry << 2) ^ (carry << 3) ^ (carry << 5);
  low01 = _mm_xor_si128(low01, _mm_cvtsi64_si128((long long)folded_carry));

  bf160_t out;
  _mm_storeu_si128((__m128i*)out.values, low01);
  out.values[2] = top32;
  return out;
}

BF160_ALWAYS_INLINE bf160_t bf160_inline_reduce_product_clmul(__m128i prod0,
                                                              __m128i prod1,
                                                              __m128i prod2) {
  const __m128i modulus = _mm_cvtsi64_si128(0x2d);
  const __m128i high01 = _mm_alignr_epi8(prod2, prod1, 4);
  const __m128i high2 =
      _mm_and_si128(_mm_srli_si128(prod2, 4),
                    _mm_cvtsi64_si128((long long)UINT64_C(0xffffffff)));

  const __m128i fold0 = _mm_clmulepi64_si128(high01, modulus, 0x00);
  const __m128i fold1 = _mm_clmulepi64_si128(high01, modulus, 0x01);
  const __m128i fold2 = _mm_clmulepi64_si128(high2, modulus, 0x00);

  __m128i low01 = _mm_xor_si128(prod0, fold0);
  low01 = _mm_xor_si128(low01, _mm_slli_si128(fold1, 8));

  uint64_t top32 = (uint64_t)(uint32_t)_mm_cvtsi128_si64(prod1);
  top32 ^= (uint64_t)_mm_extract_epi64(fold1, 1);
  top32 ^= (uint64_t)_mm_cvtsi128_si64(fold2);

  const uint64_t carry = top32 >> 32;
  top32 &= UINT64_C(0xffffffff);
  const uint64_t folded_carry = carry ^ (carry << 2) ^ (carry << 3) ^ (carry << 5);
  low01 = _mm_xor_si128(low01, _mm_cvtsi64_si128((long long)folded_carry));

  bf160_t out;
  _mm_storeu_si128((__m128i*)out.values, low01);
  out.values[2] = top32;
  return out;
}

BF160_ALWAYS_INLINE bf160_t bf160_sqr_inline(bf160_t lhs) {
  const __m128i x01 = _mm_loadu_si128((const __m128i*)lhs.values);
  const __m128i x2 =
      _mm_set_epi64x(0, (long long)(lhs.values[2] & UINT64_C(0xffffffff)));

  const __m128i prod0 = _mm_clmulepi64_si128(x01, x01, 0x00);
  const __m128i prod1 = _mm_clmulepi64_si128(x01, x01, 0x11);
  const __m128i prod2 = _mm_clmulepi64_si128(x2, x2, 0x00);
  return bf160_inline_reduce_product_clmul(prod0, prod1, prod2);
}

#if defined(SM4TH_HAVE_BF160_VPCLMUL)
BF160_ALWAYS_INLINE void bf160_inline_reduce_product4_clmul(
    bf160_t* out0, bf160_t* out1, bf160_t* out2, bf160_t* out3,
    __m512i prod0, __m512i prod1, __m512i prod2) {
  const __m512i modulus = _mm512_set1_epi64(0x2d);
  const __m512i mask32 = _mm512_set1_epi64((long long)UINT64_C(0xffffffff));
  const __m512i high01 = _mm512_alignr_epi8(prod2, prod1, 4);
  const __m512i high2 = _mm512_and_si512(_mm512_bsrli_epi128(prod2, 4), mask32);

  const __m512i fold0 = _mm512_clmulepi64_epi128(high01, modulus, 0x00);
  const __m512i fold1 = _mm512_clmulepi64_epi128(high01, modulus, 0x01);
  const __m512i fold2 = _mm512_clmulepi64_epi128(high2, modulus, 0x00);

  __m512i low01 = _mm512_xor_si512(prod0, fold0);
  low01 = _mm512_xor_si512(low01, _mm512_bslli_epi128(fold1, 8));

  __m512i top = _mm512_and_si512(prod1, mask32);
  top = _mm512_xor_si512(top, _mm512_bsrli_epi128(fold1, 8));
  top = _mm512_xor_si512(top, fold2);

  const __m512i carry = _mm512_srli_epi64(top, 32);
  const __m512i folded_carry =
      _mm512_xor_si512(_mm512_xor_si512(carry, _mm512_slli_epi64(carry, 2)),
                       _mm512_xor_si512(_mm512_slli_epi64(carry, 3),
                                        _mm512_slli_epi64(carry, 5)));
  low01 = _mm512_xor_si512(low01, folded_carry);
  top = _mm512_and_si512(top, mask32);

  uint64_t low_words[8];
  uint64_t top_words[8];
  _mm512_storeu_si512((void*)low_words, low01);
  _mm512_storeu_si512((void*)top_words, top);

  out0->values[0] = low_words[0];
  out0->values[1] = low_words[1];
  out0->values[2] = top_words[0] & UINT64_C(0xffffffff);
  out1->values[0] = low_words[2];
  out1->values[1] = low_words[3];
  out1->values[2] = top_words[2] & UINT64_C(0xffffffff);
  out2->values[0] = low_words[4];
  out2->values[1] = low_words[5];
  out2->values[2] = top_words[4] & UINT64_C(0xffffffff);
  out3->values[0] = low_words[6];
  out3->values[1] = low_words[7];
  out3->values[2] = top_words[6] & UINT64_C(0xffffffff);
}
#endif

BF160_ALWAYS_INLINE __m128i bf160_inline_xor_low_high64(__m128i value) {
  return _mm_xor_si128(value, _mm_srli_si128(value, 8));
}

BF160_ALWAYS_INLINE void bf160_mul_out_karat_inline(bf160_t* out,
                                                    const bf160_t* lhs,
                                                    const bf160_t* rhs) {
  const __m128i x01 = _mm_loadu_si128((const __m128i*)lhs->values);
  const __m128i y01 = _mm_loadu_si128((const __m128i*)rhs->values);
  const __m128i x2 = _mm_set_epi64x(0, (long long)(lhs->values[2] & UINT64_C(0xffffffff)));
  const __m128i y2 = _mm_set_epi64x(0, (long long)(rhs->values[2] & UINT64_C(0xffffffff)));

  const __m128i m00 = _mm_clmulepi64_si128(x01, y01, 0x00);
  const __m128i m11 = _mm_clmulepi64_si128(x01, y01, 0x11);
  const __m128i m22 = _mm_clmulepi64_si128(x2, y2, 0x00);

  const __m128i x01_sum = bf160_inline_xor_low_high64(x01);
  const __m128i y01_sum = bf160_inline_xor_low_high64(y01);
  const __m128i m01 = _mm_clmulepi64_si128(x01_sum, y01_sum, 0x00);

  const __m128i x12 = bf160_inline_mix64(x2, x01);
  const __m128i y12 = bf160_inline_mix64(y2, y01);
  const __m128i x12_sum = bf160_inline_xor_low_high64(x12);
  const __m128i y12_sum = bf160_inline_xor_low_high64(y12);
  const __m128i m12 = _mm_clmulepi64_si128(x12_sum, y12_sum, 0x00);

  const __m128i x02_sum = _mm_xor_si128(x01, x2);
  const __m128i y02_sum = _mm_xor_si128(y01, y2);
  const __m128i m02 = _mm_clmulepi64_si128(x02_sum, y02_sum, 0x00);

  const __m128i c1 = _mm_xor_si128(_mm_xor_si128(m01, m00), m11);
  const __m128i c2 = _mm_xor_si128(_mm_xor_si128(_mm_xor_si128(m02, m00), m11), m22);
  const __m128i c3 = _mm_xor_si128(_mm_xor_si128(m12, m11), m22);
  const __m128i prod0 = _mm_xor_si128(m00, _mm_slli_si128(c1, 8));
  const __m128i prod1 =
      _mm_xor_si128(_mm_xor_si128(_mm_srli_si128(c1, 8), c2), _mm_slli_si128(c3, 8));
  const __m128i prod2 = _mm_xor_si128(_mm_srli_si128(c3, 8), m22);

  *out = bf160_inline_reduce_product(prod0, prod1, prod2);
}

BF160_ALWAYS_INLINE bf160_product_t bf160_mul_product_faest192_inline(const bf160_t* lhs,
                                                                      const bf160_t* rhs) {
  const __m128i x0 = _mm_loadu_si128((const __m128i*)lhs->values);
  const __m128i y0 = _mm_loadu_si128((const __m128i*)rhs->values);
  const __m128i x2 = _mm_set_epi64x(0, (long long)(lhs->values[2] & UINT64_C(0xffffffff)));
  const __m128i y2 = _mm_set_epi64x(0, (long long)(rhs->values[2] & UINT64_C(0xffffffff)));

  const __m128i xlow_ylow = _mm_clmulepi64_si128(x0, y0, 0x00);
  const __m128i xhigh_yhigh = _mm_clmulepi64_si128(x2, y2, 0x00);

  const __m128i x1_cat_y0_plus_y2 = bf160_inline_mix64(_mm_xor_si128(y0, y2), x0);
  const __m128i xsum = _mm_xor_si128(_mm_xor_si128(x0, x2), x1_cat_y0_plus_y2);
  const __m128i ysum = _mm_xor_si128(y0, x1_cat_y0_plus_y2);
  const __m128i xsum_ysum = _mm_clmulepi64_si128(xsum, ysum, 0x10);

#if defined(__AVX2__)
  const __m128i x2_broadcast = _mm_broadcastq_epi64(x2);
  const __m128i y2_broadcast = _mm_broadcastq_epi64(y2);
#else
  const __m128i x2_broadcast = _mm_unpacklo_epi64(x2, x2);
  const __m128i y2_broadcast = _mm_unpacklo_epi64(y2, y2);
#endif
  const __m128i xa = _mm_xor_si128(x0, x2_broadcast);
  const __m128i ya = _mm_xor_si128(y0, y2_broadcast);

  const __m128i xya00 = _mm_clmulepi64_si128(xa, ya, 0x00);
  const __m128i xya11 = _mm_clmulepi64_si128(xa, ya, 0x11);
  const __m128i x1_cat_y0 = bf160_inline_mix64(y0, x0);
  const __m128i xsum2 = _mm_xor_si128(x0, x1_cat_y0);
  const __m128i ysum2 = _mm_xor_si128(y0, x1_cat_y0);
  const __m128i xya_sum = _mm_clmulepi64_si128(xsum2, ysum2, 0x10);

  const __m128i xya0 = _mm_xor_si128(xya00, xya11);
  const __m128i xya1 = _mm_xor_si128(xya00, xya_sum);
  const __m128i xya0_plus_xsum_ysum = _mm_xor_si128(xya0, xsum_ysum);

  const __m128i interp0 = xlow_ylow;
  const __m128i interp1 = _mm_xor_si128(xya0_plus_xsum_ysum, xhigh_yhigh);
  const __m128i interp2 = _mm_xor_si128(xya0_plus_xsum_ysum, xya1);
  const __m128i interp3 = _mm_xor_si128(_mm_xor_si128(xlow_ylow, xsum_ysum), xya1);
  const __m128i interp4 = xhigh_yhigh;

  const bf160_product_t product = {
      _mm_xor_si128(interp0, _mm_slli_si128(interp1, 8)),
      _mm_xor_si128(interp2, bf160_inline_mix64(interp3, interp1)),
      _mm_xor_si128(interp4, _mm_srli_si128(interp3, 8)),
  };
  return product;
}

#if defined(SM4TH_HAVE_BF160_VPCLMUL)
BF160_ALWAYS_INLINE __m512i bf160_load4_low128(const bf160_t* x0, const bf160_t* x1,
                                               const bf160_t* x2, const bf160_t* x3) {
  return _mm512_set_epi64((long long)x3->values[1], (long long)x3->values[0],
                          (long long)x2->values[1], (long long)x2->values[0],
                          (long long)x1->values[1], (long long)x1->values[0],
                          (long long)x0->values[1], (long long)x0->values[0]);
}

BF160_ALWAYS_INLINE __m512i bf160_load4_high32(const bf160_t* x0, const bf160_t* x1,
                                               const bf160_t* x2, const bf160_t* x3) {
  return _mm512_set_epi64(0, (long long)(x3->values[2] & UINT64_C(0xffffffff)), 0,
                          (long long)(x2->values[2] & UINT64_C(0xffffffff)), 0,
                          (long long)(x1->values[2] & UINT64_C(0xffffffff)), 0,
                          (long long)(x0->values[2] & UINT64_C(0xffffffff)));
}

BF160_ALWAYS_INLINE bf160_product4_t bf160_mul_product4_faest192_inline(
    const bf160_t* lhs0, const bf160_t* lhs1, const bf160_t* lhs2, const bf160_t* lhs3,
    const bf160_t* rhs0, const bf160_t* rhs1, const bf160_t* rhs2, const bf160_t* rhs3) {
  const __m512i x0 = bf160_load4_low128(lhs0, lhs1, lhs2, lhs3);
  const __m512i y0 = bf160_load4_low128(rhs0, rhs1, rhs2, rhs3);
  const __m512i x2 = bf160_load4_high32(lhs0, lhs1, lhs2, lhs3);
  const __m512i y2 = bf160_load4_high32(rhs0, rhs1, rhs2, rhs3);

  const __m512i xlow_ylow = _mm512_clmulepi64_epi128(x0, y0, 0x00);
  const __m512i xhigh_yhigh = _mm512_clmulepi64_epi128(x2, y2, 0x00);

  const __m512i x1_cat_y0_plus_y2 =
      bf160_inline_mix64_512(_mm512_xor_si512(y0, y2), x0);
  const __m512i xsum = _mm512_xor_si512(_mm512_xor_si512(x0, x2), x1_cat_y0_plus_y2);
  const __m512i ysum = _mm512_xor_si512(y0, x1_cat_y0_plus_y2);
  const __m512i xsum_ysum = _mm512_clmulepi64_epi128(xsum, ysum, 0x10);

  const __m512i x2_broadcast = _mm512_unpacklo_epi64(x2, x2);
  const __m512i y2_broadcast = _mm512_unpacklo_epi64(y2, y2);
  const __m512i xa = _mm512_xor_si512(x0, x2_broadcast);
  const __m512i ya = _mm512_xor_si512(y0, y2_broadcast);

  const __m512i xya00 = _mm512_clmulepi64_epi128(xa, ya, 0x00);
  const __m512i xya11 = _mm512_clmulepi64_epi128(xa, ya, 0x11);
  const __m512i x1_cat_y0 = bf160_inline_mix64_512(y0, x0);
  const __m512i xsum2 = _mm512_xor_si512(x0, x1_cat_y0);
  const __m512i ysum2 = _mm512_xor_si512(y0, x1_cat_y0);
  const __m512i xya_sum = _mm512_clmulepi64_epi128(xsum2, ysum2, 0x10);

  const __m512i xya0 = _mm512_xor_si512(xya00, xya11);
  const __m512i xya1 = _mm512_xor_si512(xya00, xya_sum);
  const __m512i xya0_plus_xsum_ysum = _mm512_xor_si512(xya0, xsum_ysum);

  const __m512i interp0 = xlow_ylow;
  const __m512i interp1 = _mm512_xor_si512(xya0_plus_xsum_ysum, xhigh_yhigh);
  const __m512i interp2 = _mm512_xor_si512(xya0_plus_xsum_ysum, xya1);
  const __m512i interp3 =
      _mm512_xor_si512(_mm512_xor_si512(xlow_ylow, xsum_ysum), xya1);
  const __m512i interp4 = xhigh_yhigh;

  const bf160_product4_t product = {
      _mm512_xor_si512(interp0, _mm512_bslli_epi128(interp1, 8)),
      _mm512_xor_si512(interp2, bf160_inline_mix64_512(interp3, interp1)),
      _mm512_xor_si512(interp4, _mm512_bsrli_epi128(interp3, 8)),
  };
  return product;
}
#endif

BF160_ALWAYS_INLINE void bf160_mul_out_faest192_inline(bf160_t* out,
                                                       const bf160_t* lhs,
                                                       const bf160_t* rhs) {
  const bf160_product_t product = bf160_mul_product_faest192_inline(lhs, rhs);
  *out = bf160_inline_reduce_product(product.v0, product.v1, product.v2);
}

BF160_ALWAYS_INLINE void bf160_mul_out_faest192_clmulred_inline(bf160_t* out,
                                                                const bf160_t* lhs,
                                                                const bf160_t* rhs) {
  const bf160_product_t product = bf160_mul_product_faest192_inline(lhs, rhs);
  *out = bf160_inline_reduce_product_clmul(product.v0, product.v1, product.v2);
}

BF160_ALWAYS_INLINE void bf160_mul4_faest192_clmulred_inline(
    bf160_t* out0, bf160_t* out1, bf160_t* out2, bf160_t* out3,
    const bf160_t* lhs0, const bf160_t* lhs1, const bf160_t* lhs2, const bf160_t* lhs3,
    const bf160_t* rhs0, const bf160_t* rhs1, const bf160_t* rhs2, const bf160_t* rhs3) {
  const bf160_product_t product0 = bf160_mul_product_faest192_inline(lhs0, rhs0);
  const bf160_product_t product1 = bf160_mul_product_faest192_inline(lhs1, rhs1);
  const bf160_product_t product2 = bf160_mul_product_faest192_inline(lhs2, rhs2);
  const bf160_product_t product3 = bf160_mul_product_faest192_inline(lhs3, rhs3);

  *out0 = bf160_inline_reduce_product_clmul(product0.v0, product0.v1, product0.v2);
  *out1 = bf160_inline_reduce_product_clmul(product1.v0, product1.v1, product1.v2);
  *out2 = bf160_inline_reduce_product_clmul(product2.v0, product2.v1, product2.v2);
  *out3 = bf160_inline_reduce_product_clmul(product3.v0, product3.v1, product3.v2);
}

#if defined(SM4TH_HAVE_BF160_VPCLMUL)
BF160_ALWAYS_INLINE void bf160_mul4_faest192_clmulred_vpclmul_inline(
    bf160_t* out0, bf160_t* out1, bf160_t* out2, bf160_t* out3,
    const bf160_t* lhs0, const bf160_t* lhs1, const bf160_t* lhs2, const bf160_t* lhs3,
    const bf160_t* rhs0, const bf160_t* rhs1, const bf160_t* rhs2, const bf160_t* rhs3) {
  const bf160_product4_t product =
      bf160_mul_product4_faest192_inline(lhs0, lhs1, lhs2, lhs3, rhs0, rhs1, rhs2, rhs3);
  bf160_inline_reduce_product4_clmul(out0, out1, out2, out3, product.v0, product.v1,
                                     product.v2);
}
#endif

BF160_ALWAYS_INLINE void bf160_mul_out_toom_inline(bf160_t* out,
                                                   const bf160_t* lhs,
                                                   const bf160_t* rhs) {
  const __m128i x0 = _mm_loadu_si128((const __m128i*)lhs->values);
  const __m128i y0 = _mm_loadu_si128((const __m128i*)rhs->values);
  const __m128i x2 = _mm_set_epi64x(0, (long long)(lhs->values[2] & UINT64_C(0xffffffff)));
  const __m128i y2 = _mm_set_epi64x(0, (long long)(rhs->values[2] & UINT64_C(0xffffffff)));

  const __m128i xlow_ylow = _mm_clmulepi64_si128(x0, y0, 0x00);
  const __m128i xhigh_yhigh = _mm_clmulepi64_si128(x2, y2, 0x00);

  const __m128i x1_cat_y0_plus_y2 = bf160_inline_mix64(_mm_xor_si128(y0, y2), x0);
  const __m128i xsum = _mm_xor_si128(_mm_xor_si128(x0, x2), x1_cat_y0_plus_y2);
  const __m128i ysum = _mm_xor_si128(y0, x1_cat_y0_plus_y2);
  const __m128i xsum_ysum = _mm_clmulepi64_si128(xsum, ysum, 0x10);

  const __m128i x2_broadcast = _mm_unpacklo_epi64(x2, x2);
  const __m128i y2_broadcast = _mm_unpacklo_epi64(y2, y2);
  const __m128i xa = _mm_xor_si128(x0, x2_broadcast);
  const __m128i ya = _mm_xor_si128(y0, y2_broadcast);

  const __m128i xya00 = _mm_clmulepi64_si128(xa, ya, 0x00);
  const __m128i xya11 = _mm_clmulepi64_si128(xa, ya, 0x11);
  const __m128i x1_cat_y0 = bf160_inline_mix64(y0, x0);
  const __m128i xsum2 = _mm_xor_si128(x0, x1_cat_y0);
  const __m128i ysum2 = _mm_xor_si128(y0, x1_cat_y0);
  const __m128i xya_sum = _mm_clmulepi64_si128(xsum2, ysum2, 0x10);

  const __m128i xya0 = _mm_xor_si128(xya00, xya11);
  const __m128i xya1 = _mm_xor_si128(xya00, xya_sum);
  const __m128i xya0_plus_xsum_ysum = _mm_xor_si128(xya0, xsum_ysum);

  const __m128i interp0 = xlow_ylow;
  const __m128i interp1 = _mm_xor_si128(xya0_plus_xsum_ysum, xhigh_yhigh);
  const __m128i interp2 = _mm_xor_si128(xya0_plus_xsum_ysum, xya1);
  const __m128i interp3 = _mm_xor_si128(_mm_xor_si128(xlow_ylow, xsum_ysum), xya1);
  const __m128i interp4 = xhigh_yhigh;

  const __m128i prod0 = _mm_xor_si128(interp0, _mm_slli_si128(interp1, 8));
  const __m128i prod1 = _mm_xor_si128(interp2, bf160_inline_mix64(interp3, interp1));
  const __m128i prod2 = _mm_xor_si128(interp4, _mm_srli_si128(interp3, 8));

  *out = bf160_inline_reduce_product(prod0, prod1, prod2);
}

BF160_ALWAYS_INLINE void bf160_mul_out_inline(bf160_t* out,
                                              const bf160_t* lhs,
                                              const bf160_t* rhs) {
  bf160_mul_out_faest192_clmulred_inline(out, lhs, rhs);
}

BF160_ALWAYS_INLINE bf160_t bf160_mul_inline(bf160_t lhs, bf160_t rhs) {
  bf160_t out;
  bf160_mul_out_inline(&out, &lhs, &rhs);
  return out;
}

#undef BF160_ALWAYS_INLINE
#endif

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
