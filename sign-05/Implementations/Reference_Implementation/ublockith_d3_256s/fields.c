/*
 *  Field implementation of GF(2^8), GF(2^64), GF(2^128), GF(2^256), GF(2^384), and GF(2^768)
 *  For GF(2^8), the irreducible polynomial for AES is X^8 + X^4 + X^3 + X^1 + 1
 *  For GF(2^8), the irreducible polynomial for SM4 is X^8 + X^7 + X^6 + X^5 + X^4 + X^2 + 1
 */

#include "fields.h"
#include "utils.h"

// GF(2^8) with X^8 + X^4 + X^3 + X^1 + 1
#define bf8_modulus (UINT8_C((1 << 4) | (1 << 3) | (1 << 1) | 1))
// GF(2^64) with X^64 + X^4 + X^3 + X^1 + 1
#define bf64_modulus (UINT64_C((1 << 4) | (1 << 3) | (1 << 1) | 1))
// GF(2^128) with X^128 + X^7 + X^2 + X^1 + 1
#define bf128_modulus (UINT64_C((1 << 7) | (1 << 2) | (1 << 1) | 1))
// GF(2^256) with X^256 + X^10 + X^5 + X^2 + 1
#define bf256_modulus (UINT64_C((1 << 10) | (1 << 5) | (1 << 2) | 1))

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

// GF(2^8) implementation

bf8_t bf8_mul(bf8_t lhs, bf8_t rhs) {
  bf8_t result = -(rhs & 1) & lhs;
  for (unsigned int idx = 1; idx < 8; ++idx) {
    const uint8_t mask = -((lhs >> 7) & 1);
    lhs                = (lhs << 1) ^ (mask & bf8_modulus);
    result ^= -((rhs >> idx) & 1) & lhs;
  }
  return result;
}

bf8_t bf8_square(bf8_t lhs) {
  bf8_t result = -(lhs & 1) & lhs;
  bf8_t rhs    = lhs;
  for (unsigned int idx = 1; idx < 8; ++idx) {
    const uint8_t mask = -((lhs >> 7) & 1);
    lhs                = (lhs << 1) ^ (mask & bf8_modulus);
    result ^= -((rhs >> idx) & 1) & lhs;
  }
  return result;
}

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

// GF(2^64) implementation

bf64_t bf64_mul(bf64_t lhs, bf64_t rhs) {
  bf64_t result = (-(rhs & 1)) & lhs;
  for (unsigned int idx = 1; idx != 64; ++idx) {
    const uint64_t mask = -((lhs >> 63) & 1);
    lhs                 = (lhs << 1) ^ (mask & bf64_modulus);
    result ^= (-((rhs >> idx) & 1)) & lhs;
  }
  return result;
}

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
 * Scalar path: unchanged loop over bf128_mul.
 */
bf128_t bf128_byte_combine(const bf128_t* x) {
  bf128_t bf_out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul(x[i], bf128_alpha[i - 1]));
  }
  return bf_out;
}

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

void bf128_sq_bit_inplace(bf128_t* tag) {
  tag[0]           = bf128_add(tag[0], bf128_add(tag[4], tag[6]));
  const bf128_t i1 = tag[1];
  tag[1]           = bf128_add(tag[4], bf128_add(tag[6], tag[7]));
  const bf128_t i2 = tag[2];
  tag[2]           = bf128_add(i1, tag[5]);
  const bf128_t i3 = tag[3];
  tag[3]           = bf128_add(bf128_add(tag[4], tag[5]), bf128_add(tag[6], tag[7]));
  tag[4]           = bf128_add(i2, bf128_add(tag[4], tag[7]));
  const bf128_t i5 = tag[5];
  tag[5]           = bf128_add(tag[5], tag[6]);
  const bf128_t i6 = tag[6];
  tag[6]           = bf128_add(i3, i5);
  tag[7]           = bf128_add(i6, tag[7]);
}

bf128_t bf128_byte_combine_sq(const bf128_t* x) {
  bf128_t bf_tmp[8];
  bf128_sq_bit(bf_tmp, x);
  return bf128_byte_combine(bf_tmp);
}

bf128_t bf128_byte_combine_bits(uint8_t x) {
  bf128_t bf_out = bf128_from_bit(get_bit(x, 0));
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul_bit(bf128_alpha[i - 1], get_bit(x, i)));
  }
  return bf_out;
}

bf128_t bf128_byte_combine_bits_sq(uint8_t x) {
  return bf128_byte_combine_bits(bits_sq(x));
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

// GF(2^256) implementation

static const bf256_t bf256_alpha[7] = {
    BF256C(U64C(0xe7, 0xfe, 0xde, 0x0b, 0x42, 0x88, 0x97, 0x96),
           U64C(0x67, 0x4e, 0x47, 0xa0, 0x38, 0x8d, 0xd6, 0xbe),
           U64C(0x6a, 0xe1, 0xf1, 0xf8, 0x45, 0x98, 0x22, 0xdf),
           U64C(0x33, 0x58, 0xc9, 0x20, 0xcf, 0xa8, 0xc9, 0x04)),
    BF256C(U64C(0xc1, 0x89, 0x22, 0xd5, 0x2a, 0xf5, 0x5a, 0xa9),
           U64C(0x2f, 0x07, 0x42, 0x2c, 0x8d, 0xc4, 0xa5, 0x2b),
           U64C(0xea, 0xb0, 0x00, 0x6c, 0x37, 0x0d, 0x4a, 0xd1),
           U64C(0xf1, 0x4a, 0x5b, 0x9c, 0x69, 0x4d, 0x4e, 0x06)),
    BF256C(U64C(0x1d, 0x9d, 0x80, 0x3f, 0x83, 0xb3, 0xda, 0x55),
           U64C(0x57, 0x0f, 0x3b, 0x53, 0x1e, 0x83, 0x71, 0x17),
           U64C(0x10, 0xac, 0x3f, 0xad, 0x3f, 0x57, 0x96, 0xfb),
           U64C(0x8d, 0xf6, 0x11, 0x70, 0xdb, 0xe3, 0x95, 0x61)),
    BF256C(U64C(0xd5, 0xcd, 0x1b, 0xb0, 0x19, 0x05, 0x01, 0xde),
           U64C(0xf6, 0xe3, 0x30, 0x1a, 0x91, 0x58, 0x27, 0x75),
           U64C(0x3f, 0xa0, 0x9e, 0x48, 0xb6, 0x78, 0x07, 0x2a),
           U64C(0x38, 0x88, 0x76, 0x4f, 0xd6, 0x4f, 0xc2, 0x56)),
    BF256C(U64C(0xb6, 0x30, 0x8a, 0xe9, 0x29, 0xf5, 0xc2, 0x98),
           U64C(0x82, 0x84, 0xf1, 0x40, 0xd4, 0xdb, 0xc4, 0x1b),
           U64C(0x81, 0xa9, 0x49, 0x7d, 0x94, 0x09, 0xbe, 0x2f),
           U64C(0xfc, 0x4f, 0x57, 0x71, 0x6d, 0x0b, 0x27, 0x22)),
    BF256C(U64C(0x0b, 0x67, 0x44, 0xde, 0xb9, 0xaf, 0x75, 0x9e),
           U64C(0xbc, 0xaf, 0xf1, 0x66, 0xc6, 0x66, 0xed, 0xac),
           U64C(0x7e, 0x1f, 0x99, 0xf2, 0x3f, 0x25, 0x01, 0xf0),
           U64C(0xf3, 0x29, 0xfa, 0xd1, 0x2f, 0x37, 0x3d, 0xc0)),
    BF256C(U64C(0x8b, 0xe8, 0x32, 0xb3, 0x98, 0xb6, 0x43, 0xba),
           U64C(0x0d, 0x6f, 0xb8, 0x25, 0xd6, 0xc4, 0x37, 0x52),
           U64C(0x45, 0x15, 0xe8, 0xf4, 0x2a, 0x2b, 0x65, 0x2f),
           U64C(0xb8, 0x7b, 0x6b, 0xd2, 0x09, 0xea, 0x3e, 0x13)),
};

bf256_t bf256_byte_combine(const bf256_t* x) {
  bf256_t bf_out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf256_add(bf_out, bf256_mul(x[i], bf256_alpha[i - 1]));
  }
  return bf_out;
}

void bf256_sq_bit(bf256_t* out_tag, const bf256_t* in_tag) {
  out_tag[0] = bf256_add(in_tag[0], bf256_add(in_tag[4], in_tag[6]));
  out_tag[1] = bf256_add(in_tag[4], bf256_add(in_tag[6], in_tag[7]));
  out_tag[2] = bf256_add(in_tag[1], in_tag[5]);
  out_tag[3] = bf256_add(bf256_add(in_tag[4], in_tag[5]), bf256_add(in_tag[6], in_tag[7]));
  out_tag[4] = bf256_add(in_tag[2], bf256_add(in_tag[4], in_tag[7]));
  out_tag[5] = bf256_add(in_tag[5], in_tag[6]);
  out_tag[6] = bf256_add(in_tag[3], in_tag[5]);
  out_tag[7] = bf256_add(in_tag[6], in_tag[7]);
}

void bf256_sq_bit_inplace(bf256_t* tag) {
  tag[0]           = bf256_add(tag[0], bf256_add(tag[4], tag[6]));
  const bf256_t i1 = tag[1];
  tag[1]           = bf256_add(tag[4], bf256_add(tag[6], tag[7]));
  const bf256_t i2 = tag[2];
  tag[2]           = bf256_add(i1, tag[5]);
  const bf256_t i3 = tag[3];
  tag[3]           = bf256_add(bf256_add(tag[4], tag[5]), bf256_add(tag[6], tag[7]));
  tag[4]           = bf256_add(i2, bf256_add(tag[4], tag[7]));
  const bf256_t i5 = tag[5];
  tag[5]           = bf256_add(tag[5], tag[6]);
  const bf256_t i6 = tag[6];
  tag[6]           = bf256_add(i3, i5);
  tag[7]           = bf256_add(i6, tag[7]);
}

bf256_t bf256_byte_combine_sq(const bf256_t* x) {
  bf256_t bf_tmp[8];
  bf256_sq_bit(bf_tmp, x);
  return bf256_byte_combine(bf_tmp);
}

bf256_t bf256_byte_combine_bits(uint8_t x) {
  bf256_t bf_out = bf256_from_bit(get_bit(x, 0));
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf256_add(bf_out, bf256_mul_bit(bf256_alpha[i - 1], get_bit(x, i)));
  }
  return bf_out;
}

bf256_t bf256_byte_combine_bits_sq(uint8_t x) {
  return bf256_byte_combine_bits(bits_sq(x));
}

static inline bf256_t bf256_and_64(bf256_t lhs, bf64_t rhs) {
  for (unsigned int i = 0; i != ARRAY_SIZE(lhs.values); ++i) {
    lhs.values[i] &= rhs;
  }
  return lhs;
}

static inline bf256_t bf256_shift_left_1(bf256_t value) {
  value.values[3] = (value.values[3] << 1) | (value.values[2] >> 63);
  value.values[2] = (value.values[2] << 1) | (value.values[1] >> 63);
  value.values[1] = (value.values[1] << 1) | (value.values[0] >> 63);
  value.values[0] = value.values[0] << 1;
  return value;
}

static inline uint64_t bf256_bit_to_uint64_mask(bf256_t value,
                                                                              unsigned int bit) {
  const unsigned int byte_idx = bit / 64;
  const unsigned int bit_idx  = bit % 64;

  return -((BF_VALUE(value, byte_idx) >> bit_idx) & 1);
}

bf256_t bf256_mul(bf256_t lhs, bf256_t rhs) {
  bf256_t result = bf256_and_64(lhs, bf256_bit_to_uint64_mask(rhs, 0));
  for (unsigned int idx = 1; idx != 256; ++idx) {
    const uint64_t mask = bf256_bit_to_uint64_mask(lhs, 256 - 1);
    lhs                 = bf256_shift_left_1(lhs);
    BF_VALUE(lhs, 0) ^= mask & bf256_modulus;

    result = bf256_add(result, bf256_and_64(lhs, bf256_bit_to_uint64_mask(rhs, idx)));
  }
  return result;
}

bf256_t bf256_mul_64(bf256_t lhs, bf64_t rhs) {
  bf256_t result = bf256_and_64(lhs, bf64_bit_to_mask(rhs, 0));
  for (unsigned int idx = 1; idx != 64; ++idx) {
    const uint64_t mask = bf256_bit_to_uint64_mask(lhs, 256 - 1);
    lhs                 = bf256_shift_left_1(lhs);
    BF_VALUE(lhs, 0) ^= mask & bf256_modulus;

    result = bf256_add(result, bf256_and_64(lhs, bf64_bit_to_mask(rhs, idx)));
  }
  return result;
}

bf256_t bf256_mul_bit(bf256_t lhs, uint8_t rhs) {
  return bf256_and_64(lhs, -((uint64_t)rhs & 1));
}

static inline bf256_t bf256_dbl(bf256_t lhs) {
  uint64_t mask = bf256_bit_to_uint64_mask(lhs, 256 - 1);
  lhs           = bf256_shift_left_1(lhs);
  BF_VALUE(lhs, 0) ^= mask & bf256_modulus;
  return lhs;
}

bf256_t bf256_sum_poly(const bf256_t* xs) {
  bf256_t ret = xs[256 - 1];
  for (size_t i = 1; i < 256; ++i) {
    ret = bf256_add(bf256_dbl(ret), xs[256 - 1 - i]);
  }
  return ret;
}

bf256_t bf256_sum_poly_bits(const uint8_t* xs) {
  bf256_t ret = bf256_from_bit(ptr_get_bit(xs, 256 - 1));
  for (size_t i = 1; i < 256; ++i) {
    ret = bf256_add(bf256_dbl(ret), bf256_from_bit(ptr_get_bit(xs, 256 - 1 - i)));
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

bf128_t bf128_byte_combine_sm4(const bf128_t* x) {
  /* Combine 8 bf128_t elements using SM4-specific alpha coefficients */
  bf128_t bf_out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul(x[i], bf128_alpha_sm4_powers[i - 1]));
  }
  return bf_out;
}

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
  /* Combine using SM4-specific alpha coefficients */
  bf128_t bf_out = tmp[0];
  for (unsigned int i = 1; i < 8; ++i) {
    bf_out = bf128_add(bf_out, bf128_mul(tmp[i], bf128_alpha_sm4_powers[i - 1]));
  }
  return bf_out;
}

bf128_t bf128_byte_combine_bits_sq_sm4(uint8_t x) {
  return bf128_byte_combine_bits_sm4(bits_sq_sm4(x));
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

/**
 * Generate a random GF(2^256) element
 */
bf256_t bf256_random(void) {
    bf256_t result;
    result.values[0] = bf_random_u64();
    result.values[1] = bf_random_u64();
    result.values[2] = bf_random_u64();
    result.values[3] = bf_random_u64();
    return result;
}

