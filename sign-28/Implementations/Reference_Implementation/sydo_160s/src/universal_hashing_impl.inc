/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "uhash.h"

#include "field.h"
#include "internal.h"

#include <string.h>

enum {
  SYDO_REF_GF64_MODULUS = (1u << 4) | (1u << 3) | (1u << 1) | 1u
};

static unsigned int sydo_ref_get_bit(const uint8_t* x, size_t bit) {
  return (unsigned int)((x[bit / 8u] >> (bit & 7u)) & 1u);
}

static void sydo_ref_flip_bit(uint8_t* x, size_t bit) {
  x[bit / 8u] ^= (uint8_t)(1u << (bit & 7u));
}

uint64_t sydo_ref_load64_le(const uint8_t* in) {
  uint64_t v = 0;
  for (size_t i = 0; i != 8u; ++i) {
    v |= (uint64_t)in[i] << (8u * i);
  }
  return v;
}

void sydo_ref_store64_le(uint8_t* out, uint64_t v) {
  for (size_t i = 0; i != 8u; ++i) {
    out[i] = (uint8_t)(v >> (8u * i));
  }
}

static uint64_t load64_partial_le(const uint8_t* in, size_t available) {
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

static void store64_partial_le(uint8_t* out, uint64_t v, size_t available) {
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

static void xor_shifted_local(uint64_t* product, const uint64_t* a, size_t words, size_t shift) {
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

static void field_reduce_512_words_local(uint64_t* x) {
  uint64_t high[9];

  memset(high, 0, sizeof(high));
  memcpy(high, x + 8u, 9u * sizeof(high[0]));
  memset(x + 8u, 0, 9u * sizeof(x[0]));
  xor_shifted_local(x, high, 9u, 0u);
  xor_shifted_local(x, high, 9u, 2u);
  xor_shifted_local(x, high, 9u, 5u);
  xor_shifted_local(x, high, 9u, 8u);

  memset(high, 0, sizeof(high));
  memcpy(high, x + 8u, 9u * sizeof(high[0]));
  memset(x + 8u, 0, 9u * sizeof(x[0]));
  xor_shifted_local(x, high, 9u, 0u);
  xor_shifted_local(x, high, 9u, 2u);
  xor_shifted_local(x, high, 9u, 5u);
  xor_shifted_local(x, high, 9u, 8u);
}

static unsigned int get_nibble_words_local(const uint64_t* x, size_t nibble) {
  const size_t bit = nibble * 4u;
  return (unsigned int)((x[bit >> 6] >> (bit & 63u)) & 0x0fu);
}

static void field_mul_wide_words_local(uint64_t* product, const uint64_t* a_words,
                                       const uint64_t* b_words, size_t lambda_bits,
                                       size_t words) {
  uint64_t table[16][9];
  const size_t table_words = words + 1u;
  const size_t nibbles = (lambda_bits + 3u) / 4u;

  memset(table, 0, sizeof(table));
  for (unsigned int v = 1; v != 16u; ++v) {
    for (unsigned int bit = 0; bit != 4u; ++bit) {
      if ((v >> bit) & 1u) {
        xor_shifted_local(table[v], a_words, words, bit);
      }
    }
  }
  for (size_t n = 0; n != nibbles; ++n) {
    const unsigned int v = get_nibble_words_local(b_words, n);
    if (v != 0u) {
      xor_shifted_local(product, table[v], table_words, 4u * n);
    }
  }
}

static bool has_high_bits_local(const uint64_t* x, size_t array_words, size_t lambda_bits) {
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

static void clear_high_bits_local(uint64_t* x, size_t array_words, size_t lambda_bits) {
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

static void extract_high_bits_local(uint64_t* high, const uint64_t* x, size_t array_words,
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

static void field_reduce_words_local(uint64_t* x, size_t array_words, size_t lambda_bits,
                                     uint32_t modulus) {
  if (lambda_bits == 512u && array_words >= 17u) {
    (void)modulus;
    field_reduce_512_words_local(x);
    return;
  }

  for (unsigned int round = 0; round != 4u && has_high_bits_local(x, array_words, lambda_bits);
       ++round) {
    const size_t word = lambda_bits >> 6;
    const size_t high_words = array_words - word;
    uint64_t high[9];

    extract_high_bits_local(high, x, array_words, lambda_bits);
    clear_high_bits_local(x, array_words, lambda_bits);
    for (unsigned int e = 0; e != 32u; ++e) {
      if ((modulus >> e) & 1u) {
        xor_shifted_local(x, high, high_words, e);
      }
    }
  }
}

void sydo_ref_field_mul_wide(uint8_t* out, const uint8_t* a, const uint8_t* b,
                             const sydo_ref_paramset_t* params) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = sydo_ref_ceil_div_size(lambda_bits, 64u);
  uint64_t a_words[8] = {0};
  uint64_t b_words[8] = {0};
  uint64_t product[17] = {0};

  memset(out, 0, 2u * lambda_bytes);
  for (size_t i = 0; i != words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    a_words[i] = load64_partial_le(a + byte_off, left);
    b_words[i] = load64_partial_le(b + byte_off, left);
  }
  if ((lambda_bits & 63u) != 0u) {
    const uint64_t mask = (UINT64_C(1) << (lambda_bits & 63u)) - 1u;
    a_words[words - 1u] &= mask;
    b_words[words - 1u] &= mask;
  }

  field_mul_wide_words_local(product, a_words, b_words, lambda_bits, words);
  for (size_t i = 0; i != 2u * words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = 2u * lambda_bytes > byte_off ? 2u * lambda_bytes - byte_off : 0u;
    store64_partial_le(out + byte_off, product[i], left);
  }
}

void sydo_ref_field_reduce_wide(uint8_t* out, const uint8_t* in,
                                const sydo_ref_paramset_t* params) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = 2u * sydo_ref_ceil_div_size(lambda_bits, 64u) + 1u;
  const uint32_t modulus = sydo_ref_field_modulus(params);
  uint64_t tmp[17] = {0};

  for (size_t i = 0; i != words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = 2u * lambda_bytes > byte_off ? 2u * lambda_bytes - byte_off : 0u;
    tmp[i] = left != 0u ? load64_partial_le(in + byte_off, left) : 0u;
  }
  field_reduce_words_local(tmp, words, lambda_bits, modulus);
  for (size_t i = 0; i != sydo_ref_ceil_div_size(lambda_bits, 64u); ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    store64_partial_le(out + byte_off, tmp[i], left);
  }
}

void sydo_ref_field_muladd_wide(uint8_t* acc, const uint8_t* a, const uint8_t* b,
                                const sydo_ref_paramset_t* params) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = sydo_ref_ceil_div_size(lambda_bits, 64u);
  uint64_t a_words[8] = {0};
  uint64_t b_words[8] = {0};
  uint64_t product[17] = {0};

  for (size_t i = 0; i != words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    a_words[i] = load64_partial_le(a + byte_off, left);
    b_words[i] = load64_partial_le(b + byte_off, left);
  }
  if ((lambda_bits & 63u) != 0u) {
    const uint64_t mask = (UINT64_C(1) << (lambda_bits & 63u)) - 1u;
    a_words[words - 1u] &= mask;
    b_words[words - 1u] &= mask;
  }

  field_mul_wide_words_local(product, a_words, b_words, lambda_bits, words);
  for (size_t i = 0; i != 2u * words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = 2u * lambda_bytes > byte_off ? 2u * lambda_bytes - byte_off : 0u;
    const uint64_t cur = load64_partial_le(acc + byte_off, left);
    store64_partial_le(acc + byte_off, cur ^ product[i], left);
  }
}

void sydo_ref_field_muladd(uint8_t* acc, const uint8_t* a, const uint8_t* b,
                           const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  uint8_t product[64];
  sydo_ref_field_mul(product, a, b, params);
  for (size_t i = 0; i != lambda_bytes; ++i) {
    acc[i] ^= product[i];
  }
}

static uint64_t gf64_mul_wide_low(uint64_t a, uint64_t b, uint64_t* high) {
  uint64_t lo = 0;
  uint64_t hi = 0;
  for (unsigned int i = 0; i != 64u; ++i) {
    if (((a >> i) & 1u) == 0) {
      continue;
    }
    if (i == 0) {
      lo ^= b;
    } else {
      lo ^= b << i;
      hi ^= b >> (64u - i);
    }
  }
  *high = hi;
  return lo;
}

static uint64_t gf64_reduce(uint64_t lo, uint64_t hi) {
  uint8_t tmp[16];
  memset(tmp, 0, sizeof(tmp));
  sydo_ref_store64_le(tmp, lo);
  sydo_ref_store64_le(tmp + 8, hi);
  for (size_t bit = 127u; bit-- > 64u;) {
    if (!sydo_ref_get_bit(tmp, bit)) {
      continue;
    }
    sydo_ref_flip_bit(tmp, bit);
    const size_t shift = bit - 64u;
    for (unsigned int e = 0; e != 32u; ++e) {
      if ((SYDO_REF_GF64_MODULUS >> e) & 1u) {
        sydo_ref_flip_bit(tmp, shift + e);
      }
    }
  }
  return sydo_ref_load64_le(tmp);
}

static uint64_t gf64_mul(uint64_t a, uint64_t b) {
  uint64_t hi = 0;
  const uint64_t lo = gf64_mul_wide_low(a, b, &hi);
  return gf64_reduce(lo, hi);
}

static uint64_t gf64_mul_a64_reduce64(uint64_t x) {
  uint64_t hi = x;
  uint64_t lo = 0;
  return gf64_reduce(lo, hi);
}

void sydo_ref_uhash_secpar_key_init(sydo_ref_uhash_secpar_key_t* key,
                                    const uint8_t* key_bytes,
                                    const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  memset(key, 0, sizeof(*key));
  memcpy(key->key_pows[0], key_bytes, lambda_bytes);
  sydo_ref_field_mul(key->key_pows[1], key->key_pows[0], key->key_pows[0], params);
}

void sydo_ref_uhash_secpar_state_init(sydo_ref_uhash_secpar_state_t* state,
                                      size_t num_coefficients) {
  memset(state, 0, sizeof(*state));
  state->pow = (int)((num_coefficients + 1u) % 2u);
  state->coefficients = num_coefficients;
}

void sydo_ref_uhash_secpar_update(sydo_ref_uhash_secpar_state_t* state,
                                  const sydo_ref_uhash_secpar_key_t* key,
                                  const uint8_t* input,
                                  const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  uint8_t reduced[64];
  if (state->pow == -1) {
    sydo_ref_field_reduce_wide(reduced, state->state, params);
    memset(state->state, 0, sizeof(state->state));
    sydo_ref_field_mul_wide(state->state, key->key_pows[1], reduced, params);
    state->pow = 1;
  }
  if (state->pow > 0) {
    sydo_ref_field_muladd_wide(state->state, key->key_pows[(size_t)state->pow - 1u], input,
                               params);
  } else {
    for (size_t i = 0; i != lambda_bytes; ++i) {
      state->state[i] ^= input[i];
    }
  }
  --state->pow;
}

void sydo_ref_uhash_secpar_finalize(const sydo_ref_uhash_secpar_state_t* state, uint8_t* out,
                                    const sydo_ref_paramset_t* params) {
  sydo_ref_field_reduce_wide(out, state->state, params);
}

void sydo_ref_uhash_secpar64_key_init(sydo_ref_uhash_secpar64_key_t* key, uint64_t key64) {
  key->key = key64;
}

void sydo_ref_uhash_secpar64_state_init(sydo_ref_uhash_secpar64_state_t* state,
                                        size_t num_coefficients) {
  (void)num_coefficients;
  memset(state, 0, sizeof(*state));
}

void sydo_ref_uhash_secpar64_update(sydo_ref_uhash_secpar64_state_t* state,
                                    const sydo_ref_uhash_secpar64_key_t* key,
                                    const uint8_t* input,
                                    const sydo_ref_paramset_t* params) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = sydo_ref_ceil_div_size(lambda_bits, 64u);
  const size_t product_words = words + 2u;
  uint64_t state_words[8] = {0};
  uint64_t table[16][9];
  uint64_t product[10] = {0};

  memset(table, 0, sizeof(table));
  for (size_t i = 0; i != words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    state_words[i] = load64_partial_le(state->state + byte_off, left);
  }
  if ((lambda_bits & 63u) != 0u) {
    state_words[words - 1u] &= (UINT64_C(1) << (lambda_bits & 63u)) - 1u;
  }

  for (unsigned int v = 1; v != 16u; ++v) {
    for (unsigned int bit = 0; bit != 4u; ++bit) {
      if ((v >> bit) & 1u) {
        xor_shifted_local(table[v], state_words, words, bit);
      }
    }
  }
  for (unsigned int n = 0; n != 16u; ++n) {
    const unsigned int v = (unsigned int)((key->key >> (4u * n)) & 0x0fu);
    if (v != 0u) {
      xor_shifted_local(product, table[v], words + 1u, 4u * n);
    }
  }

  field_reduce_words_local(product, product_words, lambda_bits, sydo_ref_field_modulus(params));
  for (size_t i = 0; i != words; ++i) {
    uint8_t tmp[8];
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    store64_partial_le(tmp, product[i], left);
    for (size_t j = 0; j != left; ++j) {
      state->state[byte_off + j] = (uint8_t)(tmp[j] ^ input[byte_off + j]);
    }
  }
}

void sydo_ref_uhash_secpar64_finalize(const sydo_ref_uhash_secpar64_state_t* state,
                                      uint8_t* out, const sydo_ref_paramset_t* params) {
  memcpy(out, state->state, sydo_ref_secpar_bytes(params));
}

void sydo_ref_uhash64_key_init(sydo_ref_uhash64_key_t* key, uint64_t key64) {
  key->key_pows[0] = key64;
  key->key_pows[1] = gf64_mul(key64, key64);
  key->key_pow_times_a64 = gf64_mul_a64_reduce64(key->key_pows[1]);
}

void sydo_ref_uhash64_state_init(sydo_ref_uhash64_state_t* state, size_t num_coefficients) {
  memset(state, 0, sizeof(*state));
  state->pow = (int)((num_coefficients + 1u) % 2u);
}

void sydo_ref_uhash64_update(sydo_ref_uhash64_state_t* state,
                             const sydo_ref_uhash64_key_t* key, uint64_t input) {
  uint64_t lo = sydo_ref_load64_le(state->state);
  uint64_t hi = sydo_ref_load64_le(state->state + 8);
  uint64_t prod_hi = 0;
  uint64_t prod_lo = 0;
  if (state->pow == -1) {
    const uint64_t reduced = gf64_reduce(lo, hi);
    prod_lo = gf64_mul_wide_low(key->key_pows[1], reduced, &prod_hi);
    lo = prod_lo;
    hi = prod_hi;
    state->pow = 1;
  }
  if (state->pow > 0) {
    prod_lo = gf64_mul_wide_low(key->key_pows[(size_t)state->pow - 1u], input, &prod_hi);
    lo ^= prod_lo;
    hi ^= prod_hi;
  } else {
    lo ^= input;
  }
  sydo_ref_store64_le(state->state, lo);
  sydo_ref_store64_le(state->state + 8, hi);
  --state->pow;
}

uint64_t sydo_ref_uhash64_finalize(const sydo_ref_uhash64_state_t* state) {
  return gf64_reduce(sydo_ref_load64_le(state->state), sydo_ref_load64_le(state->state + 8));
}
