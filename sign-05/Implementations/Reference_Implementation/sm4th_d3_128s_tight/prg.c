/*
 * SPDX-License-Identifier: MIT
 *
 * sm4th_d3_128s_tight PRG layer:
 * - only 256-bit security path is kept
 * - PRG backend is Ballet-256/256(coreE portable)
 */

#include "prg.h"

#include "endian_compat.h"

#if defined(USE_BALLET) && (USE_BALLET == 1) && defined(HAVE_BALLET_CORE) && (HAVE_BALLET_CORE == 1)
#include "Ballet256256_coreE.h"
#else
#error "sm4th_d3_128s_tight requires USE_BALLET=1 and HAVE_BALLET_CORE=1"
#endif

#include <assert.h>
#include <string.h>

#define PRG_BLOCK_SIZE 32u
#define PRG_288_BYTES 288u

#ifndef PRG_BATCH_BLOCKS
#define PRG_BATCH_BLOCKS 32
#endif

static inline size_t prg_key_bytes(unsigned int seclvl) {
  assert(seclvl == 128u || seclvl == 256u);
  return (size_t)seclvl / 8u;
}

static inline void prg_expand_ballet_key(uint8_t out[32], const uint8_t* key,
                                         unsigned int seclvl) {
  const size_t key_bytes = prg_key_bytes(seclvl);
  if (seclvl == 256u) {
    memcpy(out, key, 32u);
    return;
  }

  memcpy(out, key, key_bytes);
  for (size_t i = 0; i < key_bytes; ++i) {
    out[key_bytes + i] = (uint8_t)(key[i] ^ 0xA5u);
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

static inline void prg_increment_iv(uint8_t* iv) {
  uint32_t iv0;
  memcpy(&iv0, iv, sizeof(iv0));
  iv0 = htole32(le32toh(iv0) + 1u);
  memcpy(iv, &iv0, sizeof(iv0));
}

static void add_to_upper_word(uint8_t* iv, uint32_t tweak) {
  uint32_t iv3;
  memcpy(&iv3, iv + IV_SIZE - sizeof(iv3), sizeof(iv3));
  iv3 = htole32(le32toh(iv3) + tweak);
  memcpy(iv + IV_SIZE - sizeof(iv3), &iv3, sizeof(iv3));
}

static void prg_256_ballet_coree(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                                 size_t outlen) {
  uint64_t rk[RoundBallet256256 * 4];
  BalletGenRK_256_256_ENC((u8i*)rk, (u8i*)key);

  size_t full_blocks = outlen / PRG_BLOCK_SIZE;
  while (full_blocks) {
    const size_t batch_blocks = full_blocks > PRG_BATCH_BLOCKS ? PRG_BATCH_BLOCKS : full_blocks;
    uint8_t counter_blocks[PRG_BATCH_BLOCKS * PRG_BLOCK_SIZE];

    for (size_t i = 0; i < batch_blocks; ++i) {
      uint8_t* blk = counter_blocks + i * PRG_BLOCK_SIZE;
      memset(blk, 0, PRG_BLOCK_SIZE);
      memcpy(blk, internal_iv, IV_SIZE);
      prg_increment_iv(internal_iv);
    }

    for (size_t i = 0; i < batch_blocks; ++i) {
      Ballet256256EncDataS((u8i*)(out + i * PRG_BLOCK_SIZE),
                           (u8i*)(counter_blocks + i * PRG_BLOCK_SIZE), (u8i*)rk);
    }

    out += batch_blocks * PRG_BLOCK_SIZE;
    full_blocks -= batch_blocks;
  }

  if (outlen % PRG_BLOCK_SIZE) {
    uint8_t last_counter[PRG_BLOCK_SIZE] = {0};
    uint8_t last_block[PRG_BLOCK_SIZE];
    memcpy(last_counter, internal_iv, IV_SIZE);
    Ballet256256EncDataS((u8i*)last_block, (u8i*)last_counter, (u8i*)rk);
    memcpy(out, last_block, outlen % PRG_BLOCK_SIZE);
  }
}

static void prg_256_ballet(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                           size_t outlen) {
  prg_256_ballet_coree(key, internal_iv, out, outlen);
}

static inline void prg_256_dispatch(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                                    uint8_t* out, size_t outlen, unsigned int seclvl) {
  uint8_t ballet_key[32];
  uint8_t internal_iv[IV_SIZE];
  prg_expand_ballet_key(ballet_key, key, seclvl);
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);
  prg_256_ballet(ballet_key, internal_iv, out, outlen);
}

void prg(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
         unsigned int seclvl, size_t outlen) {
  assert(seclvl == 128u || seclvl == 256u);
  prg_256_dispatch(key, iv, tweak, out, outlen, seclvl);
}

void prg_2_lambda_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                    uint32_t tweak_base, uint8_t* out, size_t out_stride,
                                    unsigned int seclvl, size_t count) {
  const size_t outlen = 2u * (size_t)(seclvl / 8u);
  size_t i = 0;
  for (; i < count; ++i) {
    prg(keys + i * key_stride, iv, tweak_base + (uint32_t)i, out + i * out_stride, seclvl, outlen);
  }
}

void prg_2_lambda_and_288_fixed_tweak_independent_batch_with_sd(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_288, uint8_t* sd_out, size_t sd_stride, uint8_t* out_2lambda,
    size_t out_2lambda_stride, uint8_t* out_288, size_t out_288_stride, unsigned int seclvl,
    size_t count) {
  const size_t key_bytes = prg_key_bytes(seclvl);
  const size_t out_2lambda_len = out_2lambda_stride;
  size_t i = 0;

  for (; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    if (sd_out + i * sd_stride != key_i) {
      memcpy(sd_out + i * sd_stride, key_i, key_bytes);
    }
    prg(key_i, iv, tweak_2lambda, out_2lambda + i * out_2lambda_stride, seclvl,
        out_2lambda_len);
    prg(key_i, iv, tweak_288, out_288 + i * out_288_stride, seclvl, PRG_288_BYTES);
  }
}

void prg_2_lambda_and_288_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_288, uint8_t* sd_out, size_t sd_stride, size_t sd_index_base,
    size_t sd_index_xor, uint8_t* out_2lambda, size_t out_2lambda_stride, uint8_t* out_288,
    size_t out_288_stride, unsigned int seclvl, size_t count) {
  const size_t key_bytes = prg_key_bytes(seclvl);
  const size_t out_2lambda_len = out_2lambda_stride;
  size_t i = 0;

  for (; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* sd_i = prg_sd_xor_map_slot(sd_out, sd_stride, i, sd_index_base, sd_index_xor);
    uint8_t* out288_i = prg_sd_xor_map_slot(out_288, out_288_stride, i, sd_index_base,
                                            sd_index_xor);

    if (sd_i) {
      memcpy(sd_i, key_i, key_bytes);
    }
    prg(key_i, iv, tweak_2lambda, out_2lambda + i * out_2lambda_stride, seclvl,
        out_2lambda_len);
    if (out288_i) {
      prg(key_i, iv, tweak_288, out288_i, seclvl, PRG_288_BYTES);
    }
  }
}
