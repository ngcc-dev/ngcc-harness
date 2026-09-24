/*
 * SPDX-License-Identifier: MIT
 *
 * ublockith_em_d3_256s PRG layer:
 * - only 256-bit security path is kept
 * - PRG backend is Ballet-256/256(coreE portable)
 */

#include "prg.h"

#include "endian_compat.h"
#include "utils_ublock/ublock.h"

#if defined(USE_BALLET) && (USE_BALLET == 1) && defined(HAVE_BALLET_CORE) && (HAVE_BALLET_CORE == 1)
#include "Ballet256256_coreE.h"
#else
#error "ublockith_d3_256s requires USE_BALLET=1 and HAVE_BALLET_CORE=1"
#endif

#include <assert.h>
#include <string.h>

#ifndef PRG_BATCH_BLOCKS
#define PRG_BATCH_BLOCKS 32
#endif

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

  size_t full_blocks = outlen / UBLOCK_BLOCK;
  while (full_blocks) {
    const size_t batch_blocks = full_blocks > PRG_BATCH_BLOCKS ? PRG_BATCH_BLOCKS : full_blocks;
    uint8_t counter_blocks[PRG_BATCH_BLOCKS * UBLOCK_BLOCK];

    for (size_t i = 0; i < batch_blocks; ++i) {
      uint8_t* blk = counter_blocks + i * UBLOCK_BLOCK;
      memset(blk, 0, UBLOCK_BLOCK);
      memcpy(blk, internal_iv, IV_SIZE);
      prg_increment_iv(internal_iv);
    }

    for (size_t i = 0; i < batch_blocks; ++i) {
      Ballet256256EncDataS((u8i*)(out + i * UBLOCK_BLOCK),
                           (u8i*)(counter_blocks + i * UBLOCK_BLOCK), (u8i*)rk);
    }

    out += batch_blocks * UBLOCK_BLOCK;
    full_blocks -= batch_blocks;
  }

  if (outlen % UBLOCK_BLOCK) {
    uint8_t last_counter[UBLOCK_BLOCK] = {0};
    uint8_t last_block[UBLOCK_BLOCK];
    memcpy(last_counter, internal_iv, IV_SIZE);
    Ballet256256EncDataS((u8i*)last_block, (u8i*)last_counter, (u8i*)rk);
    memcpy(out, last_block, outlen % UBLOCK_BLOCK);
  }
}

static void prg_256_ballet(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                           size_t outlen) {
  prg_256_ballet_coree(key, internal_iv, out, outlen);
}

static inline void prg_256_dispatch(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                                    uint8_t* out, size_t outlen) {
  uint8_t internal_iv[IV_SIZE];
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);
  prg_256_ballet(key, internal_iv, out, outlen);
}

void prg(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
         unsigned int seclvl, size_t outlen) {
  (void)seclvl;
  assert(seclvl == 256u);
  prg_256_dispatch(key, iv, tweak, out, outlen);
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

void prg_2_lambda_fixed_tweak_independent_batch_with_sd(const uint8_t* keys, size_t key_stride,
                                                        const uint8_t* iv, uint32_t tweak,
                                                        uint8_t* sd_out, size_t sd_stride,
                                                        uint8_t* out, size_t out_stride,
                                                        unsigned int seclvl, size_t count) {
  const size_t key_bytes = (size_t)(seclvl / 8u);
  const size_t outlen = 2u * key_bytes;
  size_t i = 0;

  for (; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    memcpy(sd_out + i * sd_stride, key_i, key_bytes);
    prg(key_i, iv, tweak, out + i * out_stride, seclvl, outlen);
  }
}

static inline uint8_t* prg_sd_xor_map_slot(uint8_t* sd_out, size_t sd_stride, size_t index,
                                           size_t xor_mask) {
  return sd_out + ((index ^ xor_mask) * sd_stride);
}

void prg_2_lambda_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak, uint8_t* sd_out,
    size_t sd_stride, size_t sd_index_base, size_t sd_index_xor, uint8_t* out, size_t out_stride,
    unsigned int seclvl, size_t count) {
  const size_t key_bytes = (size_t)(seclvl / 8u);
  const size_t outlen = 2u * key_bytes;
  size_t i = 0;

  for (; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* sd_slot = prg_sd_xor_map_slot(sd_out, sd_stride, sd_index_base + i, sd_index_xor);
    memcpy(sd_slot, key_i, key_bytes);
    prg(key_i, iv, tweak, out + i * out_stride, seclvl, outlen);
  }
}

void prg_fixed_tweak_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                       uint32_t tweak, uint8_t* out, size_t out_stride,
                                       unsigned int seclvl, size_t outlen, size_t count) {
  size_t i = 0;
  for (; i < count; ++i) {
    prg(keys + i * key_stride, iv, tweak, out + i * out_stride, seclvl, outlen);
  }
}

void prg_4_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
                  unsigned int seclvl) {
  const size_t outlen = 4u * (size_t)(seclvl / 8u);
  prg(key, iv, tweak, out, seclvl, outlen);
}
