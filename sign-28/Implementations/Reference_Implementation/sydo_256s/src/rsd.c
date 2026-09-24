/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "rsd.h"

#include "aes.h"
#include "compat.h"
#include "lib/blake2/ref/blake2.h"
#include "hash.h"
#include "internal.h"
#include "primitives.h"

#include <stdlib.h>
#include <string.h>

enum {
  SYDO_REF_RSD_ELEMENTARY_VECTOR_LEN = 279
};

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

static size_t rank_full_subset_bits(const uint8_t* in, size_t n, size_t w, size_t src_off,
                                    bool* ok) {
  size_t rank = 0;
  size_t left = w;
  *ok = true;

  for (size_t pos = 0; pos != n; ++pos) {
    if (!get_bit_lsb_first(in, src_off + pos)) {
      continue;
    }
    if (left == 0 || (n - pos) < left) {
      *ok = false;
      return 0;
    }
    rank += ncr(n - pos - 1, left);
    --left;
  }

  if (left != 0) {
    *ok = false;
    return 0;
  }
  return rank;
}

static bool rank_hamming_ball_bits(size_t* rank_out, const uint8_t* in, size_t n, size_t r,
                                   size_t src_off) {
  size_t weight = 0;
  size_t prefix = 0;
  bool ok = false;

  for (size_t pos = 0; pos != n; ++pos) {
    if (get_bit_lsb_first(in, src_off + pos)) {
      ++weight;
    }
  }
  if (weight > r) {
    return false;
  }

  for (size_t w = 0; w != weight; ++w) {
    prefix += ncr(n, w);
  }
  *rank_out = prefix + rank_full_subset_bits(in, n, weight, src_off, &ok);
  return ok;
}

static void store32_le(uint8_t* dst, uint32_t v) {
  dst[0] = (uint8_t)(v & 0xffu);
  dst[1] = (uint8_t)((v >> 8) & 0xffu);
  dst[2] = (uint8_t)((v >> 16) & 0xffu);
  dst[3] = (uint8_t)((v >> 24) & 0xffu);
}

size_t sydo_ref_rsd_position_bits(const sydo_ref_paramset_t* params) {
  return sydo_ref_ceil_log2_size(params->rsd_n / params->rsd_w);
}

size_t sydo_ref_rsd_position_bytes(const sydo_ref_paramset_t* params) {
  return ((size_t)params->rsd_w * sydo_ref_rsd_position_bits(params) + 7u) / 8u;
}

size_t sydo_ref_rsd_y_storage_bytes(const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t rows = ((size_t)params->rsd_codim + params->secpar_bits - 1u) / params->secpar_bits;
  return rows * lambda_bytes;
}

static bool xof_domain(uint8_t* out, size_t out_len, uint8_t domain, const uint8_t* part0,
                       size_t part0_len, const uint8_t* part1, size_t part1_len) {
  const size_t input_len = part0_len + part1_len + 1u;
  uint8_t* input = (uint8_t*)malloc(input_len);
  bool ok;

  if (!input) {
    return false;
  }
  if (part0_len != 0) {
    memcpy(input, part0, part0_len);
  }
  if (part1_len != 0) {
    memcpy(input + part0_len, part1, part1_len);
  }
  input[input_len - 1u] = domain;
  ok = sydo_ref_xof(out, out_len, input, input_len);
  sydo_explicit_bzero(input, input_len);
  free(input);
  return ok;
}

bool sydo_ref_rsd_sample_positions(uint8_t* x_pos_out, const sydo_ref_paramset_t* params,
                                   const uint8_t* seed_sk) {
  const size_t seed_len = sydo_ref_secpar_bytes(params);
  const size_t block_len = params->rsd_n / params->rsd_w;
  const size_t pos_bits = sydo_ref_rsd_position_bits(params);
  const size_t pos_stream_len = sydo_ref_rsd_position_bytes(params);
  const uint64_t two32 = UINT64_C(1) << 32;
  const uint64_t t_max = (two32 / block_len) * block_len;
  uint8_t stream_block[32];
  size_t stream_off = sizeof(stream_block);
  uint32_t stream_counter = 0;
  size_t block_i = 0;

  if (params->rsd_n % params->rsd_w != 0 ||
      block_len != SYDO_REF_RSD_ELEMENTARY_VECTOR_LEN) {
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
      if (!xof_domain(stream_block, sizeof(stream_block), 0x58, seed_sk, seed_len, counter_bytes,
                      sizeof(counter_bytes))) {
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

bool sydo_ref_rsd_positions_to_witness(uint8_t* witness, const sydo_ref_paramset_t* params,
                                       const uint8_t* x_positions) {
  const size_t pos_bits = sydo_ref_rsd_position_bits(params);
  const size_t row_bits = SYDO_REF_RSD_BLOCK0_SIZE + SYDO_REF_RSD_BLOCK1_SIZE;
  const size_t block0_terms =
      hamming_ball_count(SYDO_REF_RSD_BLOCK0_SIZE, SYDO_REF_RSD_BLOCK0_WEIGHT);
  const size_t block1_terms =
      hamming_ball_count(SYDO_REF_RSD_BLOCK1_SIZE, SYDO_REF_RSD_BLOCK1_WEIGHT);

  if (block0_terms * block1_terms != SYDO_REF_RSD_ELEMENTARY_VECTOR_LEN) {
    return false;
  }
  if ((size_t)params->rsd_w * row_bits > (size_t)params->witness_size * 8u) {
    return false;
  }
  memset(witness, 0, params->witness_size);

  for (size_t i = 0; i != params->rsd_w; ++i) {
    const size_t idx = load_bits_lsb_first(x_positions, i * pos_bits, pos_bits);
    const size_t local0 = idx / block1_terms;
    const size_t local1 = idx % block1_terms;
    const size_t row_off = i * row_bits;
    if (idx >= SYDO_REF_RSD_ELEMENTARY_VECTOR_LEN) {
      return false;
    }
    if (!write_hamming_ball_bits(witness, SYDO_REF_RSD_BLOCK0_SIZE,
                                 SYDO_REF_RSD_BLOCK0_WEIGHT, local0, row_off)) {
      return false;
    }
    if (!write_hamming_ball_bits(witness, SYDO_REF_RSD_BLOCK1_SIZE,
                                 SYDO_REF_RSD_BLOCK1_WEIGHT, local1,
                                 row_off + SYDO_REF_RSD_BLOCK0_SIZE)) {
      return false;
    }
  }
  return true;
}

bool sydo_ref_rsd_witness_to_positions(uint8_t* x_positions, const sydo_ref_paramset_t* params,
                                       const uint8_t* witness) {
  const size_t pos_bits = sydo_ref_rsd_position_bits(params);
  const size_t row_bits = SYDO_REF_RSD_BLOCK0_SIZE + SYDO_REF_RSD_BLOCK1_SIZE;
  const size_t block1_terms =
      hamming_ball_count(SYDO_REF_RSD_BLOCK1_SIZE, SYDO_REF_RSD_BLOCK1_WEIGHT);

  memset(x_positions, 0, sydo_ref_rsd_position_bytes(params));
  for (size_t i = 0; i != params->rsd_w; ++i) {
    const size_t row_off = i * row_bits;
    size_t rank0 = 0;
    size_t rank1 = 0;
    const bool ok0 = rank_hamming_ball_bits(&rank0, witness, SYDO_REF_RSD_BLOCK0_SIZE,
                                            SYDO_REF_RSD_BLOCK0_WEIGHT, row_off);
    const bool ok1 = rank_hamming_ball_bits(&rank1, witness, SYDO_REF_RSD_BLOCK1_SIZE,
                                            SYDO_REF_RSD_BLOCK1_WEIGHT,
                                            row_off + SYDO_REF_RSD_BLOCK0_SIZE);
    const size_t idx = rank0 * block1_terms + rank1;
    if (!ok0 || !ok1 || idx >= SYDO_REF_RSD_ELEMENTARY_VECTOR_LEN) {
      return false;
    }
    store_bits_lsb_first(x_positions, i * pos_bits, pos_bits, (uint32_t)idx);
  }
  return true;
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
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  if (params->secpar_bits == 512) {
    return prg_block_512(seed_pk, (uint32_t)elem_index, out);
  }

  const size_t blocks_per_elem = (lambda_bytes + 15u) / 16u;
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
  const size_t used_bytes = (used_bits + 7u) / 8u;
  const size_t total_bytes = sydo_ref_rsd_y_storage_bytes(params);
  if ((used_bits & 7u) != 0) {
    const uint8_t keep_mask = (uint8_t)((1u << (used_bits & 7u)) - 1u);
    y_storage[used_bytes - 1u] &= keep_mask;
  }
  if (used_bytes < total_bytes) {
    memset(y_storage + used_bytes, 0, total_bytes - used_bytes);
  }
}

bool sydo_ref_rsd_compute_y(uint8_t* y_storage, const sydo_ref_paramset_t* params,
                            const uint8_t* seed_pk, const uint8_t* x_positions) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t rows = ((size_t)params->rsd_codim + params->secpar_bits - 1u) / params->secpar_bits;
  const size_t cols = params->rsd_n;
  const size_t block_len = params->rsd_n / params->rsd_w;
  const size_t pos_bits = sydo_ref_rsd_position_bits(params);
  uint8_t elem[64];

  if (lambda_bytes > sizeof(elem)) {
    return false;
  }

  memset(y_storage, 0, sydo_ref_rsd_y_storage_bytes(params));
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
  sydo_explicit_bzero(elem, sizeof(elem));
  return true;
}

bool sydo_ref_rsd_witness_matches_public_key(const sydo_ref_paramset_t* params, const uint8_t* pk,
                                             const uint8_t* witness) {
  bool ok = false;
  uint8_t* x_positions = NULL;
  uint8_t* y_storage = NULL;
  const uint8_t* seed_pk = pk;
  const uint8_t* pk_y = pk + sydo_ref_secpar_bytes(params);

  x_positions = (uint8_t*)malloc(sydo_ref_rsd_position_bytes(params));
  y_storage = (uint8_t*)malloc(sydo_ref_rsd_y_storage_bytes(params));
  if (!x_positions || !y_storage) {
    goto cleanup;
  }
  if (!sydo_ref_rsd_witness_to_positions(x_positions, params, witness)) {
    goto cleanup;
  }
  if (!sydo_ref_rsd_compute_y(y_storage, params, seed_pk, x_positions)) {
    goto cleanup;
  }
  ok = sydo_timingsafe_bcmp(y_storage, pk_y, params->public_key_size -
                                               sydo_ref_secpar_bytes(params)) == 0;

cleanup:
  if (x_positions) {
    sydo_explicit_bzero(x_positions, sydo_ref_rsd_position_bytes(params));
    free(x_positions);
  }
  if (y_storage) {
    sydo_explicit_bzero(y_storage, sydo_ref_rsd_y_storage_bytes(params));
    free(y_storage);
  }
  return ok;
}

bool sydo_ref_rsd_extend_witness(uint8_t* extended_witness, size_t extended_witness_len,
                                 const sydo_ref_paramset_t* params, const uint8_t* witness) {
  const size_t witness_bytes = sydo_ref_witness_extended_bytes(params);
  if (extended_witness_len < witness_bytes || params->witness_size < witness_bytes) {
    return false;
  }
  memset(extended_witness, 0, extended_witness_len);
  memcpy(extended_witness, witness, witness_bytes);
  return true;
}
