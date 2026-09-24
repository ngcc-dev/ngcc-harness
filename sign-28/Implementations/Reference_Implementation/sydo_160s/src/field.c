/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "field.h"

#include "internal.h"

#include <string.h>

static uint64_t load64_le_local(const uint8_t* in, size_t available) {
  uint64_t out = 0;
  const size_t take = available < 8u ? available : 8u;
  if (take == 8u) {
    return (uint64_t)in[0] | ((uint64_t)in[1] << 8) | ((uint64_t)in[2] << 16) |
           ((uint64_t)in[3] << 24) | ((uint64_t)in[4] << 32) | ((uint64_t)in[5] << 40) |
           ((uint64_t)in[6] << 48) | ((uint64_t)in[7] << 56);
  }
  for (size_t i = 0; i != take; ++i) {
    out |= (uint64_t)in[i] << (8u * i);
  }
  return out;
}

static void store64_le_local(uint8_t* out, uint64_t v, size_t available) {
  const size_t take = available < 8u ? available : 8u;
  if (take == 8u) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
    out[4] = (uint8_t)(v >> 32);
    out[5] = (uint8_t)(v >> 40);
    out[6] = (uint8_t)(v >> 48);
    out[7] = (uint8_t)(v >> 56);
    return;
  }
  for (size_t i = 0; i != take; ++i) {
    out[i] = (uint8_t)(v >> (8u * i));
  }
}

static void xor_shifted(uint64_t* product, const uint64_t* a, size_t words, size_t shift) {
  const size_t word_shift = shift >> 6;
  const unsigned int bit_shift = (unsigned int)(shift & 63u);
  for (size_t i = 0; i != words; ++i) {
    const uint64_t v = a[i];
    product[word_shift + i] ^= v << bit_shift;
    if (bit_shift != 0u) {
      product[word_shift + i + 1u] ^= v >> (64u - bit_shift);
    }
  }
}

static void field_reduce_512_words(uint64_t* x) {
  uint64_t high[9];

  memset(high, 0, sizeof(high));
  memcpy(high, x + 8u, 9u * sizeof(high[0]));
  memset(x + 8u, 0, 9u * sizeof(x[0]));
  xor_shifted(x, high, 9u, 0u);
  xor_shifted(x, high, 9u, 2u);
  xor_shifted(x, high, 9u, 5u);
  xor_shifted(x, high, 9u, 8u);

  memset(high, 0, sizeof(high));
  memcpy(high, x + 8u, 9u * sizeof(high[0]));
  memset(x + 8u, 0, 9u * sizeof(x[0]));
  xor_shifted(x, high, 9u, 0u);
  xor_shifted(x, high, 9u, 2u);
  xor_shifted(x, high, 9u, 5u);
  xor_shifted(x, high, 9u, 8u);
}

static unsigned int get_nibble_words(const uint64_t* x, size_t nibble) {
  const size_t bit = nibble * 4u;
  return (unsigned int)((x[bit >> 6] >> (bit & 63u)) & 0x0fu);
}

static void field_mul_wide_words(uint64_t* product, const uint64_t* a_words,
                                 const uint64_t* b_words, size_t lambda_bits, size_t words) {
  uint64_t table[16][9];
  const size_t table_words = words + 1u;
  const size_t nibbles = (lambda_bits + 3u) / 4u;

  memset(table, 0, sizeof(table));
  for (unsigned int v = 1; v != 16u; ++v) {
    for (unsigned int bit = 0; bit != 4u; ++bit) {
      if ((v >> bit) & 1u) {
        xor_shifted(table[v], a_words, words, bit);
      }
    }
  }
  for (size_t n = 0; n != nibbles; ++n) {
    const unsigned int v = get_nibble_words(b_words, n);
    if (v != 0u) {
      xor_shifted(product, table[v], table_words, 4u * n);
    }
  }
}

static bool has_high_bits(const uint64_t* x, size_t array_words, size_t lambda_bits) {
  const size_t word = lambda_bits >> 6;
  const unsigned int bit = (unsigned int)(lambda_bits & 63u);
  if (bit == 0u) {
    for (size_t i = word; i != array_words; ++i) {
      if (x[i] != 0u) {
        return true;
      }
    }
    return false;
  }

  if ((x[word] >> bit) != 0u) {
    return true;
  }
  for (size_t i = word + 1u; i != array_words; ++i) {
    if (x[i] != 0u) {
      return true;
    }
  }
  return false;
}

static void clear_high_bits(uint64_t* x, size_t array_words, size_t lambda_bits) {
  const size_t word = lambda_bits >> 6;
  const unsigned int bit = (unsigned int)(lambda_bits & 63u);
  if (bit == 0u) {
    for (size_t i = word; i != array_words; ++i) {
      x[i] = 0;
    }
  } else {
    x[word] &= (UINT64_C(1) << bit) - 1u;
    for (size_t i = word + 1u; i != array_words; ++i) {
      x[i] = 0;
    }
  }
}

static void extract_high_bits(uint64_t* high, const uint64_t* x, size_t array_words,
                              size_t lambda_bits) {
  const size_t word = lambda_bits >> 6;
  const unsigned int bit = (unsigned int)(lambda_bits & 63u);
  const size_t high_words = array_words - word;

  memset(high, 0, 9u * sizeof(high[0]));
  for (size_t i = 0; i != high_words; ++i) {
    const uint64_t lo = x[word + i];
    if (bit == 0u) {
      high[i] = lo;
    } else {
      const uint64_t hi = word + i + 1u < array_words ? x[word + i + 1u] : 0u;
      high[i] = (lo >> bit) | (hi << (64u - bit));
    }
  }
}

static void field_reduce_words(uint64_t* x, size_t array_words, size_t lambda_bits,
                               uint32_t modulus) {
  if (lambda_bits == 512u && array_words >= 17u) {
    (void)modulus;
    field_reduce_512_words(x);
    return;
  }

  for (unsigned int round = 0; round != 4u && has_high_bits(x, array_words, lambda_bits);
       ++round) {
    const size_t word = lambda_bits >> 6;
    const size_t high_words = array_words - word;
    uint64_t high[9];

    extract_high_bits(high, x, array_words, lambda_bits);
    clear_high_bits(x, array_words, lambda_bits);
    for (unsigned int e = 0; e != 32u; ++e) {
      if ((modulus >> e) & 1u) {
        xor_shifted(x, high, high_words, e);
      }
    }
  }
}

uint32_t sydo_ref_field_modulus(const sydo_ref_paramset_t* params) {
  switch (params->secpar_bits) {
  case 160:
    return (1u << 5) | (1u << 4) | (1u << 3) | 1u;
  case 256:
    return (1u << 10) | (1u << 5) | (1u << 2) | 1u;
  case 512:
    return (1u << 8) | (1u << 5) | (1u << 2) | 1u;
  default:
    return 0;
  }
}

void sydo_ref_field_zero(uint8_t* out, size_t len) {
  memset(out, 0, len);
}

void sydo_ref_field_one(uint8_t* out, size_t len) {
  memset(out, 0, len);
  out[0] = 1;
}

void sydo_ref_field_xor(uint8_t* out, const uint8_t* in, size_t len) {
  for (size_t i = 0; i != len; ++i) {
    out[i] ^= in[i];
  }
}

void sydo_ref_field_add(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len) {
  for (size_t i = 0; i != len; ++i) {
    out[i] = (uint8_t)(a[i] ^ b[i]);
  }
}

void sydo_ref_field_mul(uint8_t* out, const uint8_t* a, const uint8_t* b,
                        const sydo_ref_paramset_t* params) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = sydo_ref_ceil_div_size(lambda_bits, 64u);
  const size_t product_words = 2u * words + 1u;
  const uint32_t modulus = sydo_ref_field_modulus(params);
  uint64_t a_words[8] = {0};
  uint64_t b_words[8] = {0};
  uint64_t product[17] = {0};

  for (size_t i = 0; i != words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    a_words[i] = load64_le_local(a + byte_off, left);
    b_words[i] = load64_le_local(b + byte_off, left);
  }
  if ((lambda_bits & 63u) != 0u) {
    const uint64_t mask = (UINT64_C(1) << (lambda_bits & 63u)) - 1u;
    a_words[words - 1u] &= mask;
    b_words[words - 1u] &= mask;
  }

  field_mul_wide_words(product, a_words, b_words, lambda_bits, words);

  (void)product_words;
  field_reduce_words(product, product_words, lambda_bits, modulus);
  for (size_t i = 0; i != words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    store64_le_local(out + byte_off, product[i], left);
  }
}

void sydo_ref_field_mul_bit(uint8_t* out, const uint8_t* a, uint8_t bit, size_t len) {
  const uint8_t mask = (uint8_t)(0u - (bit & 1u));
  for (size_t i = 0; i != len; ++i) {
    out[i] = (uint8_t)(a[i] & mask);
  }
}

bool sydo_ref_field_is_zero(const uint8_t* a, size_t len) {
  uint8_t acc = 0;
  for (size_t i = 0; i != len; ++i) {
    acc |= a[i];
  }
  return acc == 0;
}
