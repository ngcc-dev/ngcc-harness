/*
 *  Field implementation used by sm4th_d3_128s_tight.
 *  Keeps only domains exercised by this instance.
 *  GF(2^8) uses the SM4 polynomial x^8 + x^7 + x^6 + x^5 + x^4 + x^2 + 1
 */

#include "fields.h"
#include "utils.h"

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
#include <wmmintrin.h>
#endif

// GF(2^8) with X^8 + X^4 + X^3 + X^1 + 1
#define bf8_modulus (UINT8_C((1 << 4) | (1 << 3) | (1 << 1) | 1))
// GF(2^64) with X^64 + X^4 + X^3 + X^1 + 1
#define bf64_modulus (UINT64_C((1 << 4) | (1 << 3) | (1 << 1) | 1))
// GF(2^128) with X^128 + X^7 + X^2 + X^1 + 1
#define bf128_modulus (UINT64_C((1 << 7) | (1 << 2) | (1 << 1) | 1))
// GF(2^160) with X^160 + X^5 + X^3 + X^2 + 1
#define bf160_modulus (UINT64_C((1 << 5) | (1 << 3) | (1 << 2) | 1))

#define U64C(x0, x1, x2, x3, x4, x5, x6, x7)                                                       \
  ((UINT64_C(x7) << 56) | (UINT64_C(x6) << 48) | (UINT64_C(x5) << 40) | (UINT64_C(x4) << 32) |     \
   (UINT64_C(x3) << 24) | (UINT64_C(x2) << 16) | (UINT64_C(x1) << 8) | UINT64_C(x0))

uint8_t bits_sq(uint8_t x) {
  uint8_t res = set_bit(get_bit(x, 0) ^ get_bit(x, 4) ^ get_bit(x, 6), 0);
  res |= set_bit(get_bit(x, 4) ^ get_bit(x, 6) ^ get_bit(x, 7), 1);
  res |= set_bit(get_bit(x, 1) ^ get_bit(x, 5), 2);
  res |= set_bit(get_bit(x, 4) ^ get_bit(x, 5) ^ get_bit(x, 6) ^ get_bit(x, 7), 3);
  res |= set_bit(get_bit(x, 2) ^ get_bit(x, 4) ^ get_bit(x, 7), 4);
  res |= set_bit(get_bit(x, 5) ^ get_bit(x, 6), 5);
  res |= set_bit(get_bit(x, 3) ^ get_bit(x, 5), 6);
  res |= set_bit(get_bit(x, 6) ^ get_bit(x, 7), 7);
  return res;
}

// GF(2^64) implementation

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
bf64_t bf64_mul(bf64_t lhs, bf64_t rhs) {
  const __m128i a = _mm_set_epi64x(0, (long long)lhs);
  const __m128i b = _mm_set_epi64x(0, (long long)rhs);
  const __m128i r = _mm_set_epi64x(0, (long long)bf64_modulus);

  uint64_t p[2];
  _mm_storeu_si128((__m128i*)p, _mm_clmulepi64_si128(a, b, 0x00));
  uint64_t lo = p[0];
  uint64_t hi = p[1];

  uint64_t q[2];
  _mm_storeu_si128((__m128i*)q,
                   _mm_clmulepi64_si128(_mm_set_epi64x(0, (long long)hi), r, 0x00));
  lo ^= q[0];

  /* q[1] has degree <= 3; one extra fold is sufficient. */
  if (q[1]) {
    uint64_t q2[2];
    _mm_storeu_si128((__m128i*)q2,
                     _mm_clmulepi64_si128(_mm_set_epi64x(0, (long long)q[1]), r, 0x00));
    lo ^= q2[0];
  }
  return lo;
}
#else
bf64_t bf64_mul(bf64_t lhs, bf64_t rhs) {
  bf64_t result = (-(rhs & 1)) & lhs;
  for (unsigned int idx = 1; idx != 64; ++idx) {
    const uint64_t mask = -((lhs >> 63) & 1);
    lhs                 = (lhs << 1) ^ (mask & bf64_modulus);
    result ^= (-((rhs >> idx) & 1)) & lhs;
  }
  return result;
}
#endif

#define bf64_bit_to_mask(value, bit) -((((uint64_t)(value)) >> (bit)) & 1)

// GF(2^128) implementation

static const bf128_t bf128_alpha[7] = {
    BF128C(U64C(0x0d, 0xce, 0x60, 0x55, 0xac, 0xe8, 0x3f, 0xa1),
           U64C(0x1c, 0x9a, 0x97, 0xa9, 0x55, 0x85, 0x3d, 0x05)),
    BF128C(U64C(0xe1, 0xae, 0x88, 0x34, 0xca, 0x59, 0x77, 0xec),
           U64C(0x84, 0xbb, 0xbf, 0x9c, 0x43, 0xb7, 0xf4, 0x4c)),
    BF128C(U64C(0xa8, 0x46, 0x39, 0x36, 0xae, 0x02, 0xcf, 0xbf),
           U64C(0xc6, 0xd2, 0x51, 0x7d, 0x4f, 0x60, 0xad, 0x35)),
    BF128C(U64C(0x49, 0x98, 0x2e, 0x3c, 0x48, 0x30, 0x83, 0x6b),
           U64C(0xfe, 0x22, 0xa2, 0x40, 0x46, 0x36, 0xcb, 0x0d)),
    BF128C(U64C(0xb4, 0x82, 0x1b, 0x7b, 0x27, 0x49, 0x2b, 0x25),
           U64C(0xa5, 0xde, 0x88, 0x1a, 0xe1, 0x10, 0x98, 0x54)),
    BF128C(U64C(0x22, 0xff, 0x21, 0x25, 0xef, 0xf2, 0x2b, 0xc7),
           U64C(0x75, 0x1f, 0x0c, 0x6c, 0x68, 0xa5, 0x81, 0xd6)),
    BF128C(U64C(0xbc, 0xf9, 0x36, 0xe1, 0x94, 0x8e, 0x7a, 0x7a),
           U64C(0xe0, 0x8f, 0xb7, 0x4f, 0x1a, 0x31, 0x50, 0x09)),
};

/* bf128_byte_combine: out = x[0] + x[1]*alpha[0] + ... + x[7]*alpha[6]
 *
 * PCLMUL path: all 7 products x[i]*alpha[i-1] are issued as independent
 * 128×128→256-bit carryless multiplications with no inter-product dependency,
 * letting the CPU's OOO engine overlap the ~7-cycle PCLMUL latency across all
 * seven pairs simultaneously.  Partial products (lo/hi halves) are XORed
 * together and the combined degree-255 polynomial is reduced mod p in a single
 * two-step pass rather than reducing after every multiplication.
 *
 * Scalar path: unchanged loop over bf128_mul.
 */
#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
/* Helper: 128×128 → (lo128, hi128) without reduction */
static inline void bf128_clmul_lo_hi(const bf128_t* a, const bf128_t* b,
                                     __m128i* lo, __m128i* hi) {
  __m128i va = _mm_loadu_si128((const __m128i*)a->values);
  __m128i vb = _mm_loadu_si128((const __m128i*)b->values);
  __m128i r00 = _mm_clmulepi64_si128(va, vb, 0x00);
  __m128i r11 = _mm_clmulepi64_si128(va, vb, 0x11);
  __m128i mid = _mm_xor_si128(_mm_clmulepi64_si128(va, vb, 0x01),
                               _mm_clmulepi64_si128(va, vb, 0x10));
  *lo = _mm_xor_si128(r00, _mm_slli_si128(mid, 8));
  *hi = _mm_xor_si128(r11, _mm_srli_si128(mid, 8));
}

static inline bf128_t bf128_byte_combine_pclmul(const bf128_t* x, const bf128_t* alpha) {
  /* Issue all 7 products independently so the CPU can pipeline them */
  __m128i lo[7], hi[7];
  for (unsigned int i = 0; i < 7; ++i) {
    bf128_clmul_lo_hi(&x[i + 1], &alpha[i], &lo[i], &hi[i]);
  }

  /* XOR all lo halves together, then add x[0] */
  __m128i acc_lo = lo[0];
  __m128i acc_hi = hi[0];
  for (unsigned int i = 1; i < 7; ++i) {
    acc_lo = _mm_xor_si128(acc_lo, lo[i]);
    acc_hi = _mm_xor_si128(acc_hi, hi[i]);
  }
  /* Add x[0] (degree-0 term needs no multiplication) */
  __m128i x0 = _mm_loadu_si128((const __m128i*)x[0].values);
  acc_lo = _mm_xor_si128(acc_lo, x0);

  /* Single reduction of the combined degree-255 polynomial */
  __m128i R_poly = _mm_set_epi64x(0, bf128_modulus);
  __m128i p0 = _mm_clmulepi64_si128(acc_hi, R_poly, 0x00);
  __m128i p1 = _mm_clmulepi64_si128(acc_hi, R_poly, 0x01);
  acc_lo = _mm_xor_si128(acc_lo, p0);
  acc_lo = _mm_xor_si128(acc_lo, _mm_slli_si128(p1, 8));
  __m128i t2 = _mm_srli_si128(p1, 8);
  __m128i p2 = _mm_clmulepi64_si128(t2, R_poly, 0x00);
  acc_lo = _mm_xor_si128(acc_lo, p2);

  bf128_t out;
  _mm_storeu_si128((__m128i*)out.values, acc_lo);
  return out;
}

bf128_t bf128_byte_combine(const bf128_t* x) {
  return bf128_byte_combine_pclmul(x, bf128_alpha);
}
#else
bf128_t bf128_byte_combine(const bf128_t* x) {
  bf128_t bf_out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul(x[i], bf128_alpha[i - 1]));
  }
  return bf_out;
}
#endif

void bf128_sq_bit(bf128_t* out_tag, const bf128_t* in_tag) {
  out_tag[0] = bf128_add(in_tag[0], bf128_add(in_tag[4], in_tag[6]));
  out_tag[1] = bf128_add(in_tag[4], bf128_add(in_tag[6], in_tag[7]));
  out_tag[2] = bf128_add(in_tag[1], in_tag[5]);
  out_tag[3] = bf128_add(bf128_add(in_tag[4], in_tag[5]), bf128_add(in_tag[6], in_tag[7]));
  out_tag[4] = bf128_add(in_tag[2], bf128_add(in_tag[4], in_tag[7]));
  out_tag[5] = bf128_add(in_tag[5], in_tag[6]);
  out_tag[6] = bf128_add(in_tag[3], in_tag[5]);
  out_tag[7] = bf128_add(in_tag[6], in_tag[7]);
}

bf128_t bf128_byte_combine_bits(uint8_t x) {
  bf128_t bf_out = bf128_from_bit(get_bit(x, 0));
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul_bit(bf128_alpha[i - 1], get_bit(x, i)));
  }
  return bf_out;
}

static inline bf128_t bf128_and_64(bf128_t lhs, bf64_t rhs) {
  for (unsigned int i = 0; i != ARRAY_SIZE(lhs.values); ++i) {
    lhs.values[i] &= rhs;
  }
  return lhs;
}

static inline bf128_t bf128_shift_left_1(bf128_t value) {
  value.values[1] = (value.values[1] << 1) | (value.values[0] >> 63);
  value.values[0] = value.values[0] << 1;
  return value;
}

static inline uint64_t bf128_bit_to_uint64_mask(bf128_t value, unsigned int bit) {
  const unsigned int byte_idx = bit / 64;
  const unsigned int bit_idx  = bit % 64;

  return -((BF_VALUE(value, byte_idx) >> bit_idx) & 1);
}

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
#include <wmmintrin.h>

static inline void xor_m128i_to_array(uint64_t* C, int idx, __m128i r) {
    uint64_t tmp[2];
    _mm_storeu_si128((__m128i*)tmp, r);
    C[idx] ^= tmp[0];
    C[idx+1] ^= tmp[1];
}

bf128_t bf128_mul(bf128_t lhs, bf128_t rhs) {
    __m128i a = _mm_loadu_si128((const __m128i*)&lhs.values[0]);
    __m128i b = _mm_loadu_si128((const __m128i*)&rhs.values[0]);

    __m128i r00 = _mm_clmulepi64_si128(a, b, 0x00);
    __m128i r01 = _mm_clmulepi64_si128(a, b, 0x01);
    __m128i r10 = _mm_clmulepi64_si128(a, b, 0x10);
    __m128i r11 = _mm_clmulepi64_si128(a, b, 0x11);

    __m128i t   = _mm_xor_si128(r01, r10);
    __m128i c01 = _mm_xor_si128(r00, _mm_slli_si128(t, 8));
    __m128i c23 = _mm_xor_si128(r11, _mm_srli_si128(t, 8));

    __m128i R_poly = _mm_set_epi64x(0, bf128_modulus);
    __m128i p0 = _mm_clmulepi64_si128(c23, R_poly, 0x00);
    __m128i p1 = _mm_clmulepi64_si128(c23, R_poly, 0x01);

    c01 = _mm_xor_si128(c01, p0);
    c01 = _mm_xor_si128(c01, _mm_slli_si128(p1, 8));
    __m128i t2 = _mm_srli_si128(p1, 8);
    __m128i p2 = _mm_clmulepi64_si128(t2, R_poly, 0x00);
    c01 = _mm_xor_si128(c01, p2);

    bf128_t out;
    _mm_storeu_si128((__m128i*)&out.values[0], c01);
    return out;
}

bf128_t bf128_mul_64(bf128_t lhs, bf64_t rhs) {
    __m128i a = _mm_loadu_si128((const __m128i*)&lhs.values[0]);
    __m128i b = _mm_set_epi64x(0, rhs);

    __m128i r00 = _mm_clmulepi64_si128(a, b, 0x00);
    __m128i r01 = _mm_clmulepi64_si128(a, b, 0x01);

    __m128i c01 = _mm_xor_si128(r00, _mm_slli_si128(r01, 8));
    __m128i c23 = _mm_srli_si128(r01, 8);

    __m128i R_poly = _mm_set_epi64x(0, bf128_modulus);
    __m128i p0 = _mm_clmulepi64_si128(c23, R_poly, 0x00);
    c01 = _mm_xor_si128(c01, p0);

    bf128_t out;
    _mm_storeu_si128((__m128i*)&out.values[0], c01);
    return out;
}
#else
bf128_t bf128_mul(bf128_t lhs, bf128_t rhs) {
  bf128_t result = bf128_and_64(lhs, bf128_bit_to_uint64_mask(rhs, 0));
  for (unsigned int idx = 1; idx != 128; ++idx) {
    const uint64_t mask = bf128_bit_to_uint64_mask(lhs, 128 - 1);
    lhs                 = bf128_shift_left_1(lhs);
    BF_VALUE(lhs, 0) ^= (mask & bf128_modulus);

    result = bf128_add(result, bf128_and_64(lhs, bf128_bit_to_uint64_mask(rhs, idx)));
  }
  return result;
}

bf128_t bf128_mul_64(bf128_t lhs, bf64_t rhs) {
  bf128_t result = bf128_and_64(lhs, bf64_bit_to_mask(rhs, 0));
  for (unsigned int idx = 1; idx != 64; ++idx) {
    const uint64_t mask = bf128_bit_to_uint64_mask(lhs, 128 - 1);
    lhs                 = bf128_shift_left_1(lhs);
    BF_VALUE(lhs, 0) ^= (mask & bf128_modulus);

    result = bf128_add(result, bf128_and_64(lhs, bf64_bit_to_mask(rhs, idx)));
  }
  return result;
}
#endif

bf128_t bf128_mul_bit(bf128_t lhs, uint8_t rhs) {
  return bf128_and_64(lhs, -((uint64_t)rhs & 1));
}

static inline bf128_t bf128_dbl(bf128_t lhs) {
  uint64_t mask = bf128_bit_to_uint64_mask(lhs, 128 - 1);
  lhs           = bf128_shift_left_1(lhs);
  BF_VALUE(lhs, 0) ^= (mask & bf128_modulus);

  return lhs;
}

bf128_t bf128_sum_poly(const bf128_t* xs) {
  bf128_t ret = xs[128 - 1];
  for (size_t i = 1; i < 128; ++i) {
    ret = bf128_add(bf128_dbl(ret), xs[128 - 1 - i]);
  }
  return ret;
}

// from 128 bits into a field element, only use the first 128 bits of xs
bf128_t bf128_sum_poly_bits(const uint8_t* xs) {
  bf128_t ret = bf128_from_bit(ptr_get_bit(xs, 128 - 1));
  for (size_t i = 1; i < 128; ++i) {
    ret = bf128_add(bf128_dbl(ret), bf128_from_bit(ptr_get_bit(xs, 128 - 1 - i)));
  }
  return ret;
}

// GF(2^160) implementation

static inline bf160_t bf160_and_64(bf160_t lhs, bf64_t rhs) {
  for (unsigned int i = 0; i != ARRAY_SIZE(lhs.values); ++i) {
    lhs.values[i] &= rhs;
  }
  BF_VALUE(lhs, 2) &= UINT64_C(0xffffffff);
  return lhs;
}

static inline bf160_t bf160_shift_left_1(bf160_t value) {
  value.values[2] = ((value.values[2] << 1) | (value.values[1] >> 63)) & UINT64_C(0xffffffff);
  value.values[1] = (value.values[1] << 1) | (value.values[0] >> 63);
  value.values[0] = value.values[0] << 1;
  return value;
}

static inline uint64_t bf160_bit_to_uint64_mask(bf160_t value, unsigned int bit) {
  const unsigned int byte_idx = bit / 64;
  const unsigned int bit_idx  = bit % 64;

  return -((BF_VALUE(value, byte_idx) >> bit_idx) & 1);
}

static inline bf160_t bf160_dbl(bf160_t lhs) {
  const uint64_t mask = bf160_bit_to_uint64_mask(lhs, 160 - 1);
  lhs                 = bf160_shift_left_1(lhs);
  BF_VALUE(lhs, 0) ^= mask & bf160_modulus;
  return lhs;
}

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
static inline void bf160_clmul64(uint64_t lhs, uint64_t rhs, uint64_t* lo, uint64_t* hi) {
  const __m128i a = _mm_set_epi64x(0, (long long)lhs);
  const __m128i b = _mm_set_epi64x(0, (long long)rhs);
  uint64_t out[2];
  _mm_storeu_si128((__m128i*)out, _mm_clmulepi64_si128(a, b, 0x00));
  *lo = out[0];
  *hi = out[1];
}

static inline void bf160_xor_pair(uint64_t* out, unsigned int offset,
                                  uint64_t lo, uint64_t hi) {
  out[offset] ^= lo;
  out[offset + 1] ^= hi;
}

static inline bf160_t bf160_reduce_words(uint64_t t0, uint64_t t1, uint64_t t2,
                                         uint64_t t3, uint64_t t4, uint64_t t5) {
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

  bf160_t out = BF160C(r0, r1, r2);
  return out;
}

void bf160_mul_out(bf160_t* out, const bf160_t* lhs, const bf160_t* rhs) {
  bf160_mul_out_inline(out, lhs, rhs);
}

bf160_t bf160_mul(bf160_t lhs, bf160_t rhs) {
  return bf160_mul_inline(lhs, rhs);
}

bf160_t bf160_sqr(bf160_t lhs) {
  return bf160_sqr_inline(lhs);
}

bf160_t bf160_mul_64(bf160_t lhs, bf64_t rhs) {
  uint64_t product[6] = {0, 0, 0, 0, 0, 0};
  for (unsigned int i = 0; i != 3; ++i) {
    uint64_t lo;
    uint64_t hi;
    bf160_clmul64(lhs.values[i] & (i == 2 ? UINT64_C(0xffffffff) : UINT64_MAX),
                  rhs, &lo, &hi);
    bf160_xor_pair(product, i, lo, hi);
  }
  return bf160_reduce_words(product[0], product[1], product[2],
                            product[3], product[4], product[5]);
}
#else
void bf160_mul_out(bf160_t* out, const bf160_t* lhs, const bf160_t* rhs) {
  bf160_t lhs_copy = *lhs;
  const bf160_t rhs_copy =
      BF160C(rhs->values[0], rhs->values[1], rhs->values[2] & UINT64_C(0xffffffff));
  lhs_copy.values[2] &= UINT64_C(0xffffffff);

  bf160_t result = bf160_and_64(lhs_copy, bf160_bit_to_uint64_mask(rhs_copy, 0));
  for (unsigned int idx = 1; idx != 160; ++idx) {
    lhs_copy = bf160_dbl(lhs_copy);
    result   = bf160_add(result, bf160_and_64(lhs_copy, bf160_bit_to_uint64_mask(rhs_copy, idx)));
  }
  *out = result;
}

bf160_t bf160_mul(bf160_t lhs, bf160_t rhs) {
  bf160_t result;
  bf160_mul_out(&result, &lhs, &rhs);
  return result;
}

bf160_t bf160_sqr(bf160_t lhs) {
  return bf160_mul(lhs, lhs);
}

bf160_t bf160_mul_64(bf160_t lhs, bf64_t rhs) {
  lhs.values[2] &= UINT64_C(0xffffffff);

  bf160_t result = bf160_and_64(lhs, bf64_bit_to_mask(rhs, 0));
  for (unsigned int idx = 1; idx != 64; ++idx) {
    lhs    = bf160_dbl(lhs);
    result = bf160_add(result, bf160_and_64(lhs, bf64_bit_to_mask(rhs, idx)));
  }
  return result;
}
#endif

bf160_t bf160_mul_bit(bf160_t lhs, uint8_t rhs) {
  return bf160_and_64(lhs, -((uint64_t)rhs & 1));
}

bf160_t bf160_sum_poly(const bf160_t* xs) {
  bf160_t ret = xs[160 - 1];
  for (size_t i = 1; i < 160; ++i) {
    ret = bf160_add(bf160_dbl(ret), xs[160 - 1 - i]);
  }
  return ret;
}

bf160_t bf160_sum_poly_bits(const uint8_t* xs) {
  bf160_t ret = bf160_from_bit(ptr_get_bit(xs, 160 - 1));
  for (size_t i = 1; i < 160; ++i) {
    ret = bf160_add(bf160_dbl(ret), bf160_from_bit(ptr_get_bit(xs, 160 - 1 - i)));
  }
  return ret;
}

/* ================================================================ */
/*  SM4-specific alpha array for byte_combine operations             */
/*  Roots of m(t) = t^8+t^7+t^6+t^5+t^4+t^2+1 in GF(2^128)           */
/* ================================================================ */

/* Powers of alpha_8: α^1, α^2, α^3, ..., α^7 for byte embedding
 * Used in bf128_byte_combine_bits_sm4: φ(x) = Σ(bit_i * α^i) */
static const bf128_t bf128_alpha_sm4_powers[7] = {
    /* α^1 */ BF128C(U64C(0xb8, 0x4c, 0x7b, 0x2e, 0x8b, 0xa1, 0x14, 0x84),
                     U64C(0xb9, 0x44, 0x1f, 0xb3, 0xb4, 0x95, 0xa5, 0x51)),
    /* α^2 */ BF128C(U64C(0x3f, 0x3b, 0x03, 0x68, 0x66, 0xb9, 0xcf, 0x5d),
                     U64C(0x16, 0x13, 0xd5, 0x0b, 0xc6, 0xd5, 0xb4, 0xb7)),
    /* α^3 */ BF128C(U64C(0x1c, 0xb4, 0xbd, 0x73, 0xa5, 0x20, 0xdf, 0xa2),
                     U64C(0xdf, 0x47, 0x95, 0xc6, 0xe4, 0x91, 0xa7, 0x15)),
    /* α^4 */ BF128C(U64C(0x50, 0x99, 0xde, 0x80, 0xf2, 0x3f, 0x32, 0x37),
                     U64C(0x78, 0xae, 0x9f, 0x7a, 0x0c, 0x03, 0x99, 0x40)),
    /* α^5 */ BF128C(U64C(0xc6, 0x94, 0x7b, 0xe0, 0x16, 0xef, 0x09, 0xed),
                     U64C(0x14, 0x24, 0x57, 0xad, 0xcf, 0x57, 0x12, 0xb6)),
    /* α^6 */ BF128C(U64C(0x7b, 0x1d, 0xd2, 0x3f, 0xae, 0x0a, 0x48, 0xaf),
                     U64C(0x48, 0xe0, 0xac, 0x43, 0x9f, 0x87, 0xd0, 0xcb)),
    /* α^7 */ BF128C(U64C(0x2b, 0xf4, 0x93, 0x81, 0x70, 0x5e, 0x41, 0xa0),
                     U64C(0x8c, 0x05, 0x7f, 0x98, 0xd9, 0x65, 0xdb, 0xff)),
};

static const bf160_t bf160_alpha_sm4_powers[7] = {
    /* α^1 */ BF160C(U64C(0xd5, 0x1d, 0x5b, 0x51, 0x62, 0xcb, 0x88, 0x67),
                     U64C(0xb3, 0xc6, 0x6e, 0x3e, 0x1f, 0xaa, 0x3b, 0x35),
                     UINT64_C(0x139c6970)),
    /* α^2 */ BF160C(U64C(0x35, 0x3c, 0x77, 0x91, 0x4c, 0x35, 0xf5, 0xee),
                     U64C(0xd6, 0x86, 0xb8, 0xcb, 0x40, 0x09, 0x7a, 0x7b),
                     UINT64_C(0x7d862197)),
    /* α^3 */ BF160C(U64C(0x22, 0x8e, 0x2e, 0x54, 0x19, 0x7c, 0x9b, 0xb8),
                     U64C(0x20, 0xb2, 0xe9, 0xd4, 0x1f, 0x28, 0x2a, 0x6d),
                     UINT64_C(0x4696bbf9)),
    /* α^4 */ BF160C(U64C(0x0b, 0xd7, 0x82, 0x9e, 0x1c, 0xc5, 0x6e, 0x4a),
                     U64C(0xa4, 0x52, 0xca, 0x47, 0x5a, 0x3a, 0x72, 0xe0),
                     UINT64_C(0x00a21370)),
    /* α^5 */ BF160C(U64C(0xba, 0xe6, 0xbb, 0x5e, 0xb7, 0x99, 0x21, 0x0c),
                     U64C(0x2f, 0x53, 0x49, 0x01, 0xcb, 0x9a, 0x24, 0xe5),
                     UINT64_C(0x7bfbe6eb)),
    /* α^6 */ BF160C(U64C(0x33, 0x2b, 0x8d, 0xff, 0x54, 0x20, 0x50, 0xae),
                     U64C(0xb5, 0xbe, 0xed, 0x78, 0x2a, 0x43, 0x90, 0x97),
                     UINT64_C(0x976b6b6f)),
    /* α^7 */ BF160C(U64C(0x07, 0xbd, 0x96, 0x2d, 0x41, 0xff, 0x48, 0xd5),
                     U64C(0x0c, 0x71, 0xc5, 0x81, 0xa6, 0xb7, 0x63, 0xd4),
                     UINT64_C(0x80bb0fc7)),
};

/* ================================================================ */
/*  SM4-specific GF(2^8) operations                                   */
/*  Polynomial: x^8 + x^7 + x^6 + x^5 + x^4 + x^2 + 1                */
/* ================================================================ */

#define bf8_modulus_sm4 \
    (UINT8_C((1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 2) | 1))

/*
 * bits_sq_sm4: squaring in GF(2^8) with SM4 polynomial.
 * Derived from x^{2i} mod f(x), f = x^8+x^7+x^6+x^5+x^4+x^2+1:
 *    out[0] = b0 ^ b4
 *    out[1] = b5 ^ b7
 *    out[2] = b1 ^ b4 ^ b5
 *    out[3] = b5 ^ b6 ^ b7
 *    out[4] = b2 ^ b4 ^ b5 ^ b6
 *    out[5] = b4 ^ b5 ^ b6
 *    out[6] = b3 ^ b4 ^ b6
 *    out[7] = b4 ^ b6
 */
uint8_t bits_sq_sm4(uint8_t x) {
    uint8_t res;
    res  = set_bit(get_bit(x, 0) ^ get_bit(x, 4),                              0);
    res |= set_bit(get_bit(x, 5) ^ get_bit(x, 7),                              1);
    res |= set_bit(get_bit(x, 1) ^ get_bit(x, 4) ^ get_bit(x, 5),             2);
    res |= set_bit(get_bit(x, 5) ^ get_bit(x, 6) ^ get_bit(x, 7),             3);
    res |= set_bit(get_bit(x, 2) ^ get_bit(x, 4) ^ get_bit(x, 5)
                                 ^ get_bit(x, 6),                              4);
    res |= set_bit(get_bit(x, 4) ^ get_bit(x, 5) ^ get_bit(x, 6),             5);
    res |= set_bit(get_bit(x, 3) ^ get_bit(x, 4) ^ get_bit(x, 6),             6);
    res |= set_bit(get_bit(x, 4) ^ get_bit(x, 6),                             7);
    return res;
}

bf8_t bf8_mul_sm4(bf8_t lhs, bf8_t rhs) {
    bf8_t result = -(rhs & 1) & lhs;
    for (unsigned int idx = 1; idx < 8; ++idx) {
        const uint8_t mask = -((lhs >> 7) & 1);
        lhs    = (lhs << 1) ^ (mask & bf8_modulus_sm4);
        result ^= -((rhs >> idx) & 1) & lhs;
    }
    return result;
}

bf8_t bf8_square_sm4(bf8_t lhs) {
    bf8_t result = -(lhs & 1) & lhs;
    bf8_t rhs    = lhs;
    for (unsigned int idx = 1; idx < 8; ++idx) {
        const uint8_t mask = -((lhs >> 7) & 1);
        lhs    = (lhs << 1) ^ (mask & bf8_modulus_sm4);
        result ^= -((rhs >> idx) & 1) & lhs;
    }
    return result;
}

bf8_t bf8_inv_sm4(bf8_t in) {
    const bf8_t t2   = bf8_square_sm4(in);          /* in^2   */
    const bf8_t t3   = bf8_mul_sm4(in, t2);         /* in^3   */
    const bf8_t t5   = bf8_mul_sm4(t3, t2);         /* in^5   */
    const bf8_t t7   = bf8_mul_sm4(t5, t2);         /* in^7   */
    const bf8_t t14  = bf8_square_sm4(t7);          /* in^14  */
    const bf8_t t28  = bf8_square_sm4(t14);         /* in^28  */
    const bf8_t t56  = bf8_square_sm4(t28);         /* in^56  */
    const bf8_t t63  = bf8_mul_sm4(t56, t7);        /* in^63  */
    const bf8_t t126 = bf8_square_sm4(t63);         /* in^126 */
    const bf8_t t252 = bf8_square_sm4(t126);        /* in^252 */
    return bf8_mul_sm4(t252, t2);                   /* in^254 */
}

/* ================================================================ */
/*  SM4-specific bf128 squaring operations                           */
/* ================================================================ */

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
bf128_t bf128_byte_combine_sm4(const bf128_t* x) {
  return bf128_byte_combine_pclmul(x, bf128_alpha_sm4_powers);
}
#else
bf128_t bf128_byte_combine_sm4(const bf128_t* x) {
  /* Combine 8 bf128_t elements using SM4-specific alpha coefficients */
  bf128_t bf_out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul(x[i], bf128_alpha_sm4_powers[i - 1]));
  }
  return bf_out;
}
#endif

bf128_t bf128_byte_combine_bits_sm4(uint8_t x) {
    /* Embedding: φ(x) = Σ_{i=0}^{7} bit_i(x) * α_8^i
     * where α_8 is the SM4-compatible element (root of m(t) in GF(2^128)) */
    bf128_t bf_out = bf128_from_bit(get_bit(x, 0));
    for (unsigned int i = 1; i < 8; ++i) {
        bf_out = bf128_add(bf_out, bf128_mul_bit(bf128_alpha_sm4_powers[i - 1], get_bit(x, i)));
    }
    return bf_out;
}

void bf128_sq_bit_sm4(bf128_t *out_tag, const bf128_t *in_tag) {
    /* out[0] = b0 ^ b4 */
    out_tag[0] = bf128_add(in_tag[0], in_tag[4]);
    /* out[1] = b5 ^ b7 */
    out_tag[1] = bf128_add(in_tag[5], in_tag[7]);
    /* out[2] = b1 ^ b4 ^ b5 */
    out_tag[2] = bf128_add(in_tag[1], bf128_add(in_tag[4], in_tag[5]));
    /* out[3] = b5 ^ b6 ^ b7 */
    out_tag[3] = bf128_add(in_tag[5], bf128_add(in_tag[6], in_tag[7]));
    /* out[4] = b2 ^ b4 ^ b5 ^ b6 */
    out_tag[4] = bf128_add(in_tag[2],
                     bf128_add(in_tag[4],
                         bf128_add(in_tag[5], in_tag[6])));
    /* out[5] = b4 ^ b5 ^ b6 */
    out_tag[5] = bf128_add(in_tag[4], bf128_add(in_tag[5], in_tag[6]));
    /* out[6] = b3 ^ b4 ^ b6 */
    out_tag[6] = bf128_add(in_tag[3], bf128_add(in_tag[4], in_tag[6]));
    /* out[7] = b4 ^ b6 */
    out_tag[7] = bf128_add(in_tag[4], in_tag[6]);
}

void bf128_sq_bit_inplace_sm4(bf128_t *tag) {
    /* out[0] = in[0] ^ in[4]  —  in[4] not yet touched */
    tag[0]           = bf128_add(tag[0], tag[4]);

    const bf128_t i1 = tag[1];
    /* out[1] = in[5] ^ in[7] */
    tag[1]           = bf128_add(tag[5], tag[7]);

    const bf128_t i2 = tag[2];
    /* out[2] = i1 ^ in[4] ^ in[5] */
    tag[2]           = bf128_add(i1, bf128_add(tag[4], tag[5]));

    const bf128_t i3 = tag[3];
    /* out[3] = in[5] ^ in[6] ^ in[7] */
    tag[3]           = bf128_add(tag[5], bf128_add(tag[6], tag[7]));

    const bf128_t i4 = tag[4];
    /* out[4] = i2 ^ i4 ^ in[5] ^ in[6]  (in[5], in[6] still original) */
    tag[4]           = bf128_add(i2,
                           bf128_add(i4,
                               bf128_add(tag[5], tag[6])));

    /* out[5] = i4 ^ in[5] ^ in[6]  (in[5], in[6] still original) */
    tag[5]           = bf128_add(i4, bf128_add(tag[5], tag[6]));

    const bf128_t i6 = tag[6];
    /* out[6] = i3 ^ i4 ^ i6 */
    tag[6]           = bf128_add(i3, bf128_add(i4, i6));

    /* out[7] = i4 ^ i6 */
    tag[7]           = bf128_add(i4, i6);
}

bf128_t bf128_byte_combine_sq_sm4(const bf128_t* x) {
  bf128_t tmp[8];
  bf128_sq_bit_sm4(tmp, x);
#if defined(__x86_64__) && defined(__PCLMUL__) && defined(SM4TH_FIELD_PCLMUL)
  return bf128_byte_combine_pclmul(tmp, bf128_alpha_sm4_powers);
#else
  /* Combine using SM4-specific alpha coefficients */
  bf128_t bf_out = tmp[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul(tmp[i], bf128_alpha_sm4_powers[i - 1]));
  }
  return bf_out;
#endif
}

bf128_t bf128_byte_combine_bits_sq_sm4(uint8_t x) {
  return bf128_byte_combine_bits_sm4(bits_sq_sm4(x));
}

#if defined(SM4TH_HAVE_BF160_INLINE_PCLMUL)
typedef struct {
  __m128i v0;
  __m128i v1;
  __m128i v2;
} bf160_product_acc_t;

static inline void bf160_product_acc_zero(bf160_product_acc_t* acc) {
  acc->v0 = _mm_setzero_si128();
  acc->v1 = _mm_setzero_si128();
  acc->v2 = _mm_setzero_si128();
}

static inline void bf160_product_acc_xor_field(bf160_product_acc_t* acc, const bf160_t* x) {
  acc->v0 = _mm_xor_si128(acc->v0, _mm_loadu_si128((const __m128i*)x->values));
  acc->v1 = _mm_xor_si128(
      acc->v1, _mm_cvtsi64_si128((long long)(x->values[2] & UINT64_C(0xffffffff))));
}

static inline void bf160_product_acc_xor_product(bf160_product_acc_t* acc, const bf160_t* lhs,
                                                 const bf160_t* rhs) {
  const bf160_product_t product = bf160_mul_product_faest192_inline(lhs, rhs);
  acc->v0 = _mm_xor_si128(acc->v0, product.v0);
  acc->v1 = _mm_xor_si128(acc->v1, product.v1);
  acc->v2 = _mm_xor_si128(acc->v2, product.v2);
}

static inline bf160_t bf160_product_acc_reduce(const bf160_product_acc_t* acc) {
  return bf160_inline_reduce_product_clmul(acc->v0, acc->v1, acc->v2);
}
#endif

bf160_t bf160_byte_combine_sm4(const bf160_t* x) {
#if defined(SM4TH_HAVE_BF160_INLINE_PCLMUL)
  bf160_product_acc_t acc;
  bf160_product_acc_zero(&acc);
  bf160_product_acc_xor_field(&acc, &x[0]);
  for (unsigned int i = 1; i < 8; ++i) {
    bf160_product_acc_xor_product(&acc, &x[i], &bf160_alpha_sm4_powers[i - 1]);
  }
  return bf160_product_acc_reduce(&acc);
#else
  bf160_t bf_out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf160_add(bf_out, bf160_mul(x[i], bf160_alpha_sm4_powers[i - 1]));
  }
  return bf_out;
#endif
}

bf160_t bf160_byte_combine_bits_sm4(uint8_t x) {
  bf160_t bf_out = bf160_from_bit(get_bit(x, 0));
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf160_add(bf_out, bf160_mul_bit(bf160_alpha_sm4_powers[i - 1], get_bit(x, i)));
  }
  return bf_out;
}

void bf160_sq_bit_sm4(bf160_t* out_tag, const bf160_t* in_tag) {
  out_tag[0] = bf160_add(in_tag[0], in_tag[4]);
  out_tag[1] = bf160_add(in_tag[5], in_tag[7]);
  out_tag[2] = bf160_add(in_tag[1], bf160_add(in_tag[4], in_tag[5]));
  out_tag[3] = bf160_add(in_tag[5], bf160_add(in_tag[6], in_tag[7]));
  out_tag[4] = bf160_add(in_tag[2], bf160_add(in_tag[4], bf160_add(in_tag[5], in_tag[6])));
  out_tag[5] = bf160_add(in_tag[4], bf160_add(in_tag[5], in_tag[6]));
  out_tag[6] = bf160_add(in_tag[3], bf160_add(in_tag[4], in_tag[6]));
  out_tag[7] = bf160_add(in_tag[4], in_tag[6]);
}

void bf160_sq_bit_inplace_sm4(bf160_t* tag) {
  tag[0] = bf160_add(tag[0], tag[4]);

  const bf160_t i1 = tag[1];
  tag[1] = bf160_add(tag[5], tag[7]);

  const bf160_t i2 = tag[2];
  tag[2] = bf160_add(i1, bf160_add(tag[4], tag[5]));

  const bf160_t i3 = tag[3];
  tag[3] = bf160_add(tag[5], bf160_add(tag[6], tag[7]));

  const bf160_t i4 = tag[4];
  tag[4] = bf160_add(i2, bf160_add(i4, bf160_add(tag[5], tag[6])));

  tag[5] = bf160_add(i4, bf160_add(tag[5], tag[6]));

  const bf160_t i6 = tag[6];
  tag[6] = bf160_add(i3, bf160_add(i4, i6));

  tag[7] = bf160_add(i4, i6);
}

bf160_t bf160_byte_combine_sq_sm4(const bf160_t* x) {
  bf160_t tmp[8];
  bf160_sq_bit_sm4(tmp, x);
  return bf160_byte_combine_sm4(tmp);
}

bf160_t bf160_byte_combine_bits_sq_sm4(uint8_t x) {
  return bf160_byte_combine_bits_sm4(bits_sq_sm4(x));
}

/* ================================================================ */
/*  Random Number Generation for Field Elements                    */
/* ================================================================ */

/* Internal pseudo-random state using linear congruential generator */
static uint64_t prng_state = 0xDEADBEEFDEADBEEFULL;

/**
 * Initialize PRNG with a seed value
 * For cryptographic use, this should be seeded from a proper RNG source
 */
void bf_random_seed(uint64_t seed) {
    prng_state = seed ? seed : 0xDEADBEEFDEADBEEFULL;
}

/**
 * Simple pseudo-random number generator using linear congruential method
 * Parameters from Numerical Recipes
 */
static uint64_t bf_random_u64(void) {
    /* Linear congruential generator: x_{n+1} = (a*x_n + c) mod m */
    const uint64_t a = UINT64_C(2685821657736338717);
    const uint64_t c = UINT64_C(2246822519);
    prng_state = a * prng_state + c;
    return prng_state;
}

/**
 * Generate a random GF(2^8) element
 */
bf8_t bf8_random(void) {
    return (uint8_t)(bf_random_u64() & 0xFF);
}

/**
 * Generate a random GF(2^64) element
 */
bf64_t bf64_random(void) {
    return bf_random_u64();
}

/**
 * Generate a random GF(2^128) element
 */
bf128_t bf128_random(void) {
    bf128_t result;
    result.values[0] = bf_random_u64();
    result.values[1] = bf_random_u64();
    return result;
}

bf160_t bf160_random(void) {
    bf160_t result;
    result.values[0] = bf_random_u64();
    result.values[1] = bf_random_u64();
    result.values[2] = bf_random_u64() & UINT64_C(0xffffffff);
    return result;
}
