#include "prg.h"

#include "endian_compat.h"
#include "utils_vistrutah/vistrutah.h"

#include <assert.h>
#include <string.h>

#ifdef VISTRUTITH_REF_PORTABLE
#error "Do not compile Optimized_Implementation/vistrutith_d3_512f with VISTRUTITH_REF_PORTABLE."
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

static void prg_512_counter_mode_prepared(const vistrutah_512_prepared_key_t* prepared_key,
                                          uint8_t* internal_iv, uint8_t* out, size_t outlen) {
  uint8_t counter_block[VISTRUTAH_512_BLOCK_SIZE] = {0};
  memcpy(counter_block, internal_iv, IV_SIZE);
  while (outlen >= VISTRUTAH_512_BLOCK_SIZE) {
    vistrutah_512_encrypt_prepared(counter_block, out, prepared_key);
    out += VISTRUTAH_512_BLOCK_SIZE;
    outlen -= VISTRUTAH_512_BLOCK_SIZE;
    prg_increment_iv(counter_block);
  }

  if (outlen) {
    uint8_t stream[VISTRUTAH_512_BLOCK_SIZE];
    vistrutah_512_encrypt_prepared(counter_block, stream, prepared_key);
    memcpy(out, stream, outlen);
  }

  memcpy(internal_iv, counter_block, IV_SIZE);
}

static inline void prg_512_two_blocks_prepared(const vistrutah_512_prepared_key_t* prepared_key,
                                               const uint8_t* base_iv, uint8_t* out) {
  uint8_t counter_block[VISTRUTAH_512_BLOCK_SIZE] = {0};
  memcpy(counter_block, base_iv, IV_SIZE);
  vistrutah_512_encrypt_prepared(counter_block, out, prepared_key);
  prg_increment_iv(counter_block);
  vistrutah_512_encrypt_prepared(counter_block, out + VISTRUTAH_512_BLOCK_SIZE, prepared_key);
}

static void prg_512_counter_mode_prepared_batch4(
    const vistrutah_512_prepared_key_t prepared[4], size_t lane_count, const uint8_t* base_iv,
    uint8_t* out_base, size_t out_stride, size_t outlen) {
  uint8_t counter_block[VISTRUTAH_512_BLOCK_SIZE] = {0};
  size_t produced = 0;

  assert(lane_count > 0 && lane_count <= 4u);
  memcpy(counter_block, base_iv, IV_SIZE);

  while (outlen - produced >= VISTRUTAH_512_BLOCK_SIZE) {
    vistrutah_512_encrypt_prepared_batch_same_plaintext(
        counter_block, prepared, lane_count, out_base + produced, out_stride);
    produced += VISTRUTAH_512_BLOCK_SIZE;
    prg_increment_iv(counter_block);
  }

  if (produced != outlen) {
    const size_t tail = outlen - produced;
    uint8_t stream[4u * VISTRUTAH_512_BLOCK_SIZE];
    vistrutah_512_encrypt_prepared_batch_same_plaintext(
        counter_block, prepared, lane_count, stream, VISTRUTAH_512_BLOCK_SIZE);
    for (size_t lane = 0; lane < lane_count; ++lane) {
      memcpy(out_base + lane * out_stride + produced, stream + lane * VISTRUTAH_512_BLOCK_SIZE,
             tail);
    }
  }
}

static void prg_512_counter_mode(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                                 size_t outlen) {
  vistrutah_512_prepared_key_t prepared_key;

  vistrutah_512_prepare_key(&prepared_key, key);
  prg_512_counter_mode_prepared(&prepared_key, internal_iv, out, outlen);
}

void prg(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
         unsigned int seclvl, size_t outlen) {
  uint8_t internal_iv[IV_SIZE];

  (void)seclvl;
  assert(seclvl == 512u);
  memcpy(internal_iv, iv, sizeof(internal_iv));
  add_to_upper_word(internal_iv, tweak);
  prg_512_counter_mode(key, internal_iv, out, outlen);
}

void prg_2_lambda_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                    uint32_t tweak_base, uint8_t* out, size_t out_stride,
                                    unsigned int seclvl, size_t count) {
  uint8_t tweaked_iv[IV_SIZE];
  (void)seclvl;
  assert(seclvl == 512u);
  memcpy(tweaked_iv, iv, sizeof(tweaked_iv));
  add_to_upper_word(tweaked_iv, tweak_base);
  for (size_t i = 0; i < count; ++i) {
    vistrutah_512_prepared_key_t prepared_key;
    vistrutah_512_prepare_key(&prepared_key, keys + i * key_stride);
    prg_512_two_blocks_prepared(&prepared_key, tweaked_iv, out + i * out_stride);
    add_to_upper_word(tweaked_iv, 1u);
  }
}

void prg_2_lambda_fixed_tweak_independent_batch_with_sd(const uint8_t* keys, size_t key_stride,
                                                        const uint8_t* iv, uint32_t tweak,
                                                        uint8_t* sd_out, size_t sd_stride,
                                                        uint8_t* out, size_t out_stride,
                                                        unsigned int seclvl, size_t count) {
  const size_t key_bytes = (size_t)(seclvl / 8u);
  const size_t outlen = 2u * key_bytes;
  uint8_t tweaked_iv[IV_SIZE];
  (void)seclvl;
  assert(seclvl == 512u);
  memcpy(tweaked_iv, iv, sizeof(tweaked_iv));
  add_to_upper_word(tweaked_iv, tweak);
  size_t i = 0;
  for (; i + 4u <= count; i += 4u) {
    vistrutah_512_prepared_key_t prepared4[4];
    for (size_t lane = 0; lane < 4u; ++lane) {
      const uint8_t* key_i = keys + (i + lane) * key_stride;
      vistrutah_512_prepare_key(&prepared4[lane], key_i);
      memcpy(sd_out + (i + lane) * sd_stride, key_i, key_bytes);
    }
    prg_512_counter_mode_prepared_batch4(prepared4, 4u, tweaked_iv, out + i * out_stride, out_stride,
                                         outlen);
  }
  for (; i < count; ++i) {
    vistrutah_512_prepared_key_t prepared_key;
    const uint8_t* key_i = keys + i * key_stride;
    vistrutah_512_prepare_key(&prepared_key, key_i);
    memcpy(sd_out + i * sd_stride, key_i, key_bytes);
    prg_512_two_blocks_prepared(&prepared_key, tweaked_iv, out + i * out_stride);
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
  uint8_t tweaked_iv[IV_SIZE];
  (void)seclvl;
  assert(seclvl == 512u);
  memcpy(tweaked_iv, iv, sizeof(tweaked_iv));
  add_to_upper_word(tweaked_iv, tweak);
  size_t i = 0;
  for (; i + 4u <= count; i += 4u) {
    vistrutah_512_prepared_key_t prepared4[4];
    for (size_t lane = 0; lane < 4u; ++lane) {
      const size_t idx = i + lane;
      const uint8_t* key_i = keys + idx * key_stride;
      uint8_t* sd_slot =
          prg_sd_xor_map_slot(sd_out, sd_stride, sd_index_base + idx, sd_index_xor);
      vistrutah_512_prepare_key(&prepared4[lane], key_i);
      memcpy(sd_slot, key_i, key_bytes);
    }
    prg_512_counter_mode_prepared_batch4(prepared4, 4u, tweaked_iv, out + i * out_stride, out_stride,
                                         outlen);
  }
  for (; i < count; ++i) {
    vistrutah_512_prepared_key_t prepared_key;
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* sd_slot = prg_sd_xor_map_slot(sd_out, sd_stride, sd_index_base + i, sd_index_xor);
    vistrutah_512_prepare_key(&prepared_key, key_i);
    memcpy(sd_slot, key_i, key_bytes);
    prg_512_two_blocks_prepared(&prepared_key, tweaked_iv, out + i * out_stride);
  }
}

void prg_fixed_tweak_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                       uint32_t tweak, uint8_t* out, size_t out_stride,
                                       unsigned int seclvl, size_t outlen, size_t count) {
  uint8_t tweaked_iv[IV_SIZE];
  (void)seclvl;
  assert(seclvl == 512u);
  memcpy(tweaked_iv, iv, sizeof(tweaked_iv));
  add_to_upper_word(tweaked_iv, tweak);
  size_t i = 0;
  for (; i + 4u <= count; i += 4u) {
    vistrutah_512_prepared_key_t prepared4[4];
    for (size_t lane = 0; lane < 4u; ++lane) {
      vistrutah_512_prepare_key(&prepared4[lane], keys + (i + lane) * key_stride);
    }
    prg_512_counter_mode_prepared_batch4(prepared4, 4u, tweaked_iv, out + i * out_stride, out_stride,
                                         outlen);
  }
  for (; i < count; ++i) {
    vistrutah_512_prepared_key_t prepared_key;
    uint8_t internal_iv[IV_SIZE];
    vistrutah_512_prepare_key(&prepared_key, keys + i * key_stride);
    memcpy(internal_iv, tweaked_iv, sizeof(internal_iv));
    prg_512_counter_mode_prepared(&prepared_key, internal_iv, out + i * out_stride, outlen);
  }
}

void prg_4_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
                  unsigned int seclvl) {
  const size_t outlen = 4u * (size_t)(seclvl / 8u);
  prg(key, iv, tweak, out, seclvl, outlen);
}
