/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "sydo.h"

#include "aes.h"
#include "compat.h"
#include "lib/auxfunc.h"
#include "lib/blake2/ref/blake2.h"
#include "randomness.h"
#include "bavc.h"
#include "hash.h"
#include "internal.h"
#include "macs.h"
#include "primitives.h"
#include "quicksilver.h"
#include "rsd.h"
#include "vole.h"
#include "vole_check.h"
#include "vole_commit.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
  SYDO_REF_NUM_BLOCKS = 2,
  SYDO_REF_BLOCK0_SIZE = 2,
  SYDO_REF_BLOCK0_WEIGHT = 1,
  SYDO_REF_BLOCK1_SIZE = 8,
  SYDO_REF_BLOCK1_WEIGHT = 3,
  SYDO_REF_ELEMENTARY_VECTOR_LEN = 279
};

static const sydo_ref_paramset_t sydo_ref_params[] = {
    {SYDO_REF_160S, "sydo_160s", 160, 14, 8, 132, 16461, 59, 480, 5428, 80, 174, 74, 40},
    {SYDO_REF_160F, "sydo_160f", 160, 20, 2, 138, 16461, 59, 480, 6724, 80, 174, 74, 40},
    {SYDO_REF_256S, "sydo_256s", 256, 23, 5, 225, 26226, 94, 768, 14444, 128, 278, 118, 64},
    {SYDO_REF_256F, "sydo_256f", 256, 32, 2, 236, 26226, 94, 768, 17604, 128, 278, 118, 64},
    {SYDO_REF_512S, "sydo_512s", 512, 46, 8, 445, 49941, 179, 1456, 56672, 246, 534, 224, 128},
    {SYDO_REF_512F, "sydo_512f", 512, 64, 2, 446, 49941, 179, 1456, 67716, 246, 534, 224, 128},
};

static uint16_t secpar_bytes(const sydo_ref_paramset_t* params) {
  return (uint16_t)sydo_ref_secpar_bytes(params);
}

static uint16_t y_vector_bytes(const sydo_ref_paramset_t* params) {
  return (uint16_t)(params->rsd_codim / 8);
}

static uint16_t y_storage_bytes(const sydo_ref_paramset_t* params) {
  const uint16_t lambda_bytes = secpar_bytes(params);
  const uint16_t rows =
      (uint16_t)((params->rsd_codim + params->secpar_bits - 1) / params->secpar_bits);
  return (uint16_t)(rows * lambda_bytes);
}

static uint16_t sk_seed_offset(const sydo_ref_paramset_t* params) {
  return params->public_key_size;
}

static uint16_t sk_witness_offset(const sydo_ref_paramset_t* params) {
  return (uint16_t)(params->public_key_size + secpar_bytes(params));
}

const sydo_ref_paramset_t* sydo_ref_get_paramset(sydo_ref_paramid_t id) {
  for (size_t i = 0; i != sizeof(sydo_ref_params) / sizeof(sydo_ref_params[0]); ++i) {
    if (sydo_ref_params[i].id == id) {
      return &sydo_ref_params[i];
    }
  }
  return NULL;
}

const sydo_ref_paramset_t* sydo_ref_get_paramset_by_name(const char* name) {
  if (!name) {
    return NULL;
  }
  for (size_t i = 0; i != sizeof(sydo_ref_params) / sizeof(sydo_ref_params[0]); ++i) {
    if (strcmp(sydo_ref_params[i].name, name) == 0) {
      return &sydo_ref_params[i];
    }
  }
  return NULL;
}

static bool get_bit_lsb_first(const uint8_t* bytes, size_t bit_index) {
  return ((bytes[bit_index >> 3] >> (bit_index & 7)) & 1u) != 0;
}

static void set_bit_lsb_first(uint8_t* bytes, size_t bit_index, bool bit) {
  const uint8_t mask = (uint8_t)(1u << (bit_index & 7));
  if (bit) {
    bytes[bit_index >> 3] |= mask;
  } else {
    bytes[bit_index >> 3] &= (uint8_t)~mask;
  }
}

static uint32_t load_bits_lsb_first(const uint8_t* bytes, size_t bit_offset, size_t bit_len) {
  uint32_t v = 0;
  for (size_t i = 0; i != bit_len; ++i) {
    v |= (uint32_t)get_bit_lsb_first(bytes, bit_offset + i) << i;
  }
  return v;
}

static void store_bits_lsb_first(uint8_t* bytes, size_t bit_offset, size_t bit_len, uint32_t v) {
  for (size_t i = 0; i != bit_len; ++i) {
    set_bit_lsb_first(bytes, bit_offset + i, ((v >> i) & 1u) != 0);
  }
}

static size_t ceil_log2_size(size_t x) {
  size_t bits = 0;
  size_t v = x <= 1 ? 0 : x - 1;
  while (v) {
    ++bits;
    v >>= 1;
  }
  return bits;
}

static size_t ncr(size_t n, size_t r) {
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

static size_t hamming_ball_count(size_t n, size_t r) {
  size_t total = 0;
  for (size_t w = 0; w <= r; ++w) {
    total += ncr(n, w);
  }
  return total;
}

static bool write_full_subset_bits(uint8_t* out, size_t n, size_t w, size_t rank,
                                   size_t dst_off) {
  for (size_t pos = 0; pos != n; ++pos) {
    set_bit_lsb_first(out, dst_off + pos, false);
  }

  size_t left = w;
  for (size_t pos = 0; pos != n; ++pos) {
    if (left == 0) {
      return rank == 0;
    }
    if ((n - pos) < left) {
      return false;
    }
    const size_t skip_count = ncr(n - pos - 1, left);
    if (rank >= skip_count) {
      rank -= skip_count;
      --left;
      set_bit_lsb_first(out, dst_off + pos, true);
    }
  }
  return left == 0 && rank == 0;
}

static bool write_hamming_ball_bits(uint8_t* out, size_t n, size_t r, size_t rank,
                                    size_t dst_off) {
  for (size_t w = 0; w <= r; ++w) {
    const size_t count = ncr(n, w);
    if (rank < count) {
      return write_full_subset_bits(out, n, w, rank, dst_off);
    }
    rank -= count;
  }
  return false;
}

static int auxfunc_pseudo_xof_bytes(unsigned long long output_len_bits, const uint8_t* msg,
                                    size_t msg_len, uint8_t* output) {
  static const uint8_t empty_msg = 0;
  if (msg_len == 0) {
    msg = &empty_msg;
  }

  return pseudoXOF(output_len_bits, msg, (unsigned long long)msg_len * 8, output);
}

static bool xof_domain(const sydo_ref_paramset_t* params, uint8_t* out, size_t out_len,
                       uint8_t domain, const uint8_t* part0, size_t part0_len,
                       const uint8_t* part1, size_t part1_len, const uint8_t* part2,
                       size_t part2_len) {
  const size_t input_len = part0_len + part1_len + part2_len + 1;
  uint8_t* input = (uint8_t*)malloc(input_len);
  if (!input) {
    return false;
  }
  size_t offset = 0;
  if (part0 && part0_len) {
    memcpy(input + offset, part0, part0_len);
    offset += part0_len;
  }
  if (part1 && part1_len) {
    memcpy(input + offset, part1, part1_len);
    offset += part1_len;
  }
  if (part2 && part2_len) {
    memcpy(input + offset, part2, part2_len);
    offset += part2_len;
  }
  input[offset] = domain;
  const int ret = auxfunc_pseudo_xof_bytes((unsigned long long)out_len * 8, input, input_len, out);
  sydo_explicit_bzero(input, input_len);
  free(input);
  (void)params;
  return ret == 0;
}

static size_t rsd_pos_bits(const sydo_ref_paramset_t* params) {
  return ceil_log2_size(params->rsd_n / params->rsd_w);
}

static size_t rsd_pos_stream_bytes(const sydo_ref_paramset_t* params) {
  return (params->rsd_w * rsd_pos_bits(params) + 7) / 8;
}

static bool sample_x_positions(uint8_t* x_pos_out, const sydo_ref_paramset_t* params,
                               const uint8_t* seed_sk) {
  const size_t seed_len = secpar_bytes(params);
  const size_t block_len = params->rsd_n / params->rsd_w;
  const size_t pos_bits = rsd_pos_bits(params);
  const size_t pos_stream_len = rsd_pos_stream_bytes(params);
  const uint64_t two32 = UINT64_C(1) << 32;
  const uint64_t t_max = (two32 / block_len) * block_len;
  uint8_t stream_block[32];
  size_t stream_off = sizeof(stream_block);
  uint32_t stream_counter = 0;
  size_t block_i = 0;

  if (params->rsd_n % params->rsd_w != 0 || block_len != SYDO_REF_ELEMENTARY_VECTOR_LEN) {
    return false;
  }
  memset(x_pos_out, 0, pos_stream_len);

  while (block_i < params->rsd_w) {
    if (stream_off == sizeof(stream_block)) {
      uint8_t counter_bytes[4];
      counter_bytes[0] = (uint8_t)(stream_counter & 0xffu);
      counter_bytes[1] = (uint8_t)((stream_counter >> 8) & 0xffu);
      counter_bytes[2] = (uint8_t)((stream_counter >> 16) & 0xffu);
      counter_bytes[3] = (uint8_t)((stream_counter >> 24) & 0xffu);
      if (!xof_domain(params, stream_block, sizeof(stream_block), 0x58, seed_sk, seed_len,
                      counter_bytes, sizeof(counter_bytes), NULL, 0)) {
        return false;
      }
      ++stream_counter;
      stream_off = 0;
    }

    uint32_t v = (uint32_t)stream_block[stream_off++];
    if (stream_off == sizeof(stream_block)) {
      continue;
    }
    v |= (uint32_t)stream_block[stream_off++] << 8;
    if (stream_off == sizeof(stream_block)) {
      continue;
    }
    v |= (uint32_t)stream_block[stream_off++] << 16;
    if (stream_off == sizeof(stream_block)) {
      continue;
    }
    v |= (uint32_t)stream_block[stream_off++] << 24;

    if ((uint64_t)v >= t_max) {
      continue;
    }

    store_bits_lsb_first(x_pos_out, block_i * pos_bits, pos_bits, v % block_len);
    ++block_i;
  }
  return true;
}

static bool compute_witness_from_x_positions(uint8_t* witness, const sydo_ref_paramset_t* params,
                                             const uint8_t* x_positions) {
  const size_t pos_bits = rsd_pos_bits(params);
  const size_t row_bits = SYDO_REF_BLOCK0_SIZE + SYDO_REF_BLOCK1_SIZE;
  const size_t block0_terms =
      hamming_ball_count(SYDO_REF_BLOCK0_SIZE, SYDO_REF_BLOCK0_WEIGHT);
  const size_t block1_terms =
      hamming_ball_count(SYDO_REF_BLOCK1_SIZE, SYDO_REF_BLOCK1_WEIGHT);

  if (block0_terms * block1_terms != SYDO_REF_ELEMENTARY_VECTOR_LEN) {
    return false;
  }
  if (params->rsd_w * row_bits > (size_t)params->witness_size * 8) {
    return false;
  }
  memset(witness, 0, params->witness_size);

  for (size_t i = 0; i != params->rsd_w; ++i) {
    size_t idx = load_bits_lsb_first(x_positions, i * pos_bits, pos_bits);
    const size_t local0 = idx / block1_terms;
    const size_t local1 = idx % block1_terms;
    const size_t row_off = i * row_bits;
    if (idx >= SYDO_REF_ELEMENTARY_VECTOR_LEN) {
      return false;
    }
    if (!write_hamming_ball_bits(witness, SYDO_REF_BLOCK0_SIZE, SYDO_REF_BLOCK0_WEIGHT, local0,
                                 row_off)) {
      return false;
    }
    if (!write_hamming_ball_bits(witness, SYDO_REF_BLOCK1_SIZE, SYDO_REF_BLOCK1_WEIGHT, local1,
                                 row_off + SYDO_REF_BLOCK0_SIZE)) {
      return false;
    }
  }
  return true;
}

static void store32_le(uint8_t* dst, uint32_t v) {
  dst[0] = (uint8_t)(v & 0xffu);
  dst[1] = (uint8_t)((v >> 8) & 0xffu);
  dst[2] = (uint8_t)((v >> 16) & 0xffu);
  dst[3] = (uint8_t)((v >> 24) & 0xffu);
}

static bool prg_block_128(const sydo_ref_paramset_t* params, const uint8_t* key, uint32_t counter,
                          uint8_t out[16]) {
  uint8_t input[16] = {0};
  aes_round_keys_t round_keys;
  store32_le(input, counter);

  if (params->secpar_bits == 160) {
    uint8_t padded_key[24] = {0};
    memcpy(padded_key, key, 20);
    aes192_init_round_keys(&round_keys, padded_key);
    aes192_encrypt_block(&round_keys, input, out);
    sydo_explicit_bzero(padded_key, sizeof(padded_key));
    sydo_explicit_bzero(&round_keys, sizeof(round_keys));
    return true;
  }
  if (params->secpar_bits == 256) {
    aes256_init_round_keys(&round_keys, key);
    aes256_encrypt_block(&round_keys, input, out);
    sydo_explicit_bzero(&round_keys, sizeof(round_keys));
    return true;
  }
  return false;
}

static bool prg_block_512(const uint8_t* key, uint32_t counter, uint8_t out[64]) {
  uint8_t input[64] = {0};
  store32_le(input, counter);
  return sydo_ref_blake2s_512_bc_eval(out, key, input);
}

static bool matrix_element_prg(const sydo_ref_paramset_t* params, uint8_t* out,
                               const uint8_t* seed_pk, size_t elem_index) {
  const size_t lambda_bytes = secpar_bytes(params);
  if (params->secpar_bits == 512) {
    return prg_block_512(seed_pk, (uint32_t)elem_index, out);
  }

  const size_t blocks_per_elem = (lambda_bytes + 15) / 16;
  uint8_t block[16];
  size_t copied = 0;
  for (size_t block_idx = 0; block_idx != blocks_per_elem; ++block_idx) {
    const uint32_t counter = (uint32_t)(elem_index * blocks_per_elem + block_idx);
    if (!prg_block_128(params, seed_pk, counter, block)) {
      return false;
    }
    const size_t remaining = lambda_bytes - copied;
    const size_t take = remaining < sizeof(block) ? remaining : sizeof(block);
    memcpy(out + copied, block, take);
    copied += take;
  }
  sydo_explicit_bzero(block, sizeof(block));
  return true;
}

static void clear_unused_y_bits(uint8_t* y_storage, const sydo_ref_paramset_t* params) {
  const size_t used_bits = params->rsd_codim;
  const size_t used_bytes = (used_bits + 7) / 8;
  const size_t total_bytes = y_storage_bytes(params);
  if ((used_bits & 7u) != 0) {
    const uint8_t keep_mask = (uint8_t)((1u << (used_bits & 7u)) - 1u);
    y_storage[used_bytes - 1] &= keep_mask;
  }
  if (used_bytes < total_bytes) {
    memset(y_storage + used_bytes, 0, total_bytes - used_bytes);
  }
}

static bool compute_y(uint8_t* y_storage, const sydo_ref_paramset_t* params, const uint8_t* seed_pk,
                      const uint8_t* x_positions) {
  const uint16_t lambda_bytes = secpar_bytes(params);
  const size_t rows = (params->rsd_codim + params->secpar_bits - 1) / params->secpar_bits;
  const size_t cols = params->rsd_n;
  const size_t block_len = params->rsd_n / params->rsd_w;
  const size_t pos_bits = rsd_pos_bits(params);
  uint8_t elem[64];

  if (lambda_bytes > sizeof(elem)) {
    return false;
  }

  memset(y_storage, 0, y_storage_bytes(params));
  for (size_t row = 0; row != rows; ++row) {
    uint8_t* acc = y_storage + row * lambda_bytes;
    for (size_t block_i = 0; block_i != params->rsd_w; ++block_i) {
      const uint32_t pos = load_bits_lsb_first(x_positions, block_i * pos_bits, pos_bits);
      const uint32_t col = (uint32_t)(block_i * block_len + pos);
      const size_t elem_index = row * cols + col;
      if (!matrix_element_prg(params, elem, seed_pk, elem_index)) {
        return false;
      }
      for (size_t b = 0; b != lambda_bytes; ++b) {
        acc[b] ^= elem[b];
      }
    }
  }
  clear_unused_y_bits(y_storage, params);
  return true;
}

static bool pack_public_key(uint8_t* pk, const sydo_ref_paramset_t* params, const uint8_t* seed_pk,
                            const uint8_t* y_storage) {
  memcpy(pk, seed_pk, secpar_bytes(params));
  memcpy(pk + secpar_bytes(params), y_storage, y_vector_bytes(params));
  return true;
}

static bool sydo_ref_hash_parts(uint8_t* out, size_t out_len, const void* p0, size_t l0,
                                const void* p1, size_t l1, const void* p2, size_t l2,
                                const void* p3, size_t l3, const void* p4, size_t l4,
                                uint8_t domain) {
  const size_t input_len = l0 + l1 + l2 + l3 + l4 + 1u;
  uint8_t* input = (uint8_t*)malloc(input_len);
  size_t off = 0;
  bool ok;

  if (!input) {
    return false;
  }
  if (l0 != 0) {
    memcpy(input + off, p0, l0);
    off += l0;
  }
  if (l1 != 0) {
    memcpy(input + off, p1, l1);
    off += l1;
  }
  if (l2 != 0) {
    memcpy(input + off, p2, l2);
    off += l2;
  }
  if (l3 != 0) {
    memcpy(input + off, p3, l3);
    off += l3;
  }
  if (l4 != 0) {
    memcpy(input + off, p4, l4);
    off += l4;
  }
  input[off] = domain;
  ok = sydo_ref_xof(out, out_len, input, input_len);
  sydo_explicit_bzero(input, input_len);
  free(input);
  return ok;
}

static void expand_bits_to_bytes(uint8_t* output, size_t num_bits, const uint8_t* x) {
  for (size_t i = 0; i != num_bits; ++i) {
    output[i] = (uint8_t)(0u - ((x[i / 8u] >> (i & 7u)) & 1u));
  }
}

static void store32_le_public(uint8_t* dst, uint32_t v) {
  dst[0] = (uint8_t)(v & 0xffu);
  dst[1] = (uint8_t)((v >> 8) & 0xffu);
  dst[2] = (uint8_t)((v >> 16) & 0xffu);
  dst[3] = (uint8_t)((v >> 24) & 0xffu);
}

static bool trace_enabled(void) {
  return getenv("SYDO_REF_TRACE") != NULL;
}

static clock_t trace_clock(void) {
  return clock();
}

static void trace_stage(const char* params_name, const char* stage, clock_t start) {
  if (trace_enabled()) {
    const double ms = 1000.0 * (double)(clock() - start) / (double)CLOCKS_PER_SEC;
    fprintf(stderr, "%s %s %.2f ms\n", params_name, stage, ms);
  }
}

static uint32_t load32_le_public(const uint8_t* src) {
  return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) |
         ((uint32_t)src[3] << 24);
}

static bool delta_has_required_zero_bits(const sydo_ref_paramset_t* params, const uint8_t* delta) {
  const size_t zero_bits = sydo_ref_unused_delta_bits(params);
  for (size_t i = 0; i != zero_bits; ++i) {
    const size_t bit = (size_t)params->secpar_bits - 1u - i;
    if (((delta[bit / 8u] >> (bit & 7u)) & 1u) != 0u) {
      return false;
    }
  }
  return true;
}

int sydo_ref_keygen_from_seed(const sydo_ref_paramset_t* params, uint8_t* pk, size_t pk_len,
                              uint8_t* sk, size_t sk_len, const uint8_t* seed,
                              size_t seed_len) {
  int ret = -1;
  uint8_t* x_positions = NULL;
  uint8_t* y_storage = NULL;

  if (!params || !pk || !sk || !seed) {
    return -1;
  }
  if (pk_len < params->public_key_size || sk_len < params->secret_key_size ||
      seed_len != params->keygen_seed_size) {
    return -1;
  }

  x_positions = (uint8_t*)malloc(rsd_pos_stream_bytes(params));
  y_storage = (uint8_t*)malloc(y_storage_bytes(params));
  if (!x_positions || !y_storage) {
    goto cleanup;
  }

  const uint8_t* seed_sk = seed;
  const uint8_t* seed_pk = seed + secpar_bytes(params);
  uint8_t* witness = sk + sk_witness_offset(params);

  if (!sample_x_positions(x_positions, params, seed_sk)) {
    goto cleanup;
  }
  if (!compute_witness_from_x_positions(witness, params, x_positions)) {
    goto cleanup;
  }
  if (!compute_y(y_storage, params, seed_pk, x_positions)) {
    goto cleanup;
  }

  pack_public_key(pk, params, seed_pk, y_storage);
  memcpy(sk, pk, params->public_key_size);
  memcpy(sk + sk_seed_offset(params), seed_sk, secpar_bytes(params));
  ret = 0;

cleanup:
  if (x_positions) {
    sydo_explicit_bzero(x_positions, rsd_pos_stream_bytes(params));
    free(x_positions);
  }
  if (y_storage) {
    sydo_explicit_bzero(y_storage, y_storage_bytes(params));
    free(y_storage);
  }
  return ret;
}

int sydo_ref_derive_public_key(const sydo_ref_paramset_t* params, uint8_t* pk, size_t pk_len,
                               const uint8_t* sk, size_t sk_len) {
  int ret = -1;
  uint8_t* x_positions = NULL;
  uint8_t* y_storage = NULL;

  if (!params || !pk || !sk) {
    return -1;
  }
  if (pk_len < params->public_key_size || sk_len < params->secret_key_size) {
    return -1;
  }

  x_positions = (uint8_t*)malloc(rsd_pos_stream_bytes(params));
  y_storage = (uint8_t*)malloc(y_storage_bytes(params));
  if (!x_positions || !y_storage) {
    goto cleanup;
  }

  const uint8_t* seed_pk = sk;
  const uint8_t* seed_sk = sk + sk_seed_offset(params);
  if (!sample_x_positions(x_positions, params, seed_sk)) {
    goto cleanup;
  }
  if (!compute_y(y_storage, params, seed_pk, x_positions)) {
    goto cleanup;
  }
  pack_public_key(pk, params, seed_pk, y_storage);
  ret = 0;

cleanup:
  if (x_positions) {
    sydo_explicit_bzero(x_positions, rsd_pos_stream_bytes(params));
    free(x_positions);
  }
  if (y_storage) {
    sydo_explicit_bzero(y_storage, y_storage_bytes(params));
    free(y_storage);
  }
  return ret;
}

int sydo_ref_validate_keypair(const sydo_ref_paramset_t* params, const uint8_t* pk, size_t pk_len,
                              const uint8_t* sk, size_t sk_len) {
  uint8_t* pk_check;
  int ret;

  if (!params || !pk || !sk || pk_len != params->public_key_size ||
      sk_len != params->secret_key_size) {
    return -1;
  }

  pk_check = (uint8_t*)malloc(params->public_key_size);
  if (!pk_check) {
    return -1;
  }
  ret = sydo_ref_derive_public_key(params, pk_check, params->public_key_size, sk, sk_len);
  if (ret == 0) {
    ret = sydo_timingsafe_bcmp(pk_check, pk, params->public_key_size) == 0 ? 0 : 1;
  }
  sydo_explicit_bzero(pk_check, params->public_key_size);
  free(pk_check);
  return ret;
}

int sydo_ref_keygen(const sydo_ref_paramset_t* params, uint8_t* pk, size_t pk_len, uint8_t* sk,
                    size_t sk_len) {
  uint8_t seed[128];
  if (!params || params->keygen_seed_size > sizeof(seed)) {
    return -1;
  }
  if (rand_bytes(seed, params->keygen_seed_size) != 0) {
    return -1;
  }
  const int ret = sydo_ref_keygen_from_seed(params, pk, pk_len, sk, sk_len, seed,
                                            params->keygen_seed_size);
  sydo_explicit_bzero(seed, sizeof(seed));
  return ret;
}

int sydo_ref_sign_from_seed(const sydo_ref_paramset_t* params, uint8_t* sig, size_t sig_len,
                            const uint8_t* msg, size_t msg_len, const uint8_t* sk,
                            size_t sk_len, const uint8_t* seed, size_t seed_len) {
  uint8_t* pk_check = NULL;
  uint8_t* u = NULL;
  uint8_t* v = NULL;
  uint8_t* vole_check_transcript = NULL;
  uint8_t* qs_check = NULL;
  uint8_t* u_extended = NULL;
  uint8_t* macs_extended = NULL;
  uint8_t* delta_bytes = NULL;
  const uint8_t* pk = NULL;
  const uint8_t* seed_sk = NULL;
  const uint8_t* witness = NULL;
  sydo_ref_bavc_t bavc;
  int ret = -1;

  sydo_ref_bavc_init(&bavc);
  if (!params || !sig || !sk || !seed || (!msg && msg_len != 0)) {
    return -1;
  }
  if (sig_len < params->signature_size || sk_len != params->secret_key_size ||
      seed_len != secpar_bytes(params)) {
    return -1;
  }

  const sydo_ref_signature_layout_t layout = sydo_ref_signature_layout(params);
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t col_bytes = col_blocks * SYDO_REF_VOLE_BLOCK_BYTES;
  const size_t commit_check_bytes = 2u * lambda_bytes;
  const size_t vc_transcript_bytes = sydo_ref_vole_check_transcript_size(params);
  const size_t qs_check_bytes = lambda_bytes;
  const size_t rows_ext = sydo_ref_quicksilver_rows_padded_extended(params);
  const size_t u_extended_bytes = sydo_ref_witness_extended_bytes(params) + layout.qs_proof_size;

  if (layout.total_size != params->signature_size) {
    return -1;
  }

  pk = sk;
  seed_sk = sk + sk_seed_offset(params);
  witness = sk + sk_witness_offset(params);

  pk_check = (uint8_t*)malloc(params->public_key_size);
  if (!pk_check) {
    return -1;
  }
  if (sydo_ref_derive_public_key(params, pk_check, params->public_key_size, sk, sk_len) != 0 ||
      sydo_timingsafe_bcmp(pk_check, pk, params->public_key_size) != 0) {
    goto cleanup;
  }

  memset(sig, 0, sig_len);
  u = (uint8_t*)malloc(col_bytes);
  v = (uint8_t*)calloc((size_t)params->secpar_bits, col_bytes);
  vole_check_transcript = (uint8_t*)malloc(vc_transcript_bytes);
  qs_check = (uint8_t*)malloc(qs_check_bytes);
  u_extended = (uint8_t*)calloc(1u, u_extended_bytes);
  macs_extended = (uint8_t*)malloc(rows_ext * lambda_bytes);
  delta_bytes = (uint8_t*)malloc(sydo_ref_delta_bits(params));
  uint8_t* mu = (uint8_t*)malloc(2u * lambda_bytes);
  uint8_t* seed_iv = (uint8_t*)malloc(2u * lambda_bytes);
  uint8_t* commit_check = (uint8_t*)malloc(commit_check_bytes);
  uint8_t* chal1 = (uint8_t*)malloc(sydo_ref_vole_check_challenge_size(params));
  uint8_t* chal2 = (uint8_t*)malloc(sydo_ref_qs_challenge_bytes(params));
  if (!u || !v || !vole_check_transcript || !qs_check || !u_extended || !macs_extended ||
      !delta_bytes || !mu || !seed_iv || !commit_check || !chal1 || !chal2) {
    free(mu);
    free(seed_iv);
    free(commit_check);
    free(chal1);
    free(chal2);
    goto cleanup;
  }

  if (!sydo_ref_hash_parts(mu, 2u * lambda_bytes, pk, params->public_key_size, msg, msg_len,
                           NULL, 0, NULL, 0, NULL, 0, 0x08)) {
    goto sign_buffers_cleanup;
  }
  clock_t stage_start = trace_clock();
  if (!sydo_ref_hash_parts(seed_iv, 2u * lambda_bytes, seed_sk, lambda_bytes, mu,
                           2u * lambda_bytes, seed, seed_len, NULL, 0, NULL, 0, 0x03)) {
    goto sign_buffers_cleanup;
  }
  trace_stage(params->name, "sign seed_iv", stage_start);

  const uint8_t* commit_seed = seed_iv;
  const uint8_t* iv = seed_iv + lambda_bytes;
  stage_start = trace_clock();
  if (!sydo_ref_vole_commit(params, commit_seed, iv, &bavc, u, v, sig + layout.vole_commit_offset,
                            commit_check)) {
    goto sign_buffers_cleanup;
  }
  trace_stage(params->name, "sign vole_commit", stage_start);
  stage_start = trace_clock();
  if (!sydo_ref_hash_parts(chal1, sydo_ref_vole_check_challenge_size(params), mu,
                           2u * lambda_bytes, commit_check, commit_check_bytes,
                           sig + layout.vole_commit_offset, layout.vole_commit_size, iv,
                           lambda_bytes, NULL, 0, 0x09)) {
    goto sign_buffers_cleanup;
  }
  trace_stage(params->name, "sign chal1", stage_start);
  stage_start = trace_clock();
  if (!sydo_ref_vole_check_sender(params, u, v, chal1, sig + layout.vole_check_offset,
                                  vole_check_transcript, vc_transcript_bytes)) {
    goto sign_buffers_cleanup;
  }
  trace_stage(params->name, "sign vole_check", stage_start);

  stage_start = trace_clock();
  memcpy(sig + layout.correction_offset, u, layout.correction_size);
  for (size_t i = 0; i != layout.correction_size; ++i) {
    sig[layout.correction_offset + i] ^= witness[i];
  }
  trace_stage(params->name, "sign correction", stage_start);
  stage_start = trace_clock();
  if (!sydo_ref_hash_parts(chal2, sydo_ref_qs_challenge_bytes(params), chal1,
                           sydo_ref_vole_check_challenge_size(params),
                           vole_check_transcript, vc_transcript_bytes,
                           sig + layout.correction_offset, layout.correction_size, NULL, 0,
                           NULL, 0, 0x0A)) {
    goto sign_buffers_cleanup;
  }
  if (!sydo_ref_rsd_extend_witness(u_extended, u_extended_bytes, params, witness)) {
    goto sign_buffers_cleanup;
  }
  memcpy(u_extended + sydo_ref_witness_extended_bytes(params),
         u + sydo_ref_witness_bits(params) / 8u, layout.qs_proof_size);
  if (!sydo_ref_compute_macs_extended_prover(params, v, macs_extended)) {
    goto sign_buffers_cleanup;
  }
  if (!sydo_ref_qs_prove_rsd(params, pk, chal2, u_extended, macs_extended,
                             sig + layout.qs_proof_offset, qs_check)) {
    goto sign_buffers_cleanup;
  }
  trace_stage(params->name, "sign qs_rsd", stage_start);

  bool opened = false;
  stage_start = trace_clock();
  for (uint32_t counter = 0; counter != UINT32_MAX; ++counter) {
    uint8_t counter_bytes[4];
    store32_le_public(counter_bytes, counter);
    if (!sydo_ref_hash_parts(sig + layout.delta_offset, layout.delta_size, chal2,
                             sydo_ref_qs_challenge_bytes(params), qs_check, qs_check_bytes,
                             sig + layout.qs_proof_offset, layout.qs_proof_size, counter_bytes,
                             sizeof(counter_bytes), NULL, 0, 0x0B)) {
      goto sign_buffers_cleanup;
    }
    if (!delta_has_required_zero_bits(params, sig + layout.delta_offset)) {
      continue;
    }
    expand_bits_to_bytes(delta_bytes, sydo_ref_delta_bits(params), sig + layout.delta_offset);
    if (sydo_ref_bavc_open(params, &bavc, delta_bytes, sig + layout.bavc_open_offset)) {
      memcpy(sig + layout.iv_offset, iv, layout.iv_size);
      store32_le_public(sig + layout.grinding_counter_offset, counter);
      opened = true;
      break;
    }
  }
  trace_stage(params->name, "sign grind_open", stage_start);
  if (!opened) {
    goto sign_buffers_cleanup;
  }

  ret = 0;

sign_buffers_cleanup:
  if (mu) {
    sydo_explicit_bzero(mu, 2u * lambda_bytes);
    free(mu);
  }
  if (seed_iv) {
    sydo_explicit_bzero(seed_iv, 2u * lambda_bytes);
    free(seed_iv);
  }
  if (commit_check) {
    sydo_explicit_bzero(commit_check, commit_check_bytes);
    free(commit_check);
  }
  if (chal1) {
    sydo_explicit_bzero(chal1, sydo_ref_vole_check_challenge_size(params));
    free(chal1);
  }
  if (chal2) {
    sydo_explicit_bzero(chal2, sydo_ref_qs_challenge_bytes(params));
    free(chal2);
  }

cleanup:
  sydo_ref_bavc_clear(&bavc);
  if (u) {
    sydo_explicit_bzero(u, col_bytes);
    free(u);
  }
  if (v) {
    sydo_explicit_bzero(v, (size_t)params->secpar_bits * col_bytes);
    free(v);
  }
  if (vole_check_transcript) {
    sydo_explicit_bzero(vole_check_transcript, vc_transcript_bytes);
    free(vole_check_transcript);
  }
  if (qs_check) {
    sydo_explicit_bzero(qs_check, qs_check_bytes);
    free(qs_check);
  }
  if (u_extended) {
    sydo_explicit_bzero(u_extended, u_extended_bytes);
    free(u_extended);
  }
  if (macs_extended) {
    sydo_explicit_bzero(macs_extended, rows_ext * lambda_bytes);
    free(macs_extended);
  }
  if (delta_bytes) {
    sydo_explicit_bzero(delta_bytes, sydo_ref_delta_bits(params));
    free(delta_bytes);
  }
  sydo_ref_prg_cache_clear();
  if (pk_check) {
    sydo_explicit_bzero(pk_check, params->public_key_size);
    free(pk_check);
  }
  return ret;
}

int sydo_ref_sign(const sydo_ref_paramset_t* params, uint8_t* sig, size_t sig_len,
                  const uint8_t* msg, size_t msg_len, const uint8_t* sk, size_t sk_len) {
  uint8_t seed[64];
  int ret = -1;

  if (!params || secpar_bytes(params) > sizeof(seed)) {
    return -1;
  }
  if (rand_bytes(seed, secpar_bytes(params)) != 0) {
    return -1;
  }
  ret = sydo_ref_sign_from_seed(params, sig, sig_len, msg, msg_len, sk, sk_len, seed,
                                secpar_bytes(params));
  sydo_explicit_bzero(seed, sizeof(seed));
  return ret;
}

int sydo_ref_verify(const sydo_ref_paramset_t* params, const uint8_t* pk, size_t pk_len,
                    const uint8_t* sig, size_t sig_len, const uint8_t* msg, size_t msg_len) {
  uint8_t* q = NULL;
  uint8_t* delta_bytes = NULL;
  uint8_t* commit_check = NULL;
  uint8_t* chal1 = NULL;
  uint8_t* chal2 = NULL;
  uint8_t* vole_check_transcript = NULL;
  uint8_t* qs_check = NULL;
  uint8_t* macs_extended = NULL;
  uint8_t* correction_padded = NULL;
  uint8_t* expected_delta = NULL;
  uint8_t* mu = NULL;
  int ret = -1;

  if (!params || !pk || !sig || (!msg && msg_len != 0)) {
    return -1;
  }
  if (pk_len != params->public_key_size || sig_len != params->signature_size) {
    return -1;
  }
  const sydo_ref_signature_layout_t layout = sydo_ref_signature_layout(params);
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t col_bytes = col_blocks * SYDO_REF_VOLE_BLOCK_BYTES;
  const size_t commit_check_bytes = 2u * lambda_bytes;
  const size_t vc_transcript_bytes = sydo_ref_vole_check_transcript_size(params);
  const size_t qs_check_bytes = lambda_bytes;
  const size_t rows_ext = sydo_ref_quicksilver_rows_padded_extended(params);
  const size_t correction_padded_bytes =
      sydo_ref_witness_blocks(params) * SYDO_REF_VOLE_BLOCK_BYTES;
  if (layout.total_size != params->signature_size) {
    return -1;
  }
  if (!delta_has_required_zero_bits(params, sig + layout.delta_offset)) {
    return -1;
  }
  q = (uint8_t*)calloc((size_t)params->secpar_bits, col_bytes);
  delta_bytes = (uint8_t*)malloc(sydo_ref_delta_bits(params));
  commit_check = (uint8_t*)malloc(commit_check_bytes);
  chal1 = (uint8_t*)malloc(sydo_ref_vole_check_challenge_size(params));
  chal2 = (uint8_t*)malloc(sydo_ref_qs_challenge_bytes(params));
  vole_check_transcript = (uint8_t*)malloc(vc_transcript_bytes);
  qs_check = (uint8_t*)malloc(qs_check_bytes);
  macs_extended = (uint8_t*)malloc(rows_ext * lambda_bytes);
  correction_padded = (uint8_t*)calloc(1u, correction_padded_bytes);
  expected_delta = (uint8_t*)malloc(layout.delta_size);
  mu = (uint8_t*)malloc(2u * lambda_bytes);
  if (!q || !delta_bytes || !commit_check || !chal1 || !chal2 || !vole_check_transcript ||
      !qs_check || !macs_extended || !correction_padded || !expected_delta || !mu) {
    goto cleanup;
  }
  expand_bits_to_bytes(delta_bytes, sydo_ref_delta_bits(params), sig + layout.delta_offset);
  clock_t verify_stage_start = trace_clock();
  if (!sydo_ref_vole_reconstruct(params, sig + layout.iv_offset, q, delta_bytes,
                                 sig + layout.vole_commit_offset, sig + layout.bavc_open_offset,
                                 commit_check)) {
    goto cleanup;
  }
  trace_stage(params->name, "verify vole_reconstruct", verify_stage_start);
  verify_stage_start = trace_clock();
  if (!sydo_ref_hash_parts(mu, 2u * lambda_bytes, pk, params->public_key_size, msg, msg_len,
                           NULL, 0, NULL, 0, NULL, 0, 0x08)) {
    goto cleanup;
  }
  if (!sydo_ref_hash_parts(chal1, sydo_ref_vole_check_challenge_size(params), mu,
                           2u * lambda_bytes, commit_check, commit_check_bytes,
                           sig + layout.vole_commit_offset, layout.vole_commit_size,
                           sig + layout.iv_offset, layout.iv_size, NULL, 0, 0x09)) {
    goto cleanup;
  }
  if (!sydo_ref_vole_check_receiver(params, q, delta_bytes, chal1, sig + layout.vole_check_offset,
                                    vole_check_transcript, vc_transcript_bytes)) {
    goto cleanup;
  }
  trace_stage(params->name, "verify vole_check", verify_stage_start);
  verify_stage_start = trace_clock();
  if (!sydo_ref_hash_parts(chal2, sydo_ref_qs_challenge_bytes(params), chal1,
                           sydo_ref_vole_check_challenge_size(params),
                           vole_check_transcript, vc_transcript_bytes,
                           sig + layout.correction_offset, layout.correction_size, NULL, 0,
                           NULL, 0, 0x0A)) {
    goto cleanup;
  }
  memcpy(correction_padded, sig + layout.correction_offset, layout.correction_size);
  if (!sydo_ref_compute_macs_extended_verifier(params, q, correction_padded, delta_bytes,
                                               macs_extended)) {
    if (trace_enabled()) {
      fprintf(stderr, "%s verify macs_extended failed\n", params->name);
    }
    goto cleanup;
  }
  if (!sydo_ref_qs_verify_rsd(params, pk, chal2, sig + layout.delta_offset, macs_extended,
                              sig + layout.qs_proof_offset, qs_check)) {
    if (trace_enabled()) {
      fprintf(stderr, "%s verify qs_rsd failed\n", params->name);
    }
    goto cleanup;
  }
  trace_stage(params->name, "verify qs_rsd", verify_stage_start);
  verify_stage_start = trace_clock();
  uint8_t counter_bytes[4];
  const uint32_t counter = load32_le_public(sig + layout.grinding_counter_offset);
  store32_le_public(counter_bytes, counter);
  if (!sydo_ref_hash_parts(expected_delta, layout.delta_size, chal2,
                           sydo_ref_qs_challenge_bytes(params), qs_check, qs_check_bytes,
                           sig + layout.qs_proof_offset, layout.qs_proof_size, counter_bytes,
                           sizeof(counter_bytes), NULL, 0, 0x0B)) {
    if (trace_enabled()) {
      fprintf(stderr, "%s verify delta hash failed\n", params->name);
    }
    goto cleanup;
  }
  if (sydo_timingsafe_bcmp(expected_delta, sig + layout.delta_offset, layout.delta_size) != 0) {
    if (trace_enabled()) {
      fprintf(stderr, "%s verify delta mismatch\n", params->name);
    }
    goto cleanup;
  }
  trace_stage(params->name, "verify transcript", verify_stage_start);

  ret = 0;

cleanup:
  if (q) {
    sydo_explicit_bzero(q, (size_t)params->secpar_bits * col_bytes);
    free(q);
  }
  if (delta_bytes) {
    sydo_explicit_bzero(delta_bytes, sydo_ref_delta_bits(params));
    free(delta_bytes);
  }
  if (commit_check) {
    sydo_explicit_bzero(commit_check, commit_check_bytes);
    free(commit_check);
  }
  if (chal1) {
    sydo_explicit_bzero(chal1, sydo_ref_vole_check_challenge_size(params));
    free(chal1);
  }
  if (chal2) {
    sydo_explicit_bzero(chal2, sydo_ref_qs_challenge_bytes(params));
    free(chal2);
  }
  if (vole_check_transcript) {
    sydo_explicit_bzero(vole_check_transcript, vc_transcript_bytes);
    free(vole_check_transcript);
  }
  if (qs_check) {
    sydo_explicit_bzero(qs_check, qs_check_bytes);
    free(qs_check);
  }
  if (macs_extended) {
    sydo_explicit_bzero(macs_extended, rows_ext * lambda_bytes);
    free(macs_extended);
  }
  if (correction_padded) {
    sydo_explicit_bzero(correction_padded, correction_padded_bytes);
    free(correction_padded);
  }
  if (expected_delta) {
    sydo_explicit_bzero(expected_delta, layout.delta_size);
    free(expected_delta);
  }
  if (mu) {
    sydo_explicit_bzero(mu, 2u * lambda_bytes);
    free(mu);
  }
  sydo_ref_prg_cache_clear();
  return ret;
}
