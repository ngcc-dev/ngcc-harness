/*
 * SPDX-License-Identifier: MIT
 *
 * Pseudorandom generator wrappers (SM4-only, pure C reference path).
 */
#include "prg.h"

#include "utils_sm4/sm4.h"

#include <assert.h>
#include <string.h>

#ifndef PRG_BLOCK_SIZE
#define PRG_BLOCK_SIZE SM4_BLOCK_SIZE
#endif

#ifndef PRG_BATCH_BLOCKS
#define PRG_BATCH_BLOCKS 32
#endif

#define PRG_162_BYTES 162u

static inline void prg_increment_iv(uint8_t* iv) {
  uint32_t iv0;
  memcpy(&iv0, iv, sizeof(uint32_t));
  iv0 = htole32(le32toh(iv0) + 1);
  memcpy(iv, &iv0, sizeof(uint32_t));
}

static void add_to_upper_word(uint8_t* iv, uint32_t tweak) {
  uint32_t iv3;
  memcpy(&iv3, iv + PRG_BLOCK_SIZE - sizeof(uint32_t), sizeof(uint32_t));
  iv3 = htole32(le32toh(iv3) + tweak);
  memcpy(iv + PRG_BLOCK_SIZE - sizeof(uint32_t), &iv3, sizeof(uint32_t));
}

static void generic_prg(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                        unsigned int seclvl, size_t outlen) {
  assert(seclvl == 128u);

  generic_sm4_ecb_t ctx;
  int ret = generic_sm4_ecb_new(&ctx, key, seclvl);
  assert(ret == 0);
  (void)ret;

  uint8_t counter_blocks[PRG_BATCH_BLOCKS * PRG_BLOCK_SIZE];
  size_t full_blocks = outlen / PRG_BLOCK_SIZE;

  while (full_blocks) {
    const size_t batch_blocks = full_blocks > PRG_BATCH_BLOCKS ? PRG_BATCH_BLOCKS : full_blocks;

    for (size_t i = 0; i < batch_blocks; ++i) {
      memcpy(counter_blocks + i * PRG_BLOCK_SIZE, internal_iv, PRG_BLOCK_SIZE);
      prg_increment_iv(internal_iv);
    }

    ret = generic_sm4_ecb_encrypt(&ctx, out, counter_blocks, batch_blocks);
    assert(ret == 0);
    (void)ret;

    out += batch_blocks * PRG_BLOCK_SIZE;
    full_blocks -= batch_blocks;
  }

  if (outlen % PRG_BLOCK_SIZE) {
    uint8_t last_block[PRG_BLOCK_SIZE];
    ret = generic_sm4_ecb_encrypt(&ctx, last_block, internal_iv, 1);
    assert(ret == 0);
    (void)ret;
    memcpy(out, last_block, outlen % PRG_BLOCK_SIZE);
  }

  generic_sm4_ecb_free(&ctx);
}

static void generic_prg_with_ctx(generic_sm4_ecb_t* ctx, uint8_t* internal_iv, uint8_t* out,
                                 size_t outlen) {
  uint8_t counter_blocks[PRG_BATCH_BLOCKS * PRG_BLOCK_SIZE];
  size_t full_blocks = outlen / PRG_BLOCK_SIZE;

  while (full_blocks) {
    const size_t batch_blocks = full_blocks > PRG_BATCH_BLOCKS ? PRG_BATCH_BLOCKS : full_blocks;

    for (size_t i = 0; i < batch_blocks; ++i) {
      memcpy(counter_blocks + i * PRG_BLOCK_SIZE, internal_iv, PRG_BLOCK_SIZE);
      prg_increment_iv(internal_iv);
    }

    const int ret = generic_sm4_ecb_encrypt(ctx, out, counter_blocks, batch_blocks);
    assert(ret == 0);
    (void)ret;

    out += batch_blocks * PRG_BLOCK_SIZE;
    full_blocks -= batch_blocks;
  }

  if (outlen % PRG_BLOCK_SIZE) {
    uint8_t last_block[PRG_BLOCK_SIZE];
    const int ret = generic_sm4_ecb_encrypt(ctx, last_block, internal_iv, 1);
    assert(ret == 0);
    (void)ret;
    memcpy(out, last_block, outlen % PRG_BLOCK_SIZE);
  }
}

static inline void prg_2_lambda_scalar(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                                       uint8_t* out, unsigned int seclvl) {
  uint8_t internal_iv[PRG_BLOCK_SIZE];
  memcpy(internal_iv, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(internal_iv, tweak);
  generic_prg(key, internal_iv, out, seclvl, (size_t)(2u * (seclvl / 8u)));
}

static inline void prg_2lambda_and_variable_same_key(const uint8_t* key, const uint8_t* iv,
                                                     uint32_t tweak_2lambda, uint32_t tweak_var,
                                                     uint8_t* out_2lambda, uint8_t* out_var,
                                                     size_t var_len, unsigned int seclvl) {
  uint8_t iv_2lambda[PRG_BLOCK_SIZE];
  uint8_t iv_var[PRG_BLOCK_SIZE];
  generic_sm4_ecb_t ctx;
  int ret = generic_sm4_ecb_new(&ctx, key, seclvl);
  assert(ret == 0);
  (void)ret;

  memcpy(iv_2lambda, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(iv_2lambda, tweak_2lambda);
  generic_prg_with_ctx(&ctx, iv_2lambda, out_2lambda, (size_t)(2u * (seclvl / 8u)));

  memcpy(iv_var, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(iv_var, tweak_var);
  generic_prg_with_ctx(&ctx, iv_var, out_var, var_len);

  generic_sm4_ecb_free(&ctx);
}

void prg_2_lambda_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                    uint32_t tweak_base, uint8_t* out, size_t out_stride,
                                    unsigned int seclvl, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    prg_2_lambda_scalar(keys + i * key_stride, iv, tweak_base + (uint32_t)i, out + i * out_stride,
                        seclvl);
  }
}

static inline uint8_t* prg_sd_xor_map_slot(uint8_t* sd_out, size_t sd_stride, size_t index,
                                           size_t sd_index_base, size_t sd_index_xor) {
  const size_t mapped = (sd_index_base + index) ^ sd_index_xor;
  if (mapped == 0) {
    return NULL;
  }
  return sd_out + mapped * sd_stride;
}

static void prg_2_lambda_and_fixed_tweak_independent_batch_with_sd(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_var, uint8_t* sd_out, size_t sd_stride, uint8_t* out_2lambda,
    size_t out_2lambda_stride, uint8_t* out_var, size_t out_var_stride, size_t out_var_len,
    unsigned int seclvl, size_t count) {
  if (count == 0) {
    return;
  }

  const size_t lambda_bytes = seclvl / 8u;
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    memcpy(sd_out + i * sd_stride, key_i, lambda_bytes);
    prg_2lambda_and_variable_same_key(
        key_i, iv, tweak_2lambda, tweak_var, out_2lambda + i * out_2lambda_stride,
        out_var + i * out_var_stride, out_var_len, seclvl);
  }
}

static void prg_2_lambda_and_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_var, uint8_t* sd_out, size_t sd_stride, size_t sd_index_base,
    size_t sd_index_xor, uint8_t* out_2lambda, size_t out_2lambda_stride, uint8_t* out_var,
    size_t out_var_stride, size_t out_var_len, unsigned int seclvl, size_t count) {
  if (count == 0) {
    return;
  }

  const size_t lambda_bytes = seclvl / 8u;
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* sd_i = prg_sd_xor_map_slot(sd_out, sd_stride, i, sd_index_base, sd_index_xor);
    uint8_t* out_var_i =
        prg_sd_xor_map_slot(out_var, out_var_stride, i, sd_index_base, sd_index_xor);

    if (sd_i) {
      memcpy(sd_i, key_i, lambda_bytes);
    }
    if (out_var_i) {
      prg_2lambda_and_variable_same_key(key_i, iv, tweak_2lambda, tweak_var,
                                        out_2lambda + i * out_2lambda_stride, out_var_i,
                                        out_var_len, seclvl);
    } else {
      prg_2_lambda_scalar(key_i, iv, tweak_2lambda, out_2lambda + i * out_2lambda_stride,
                          seclvl);
    }
  }
}

void prg_2_lambda_and_162_fixed_tweak_independent_batch_with_sd(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_162, uint8_t* sd_out, size_t sd_stride, uint8_t* out_2lambda,
    size_t out_2lambda_stride, uint8_t* out_162, size_t out_162_stride, unsigned int seclvl,
    size_t count) {
  prg_2_lambda_and_fixed_tweak_independent_batch_with_sd(
      keys, key_stride, iv, tweak_2lambda, tweak_162, sd_out, sd_stride, out_2lambda,
      out_2lambda_stride, out_162, out_162_stride, PRG_162_BYTES, seclvl, count);
}

void prg_2_lambda_and_162_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_162, uint8_t* sd_out, size_t sd_stride, size_t sd_index_base,
    size_t sd_index_xor, uint8_t* out_2lambda, size_t out_2lambda_stride, uint8_t* out_162,
    size_t out_162_stride, unsigned int seclvl, size_t count) {
  prg_2_lambda_and_fixed_tweak_independent_batch_with_sd_xor_map(
      keys, key_stride, iv, tweak_2lambda, tweak_162, sd_out, sd_stride, sd_index_base,
      sd_index_xor, out_2lambda, out_2lambda_stride, out_162, out_162_stride, PRG_162_BYTES,
      seclvl, count);
}
