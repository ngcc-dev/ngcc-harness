/*
 * Minimal field implementation for vistrutith_d3_512s.
 */

#include "fields.h"
#include "utils.h"

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

bf64_t bf64_mul(bf64_t lhs, bf64_t rhs) {
  bf64_t result = (-(rhs & 1)) & lhs;
  for (unsigned int idx = 1; idx < 64; ++idx) {
    const uint64_t mask = -((lhs >> 63) & 1);
    lhs                 = (lhs << 1) ^ (mask & bf64_modulus);
    result ^= (-((rhs >> idx) & 1)) & lhs;
  }
  return result;
}

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
  bf512_t out = x[0];
  for (unsigned int i = 1; i < 8; ++i) {
    out = bf512_add(out, bf512_mul(x[i], bf512_beta8_powers[i - 1]));
  }
  return out;
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
