/*
 * Minimal field implementation for vistrutith_d3_512f.
 */

#include "fields.h"
#include "utils.h"

#if defined(__x86_64__) && defined(VISTRUTITH_FIELD_PCLMUL)
#include <immintrin.h>
#endif

// GF(2^8): x^8 + x^4 + x^3 + x + 1
#define bf8_modulus (UINT8_C((1 << 4) | (1 << 3) | (1 << 1) | 1))
// GF(2^64): x^64 + x^4 + x^3 + x + 1
#define bf64_modulus (UINT64_C((1 << 4) | (1 << 3) | (1 << 1) | 1))
// GF(2^512): x^512 + x^8 + x^5 + x^2 + 1
#define bf512_modulus (UINT64_C((1 << 8) | (1 << 5) | (1 << 2) | 1))

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

bf8_t bf8_mul(bf8_t lhs, bf8_t rhs) {
  bf8_t result = -(rhs & 1) & lhs;
  for (unsigned int idx = 1; idx < 8; ++idx) {
    const uint8_t mask = -((lhs >> 7) & 1);
    lhs                = (lhs << 1) ^ (mask & bf8_modulus);
    result ^= -((rhs >> idx) & 1) & lhs;
  }
  return result;
}

bf8_t bf8_square(bf8_t lhs) { return bf8_mul(lhs, lhs); }

bf8_t bf8_inv(bf8_t in) {
  const bf8_t t2   = bf8_square(in);
  const bf8_t t3   = bf8_mul(in, t2);
  const bf8_t t5   = bf8_mul(t3, t2);
  const bf8_t t7   = bf8_mul(t5, t2);
  const bf8_t t14  = bf8_square(t7);
  const bf8_t t28  = bf8_square(t14);
  const bf8_t t56  = bf8_square(t28);
  const bf8_t t63  = bf8_mul(t56, t7);
  const bf8_t t126 = bf8_square(t63);
  const bf8_t t252 = bf8_square(t126);
  return bf8_mul(t252, t2);
}

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(VISTRUTITH_FIELD_PCLMUL)
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
  for (unsigned int idx = 1; idx < 64; ++idx) {
    const uint64_t mask = -((lhs >> 63) & 1);
    lhs                 = (lhs << 1) ^ (mask & bf64_modulus);
    result ^= (-((rhs >> idx) & 1)) & lhs;
  }
  return result;
}
#endif

#define bf64_bit_to_mask(value, bit) -((((uint64_t)(value)) >> (bit)) & 1)

static inline bf512_t bf512_and_64(bf512_t lhs, bf64_t rhs) {
  for (unsigned int i = 0; i < ARRAY_SIZE(lhs.values); ++i) {
    lhs.values[i] &= rhs;
  }
  return lhs;
}

static inline bf512_t bf512_shift_left_1(bf512_t value) {
  for (unsigned int i = ARRAY_SIZE(value.values) - 1; i; --i) {
    value.values[i] = (value.values[i] << 1) | (value.values[i - 1] >> 63);
  }
  value.values[0] <<= 1;
  return value;
}

static inline uint64_t bf512_bit_to_uint64_mask(bf512_t value, unsigned int bit) {
  const unsigned int byte_idx = bit / 64;
  const unsigned int bit_idx  = bit % 64;
  return -((BF_VALUE(value, byte_idx) >> bit_idx) & 1);
}

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(VISTRUTITH_FIELD_PCLMUL)
static inline void clmul64_vec(__m128i lhs, __m128i rhs, uint64_t* lo, uint64_t* hi) {
  const __m128i p = _mm_clmulepi64_si128(lhs, rhs, 0x00);
  *lo             = (uint64_t)_mm_cvtsi128_si64(p);
  *hi             = (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(p, 8));
}

static inline void bf512_reduce_high_word(uint64_t* acc, unsigned int hi_idx, uint64_t hi_word) {
  const unsigned int lo_idx = hi_idx - 8;
  acc[lo_idx] ^= hi_word;
  acc[lo_idx] ^= hi_word << 2;
  acc[lo_idx] ^= hi_word << 5;
  acc[lo_idx] ^= hi_word << 8;
  acc[lo_idx + 1] ^= hi_word >> 62;
  acc[lo_idx + 1] ^= hi_word >> 59;
  acc[lo_idx + 1] ^= hi_word >> 56;
}

static inline void karatsuba_mul2_u64(const uint64_t a[2], const uint64_t b[2], uint64_t out[4]) {
  uint64_t z0_lo, z0_hi, z2_lo, z2_hi, z1_lo, z1_hi;
  clmul64_vec(_mm_cvtsi64_si128((long long)a[0]), _mm_cvtsi64_si128((long long)b[0]), &z0_lo,
              &z0_hi);
  clmul64_vec(_mm_cvtsi64_si128((long long)a[1]), _mm_cvtsi64_si128((long long)b[1]), &z2_lo,
              &z2_hi);
  clmul64_vec(_mm_cvtsi64_si128((long long)(a[0] ^ a[1])),
              _mm_cvtsi64_si128((long long)(b[0] ^ b[1])), &z1_lo, &z1_hi);

  const uint64_t mid_lo = z1_lo ^ z0_lo ^ z2_lo;
  const uint64_t mid_hi = z1_hi ^ z0_hi ^ z2_hi;

  out[0] = z0_lo;
  out[1] = z0_hi ^ mid_lo;
  out[2] = z2_lo ^ mid_hi;
  out[3] = z2_hi;
}

static inline void karatsuba_mul4_u64(const uint64_t a[4], const uint64_t b[4], uint64_t out[8]) {
  uint64_t z0[4], z1[4], z2[4], mid[4];
  uint64_t am[2], bm[2];

  karatsuba_mul2_u64(a, b, z0);
  karatsuba_mul2_u64(a + 2, b + 2, z2);
  for (unsigned int i = 0; i < 2; ++i) {
    am[i] = a[i] ^ a[i + 2];
    bm[i] = b[i] ^ b[i + 2];
  }
  karatsuba_mul2_u64(am, bm, z1);

  for (unsigned int i = 0; i < 4; ++i) {
    mid[i] = z1[i] ^ z0[i] ^ z2[i];
  }

  for (unsigned int i = 0; i < 8; ++i) {
    out[i] = 0;
  }
  for (unsigned int i = 0; i < 4; ++i) {
    out[i] ^= z0[i];
    out[i + 2] ^= mid[i];
    out[i + 4] ^= z2[i];
  }
}

static inline void karatsuba_mul8_u64(const uint64_t a[8], const uint64_t b[8], uint64_t out[16]) {
  uint64_t z0[8], z1[8], z2[8], mid[8];
  uint64_t am[4], bm[4];

  karatsuba_mul4_u64(a, b, z0);
  karatsuba_mul4_u64(a + 4, b + 4, z2);
  for (unsigned int i = 0; i < 4; ++i) {
    am[i] = a[i] ^ a[i + 4];
    bm[i] = b[i] ^ b[i + 4];
  }
  karatsuba_mul4_u64(am, bm, z1);

  for (unsigned int i = 0; i < 8; ++i) {
    mid[i] = z1[i] ^ z0[i] ^ z2[i];
  }

  for (unsigned int i = 0; i < 16; ++i) {
    out[i] = 0;
  }
  for (unsigned int i = 0; i < 8; ++i) {
    out[i] ^= z0[i];
    out[i + 4] ^= mid[i];
    out[i + 8] ^= z2[i];
  }
}
#endif

#if defined(__x86_64__) && defined(__PCLMUL__) && defined(VISTRUTITH_FIELD_PCLMUL)
bf512_t bf512_mul(bf512_t lhs, bf512_t rhs) {
  uint64_t acc[16];
  karatsuba_mul8_u64(lhs.values, rhs.values, acc);

  for (int idx = 15; idx >= 8; --idx) {
    const uint64_t hi_word = acc[idx];
    if (hi_word) {
      bf512_reduce_high_word(acc, (unsigned int)idx, hi_word);
      acc[idx] = 0;
    }
  }

  bf512_t result;
  for (unsigned int i = 0; i < ARRAY_SIZE(result.values); ++i) {
    result.values[i] = acc[i];
  }
  return result;
}

bf512_t bf512_mul_64(bf512_t lhs, bf64_t rhs) {
  uint64_t acc[9] = {0};
  const __m128i rhs_vec = _mm_cvtsi64_si128((long long)rhs);

  for (unsigned int i = 0; i < ARRAY_SIZE(lhs.values); ++i) {
    uint64_t lo, hi;
    clmul64_vec(_mm_cvtsi64_si128((long long)lhs.values[i]), rhs_vec, &lo, &hi);
    acc[i] ^= lo;
    acc[i + 1] ^= hi;
  }

  if (acc[8]) {
    bf512_reduce_high_word(acc, 8, acc[8]);
  }

  bf512_t result;
  for (unsigned int i = 0; i < ARRAY_SIZE(result.values); ++i) {
    result.values[i] = acc[i];
  }
  return result;
}

#else
bf512_t bf512_mul(bf512_t lhs, bf512_t rhs) {
  bf512_t result = bf512_and_64(lhs, bf512_bit_to_uint64_mask(rhs, 0));
  for (unsigned int idx = 1; idx < 512; ++idx) {
    const uint64_t mask = bf512_bit_to_uint64_mask(lhs, 511);
    lhs                 = bf512_shift_left_1(lhs);
    BF_VALUE(lhs, 0) ^= mask & bf512_modulus;
    result = bf512_add(result, bf512_and_64(lhs, bf512_bit_to_uint64_mask(rhs, idx)));
  }
  return result;
}

bf512_t bf512_mul_64(bf512_t lhs, bf64_t rhs) {
  bf512_t result = bf512_and_64(lhs, bf64_bit_to_mask(rhs, 0));
  for (unsigned int idx = 1; idx < 64; ++idx) {
    const uint64_t mask = bf512_bit_to_uint64_mask(lhs, 511);
    lhs                 = bf512_shift_left_1(lhs);
    BF_VALUE(lhs, 0) ^= mask & bf512_modulus;
    result = bf512_add(result, bf512_and_64(lhs, bf64_bit_to_mask(rhs, idx)));
  }
  return result;
}
#endif

bf512_t bf512_mul_bit(bf512_t lhs, uint8_t rhs) {
  return bf512_and_64(lhs, -((uint64_t)rhs & 1));
}

/* beta_8 powers for byte embedding over GF(2^8) (AES polynomial):
 * phi(x) = sum_{i=0}^7 bit_i(x) * beta_8^i in GF(2^512), where
 * m(beta_8)=beta_8^8+beta_8^4+beta_8^3+beta_8+1 = 0. */
static const bf512_t bf512_beta8_powers[7] = {
    BF512C(UINT64_C(0xd10a90dbfbf0ab48), UINT64_C(0xc6918a31b784e286),
           UINT64_C(0xfebe70692aade602), UINT64_C(0x5066d80aee92d170),
           UINT64_C(0x98f51080f5f0a16f), UINT64_C(0x40b1540ae406d71f),
           UINT64_C(0x3be042210fc776cb), UINT64_C(0xe33798f4ec1cbbd9)),
    BF512C(UINT64_C(0xe76ef44d9176df22), UINT64_C(0x7d1fb2306449111b),
           UINT64_C(0xd345568e76e43bfd), UINT64_C(0x0058b1372714488a),
           UINT64_C(0x5abe43a7a38b5ce7), UINT64_C(0xa8fac14641908164),
           UINT64_C(0x8b453f58ecaa6860), UINT64_C(0x90b4b2bd7d5eb34b)),
    BF512C(UINT64_C(0x5de7a890b1db560b), UINT64_C(0xb6da7d8cfc4655b2),
           UINT64_C(0x450a93e2819878ff), UINT64_C(0xe313e7568f4207d9),
           UINT64_C(0xd4b359f402b8ca64), UINT64_C(0xa2e2a68fd71ad70b),
           UINT64_C(0x649911d470a0dd8e), UINT64_C(0x115c71b3ad136af6)),
    BF512C(UINT64_C(0x90f919f67f1ad78d), UINT64_C(0x6c8dbac200ecd3fd),
           UINT64_C(0x457d75084032d706), UINT64_C(0x884fedda543343d6),
           UINT64_C(0xca49cdd28813911c), UINT64_C(0x5de65361fea67c4b),
           UINT64_C(0xbf332bd8e4e35cfc), UINT64_C(0x654cf25cb0d9ae37)),
    BF512C(UINT64_C(0x2eff156ce821019f), UINT64_C(0x6a39e71f9e3cbd72),
           UINT64_C(0xe557321029d41b62), UINT64_C(0x661a6e7b24083a60),
           UINT64_C(0xe7281c8ee3f3e0cf), UINT64_C(0x02fa4e5e83af7882),
           UINT64_C(0x6fc350cbe9a65b9a), UINT64_C(0xd086bbc0cdcf394b)),
    BF512C(UINT64_C(0x41d9e95e4cd6d730), UINT64_C(0x364da0b7797bf250),
           UINT64_C(0x77b0789a33f009af), UINT64_C(0x95dfd19dd7dc5eab),
           UINT64_C(0xa14c951a7a3c4a9c), UINT64_C(0xc6092d20657a7343),
           UINT64_C(0xce6130a3899fe693), UINT64_C(0x70332f02b47ce5e8)),
    BF512C(UINT64_C(0x1e939631ee83d266), UINT64_C(0xdf444abbf7cb73d1),
           UINT64_C(0x9cc0681d3346c163), UINT64_C(0xd6bf0917cfe93dca),
           UINT64_C(0xa6463b048676af7b), UINT64_C(0xba2e3cfb14bee82a),
           UINT64_C(0x82e8e82110bf7981), UINT64_C(0x82f9b7671760aded)),
};

bf512_t bf512_byte_combine(const bf512_t* x) {
#if defined(__x86_64__) && defined(__PCLMUL__) && defined(VISTRUTITH_FIELD_PCLMUL)
  uint64_t acc[16] = {0};
  for (unsigned int i = 0; i < ARRAY_SIZE(x[0].values); ++i) {
    acc[i] = x[0].values[i];
  }

  for (unsigned int k = 1; k < 8; ++k) {
    uint64_t prod[16];
    karatsuba_mul8_u64(x[k].values, bf512_beta8_powers[k - 1].values, prod);
    for (unsigned int i = 0; i < ARRAY_SIZE(prod); ++i) {
      acc[i] ^= prod[i];
    }
  }

  for (int idx = 15; idx >= 8; --idx) {
    const uint64_t hi_word = acc[idx];
    if (hi_word) {
      bf512_reduce_high_word(acc, (unsigned int)idx, hi_word);
      acc[idx] = 0;
    }
  }

  bf512_t out;
  for (unsigned int i = 0; i < ARRAY_SIZE(out.values); ++i) {
    out.values[i] = acc[i];
  }
  return out;
#else
  bf512_t out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    out = bf512_add(out, bf512_mul(x[i], bf512_beta8_powers[i - 1]));
  }
  return out;
#endif
}

void bf512_sq_bit(bf512_t* out_tag, const bf512_t* in_tag) {
  out_tag[0] = bf512_add(in_tag[0], bf512_add(in_tag[4], in_tag[6]));
  out_tag[1] = bf512_add(in_tag[4], bf512_add(in_tag[6], in_tag[7]));
  out_tag[2] = bf512_add(in_tag[1], in_tag[5]);
  out_tag[3] = bf512_add(bf512_add(in_tag[4], in_tag[5]), bf512_add(in_tag[6], in_tag[7]));
  out_tag[4] = bf512_add(in_tag[2], bf512_add(in_tag[4], in_tag[7]));
  out_tag[5] = bf512_add(in_tag[5], in_tag[6]);
  out_tag[6] = bf512_add(in_tag[3], in_tag[5]);
  out_tag[7] = bf512_add(in_tag[6], in_tag[7]);
}

void bf512_sq_bit_inplace(bf512_t* tag) {
  tag[0]           = bf512_add(tag[0], bf512_add(tag[4], tag[6]));
  const bf512_t i1 = tag[1];
  tag[1]           = bf512_add(tag[4], bf512_add(tag[6], tag[7]));
  const bf512_t i2 = tag[2];
  tag[2]           = bf512_add(i1, tag[5]);
  const bf512_t i3 = tag[3];
  tag[3]           = bf512_add(bf512_add(tag[4], tag[5]), bf512_add(tag[6], tag[7]));
  tag[4]           = bf512_add(i2, bf512_add(tag[4], tag[7]));
  const bf512_t i5 = tag[5];
  tag[5]           = bf512_add(tag[5], tag[6]);
  const bf512_t i6 = tag[6];
  tag[6]           = bf512_add(i3, i5);
  tag[7]           = bf512_add(i6, tag[7]);
}

bf512_t bf512_byte_combine_sq(const bf512_t* x) {
  bf512_t tmp[8];
  bf512_sq_bit(tmp, x);
  return bf512_byte_combine(tmp);
}

bf512_t bf512_byte_combine_bits(uint8_t x) {
  bf512_t out = bf512_from_bit(get_bit(x, 0));
  for (unsigned int i = 1; i < 8; ++i) {
    out = bf512_add(out, bf512_mul_bit(bf512_beta8_powers[i - 1], get_bit(x, i)));
  }
  return out;
}

bf512_t bf512_byte_combine_bits_sq(uint8_t x) {
  return bf512_byte_combine_bits(bits_sq(x));
}

static inline bf512_t bf512_dbl(bf512_t lhs) {
  const uint64_t mask = bf512_bit_to_uint64_mask(lhs, 511);
  lhs                 = bf512_shift_left_1(lhs);
  BF_VALUE(lhs, 0) ^= mask & bf512_modulus;
  return lhs;
}

bf512_t bf512_sum_poly(const bf512_t* xs) {
  bf512_t ret = xs[511];
  for (size_t i = 1; i < 512; ++i) {
    ret = bf512_add(bf512_dbl(ret), xs[511 - i]);
  }
  return ret;
}

bf512_t bf512_sum_poly_bits(const uint8_t* xs) {
  bf512_t ret = bf512_from_bit(ptr_get_bit(xs, 511));
  for (size_t i = 1; i < 512; ++i) {
    ret = bf512_add(bf512_dbl(ret), bf512_from_bit(ptr_get_bit(xs, 511 - i)));
  }
  return ret;
}
