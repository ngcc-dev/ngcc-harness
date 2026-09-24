/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "quicksilver.h"

#include "aes.h"
#include "compat.h"
#include "lib/blake2/ref/blake2.h"
#include "field.h"
#include "internal.h"
#include "primitives.h"
#include "uhash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool qs_trace_enabled(void) {
  return getenv("SYDO_REF_TRACE") != NULL;
}

static clock_t qs_trace_clock(void) {
  return clock();
}

static void qs_trace_stage(const sydo_ref_paramset_t* params, const char* stage, clock_t start) {
  if (qs_trace_enabled()) {
    const double ms = 1000.0 * (double)(clock() - start) / (double)CLOCKS_PER_SEC;
    fprintf(stderr, "%s qs %s %.2f ms\n", params->name, stage, ms);
  }
}

static unsigned int get_bit(const uint8_t* x, size_t bit) {
  return (unsigned int)((x[bit / 8u] >> (bit & 7u)) & 1u);
}

static void flip_bit(uint8_t* x, size_t bit) {
  x[bit / 8u] ^= (uint8_t)(1u << (bit & 7u));
}

static void reduce_2lambda(uint8_t* out, const uint8_t* in, const sydo_ref_paramset_t* params) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const uint32_t modulus = sydo_ref_field_modulus(params);
  uint8_t tmp[128];

  memset(tmp, 0, sizeof(tmp));
  memcpy(tmp, in, 2u * lambda_bytes);
  for (size_t bit = 2u * lambda_bits - 1u; bit-- > lambda_bits;) {
    if (!get_bit(tmp, bit)) {
      continue;
    }
    flip_bit(tmp, bit);
    const size_t shift = bit - lambda_bits;
    for (unsigned int e = 0; e != 32u; ++e) {
      if ((modulus >> e) & 1u) {
        flip_bit(tmp, shift + e);
      }
    }
  }
  memcpy(out, tmp, lambda_bytes);
}

static void combine_mac_masks_reduced(const sydo_ref_paramset_t* params, const uint8_t* macs,
                                      size_t bit_offset, uint8_t* out) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  uint8_t acc[128];

  memset(acc, 0, sizeof(acc));
  for (size_t i = 0; i != lambda_bits; ++i) {
    const uint8_t* mac = macs + (bit_offset + i) * lambda_bytes;
    for (size_t bit = 0; bit != lambda_bits; ++bit) {
      if (get_bit(mac, bit)) {
        flip_bit(acc, i + bit);
      }
    }
  }
  reduce_2lambda(out, acc, params);
}

static void combine_mac_masks_wide(const sydo_ref_paramset_t* params, const uint8_t* macs,
                                   size_t bit_offset, uint8_t* out) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);

  memset(out, 0, 2u * lambda_bytes);
  for (size_t i = 0; i != lambda_bits; ++i) {
    const uint8_t* mac = macs + (bit_offset + i) * lambda_bytes;
    for (size_t bit = 0; bit != lambda_bits; ++bit) {
      if (get_bit(mac, bit)) {
        flip_bit(out, i + bit);
      }
    }
  }
}

static void field_mul_inplace(uint8_t* out, const uint8_t* a, const uint8_t* b,
                              const sydo_ref_paramset_t* params) {
  uint8_t tmp[64];
  sydo_ref_field_mul(tmp, a, b, params);
  memcpy(out, tmp, sydo_ref_secpar_bytes(params));
}

typedef struct rsd_row_cache_t {
  uint8_t c0[SYDO_REF_RSD_BLOCK0_SIZE][64];
  uint8_t c1[SYDO_REF_RSD_BLOCK1_SIZE][64];
  uint8_t a0[SYDO_REF_RSD_BLOCK0_SIZE];
  uint8_t a1[SYDO_REF_RSD_BLOCK1_SIZE];
  uint8_t t2[28][64];
  uint8_t t3[56][64];
  uint8_t u2[28];
  uint8_t u3[56];
} rsd_row_cache_t;

static void field_mul_to(uint8_t* out, const uint8_t* a, const uint8_t* b,
                         const sydo_ref_paramset_t* params);

static size_t ncr_local(size_t n, size_t r) {
  size_t res = 1;
  if (r > n) {
    return 0;
  }
  if (r > n - r) {
    r = n - r;
  }
  for (size_t i = 1; i <= r; ++i) {
    res = (res * (n - r + i)) / i;
  }
  return res;
}

static size_t bit_count_size(size_t x) {
  size_t c = 0;
  while (x != 0u) {
    c += x & 1u;
    x >>= 1;
  }
  return c;
}

typedef struct fixed_field_mul_t {
  uint64_t table[256][9];
  size_t words;
  size_t lambda_bits;
  size_t lambda_bytes;
} fixed_field_mul_t;

static uint64_t load64_partial_qs(const uint8_t* in, size_t available) {
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

static void store64_partial_qs(uint8_t* out, uint64_t v, size_t available) {
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

static void xor_shifted_qs(uint64_t* product, const uint64_t* a, size_t words, size_t shift) {
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

static void fixed_field_mul_init(fixed_field_mul_t* ctx, const uint8_t* a,
                                 const sydo_ref_paramset_t* params) {
  uint64_t a_words[8] = {0};
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = sydo_ref_ceil_div_size(lambda_bits, 64u);

  memset(ctx, 0, sizeof(*ctx));
  ctx->words = words;
  ctx->lambda_bits = lambda_bits;
  ctx->lambda_bytes = lambda_bytes;
  for (size_t i = 0; i != words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
    a_words[i] = load64_partial_qs(a + byte_off, left);
  }
  if ((lambda_bits & 63u) != 0u) {
    a_words[words - 1u] &= (UINT64_C(1) << (lambda_bits & 63u)) - 1u;
  }
  for (unsigned int v = 1; v != 256u; ++v) {
    for (unsigned int bit = 0; bit != 8u; ++bit) {
      if ((v >> bit) & 1u) {
        xor_shifted_qs(ctx->table[v], a_words, words, bit);
      }
    }
  }
}

static void fixed_field_muladd_wide(uint8_t* acc, const fixed_field_mul_t* ctx,
                                    const uint8_t* b) {
  uint64_t product[17] = {0};
  const size_t words = ctx->words;
  const size_t lambda_bits = ctx->lambda_bits;
  const size_t lambda_bytes = ctx->lambda_bytes;
  const size_t full_bytes = lambda_bits / 8u;
  const unsigned int tail_bits = (unsigned int)(lambda_bits & 7u);

  for (size_t n = 0; n != full_bytes; ++n) {
    const unsigned int v = b[n];
    if (v != 0u) {
      xor_shifted_qs(product, ctx->table[v], words + 1u, 8u * n);
    }
  }
  if (tail_bits != 0u) {
    const unsigned int v = b[full_bytes] & ((1u << tail_bits) - 1u);
    if (v != 0u) {
      xor_shifted_qs(product, ctx->table[v], words + 1u, 8u * full_bytes);
    }
  }
  for (size_t i = 0; i != 2u * words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = 2u * lambda_bytes > byte_off ? 2u * lambda_bytes - byte_off : 0u;
    const uint64_t cur = load64_partial_qs(acc + byte_off, left);
    store64_partial_qs(acc + byte_off, cur ^ product[i], left);
  }
}

static void fixed_field_muladd_words(uint64_t* acc, const fixed_field_mul_t* ctx,
                                     const uint8_t* b) {
  uint64_t product[17] = {0};
  const size_t words = ctx->words;
  const size_t lambda_bits = ctx->lambda_bits;
  const size_t full_bytes = lambda_bits / 8u;
  const unsigned int tail_bits = (unsigned int)(lambda_bits & 7u);

  for (size_t n = 0; n != full_bytes; ++n) {
    const unsigned int v = b[n];
    if (v != 0u) {
      xor_shifted_qs(product, ctx->table[v], words + 1u, 8u * n);
    }
  }
  if (tail_bits != 0u) {
    const unsigned int v = b[full_bytes] & ((1u << tail_bits) - 1u);
    if (v != 0u) {
      xor_shifted_qs(product, ctx->table[v], words + 1u, 8u * full_bytes);
    }
  }
  for (size_t i = 0; i != 2u * words; ++i) {
    acc[i] ^= product[i];
  }
}

static void wide_words_to_bytes(uint8_t* out, const uint64_t* in,
                                const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = sydo_ref_ceil_div_size(params->secpar_bits, 64u);
  for (size_t i = 0; i != 2u * words; ++i) {
    const size_t byte_off = i * 8u;
    const size_t left = 2u * lambda_bytes > byte_off ? 2u * lambda_bytes - byte_off : 0u;
    store64_partial_qs(out + byte_off, in[i], left);
  }
}

static size_t hamming_ball_count_local(size_t n, size_t r) {
  size_t total = 0;
  for (size_t w = 0; w <= r; ++w) {
    total += ncr_local(n, w);
  }
  return total;
}

bool sydo_ref_qs_prove_mask_only(const sydo_ref_paramset_t* params, const uint8_t* witness_ext,
                                 const uint8_t* macs_extended, uint8_t* proof,
                                 uint8_t* check) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t witness_bits = sydo_ref_witness_extended_bytes(params) * 8u;
  uint8_t coeffs[SYDO_REF_RSD_QS_DEGREE][64];

  memset(coeffs, 0, sizeof(coeffs));
  for (size_t i = 0; i != SYDO_REF_RSD_QS_DEGREE - 1u; ++i) {
    combine_mac_masks_reduced(params, macs_extended, witness_bits + i * params->secpar_bits,
                              coeffs[i]);
    memcpy(coeffs[i + 1], witness_ext + witness_bits / 8u + i * lambda_bytes, lambda_bytes);
  }

  memcpy(check, coeffs[0], lambda_bytes);
  for (size_t i = 1; i != SYDO_REF_RSD_QS_DEGREE; ++i) {
    memcpy(proof + (i - 1u) * lambda_bytes, coeffs[i], lambda_bytes);
  }
  return true;
}

bool sydo_ref_qs_verify_mask_only(const sydo_ref_paramset_t* params, const uint8_t* delta,
                                  const uint8_t* macs_extended, const uint8_t* proof,
                                  uint8_t* check) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t witness_bits = sydo_ref_witness_extended_bytes(params) * 8u;
  uint8_t acc[64];
  uint8_t term[64];
  uint8_t delta_power[64];

  combine_mac_masks_reduced(params, macs_extended, witness_bits, acc);
  memcpy(delta_power, delta, lambda_bytes);

  for (size_t i = 1; i != SYDO_REF_RSD_QS_DEGREE - 1u; ++i) {
    combine_mac_masks_reduced(params, macs_extended, witness_bits + i * params->secpar_bits,
                              term);
    for (size_t j = 0; j != lambda_bytes; ++j) {
      term[j] ^= proof[(i - 1u) * lambda_bytes + j];
    }
    field_mul_inplace(term, term, delta_power, params);
    for (size_t j = 0; j != lambda_bytes; ++j) {
      acc[j] ^= term[j];
    }
    field_mul_inplace(delta_power, delta_power, delta, params);
  }

  memcpy(term, proof + (SYDO_REF_RSD_QS_DEGREE - 2u) * lambda_bytes, lambda_bytes);
  field_mul_inplace(term, term, delta_power, params);
  for (size_t j = 0; j != lambda_bytes; ++j) {
    acc[j] ^= term[j];
  }
  memcpy(check, acc, lambda_bytes);
  return true;
}

typedef struct qs_poly_t {
  uint8_t coeff[SYDO_REF_RSD_QS_DEGREE + 1u][64];
} qs_poly_t;

typedef struct qs_hash_state_t {
  sydo_ref_uhash_secpar_key_t key_s;
  sydo_ref_uhash_secpar64_key_t key_64;
  sydo_ref_uhash_secpar_state_t state_s[SYDO_REF_RSD_QS_DEGREE];
  sydo_ref_uhash_secpar64_state_t state_64[SYDO_REF_RSD_QS_DEGREE];
  uint8_t combination[2][64];
} qs_hash_state_t;

static void qs_poly_zero(qs_poly_t* p) {
  memset(p, 0, sizeof(*p));
}

static void qs_poly_const(qs_poly_t* p, const uint8_t* c, const sydo_ref_paramset_t* params) {
  qs_poly_zero(p);
  memcpy(p->coeff[0], c, sydo_ref_secpar_bytes(params));
}

static void qs_poly_bit(qs_poly_t* p, const uint8_t* macs, size_t row, unsigned int bit,
                        const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  qs_poly_zero(p);
  memcpy(p->coeff[0], macs + row * lambda_bytes, lambda_bytes);
  p->coeff[1][0] = (uint8_t)(bit & 1u);
}

static void qs_poly_eval_var(qs_poly_t* p, const uint8_t* macs, size_t row,
                             const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  qs_poly_zero(p);
  memcpy(p->coeff[0], macs + row * lambda_bytes, lambda_bytes);
}

static void qs_poly_add(qs_poly_t* out, const qs_poly_t* in, const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  for (size_t d = 0; d != SYDO_REF_RSD_QS_DEGREE + 1u; ++d) {
    for (size_t i = 0; i != lambda_bytes; ++i) {
      out->coeff[d][i] ^= in->coeff[d][i];
    }
  }
}

static void qs_poly_mul(qs_poly_t* out, const qs_poly_t* a, const qs_poly_t* b,
                        const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  qs_poly_t r;
  qs_poly_zero(&r);
  for (size_t i = 0; i != SYDO_REF_RSD_QS_DEGREE + 1u; ++i) {
    for (size_t j = 0; j + i != SYDO_REF_RSD_QS_DEGREE + 1u; ++j) {
      sydo_ref_field_muladd(r.coeff[i + j], a->coeff[i], b->coeff[j], params);
    }
  }
  memcpy(out, &r, sizeof(r));
  (void)lambda_bytes;
}

static void qs_poly_mul_const(qs_poly_t* out, const qs_poly_t* a, const uint8_t* c,
                              const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  qs_poly_zero(out);
  for (size_t d = 0; d != SYDO_REF_RSD_QS_DEGREE + 1u; ++d) {
    sydo_ref_field_mul(out->coeff[d], a->coeff[d], c, params);
  }
  (void)lambda_bytes;
}

static bool qs_hash_init(qs_hash_state_t* st, const sydo_ref_paramset_t* params,
                         const uint8_t* challenge) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t num_constraints =
      (size_t)params->rsd_w *
          (ncr_local(SYDO_REF_RSD_BLOCK0_SIZE, SYDO_REF_RSD_BLOCK0_WEIGHT + 1u) +
           ncr_local(SYDO_REF_RSD_BLOCK1_SIZE, SYDO_REF_RSD_BLOCK1_WEIGHT + 1u)) +
      SYDO_REF_RSD_Y_VECTOR_LEN;

  memset(st, 0, sizeof(*st));
  memcpy(st->combination[0], challenge, lambda_bytes);
  memcpy(st->combination[1], challenge + lambda_bytes, lambda_bytes);
  sydo_ref_uhash_secpar_key_init(&st->key_s, challenge + 2u * lambda_bytes, params);
  sydo_ref_uhash_secpar64_key_init(
      &st->key_64, sydo_ref_load64_le(challenge + 3u * lambda_bytes));
  for (size_t i = 0; i != SYDO_REF_RSD_QS_DEGREE; ++i) {
    sydo_ref_uhash_secpar_state_init(&st->state_s[i], num_constraints);
    sydo_ref_uhash_secpar64_state_init(&st->state_64[i], num_constraints);
  }
  return true;
}

static void qs_hash_add_coeff(qs_hash_state_t* st, size_t idx, const uint8_t* coeff,
                              const sydo_ref_paramset_t* params) {
  sydo_ref_uhash_secpar_update(&st->state_s[idx], &st->key_s, coeff, params);
  sydo_ref_uhash_secpar64_update(&st->state_64[idx], &st->key_64, coeff, params);
}

static void qs_hash_add_constraint_coeffs_prover(qs_hash_state_t* st, uint8_t coeffs[][64],
                                                 size_t degree,
                                                 const sydo_ref_paramset_t* params) {
  const size_t shift = SYDO_REF_RSD_QS_DEGREE - degree;
  uint8_t zero[64];
  memset(zero, 0, sizeof(zero));
  for (size_t i = 0; i != SYDO_REF_RSD_QS_DEGREE; ++i) {
    qs_hash_add_coeff(st, i, i >= shift ? coeffs[i - shift] : zero, params);
  }
}

static void qs_hash_add_constraint_verifier(qs_hash_state_t* st, const uint8_t* eval,
                                            size_t degree, uint8_t delta_powers[][64],
                                            const sydo_ref_paramset_t* params) {
  uint8_t scaled[64];
  const size_t shift = SYDO_REF_RSD_QS_DEGREE - degree;
  if (shift == 0u) {
    qs_hash_add_coeff(st, 0, eval, params);
    return;
  }
  sydo_ref_field_mul(scaled, eval, delta_powers[shift - 1u], params);
  qs_hash_add_coeff(st, 0, scaled, params);
}

static void qs_hash_finalize_one(qs_hash_state_t* st, size_t idx, const uint8_t* mask,
                                 const sydo_ref_paramset_t* params, uint8_t* out) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  uint8_t h0[64];
  uint8_t h1[64];
  uint8_t acc[128];
  memset(acc, 0, sizeof(acc));
  memcpy(acc, mask, 2u * lambda_bytes);
  sydo_ref_uhash_secpar_finalize(&st->state_s[idx], h0, params);
  sydo_ref_uhash_secpar64_finalize(&st->state_64[idx], h1, params);
  sydo_ref_field_muladd_wide(acc, st->combination[0], h0, params);
  sydo_ref_field_muladd_wide(acc, st->combination[1], h1, params);
  sydo_ref_field_reduce_wide(out, acc, params);
}

static void qs_make_mask_prover(const sydo_ref_paramset_t* params, const uint8_t* witness_ext,
                                const uint8_t* macs_extended, uint8_t masks[][128]) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t witness_bits = sydo_ref_witness_extended_bytes(params) * 8u;
  memset(masks, 0, SYDO_REF_RSD_QS_DEGREE * 128u);
  for (size_t i = 0; i != SYDO_REF_RSD_QS_DEGREE - 1u; ++i) {
    uint8_t mask_mac[128];
    combine_mac_masks_wide(params, macs_extended, witness_bits + i * params->secpar_bits,
                           mask_mac);
    for (size_t j = 0; j != 2u * lambda_bytes; ++j) {
      masks[i][j] ^= mask_mac[j];
    }
    for (size_t j = 0; j != lambda_bytes; ++j) {
      masks[i + 1u][j] ^= witness_ext[witness_bits / 8u + i * lambda_bytes + j];
    }
  }
}

static void qs_make_mask_verifier(const sydo_ref_paramset_t* params, const uint8_t* delta,
                                  const uint8_t* macs_extended, const uint8_t* proof,
                                  uint8_t mask[128]) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t witness_bits = sydo_ref_witness_extended_bytes(params) * 8u;
  uint8_t acc[128];
  uint8_t wide[128];
  uint8_t term[64];
  uint8_t delta_power[64];
  memset(mask, 0, 128u);
  combine_mac_masks_wide(params, macs_extended, witness_bits, acc);
  memcpy(delta_power, delta, lambda_bytes);
  for (size_t i = 1; i != SYDO_REF_RSD_QS_DEGREE - 1u; ++i) {
    combine_mac_masks_reduced(params, macs_extended, witness_bits + i * params->secpar_bits,
                              term);
    for (size_t j = 0; j != lambda_bytes; ++j) {
      term[j] ^= proof[(i - 1u) * lambda_bytes + j];
    }
    sydo_ref_field_mul_wide(wide, term, delta_power, params);
    for (size_t j = 0; j != 2u * lambda_bytes; ++j) {
      acc[j] ^= wide[j];
    }
    field_mul_inplace(delta_power, delta_power, delta, params);
  }
  memcpy(term, proof + (SYDO_REF_RSD_QS_DEGREE - 2u) * lambda_bytes, lambda_bytes);
  sydo_ref_field_mul_wide(wide, term, delta_power, params);
  for (size_t j = 0; j != 2u * lambda_bytes; ++j) {
    acc[j] ^= wide[j];
  }
  memcpy(mask, acc, 2u * lambda_bytes);
}

typedef struct matrix_prg_ctx_t {
  const sydo_ref_paramset_t* params;
  const uint8_t* seed_pk;
  aes_round_keys_t round_keys;
} matrix_prg_ctx_t;

static bool matrix_prg_init(matrix_prg_ctx_t* ctx, const sydo_ref_paramset_t* params,
                            const uint8_t* seed_pk) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->params = params;
  ctx->seed_pk = seed_pk;
  if (params->secpar_bits == 160) {
    uint8_t padded_key[24] = {0};
    memcpy(padded_key, seed_pk, 20u);
    aes192_init_round_keys(&ctx->round_keys, padded_key);
    sydo_explicit_bzero(padded_key, sizeof(padded_key));
    return true;
  }
  if (params->secpar_bits == 256) {
    aes256_init_round_keys(&ctx->round_keys, seed_pk);
    return true;
  }
  if (params->secpar_bits == 512) {
    return true;
  }
  return false;
}

static bool prg_block_128(const matrix_prg_ctx_t* ctx, uint32_t counter, uint8_t out[16]) {
  uint8_t input[16] = {0};
  const sydo_ref_paramset_t* params = ctx->params;
  input[0] = (uint8_t)counter;
  input[1] = (uint8_t)(counter >> 8);
  input[2] = (uint8_t)(counter >> 16);
  input[3] = (uint8_t)(counter >> 24);
  if (params->secpar_bits == 160) {
    aes192_encrypt_block(&ctx->round_keys, input, out);
    return true;
  }
  if (params->secpar_bits == 256) {
    aes256_encrypt_block(&ctx->round_keys, input, out);
    return true;
  }
  return false;
}

static bool matrix_element_prg(const matrix_prg_ctx_t* ctx, uint8_t* out, size_t elem_index) {
  const sydo_ref_paramset_t* params = ctx->params;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  if (params->secpar_bits == 512) {
    uint8_t input[64] = {0};
    input[0] = (uint8_t)elem_index;
    input[1] = (uint8_t)(elem_index >> 8);
    input[2] = (uint8_t)(elem_index >> 16);
    input[3] = (uint8_t)(elem_index >> 24);
    return sydo_ref_blake2s_512_bc_eval(out, ctx->seed_pk, input);
  }
  uint8_t block[16];
  const size_t blocks_per_elem = (lambda_bytes + 15u) / 16u;
  size_t copied = 0;
  for (size_t block_idx = 0; block_idx != blocks_per_elem; ++block_idx) {
    if (!prg_block_128(ctx, (uint32_t)(elem_index * blocks_per_elem + block_idx), block)) {
      return false;
    }
    const size_t take = lambda_bytes - copied < sizeof(block) ? lambda_bytes - copied
                                                              : sizeof(block);
    memcpy(out + copied, block, take);
    copied += take;
  }
  return true;
}

static void mask_public_y_row(const sydo_ref_paramset_t* params, size_t row, uint8_t* y) {
  const size_t lambda_bits = params->secpar_bits;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t start = row * lambda_bits;
  if (start + lambda_bits <= params->rsd_codim) {
    return;
  }
  if (start >= params->rsd_codim) {
    memset(y, 0, lambda_bytes);
    return;
  }
  const size_t used = params->rsd_codim - start;
  const size_t used_bytes = (used + 7u) / 8u;
  if ((used & 7u) != 0u) {
    y[used_bytes - 1u] &= (uint8_t)((1u << (used & 7u)) - 1u);
  }
  if (used_bytes < lambda_bytes) {
    memset(y + used_bytes, 0, lambda_bytes - used_bytes);
  }
}

static bool fold_public_data(const sydo_ref_paramset_t* params, const uint8_t* pk,
                             const uint8_t* challenge, uint8_t* folded_h, uint8_t* folded_y) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t words = sydo_ref_ceil_div_size(params->secpar_bits, 64u);
  const size_t wide_words = 2u * words;
  const size_t wide_bytes = wide_words * sizeof(uint64_t);
  const size_t rows = SYDO_REF_RSD_Y_VECTOR_LEN;
  const size_t cols = params->rsd_n;
  const size_t base = (3u * (size_t)params->secpar_bits + 64u) / 8u;
  const uint8_t* seed_pk = pk;
  const uint8_t* y_packed = pk + lambda_bytes;
  uint8_t h[64];
  uint8_t y_acc[128];
  uint8_t alpha[SYDO_REF_RSD_Y_VECTOR_LEN][64];
  fixed_field_mul_t alpha_mul[SYDO_REF_RSD_Y_VECTOR_LEN];
  uint8_t y_row[64];
  matrix_prg_ctx_t matrix_ctx;
  uint64_t* h_acc_all;

  if (!matrix_prg_init(&matrix_ctx, params, seed_pk)) {
    return false;
  }
  h_acc_all = (uint64_t*)calloc(cols, wide_bytes);
  if (!h_acc_all) {
    return false;
  }
  memset(folded_h, 0, cols * lambda_bytes);
  memset(y_acc, 0, sizeof(y_acc));
  for (size_t row = 0; row != rows; ++row) {
    memset(alpha[row], 0, sizeof(alpha[row]));
    memcpy(alpha[row], challenge + base + row * lambda_bytes, lambda_bytes);
    fixed_field_mul_init(&alpha_mul[row], alpha[row], params);
    memset(y_row, 0, sizeof(y_row));
    for (size_t i = 0; i != lambda_bytes; ++i) {
      const size_t packed_idx = row * lambda_bytes + i;
      if (packed_idx < params->public_key_size - lambda_bytes) {
        y_row[i] = y_packed[packed_idx];
      }
    }
    mask_public_y_row(params, row, y_row);
    fixed_field_muladd_wide(y_acc, &alpha_mul[row], y_row);
  }
  sydo_ref_field_reduce_wide(folded_y, y_acc, params);
  for (size_t row = 0; row != rows; ++row) {
    for (size_t col = 0; col != cols; ++col) {
      const size_t elem = row * cols + col;
      if (!matrix_element_prg(&matrix_ctx, h, elem)) {
        sydo_explicit_bzero(h_acc_all, cols * wide_bytes);
        free(h_acc_all);
        return false;
      }
      mask_public_y_row(params, row, h);
      fixed_field_muladd_words(h_acc_all + col * wide_words, &alpha_mul[row], h);
    }
  }
  for (size_t col = 0; col != cols; ++col) {
    uint8_t wide[128];
    memset(wide, 0, sizeof(wide));
    wide_words_to_bytes(wide, h_acc_all + col * wide_words, params);
    sydo_ref_field_reduce_wide(folded_h + col * lambda_bytes, wide, params);
  }
  sydo_explicit_bzero(h_acc_all, cols * wide_bytes);
  free(h_acc_all);
  return true;
}

static size_t ball_rank_from_mask(size_t n, size_t r, size_t mask) {
  size_t rank = 0;
  const size_t weight = bit_count_size(mask);
  for (size_t w = 0; w != weight; ++w) {
    rank += ncr_local(n, w);
  }
  size_t subset_rank = 0;
  size_t left = weight;
  for (size_t pos = 0; pos != n; ++pos) {
    if (left == 0u) {
      break;
    }
    if ((mask & ((size_t)1u << pos)) == 0u) {
      continue;
    }
    subset_rank += ncr_local(n - pos - 1u, left);
    --left;
  }
  (void)r;
  return rank + subset_rank;
}

static size_t pair_rank8(size_t i, size_t j);
static void pair_from_rank8(size_t rank, size_t* i_out, size_t* j_out);
static void triple_from_rank8(size_t rank, size_t* i_out, size_t* j_out, size_t* k_out);
static size_t triple_rank8(size_t i, size_t j, size_t k);

static size_t block0_ball_rank(size_t mask) {
  static const uint8_t ranks[4] = {0, 2, 1, 3};
  return ranks[mask];
}

static size_t block1_single_rank(size_t i) {
  return 8u - i;
}

static size_t block1_pair_rank(size_t i, size_t j) {
  return 9u + pair_rank8(i, j);
}

static size_t block1_triple_rank(size_t triple_rank) {
  return 37u + triple_rank;
}

static void quad_from_rank8(size_t rank, size_t* i_out, size_t* j_out, size_t* k_out,
                            size_t* l_out);

static void build_row_cache_prover(rsd_row_cache_t* cache, const sydo_ref_paramset_t* params,
                                   const uint8_t* witness_ext, const uint8_t* macs,
                                   size_t row) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t off = row * SYDO_REF_RSD_EXTENDED_SUBVECTOR_BITS;

  memset(cache, 0, sizeof(*cache));
  for (size_t i = 0; i != SYDO_REF_RSD_BLOCK0_SIZE; ++i) {
    memcpy(cache->c0[i], macs + (off + i) * lambda_bytes, lambda_bytes);
    cache->a0[i] = (uint8_t)get_bit(witness_ext, off + i);
  }
  for (size_t i = 0; i != SYDO_REF_RSD_BLOCK1_SIZE; ++i) {
    memcpy(cache->c1[i], macs + (off + SYDO_REF_RSD_BLOCK0_SIZE + i) * lambda_bytes,
           lambda_bytes);
    cache->a1[i] =
        (uint8_t)get_bit(witness_ext, off + SYDO_REF_RSD_BLOCK0_SIZE + i);
  }
  for (size_t p = 0; p != 28u; ++p) {
    size_t i;
    size_t j;
    pair_from_rank8(p, &i, &j);
    field_mul_to(cache->t2[p], cache->c1[i], cache->c1[j], params);
    cache->u2[p] = (uint8_t)(cache->a1[i] & cache->a1[j]);
  }
  for (size_t t = 0; t != 56u; ++t) {
    size_t i;
    size_t j;
    size_t k;
    triple_from_rank8(t, &i, &j, &k);
    const size_t ij = pair_rank8(i, j);
    field_mul_to(cache->t3[t], cache->t2[ij], cache->c1[k], params);
    cache->u3[t] = (uint8_t)(cache->u2[ij] & cache->a1[k]);
  }
}

static void build_row_cache_verifier(rsd_row_cache_t* cache, const sydo_ref_paramset_t* params,
                                     const uint8_t* macs, size_t row) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t off = row * SYDO_REF_RSD_EXTENDED_SUBVECTOR_BITS;

  memset(cache, 0, sizeof(*cache));
  for (size_t i = 0; i != SYDO_REF_RSD_BLOCK0_SIZE; ++i) {
    memcpy(cache->c0[i], macs + (off + i) * lambda_bytes, lambda_bytes);
  }
  for (size_t i = 0; i != SYDO_REF_RSD_BLOCK1_SIZE; ++i) {
    memcpy(cache->c1[i], macs + (off + SYDO_REF_RSD_BLOCK0_SIZE + i) * lambda_bytes,
           lambda_bytes);
  }
  for (size_t p = 0; p != 28u; ++p) {
    size_t i;
    size_t j;
    pair_from_rank8(p, &i, &j);
    field_mul_to(cache->t2[p], cache->c1[i], cache->c1[j], params);
  }
  for (size_t t = 0; t != 56u; ++t) {
    size_t i;
    size_t j;
    size_t k;
    triple_from_rank8(t, &i, &j, &k);
    field_mul_to(cache->t3[t], cache->t2[pair_rank8(i, j)], cache->c1[k], params);
  }
}

static void emit_membership_constraints_prover(qs_hash_state_t* st,
                                               const sydo_ref_paramset_t* params,
                                               const uint8_t* witness_ext,
                                               const uint8_t* macs,
                                               const rsd_row_cache_t* caches) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  uint8_t zero[64];
  memset(zero, 0, sizeof(zero));
  for (size_t row = 0; row != params->rsd_w; ++row) {
    rsd_row_cache_t local_cache;
    const rsd_row_cache_t* cache = caches ? &caches[row] : &local_cache;
    uint8_t coeff2[3][64];
    uint8_t coeff4[4][64];

    if (!caches) {
      build_row_cache_prover(&local_cache, params, witness_ext, macs, row);
    }

    memset(coeff2, 0, sizeof(coeff2));
    sydo_ref_field_mul(coeff2[0], cache->c0[0], cache->c0[1], params);
    if (cache->a0[0]) {
      memcpy(coeff2[1], cache->c0[1], lambda_bytes);
    }
    if (cache->a0[1]) {
      for (size_t b = 0; b != lambda_bytes; ++b) {
        coeff2[1][b] ^= cache->c0[0][b];
      }
    }
    qs_hash_add_constraint_coeffs_prover(st, coeff2, 2u, params);

    for (size_t subset_rank = 0; subset_rank != ncr_local(SYDO_REF_RSD_BLOCK1_SIZE, 4u);
         ++subset_rank) {
      size_t i;
      size_t j;
      size_t k;
      size_t l;
      quad_from_rank8(subset_rank, &i, &j, &k, &l);
      memset(coeff4, 0, sizeof(coeff4));
      sydo_ref_field_mul(coeff4[0], cache->t2[pair_rank8(i, j)],
                         cache->t2[pair_rank8(k, l)], params);

      if (cache->a1[i]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[1][b] ^= cache->t3[triple_rank8(j, k, l)][b];
        }
      }
      if (cache->a1[j]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[1][b] ^= cache->t3[triple_rank8(i, k, l)][b];
        }
      }
      if (cache->a1[k]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[1][b] ^= cache->t3[triple_rank8(i, j, l)][b];
        }
      }
      if (cache->a1[l]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[1][b] ^= cache->t3[triple_rank8(i, j, k)][b];
        }
      }

      if (cache->a1[i] & cache->a1[j]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[2][b] ^= cache->t2[pair_rank8(k, l)][b];
        }
      }
      if (cache->a1[i] & cache->a1[k]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[2][b] ^= cache->t2[pair_rank8(j, l)][b];
        }
      }
      if (cache->a1[i] & cache->a1[l]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[2][b] ^= cache->t2[pair_rank8(j, k)][b];
        }
      }
      if (cache->a1[j] & cache->a1[k]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[2][b] ^= cache->t2[pair_rank8(i, l)][b];
        }
      }
      if (cache->a1[j] & cache->a1[l]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[2][b] ^= cache->t2[pair_rank8(i, k)][b];
        }
      }
      if (cache->a1[k] & cache->a1[l]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[2][b] ^= cache->t2[pair_rank8(i, j)][b];
        }
      }

      if (cache->a1[i] & cache->a1[j] & cache->a1[k]) {
        memcpy(coeff4[3], cache->c1[l], lambda_bytes);
      }
      if (cache->a1[i] & cache->a1[j] & cache->a1[l]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[3][b] ^= cache->c1[k][b];
        }
      }
      if (cache->a1[i] & cache->a1[k] & cache->a1[l]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[3][b] ^= cache->c1[j][b];
        }
      }
      if (cache->a1[j] & cache->a1[k] & cache->a1[l]) {
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff4[3][b] ^= cache->c1[i][b];
        }
      }
      qs_hash_add_constraint_coeffs_prover(st, coeff4, 4u, params);
    }
  }
  (void)zero;
}

static void emit_membership_constraints_verifier(qs_hash_state_t* st,
                                                 const sydo_ref_paramset_t* params,
                                                 const uint8_t* macs,
                                                 uint8_t delta_powers[][64],
                                                 const rsd_row_cache_t* caches) {
  for (size_t row = 0; row != params->rsd_w; ++row) {
    rsd_row_cache_t local_cache;
    const rsd_row_cache_t* cache = caches ? &caches[row] : &local_cache;
    uint8_t eval[64];

    if (!caches) {
      build_row_cache_verifier(&local_cache, params, macs, row);
    }

    sydo_ref_field_mul(eval, cache->c0[0], cache->c0[1], params);
    qs_hash_add_constraint_verifier(st, eval, 2u, delta_powers, params);
    for (size_t subset_rank = 0; subset_rank != ncr_local(SYDO_REF_RSD_BLOCK1_SIZE, 4u);
         ++subset_rank) {
      size_t i;
      size_t j;
      size_t k;
      size_t l;
      quad_from_rank8(subset_rank, &i, &j, &k, &l);
      sydo_ref_field_mul(eval, cache->t2[pair_rank8(i, j)],
                         cache->t2[pair_rank8(k, l)], params);
      qs_hash_add_constraint_verifier(st, eval, 4u, delta_powers, params);
    }
  }
}

static bool rsd_mask_valid(size_t mask) {
  const size_t m0 = mask & ((1u << SYDO_REF_RSD_BLOCK0_SIZE) - 1u);
  const size_t m1 = mask >> SYDO_REF_RSD_BLOCK0_SIZE;
  return bit_count_size(m0) <= SYDO_REF_RSD_BLOCK0_WEIGHT &&
         bit_count_size(m1) <= SYDO_REF_RSD_BLOCK1_WEIGHT;
}

static size_t rsd_mask_degree(size_t mask) {
  return bit_count_size(mask & ((1u << SYDO_REF_RSD_BLOCK0_SIZE) - 1u)) +
         bit_count_size(mask >> SYDO_REF_RSD_BLOCK0_SIZE);
}

static void build_folded_coeffs_for_row(uint8_t* coeffs, const sydo_ref_paramset_t* params,
                                        const uint8_t* folded_h, size_t row) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t block1_terms =
      hamming_ball_count_local(SYDO_REF_RSD_BLOCK1_SIZE, SYDO_REF_RSD_BLOCK1_WEIGHT);
  const size_t h_base = row * ((size_t)params->rsd_n / params->rsd_w);
  const size_t mask_count =
      (size_t)1u << (SYDO_REF_RSD_BLOCK0_SIZE + SYDO_REF_RSD_BLOCK1_SIZE);

  memset(coeffs, 0, mask_count * lambda_bytes);
  for (size_t p0 = 0; p0 != (1u << SYDO_REF_RSD_BLOCK0_SIZE); ++p0) {
    if (bit_count_size(p0) > SYDO_REF_RSD_BLOCK0_WEIGHT) {
      continue;
    }
    for (size_t p1 = 0; p1 != (1u << SYDO_REF_RSD_BLOCK1_SIZE); ++p1) {
      if (bit_count_size(p1) > SYDO_REF_RSD_BLOCK1_WEIGHT) {
        continue;
      }
      const size_t rank0 =
          ball_rank_from_mask(SYDO_REF_RSD_BLOCK0_SIZE, SYDO_REF_RSD_BLOCK0_WEIGHT, p0);
      const size_t rank1 =
          ball_rank_from_mask(SYDO_REF_RSD_BLOCK1_SIZE, SYDO_REF_RSD_BLOCK1_WEIGHT, p1);
      const size_t h_idx = h_base + rank0 * block1_terms + rank1;
      const size_t mask = p0 | (p1 << SYDO_REF_RSD_BLOCK0_SIZE);
      memcpy(coeffs + mask * lambda_bytes, folded_h + h_idx * lambda_bytes, lambda_bytes);
    }
  }

  for (size_t bit = 0; bit != SYDO_REF_RSD_BLOCK0_SIZE + SYDO_REF_RSD_BLOCK1_SIZE; ++bit) {
    const size_t bit_mask = (size_t)1u << bit;
    for (size_t mask = 0; mask != mask_count; ++mask) {
      if ((mask & bit_mask) == 0u) {
        continue;
      }
      uint8_t* dst = coeffs + mask * lambda_bytes;
      const uint8_t* src = coeffs + (mask ^ bit_mask) * lambda_bytes;
      for (size_t i = 0; i != lambda_bytes; ++i) {
        dst[i] ^= src[i];
      }
    }
  }
}

static void build_row_monomials_prover(qs_poly_t* mon, const sydo_ref_paramset_t* params,
                                       const uint8_t* witness_ext, const uint8_t* macs,
                                       size_t off) {
  uint8_t one[64];
  qs_poly_t vars[SYDO_REF_RSD_INPUT_REDUCED_BITS];
  memset(one, 0, sizeof(one));
  one[0] = 1u;
  qs_poly_const(&mon[0], one, params);
  for (size_t bit = 0; bit != SYDO_REF_RSD_INPUT_REDUCED_BITS; ++bit) {
    qs_poly_bit(&vars[bit], macs, off + bit, get_bit(witness_ext, off + bit), params);
  }
  for (size_t mask = 1; mask != ((size_t)1u << SYDO_REF_RSD_INPUT_REDUCED_BITS); ++mask) {
    if (!rsd_mask_valid(mask)) {
      continue;
    }
    size_t lowbit = 0;
    while (((mask >> lowbit) & 1u) == 0u) {
      ++lowbit;
    }
    qs_poly_mul(&mon[mask], &mon[mask ^ ((size_t)1u << lowbit)], &vars[lowbit], params);
  }
}

static void build_row_monomials_verifier(qs_poly_t* mon, const sydo_ref_paramset_t* params,
                                         const uint8_t* macs, size_t off) {
  uint8_t one[64];
  qs_poly_t vars[SYDO_REF_RSD_INPUT_REDUCED_BITS];
  memset(one, 0, sizeof(one));
  one[0] = 1u;
  qs_poly_const(&mon[0], one, params);
  for (size_t bit = 0; bit != SYDO_REF_RSD_INPUT_REDUCED_BITS; ++bit) {
    qs_poly_eval_var(&vars[bit], macs, off + bit, params);
  }
  for (size_t mask = 1; mask != ((size_t)1u << SYDO_REF_RSD_INPUT_REDUCED_BITS); ++mask) {
    if (!rsd_mask_valid(mask)) {
      continue;
    }
    size_t lowbit = 0;
    while (((mask >> lowbit) & 1u) == 0u) {
      ++lowbit;
    }
    qs_poly_mul(&mon[mask], &mon[mask ^ ((size_t)1u << lowbit)], &vars[lowbit], params);
  }
}

static void field_xor_inplace(uint8_t* out, const uint8_t* in, size_t len) {
  for (size_t i = 0; i != len; ++i) {
    out[i] ^= in[i];
  }
}

static void field_mul_to(uint8_t* out, const uint8_t* a, const uint8_t* b,
                         const sydo_ref_paramset_t* params) {
  sydo_ref_field_mul(out, a, b, params);
}

static void field_muladd_wide_direct(uint8_t* acc, const uint8_t* a, const uint8_t* b,
                                     const sydo_ref_paramset_t* params) {
  sydo_ref_field_muladd_wide(acc, a, b, params);
}

static size_t pair_rank8(size_t i, size_t j) {
  static const uint8_t pair_rank[8][8] = {
      {255, 27, 26, 25, 24, 23, 22, 21}, {27, 255, 20, 19, 18, 17, 16, 15},
      {26, 20, 255, 14, 13, 12, 11, 10}, {25, 19, 14, 255, 9, 8, 7, 6},
      {24, 18, 13, 9, 255, 5, 4, 3},    {23, 17, 12, 8, 5, 255, 2, 1},
      {22, 16, 11, 7, 4, 2, 255, 0},     {21, 15, 10, 6, 3, 1, 0, 255}};
  return pair_rank[i][j];
}

static void subset_from_rank(size_t n, size_t k, size_t rank, size_t* out) {
  size_t left = k;
  size_t out_pos = 0;
  for (size_t pos = 0; pos != n && left != 0u; ++pos) {
    const size_t skip_count = ncr_local(n - pos - 1u, left);
    if (rank >= skip_count) {
      rank -= skip_count;
      out[out_pos++] = pos;
      --left;
    }
  }
}

static void pair_from_rank8(size_t rank, size_t* i_out, size_t* j_out) {
  static const uint8_t pair_i[28] = {6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2,
                                     2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
  static const uint8_t pair_j[28] = {7, 7, 6, 7, 6, 5, 7, 6, 5, 4, 7, 6, 5, 4,
                                     3, 7, 6, 5, 4, 3, 2, 7, 6, 5, 4, 3, 2, 1};
  *i_out = pair_i[rank];
  *j_out = pair_j[rank];
}

static void triple_from_rank8(size_t rank, size_t* i_out, size_t* j_out, size_t* k_out) {
  static const uint8_t triple_i[56] = {
      5, 4, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static const uint8_t triple_j[56] = {
      6, 6, 5, 5, 6, 5, 5, 4, 4, 4, 6, 5, 5, 4, 4, 4, 3, 3, 3,
      3, 6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 6, 5, 5,
      4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1};
  static const uint8_t triple_k[56] = {
      7, 7, 7, 6, 7, 7, 6, 7, 6, 5, 7, 7, 6, 7, 6, 5, 7, 6, 5,
      4, 7, 7, 6, 7, 6, 5, 7, 6, 5, 4, 7, 6, 5, 4, 3, 7, 7, 6,
      7, 6, 5, 7, 6, 5, 4, 7, 6, 5, 4, 3, 7, 6, 5, 4, 3, 2};
  *i_out = triple_i[rank];
  *j_out = triple_j[rank];
  *k_out = triple_k[rank];
}

static size_t triple_rank8(size_t i, size_t j, size_t k) {
  static uint8_t rank_table[8][8][8];
  static uint8_t initialized = 0;
  if (!initialized) {
    memset(rank_table, 0xff, sizeof(rank_table));
    for (size_t rank = 0; rank != 56u; ++rank) {
      size_t a;
      size_t b;
      size_t c;
      triple_from_rank8(rank, &a, &b, &c);
      rank_table[a][b][c] = (uint8_t)rank;
      rank_table[a][c][b] = (uint8_t)rank;
      rank_table[b][a][c] = (uint8_t)rank;
      rank_table[b][c][a] = (uint8_t)rank;
      rank_table[c][a][b] = (uint8_t)rank;
      rank_table[c][b][a] = (uint8_t)rank;
    }
    initialized = 1;
  }
  return rank_table[i][j][k];
}

static void quad_from_rank8(size_t rank, size_t* i_out, size_t* j_out, size_t* k_out,
                            size_t* l_out) {
  size_t out[4] = {0, 1, 2, 3};
  subset_from_rank(SYDO_REF_RSD_BLOCK1_SIZE, 4u, rank, out);
  *i_out = out[0];
  *j_out = out[1];
  *k_out = out[2];
  *l_out = out[3];
}

static const uint8_t* folded_h_at(const uint8_t* h, const sydo_ref_paramset_t* params,
                                  size_t row, size_t rank0, size_t rank1) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t block1_terms =
      hamming_ball_count_local(SYDO_REF_RSD_BLOCK1_SIZE, SYDO_REF_RSD_BLOCK1_WEIGHT);
  const size_t subvector_terms = (size_t)params->rsd_n / params->rsd_w;
  return h + (row * subvector_terms + rank0 * block1_terms + rank1) * lambda_bytes;
}

static void add_folded_ip_direct_prover(qs_poly_t* folded, const sydo_ref_paramset_t* params,
                                        const uint8_t* folded_h, const uint8_t* witness_ext,
                                        const uint8_t* macs,
                                        const rsd_row_cache_t* caches) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t wide_bytes = 2u * lambda_bytes;
  const size_t rank0_zero = block0_ball_rank(0);
  uint8_t stage0[3][SYDO_REF_RSD_BLOCK1_WEIGHT + 1u][64];
  uint8_t pair_mix[28][3][64];
  uint8_t single_mix[SYDO_REF_RSD_BLOCK1_SIZE][3][64];

  for (size_t row = 0; row != params->rsd_w; ++row) {
    rsd_row_cache_t local_cache;
    const rsd_row_cache_t* cache = caches ? &caches[row] : &local_cache;
    if (!caches) {
      build_row_cache_prover(&local_cache, params, witness_ext, macs, row);
    }

    memset(stage0, 0, sizeof(stage0));
    memset(pair_mix, 0, sizeof(pair_mix));
    memset(single_mix, 0, sizeof(single_mix));

    for (size_t r0 = 0; r0 != 3u; ++r0) {
      const uint8_t* h0 = folded_h_at(folded_h, params, row, r0, 0);
      field_xor_inplace(stage0[r0][SYDO_REF_RSD_BLOCK1_WEIGHT], h0, lambda_bytes);

      for (size_t i = 0; i != SYDO_REF_RSD_BLOCK1_SIZE; ++i) {
        uint8_t coeff[64];
        const size_t rank_i = block1_single_rank(i);
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff[b] = (uint8_t)(h0[b] ^ folded_h_at(folded_h, params, row, r0, rank_i)[b]);
        }
        field_xor_inplace(single_mix[i][r0], coeff, lambda_bytes);
        if (cache->a1[i]) {
          field_xor_inplace(stage0[r0][SYDO_REF_RSD_BLOCK1_WEIGHT], coeff, lambda_bytes);
        }
      }

      for (size_t p = 0; p != 28u; ++p) {
        size_t i;
        size_t j;
        uint8_t coeff[64];
        pair_from_rank8(p, &i, &j);
        const size_t rank_i = block1_single_rank(i);
        const size_t rank_j = block1_single_rank(j);
        const size_t rank_ij = block1_pair_rank(i, j);
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff[b] = (uint8_t)(h0[b] ^ folded_h_at(folded_h, params, row, r0, rank_i)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_j)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_ij)[b]);
        }
        field_xor_inplace(pair_mix[p][r0], coeff, lambda_bytes);
        if (cache->a1[j]) {
          field_xor_inplace(single_mix[i][r0], coeff, lambda_bytes);
        }
        if (cache->a1[i]) {
          field_xor_inplace(single_mix[j][r0], coeff, lambda_bytes);
        }
        if (cache->u2[p]) {
          field_xor_inplace(stage0[r0][SYDO_REF_RSD_BLOCK1_WEIGHT], coeff, lambda_bytes);
        }
      }

      {
        uint8_t out0_acc[128];
        uint8_t out1_acc[128];
        uint8_t out2_acc[128];
        uint8_t triple_coeffs[56][64];
        memset(out0_acc, 0, sizeof(out0_acc));
        memset(out1_acc, 0, sizeof(out1_acc));
        memset(out2_acc, 0, sizeof(out2_acc));
        for (size_t t = 0; t != 56u; ++t) {
          size_t i;
          size_t j;
          size_t k;
          triple_from_rank8(t, &i, &j, &k);
          const size_t ij = pair_rank8(i, j);
          const size_t rank_k = block1_single_rank(k);
          const size_t rank_ik = block1_pair_rank(i, k);
          const size_t rank_jk = block1_pair_rank(j, k);
          const size_t rank_ijk = block1_triple_rank(t);
          for (size_t b = 0; b != lambda_bytes; ++b) {
            triple_coeffs[t][b] =
                (uint8_t)(pair_mix[ij][r0][b] ^
                          folded_h_at(folded_h, params, row, r0, rank_k)[b] ^
                          folded_h_at(folded_h, params, row, r0, rank_ik)[b] ^
                          folded_h_at(folded_h, params, row, r0, rank_jk)[b] ^
                          folded_h_at(folded_h, params, row, r0, rank_ijk)[b]);
          }
        }
        for (size_t t = 0; t != 56u; ++t) {
          size_t i;
          size_t j;
          size_t k;
          triple_from_rank8(t, &i, &j, &k);
          const size_t ij = pair_rank8(i, j);
          const size_t ik = pair_rank8(i, k);
          const size_t jk = pair_rank8(j, k);
          const uint8_t* coeff = triple_coeffs[t];
          field_muladd_wide_direct(out0_acc, coeff, cache->t3[t], params);
          if (cache->a1[i]) {
            field_xor_inplace(pair_mix[jk][r0], coeff, lambda_bytes);
          }
          if (cache->a1[j]) {
            field_xor_inplace(pair_mix[ik][r0], coeff, lambda_bytes);
          }
          if (cache->a1[k]) {
            field_xor_inplace(pair_mix[ij][r0], coeff, lambda_bytes);
          }
          if (cache->u2[jk]) {
            field_xor_inplace(single_mix[i][r0], coeff, lambda_bytes);
          }
          if (cache->u2[ik]) {
            field_xor_inplace(single_mix[j][r0], coeff, lambda_bytes);
          }
          if (cache->u2[ij]) {
            field_xor_inplace(single_mix[k][r0], coeff, lambda_bytes);
          }
          if (cache->u3[t]) {
            field_xor_inplace(stage0[r0][SYDO_REF_RSD_BLOCK1_WEIGHT], coeff, lambda_bytes);
          }
        }
        sydo_ref_field_reduce_wide(stage0[r0][0], out0_acc, params);
        for (size_t p = 0; p != 28u; ++p) {
          field_muladd_wide_direct(out1_acc, pair_mix[p][r0], cache->t2[p], params);
        }
        sydo_ref_field_reduce_wide(stage0[r0][1], out1_acc, params);
        for (size_t i = 0; i != SYDO_REF_RSD_BLOCK1_SIZE; ++i) {
          field_muladd_wide_direct(out2_acc, single_mix[i][r0], cache->c1[i], params);
        }
        sydo_ref_field_reduce_wide(stage0[r0][2], out2_acc, params);
      }
    }

    for (size_t t = 0; t != SYDO_REF_RSD_BLOCK1_WEIGHT; ++t) {
      field_xor_inplace(folded->coeff[1u + t], stage0[rank0_zero][t], lambda_bytes);
    }
    {
      uint8_t selected_h[64];
      uint8_t rest_acc[SYDO_REF_RSD_BLOCK1_WEIGHT + 1u][128];
      memcpy(selected_h, stage0[rank0_zero][SYDO_REF_RSD_BLOCK1_WEIGHT], lambda_bytes);
      memset(rest_acc, 0, sizeof(rest_acc));
      for (size_t i = 0; i != SYDO_REF_RSD_BLOCK0_SIZE; ++i) {
        const size_t rank = block0_ball_rank((size_t)1u << i);
        for (size_t t = 0; t != SYDO_REF_RSD_BLOCK1_WEIGHT + 1u; ++t) {
          uint8_t coeff[64];
          for (size_t b = 0; b != lambda_bytes; ++b) {
            coeff[b] = (uint8_t)(stage0[rank0_zero][t][b] ^ stage0[rank][t][b]);
          }
          field_muladd_wide_direct(rest_acc[t], coeff, cache->c0[i], params);
          if (t < SYDO_REF_RSD_BLOCK1_WEIGHT) {
            if (cache->a0[i]) {
              field_xor_inplace(folded->coeff[1u + t], coeff, lambda_bytes);
            }
          } else if (cache->a0[i]) {
            field_xor_inplace(selected_h, coeff, lambda_bytes);
          }
        }
      }
      for (size_t t = 0; t != SYDO_REF_RSD_BLOCK1_WEIGHT + 1u; ++t) {
        uint8_t reduced[64];
        sydo_ref_field_reduce_wide(reduced, rest_acc[t], params);
        field_xor_inplace(folded->coeff[t], reduced, lambda_bytes);
      }
      field_xor_inplace(folded->coeff[SYDO_REF_RSD_QS_DEGREE], selected_h, lambda_bytes);
    }
    (void)wide_bytes;
  }
}

static void add_folded_ip_prover(qs_poly_t* folded, const sydo_ref_paramset_t* params,
                                 const uint8_t* folded_h, const uint8_t* witness_ext,
                                 const uint8_t* macs, const rsd_row_cache_t* caches) {
  add_folded_ip_direct_prover(folded, params, folded_h, witness_ext, macs, caches);
  return;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t mask_count =
      (size_t)1u << (SYDO_REF_RSD_BLOCK0_SIZE + SYDO_REF_RSD_BLOCK1_SIZE);
  qs_poly_t* mon = (qs_poly_t*)calloc(mask_count, sizeof(*mon));
  uint8_t* coeffs = (uint8_t*)malloc(mask_count * lambda_bytes);
  if (!mon || !coeffs) {
    free(mon);
    free(coeffs);
    return;
  }
  for (size_t row = 0; row != params->rsd_w; ++row) {
    const size_t off = row * SYDO_REF_RSD_EXTENDED_SUBVECTOR_BITS;
    memset(mon, 0, mask_count * sizeof(*mon));
    build_row_monomials_prover(mon, params, witness_ext, macs, off);
    build_folded_coeffs_for_row(coeffs, params, folded_h, row);
    for (size_t mask = 0; mask != mask_count; ++mask) {
      if (!rsd_mask_valid(mask)) {
        continue;
      }
      qs_poly_t tmp;
      qs_poly_t shifted;
      const size_t degree = rsd_mask_degree(mask);
      const size_t shift = SYDO_REF_RSD_QS_DEGREE - degree;
      qs_poly_mul_const(&tmp, &mon[mask], coeffs + mask * lambda_bytes, params);
      qs_poly_zero(&shifted);
      for (size_t d = 0; d + shift != SYDO_REF_RSD_QS_DEGREE + 1u; ++d) {
        memcpy(shifted.coeff[d + shift], tmp.coeff[d], lambda_bytes);
      }
      qs_poly_add(folded, &shifted, params);
    }
  }
  sydo_explicit_bzero(mon, mask_count * sizeof(*mon));
  sydo_explicit_bzero(coeffs, mask_count * lambda_bytes);
  free(mon);
  free(coeffs);
}

static void add_folded_ip_direct_verifier(qs_poly_t* folded, const sydo_ref_paramset_t* params,
                                          const uint8_t* folded_h, const uint8_t* macs,
                                          uint8_t delta_powers[][64],
                                          const rsd_row_cache_t* caches) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t rank0_zero = block0_ball_rank(0);
  uint8_t stage0[3][64];

  for (size_t row = 0; row != params->rsd_w; ++row) {
    rsd_row_cache_t local_cache;
    const rsd_row_cache_t* cache = caches ? &caches[row] : &local_cache;
    uint8_t scaled_bits[SYDO_REF_RSD_BLOCK1_SIZE][64];
    uint8_t scaled_pairs[28][64];
    if (!caches) {
      build_row_cache_verifier(&local_cache, params, macs, row);
    }
    for (size_t i = 0; i != SYDO_REF_RSD_BLOCK1_SIZE; ++i) {
      field_mul_to(scaled_bits[i], cache->c1[i], delta_powers[1], params);
    }
    for (size_t p = 0; p != 28u; ++p) {
      field_mul_to(scaled_pairs[p], cache->t2[p], delta_powers[0], params);
    }

    for (size_t r0 = 0; r0 != 3u; ++r0) {
      uint8_t acc[128];
      const uint8_t* h0 = folded_h_at(folded_h, params, row, r0, 0);
      memset(acc, 0, sizeof(acc));
      field_muladd_wide_direct(acc, h0, delta_powers[2], params);

      for (size_t i = 0; i != SYDO_REF_RSD_BLOCK1_SIZE; ++i) {
        uint8_t coeff[64];
        const size_t rank_i = block1_single_rank(i);
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff[b] = (uint8_t)(h0[b] ^ folded_h_at(folded_h, params, row, r0, rank_i)[b]);
        }
        field_muladd_wide_direct(acc, coeff, scaled_bits[i], params);
      }

      for (size_t p = 0; p != 28u; ++p) {
        size_t i;
        size_t j;
        uint8_t coeff[64];
        pair_from_rank8(p, &i, &j);
        const size_t rank_i = block1_single_rank(i);
        const size_t rank_j = block1_single_rank(j);
        const size_t rank_ij = block1_pair_rank(i, j);
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff[b] = (uint8_t)(h0[b] ^ folded_h_at(folded_h, params, row, r0, rank_i)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_j)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_ij)[b]);
        }
        field_muladd_wide_direct(acc, coeff, scaled_pairs[p], params);
      }

      for (size_t t = 0; t != 56u; ++t) {
        size_t i;
        size_t j;
        size_t k;
        uint8_t coeff[64];
        triple_from_rank8(t, &i, &j, &k);
        const size_t rank_i = block1_single_rank(i);
        const size_t rank_j = block1_single_rank(j);
        const size_t rank_k = block1_single_rank(k);
        const size_t rank_ij = block1_pair_rank(i, j);
        const size_t rank_ik = block1_pair_rank(i, k);
        const size_t rank_jk = block1_pair_rank(j, k);
        const size_t rank_ijk = block1_triple_rank(t);
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff[b] = (uint8_t)(h0[b] ^ folded_h_at(folded_h, params, row, r0, rank_i)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_j)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_k)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_ij)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_ik)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_jk)[b] ^
                               folded_h_at(folded_h, params, row, r0, rank_ijk)[b]);
        }
        field_muladd_wide_direct(acc, coeff, cache->t3[t], params);
      }
      sydo_ref_field_reduce_wide(stage0[r0], acc, params);
    }

    {
      uint8_t acc[128];
      memset(acc, 0, sizeof(acc));
      field_muladd_wide_direct(acc, stage0[rank0_zero], delta_powers[0], params);
      for (size_t i = 0; i != SYDO_REF_RSD_BLOCK0_SIZE; ++i) {
        const size_t rank = block0_ball_rank((size_t)1u << i);
        uint8_t coeff[64];
        for (size_t b = 0; b != lambda_bytes; ++b) {
          coeff[b] = (uint8_t)(stage0[rank0_zero][b] ^ stage0[rank][b]);
        }
        field_muladd_wide_direct(acc, coeff, cache->c0[i], params);
      }
      {
        uint8_t reduced[64];
        sydo_ref_field_reduce_wide(reduced, acc, params);
        field_xor_inplace(folded->coeff[0], reduced, lambda_bytes);
      }
    }
  }
}

static void add_folded_ip_verifier(qs_poly_t* folded, const sydo_ref_paramset_t* params,
                                   const uint8_t* folded_h, const uint8_t* macs,
                                   uint8_t delta_powers[][64],
                                   const rsd_row_cache_t* caches) {
  add_folded_ip_direct_verifier(folded, params, folded_h, macs, delta_powers, caches);
  return;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t mask_count =
      (size_t)1u << (SYDO_REF_RSD_BLOCK0_SIZE + SYDO_REF_RSD_BLOCK1_SIZE);
  qs_poly_t* mon = (qs_poly_t*)calloc(mask_count, sizeof(*mon));
  uint8_t* coeffs = (uint8_t*)malloc(mask_count * lambda_bytes);
  if (!mon || !coeffs) {
    free(mon);
    free(coeffs);
    return;
  }
  for (size_t row = 0; row != params->rsd_w; ++row) {
    const size_t off = row * SYDO_REF_RSD_EXTENDED_SUBVECTOR_BITS;
    memset(mon, 0, mask_count * sizeof(*mon));
    build_row_monomials_verifier(mon, params, macs, off);
    build_folded_coeffs_for_row(coeffs, params, folded_h, row);
    for (size_t mask = 0; mask != mask_count; ++mask) {
      if (!rsd_mask_valid(mask)) {
        continue;
      }
      qs_poly_t tmp;
      qs_poly_t shifted;
      const size_t degree = rsd_mask_degree(mask);
      const size_t shift = SYDO_REF_RSD_QS_DEGREE - degree;
      qs_poly_mul_const(&tmp, &mon[mask], coeffs + mask * lambda_bytes, params);
      qs_poly_zero(&shifted);
      for (size_t d = 0; d + shift != SYDO_REF_RSD_QS_DEGREE + 1u; ++d) {
        memcpy(shifted.coeff[d + shift], tmp.coeff[d], lambda_bytes);
      }
      qs_poly_add(folded, &shifted, params);
    }
  }
  sydo_explicit_bzero(mon, mask_count * sizeof(*mon));
  sydo_explicit_bzero(coeffs, mask_count * lambda_bytes);
  free(mon);
  free(coeffs);
}

static void emit_padding_constraints_prover(qs_hash_state_t* st,
                                            const sydo_ref_paramset_t* params) {
  uint8_t zero[64];
  memset(zero, 0, sizeof(zero));
  for (size_t i = 0; i != SYDO_REF_RSD_Y_VECTOR_LEN; ++i) {
    for (size_t d = 0; d != SYDO_REF_RSD_QS_DEGREE; ++d) {
      qs_hash_add_coeff(st, d, zero, params);
    }
  }
}

static void emit_padding_constraints_verifier(qs_hash_state_t* st,
                                              const sydo_ref_paramset_t* params,
                                              uint8_t delta_powers[][64]) {
  uint8_t zero[64];
  memset(zero, 0, sizeof(zero));
  for (size_t i = 0; i != SYDO_REF_RSD_Y_VECTOR_LEN; ++i) {
    qs_hash_add_constraint_verifier(st, zero, SYDO_REF_RSD_QS_DEGREE, delta_powers, params);
  }
}

bool sydo_ref_qs_prove_rsd(const sydo_ref_paramset_t* params, const uint8_t* pk,
                           const uint8_t* challenge, const uint8_t* witness_ext,
                           const uint8_t* macs_extended, uint8_t* proof, uint8_t* check) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  qs_hash_state_t st;
  qs_poly_t folded;
  uint8_t* folded_h = NULL;
  rsd_row_cache_t* row_caches = NULL;
  uint8_t folded_y[64];
  uint8_t masks[SYDO_REF_RSD_QS_DEGREE][128];
  clock_t stage_start;

  folded_h = (uint8_t*)malloc((size_t)params->rsd_n * lambda_bytes);
  row_caches = (rsd_row_cache_t*)calloc(params->rsd_w, sizeof(*row_caches));
  if (!folded_h || !row_caches) {
    free(folded_h);
    free(row_caches);
    return false;
  }
  stage_start = qs_trace_clock();
  for (size_t row = 0; row != params->rsd_w; ++row) {
    build_row_cache_prover(&row_caches[row], params, witness_ext, macs_extended, row);
  }
  qs_trace_stage(params, "prove row_cache", stage_start);
  qs_hash_init(&st, params, challenge);
  stage_start = qs_trace_clock();
  emit_membership_constraints_prover(&st, params, witness_ext, macs_extended, row_caches);
  qs_trace_stage(params, "prove membership", stage_start);
  qs_poly_zero(&folded);
  stage_start = qs_trace_clock();
  if (!fold_public_data(params, pk, challenge, folded_h, folded_y)) {
    free(folded_h);
    sydo_explicit_bzero(row_caches, (size_t)params->rsd_w * sizeof(*row_caches));
    free(row_caches);
    return false;
  }
  qs_trace_stage(params, "prove fold_public", stage_start);
  stage_start = qs_trace_clock();
  add_folded_ip_prover(&folded, params, folded_h, witness_ext, macs_extended, row_caches);
  qs_trace_stage(params, "prove folded_ip", stage_start);
  qs_poly_t y_const;
  qs_poly_zero(&y_const);
  memcpy(y_const.coeff[SYDO_REF_RSD_QS_DEGREE], folded_y, lambda_bytes);
  qs_poly_add(&folded, &y_const, params);
  stage_start = qs_trace_clock();
  emit_padding_constraints_prover(&st, params);
  qs_make_mask_prover(params, witness_ext, macs_extended, masks);
  qs_hash_finalize_one(&st, 0, masks[0], params, check);
  for (size_t d = 1; d != SYDO_REF_RSD_QS_DEGREE; ++d) {
    qs_hash_finalize_one(&st, d, masks[d], params, proof + (d - 1u) * lambda_bytes);
  }
  for (size_t d = 0; d != SYDO_REF_RSD_QS_DEGREE; ++d) {
    uint8_t* dst = d == 0 ? check : proof + (d - 1u) * lambda_bytes;
    for (size_t j = 0; j != lambda_bytes; ++j) {
      dst[j] ^= folded.coeff[d][j];
    }
  }
  qs_trace_stage(params, "prove finalize", stage_start);
  sydo_explicit_bzero(folded_h, (size_t)params->rsd_n * lambda_bytes);
  sydo_explicit_bzero(row_caches, (size_t)params->rsd_w * sizeof(*row_caches));
  free(folded_h);
  free(row_caches);
  return true;
}

bool sydo_ref_qs_verify_rsd(const sydo_ref_paramset_t* params, const uint8_t* pk,
                            const uint8_t* challenge, const uint8_t* delta,
                            const uint8_t* macs_extended, const uint8_t* proof,
                            uint8_t* check) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  qs_hash_state_t st;
  qs_poly_t folded;
  uint8_t* folded_h = NULL;
  rsd_row_cache_t* row_caches = NULL;
  uint8_t folded_y[64];
  uint8_t mask[128];
  uint8_t delta_powers[SYDO_REF_RSD_QS_DEGREE][64];
  clock_t stage_start;

  folded_h = (uint8_t*)malloc((size_t)params->rsd_n * lambda_bytes);
  row_caches = (rsd_row_cache_t*)calloc(params->rsd_w, sizeof(*row_caches));
  if (!folded_h || !row_caches) {
    free(folded_h);
    free(row_caches);
    return false;
  }
  stage_start = qs_trace_clock();
  for (size_t row = 0; row != params->rsd_w; ++row) {
    build_row_cache_verifier(&row_caches[row], params, macs_extended, row);
  }
  qs_trace_stage(params, "verify row_cache", stage_start);
  memcpy(delta_powers[0], delta, lambda_bytes);
  for (size_t i = 1; i != SYDO_REF_RSD_QS_DEGREE; ++i) {
    sydo_ref_field_mul(delta_powers[i], delta_powers[i - 1u], delta, params);
  }
  qs_hash_init(&st, params, challenge);
  stage_start = qs_trace_clock();
  emit_membership_constraints_verifier(&st, params, macs_extended, delta_powers, row_caches);
  qs_trace_stage(params, "verify membership", stage_start);
  qs_poly_zero(&folded);
  stage_start = qs_trace_clock();
  if (!fold_public_data(params, pk, challenge, folded_h, folded_y)) {
    free(folded_h);
    sydo_explicit_bzero(row_caches, (size_t)params->rsd_w * sizeof(*row_caches));
    free(row_caches);
    return false;
  }
  qs_trace_stage(params, "verify fold_public", stage_start);
  stage_start = qs_trace_clock();
  add_folded_ip_verifier(&folded, params, folded_h, macs_extended, delta_powers, row_caches);
  qs_trace_stage(params, "verify folded_ip", stage_start);
  {
    qs_poly_t y_const;
    qs_poly_zero(&y_const);
    memcpy(y_const.coeff[SYDO_REF_RSD_QS_DEGREE], folded_y, lambda_bytes);
    qs_poly_add(&folded, &y_const, params);
  }
  stage_start = qs_trace_clock();
  emit_padding_constraints_verifier(&st, params, delta_powers);
  qs_make_mask_verifier(params, delta, macs_extended, proof, mask);
  qs_hash_finalize_one(&st, 0, mask, params, check);
  {
    uint8_t folded_eval[64];
    memset(folded_eval, 0, sizeof(folded_eval));
    for (size_t j = 0; j != lambda_bytes; ++j) {
      folded_eval[j] ^= folded.coeff[0][j];
    }
    for (size_t d = 1; d != SYDO_REF_RSD_QS_DEGREE + 1u; ++d) {
      uint8_t term[64];
      sydo_ref_field_mul(term, folded.coeff[d], delta_powers[d - 1u], params);
      for (size_t j = 0; j != lambda_bytes; ++j) {
        folded_eval[j] ^= term[j];
      }
    }
    for (size_t j = 0; j != lambda_bytes; ++j) {
      check[j] ^= folded_eval[j];
    }
  }
  qs_trace_stage(params, "verify finalize", stage_start);
  sydo_explicit_bzero(folded_h, (size_t)params->rsd_n * lambda_bytes);
  sydo_explicit_bzero(row_caches, (size_t)params->rsd_w * sizeof(*row_caches));
  free(folded_h);
  free(row_caches);
  return true;
}
