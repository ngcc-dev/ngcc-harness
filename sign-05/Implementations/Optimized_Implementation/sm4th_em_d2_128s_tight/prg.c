/*
 * SPDX-License-Identifier: MIT
 *
 * sm4th_em_d2_128s_tight PRG layer:
 * - only 256-bit security path is kept
 * - PRG backend is Ballet-256/256 (coreE portable, AVX2 parallel-enc on x86 when PRG_ACCE=1)
 */

#include "prg.h"

#include "endian_compat.h"
#include "x86_caps.h"

#if defined(USE_BALLET) && (USE_BALLET == 1) && defined(HAVE_BALLET_CORE) && (HAVE_BALLET_CORE == 1)
#include "Ballet256256_coreE.h"
#if defined(PRG_ACCE) && (PRG_ACCE == 1) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#define PRG_HAS_BALLET_AVX2 1
#define PRG_HAS_4KEY_FIXED_TWEAK_AVX2 ((IV_SIZE == 20u) || (IV_SIZE == 32u))
#else
#define PRG_HAS_BALLET_AVX2 0
#endif
#else
#error "sm4th_em_d2_128s_tight requires USE_BALLET=1 and HAVE_BALLET_CORE=1"
#endif

#include <assert.h>
#include <string.h>

#define PRG_BLOCK_SIZE 32u
#define PRG_170_BYTES 170u

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

#if PRG_HAS_BALLET_AVX2
static inline __m256i prg_rotl64_avx2(__m256i x, int n) {
  return _mm256_or_si256(_mm256_slli_epi64(x, n), _mm256_srli_epi64(x, 64 - n));
}

static inline __m256i prg_keymix_avx2(__m256i x, __m256i y) {
  const __m256i s3 = _mm256_or_si256(_mm256_slli_epi64(x, 3), _mm256_srli_epi64(y, 61));
  const __m256i s5 = _mm256_or_si256(_mm256_slli_epi64(x, 5), _mm256_srli_epi64(y, 59));
  return _mm256_xor_si256(s3, s5);
}

static inline void prg_ballet256_round_avx2(const __m256i rk0, const __m256i rk1,
                                            const __m256i rk2, const __m256i rk3, __m256i* a,
                                            __m256i* b, __m256i* c, __m256i* d) {
  __m256i t0 = _mm256_xor_si256(*b, *c);
  *b = _mm256_xor_si256(*b, rk1);
  *c = _mm256_xor_si256(*c, rk0);
  *b = prg_rotl64_avx2(*b, 6);
  *d = prg_rotl64_avx2(*d, 15);
  *a = prg_rotl64_avx2(*a, 6);
  *a = _mm256_add_epi64(*a, prg_rotl64_avx2(t0, 9));
  *d = _mm256_add_epi64(*d, prg_rotl64_avx2(t0, 14));
  t0 = _mm256_xor_si256(*a, *d);
  *b = _mm256_add_epi64(*b, prg_rotl64_avx2(t0, 9));
  *d = _mm256_xor_si256(*d, rk2);
  *a = _mm256_xor_si256(*a, rk3);
  *c = prg_rotl64_avx2(*c, 15);
  *c = _mm256_add_epi64(*c, prg_rotl64_avx2(t0, 14));
}

static inline uint64_t prg_load64_be(const uint8_t* p) {
  uint64_t v;
  memcpy(&v, p, sizeof(v));
  return (uint64_t)__builtin_bswap64(v);
}

static inline uint64_t prg_load64_be_words(uint32_t lo_le, uint32_t hi_le) {
  return (uint64_t)__builtin_bswap64((((uint64_t)hi_le) << 32) | (uint64_t)lo_le);
}

static inline void prg_store64_swapped(uint8_t* p, uint64_t v) {
  const uint64_t s = (uint64_t)__builtin_bswap64(v);
  memcpy(p, &s, sizeof(s));
}

static void prg_fixed_tweak_init_4keys_avx2(const uint8_t* iv, uint32_t tweak,
                                            uint32_t* ctr32_host, uint64_t* ctr_hi,
                                            __m256i* b_init, __m256i* c_init,
                                            __m256i* d_init) {
  uint8_t iv_tweak[IV_SIZE];
  memcpy(iv_tweak, iv, IV_SIZE);
  add_to_upper_word(iv_tweak, tweak);

  uint32_t ctr32_le;
  uint32_t iv_word1_le;
  memcpy(&ctr32_le, iv_tweak, sizeof(ctr32_le));
  memcpy(&iv_word1_le, iv_tweak + 4u, sizeof(iv_word1_le));
  *ctr_hi = ((uint64_t)iv_word1_le) << 32;
  *ctr32_host = le32toh(ctr32_le);
  *b_init = _mm256_set1_epi64x((long long)prg_load64_be(iv_tweak + 8u));

  if (IV_SIZE == 20u) {
    uint32_t iv_word4_le;
    memcpy(&iv_word4_le, iv_tweak + 16u, sizeof(iv_word4_le));
    *c_init = _mm256_set1_epi64x((long long)prg_load64_be_words(iv_word4_le, 0u));
    *d_init = _mm256_setzero_si256();
  } else {
    assert(IV_SIZE == 32u);
    *c_init = _mm256_set1_epi64x((long long)prg_load64_be(iv_tweak + 16u));
    *d_init = _mm256_set1_epi64x((long long)prg_load64_be(iv_tweak + 24u));
  }
}

static void prg_independent_tweak_init_4keys_avx2(const uint8_t* iv, uint32_t tweak_base,
                                                  size_t index, uint32_t* ctr32_host,
                                                  uint64_t* ctr_hi, __m256i* b_init,
                                                  __m256i* c_init, __m256i* d_init) {
  uint32_t ctr32_le;
  uint32_t iv_word1_le;
  memcpy(&ctr32_le, iv, sizeof(ctr32_le));
  memcpy(&iv_word1_le, iv + 4u, sizeof(iv_word1_le));
  *ctr_hi = ((uint64_t)iv_word1_le) << 32;
  *ctr32_host = le32toh(ctr32_le);
  *b_init = _mm256_set1_epi64x((long long)prg_load64_be(iv + 8u));

  const uint32_t t0 = tweak_base + (uint32_t)index;
  if (IV_SIZE == 20u) {
    uint32_t iv_word4_le;
    memcpy(&iv_word4_le, iv + 16u, sizeof(iv_word4_le));
    const uint32_t iv_word4_host = le32toh(iv_word4_le);
    const uint64_t c0 = prg_load64_be_words(htole32(iv_word4_host + t0), 0u);
    const uint64_t c1 = prg_load64_be_words(htole32(iv_word4_host + t0 + 1u), 0u);
    const uint64_t c2 = prg_load64_be_words(htole32(iv_word4_host + t0 + 2u), 0u);
    const uint64_t c3 = prg_load64_be_words(htole32(iv_word4_host + t0 + 3u), 0u);
    *c_init = _mm256_set_epi64x((long long)c3, (long long)c2, (long long)c1, (long long)c0);
    *d_init = _mm256_setzero_si256();
  } else {
    assert(IV_SIZE == 32u);
    uint32_t iv_word6_le;
    uint32_t iv_word7_le;
    memcpy(&iv_word6_le, iv + 24u, sizeof(iv_word6_le));
    memcpy(&iv_word7_le, iv + 28u, sizeof(iv_word7_le));
    const uint32_t iv_word7_host = le32toh(iv_word7_le);
    const uint64_t d0 = prg_load64_be_words(iv_word6_le, htole32(iv_word7_host + t0));
    const uint64_t d1 = prg_load64_be_words(iv_word6_le, htole32(iv_word7_host + t0 + 1u));
    const uint64_t d2 = prg_load64_be_words(iv_word6_le, htole32(iv_word7_host + t0 + 2u));
    const uint64_t d3 = prg_load64_be_words(iv_word6_le, htole32(iv_word7_host + t0 + 3u));
    *c_init = _mm256_set1_epi64x((long long)prg_load64_be(iv + 16u));
    *d_init = _mm256_set_epi64x((long long)d3, (long long)d2, (long long)d1, (long long)d0);
  }
}

static void prg_ballet256_enc4_avx2(const uint64_t* rk, const uint8_t in[4][PRG_BLOCK_SIZE],
                                    uint8_t out[4][PRG_BLOCK_SIZE]) {
  uint64_t a_lane[4], b_lane[4], c_lane[4], d_lane[4];
  for (size_t lane = 0; lane < 4; ++lane) {
    a_lane[lane] = prg_load64_be(in[lane] + 0);
    b_lane[lane] = prg_load64_be(in[lane] + 8);
    c_lane[lane] = prg_load64_be(in[lane] + 16);
    d_lane[lane] = prg_load64_be(in[lane] + 24);
  }

  __m256i a = _mm256_set_epi64x((long long)a_lane[3], (long long)a_lane[2], (long long)a_lane[1],
                                (long long)a_lane[0]);
  __m256i b = _mm256_set_epi64x((long long)b_lane[3], (long long)b_lane[2], (long long)b_lane[1],
                                (long long)b_lane[0]);
  __m256i c = _mm256_set_epi64x((long long)c_lane[3], (long long)c_lane[2], (long long)c_lane[1],
                                (long long)c_lane[0]);
  __m256i d = _mm256_set_epi64x((long long)d_lane[3], (long long)d_lane[2], (long long)d_lane[1],
                                (long long)d_lane[0]);

  for (size_t i = 0; i < (size_t)(RoundBallet256256 * 2); i += 4) {
    const __m256i rk0 = _mm256_set1_epi64x((long long)rk[i + 0]);
    const __m256i rk1 = _mm256_set1_epi64x((long long)rk[i + 1]);
    const __m256i rk2 = _mm256_set1_epi64x((long long)rk[i + 2]);
    const __m256i rk3 = _mm256_set1_epi64x((long long)rk[i + 3]);

    __m256i t0 = _mm256_xor_si256(b, c);
    b = _mm256_xor_si256(b, rk1);
    c = _mm256_xor_si256(c, rk0);
    b = prg_rotl64_avx2(b, 6);
    d = prg_rotl64_avx2(d, 15);
    a = prg_rotl64_avx2(a, 6);
    a = _mm256_add_epi64(a, prg_rotl64_avx2(t0, 9));
    d = _mm256_add_epi64(d, prg_rotl64_avx2(t0, 14));
    t0 = _mm256_xor_si256(a, d);
    b = _mm256_add_epi64(b, prg_rotl64_avx2(t0, 9));
    d = _mm256_xor_si256(d, rk2);
    a = _mm256_xor_si256(a, rk3);
    c = prg_rotl64_avx2(c, 15);
    c = _mm256_add_epi64(c, prg_rotl64_avx2(t0, 14));
  }

  _mm256_storeu_si256((__m256i*)a_lane, a);
  _mm256_storeu_si256((__m256i*)b_lane, b);
  _mm256_storeu_si256((__m256i*)c_lane, c);
  _mm256_storeu_si256((__m256i*)d_lane, d);

  for (size_t lane = 0; lane < 4; ++lane) {
    prg_store64_swapped(out[lane] + 0, b_lane[lane]);
    prg_store64_swapped(out[lane] + 8, a_lane[lane]);
    prg_store64_swapped(out[lane] + 16, d_lane[lane]);
    prg_store64_swapped(out[lane] + 24, c_lane[lane]);
  }
}

static void prg_ballet256_genrk4_pack_avx2(const uint8_t* keys, size_t key_stride,
                                            __m256i* rk_pack) {
  __m256i k0 = _mm256_set_epi64x((long long)prg_load64_be(keys + 3u * key_stride + 8),
                                 (long long)prg_load64_be(keys + 2u * key_stride + 8),
                                 (long long)prg_load64_be(keys + 1u * key_stride + 8),
                                 (long long)prg_load64_be(keys + 0u * key_stride + 8));
  __m256i k1 = _mm256_set_epi64x((long long)prg_load64_be(keys + 3u * key_stride + 0),
                                 (long long)prg_load64_be(keys + 2u * key_stride + 0),
                                 (long long)prg_load64_be(keys + 1u * key_stride + 0),
                                 (long long)prg_load64_be(keys + 0u * key_stride + 0));
  __m256i k2 = _mm256_set_epi64x((long long)prg_load64_be(keys + 3u * key_stride + 24),
                                 (long long)prg_load64_be(keys + 2u * key_stride + 24),
                                 (long long)prg_load64_be(keys + 1u * key_stride + 24),
                                 (long long)prg_load64_be(keys + 0u * key_stride + 24));
  __m256i k3 = _mm256_set_epi64x((long long)prg_load64_be(keys + 3u * key_stride + 16),
                                 (long long)prg_load64_be(keys + 2u * key_stride + 16),
                                 (long long)prg_load64_be(keys + 1u * key_stride + 16),
                                 (long long)prg_load64_be(keys + 0u * key_stride + 16));

  size_t p = 0;
  for (size_t round = 0; round < (size_t)RoundBallet256256; round += 2) {
    rk_pack[p++] = k0;
    rk_pack[p++] = k1;
    k0 = _mm256_xor_si256(
        k0, _mm256_xor_si256(prg_keymix_avx2(k2, k3), _mm256_set1_epi64x((long long)round)));
    k1 = _mm256_xor_si256(k1, prg_keymix_avx2(k3, k2));

    rk_pack[p++] = k2;
    rk_pack[p++] = k3;
    k2 =
        _mm256_xor_si256(k2, _mm256_xor_si256(prg_keymix_avx2(k0, k1),
                                              _mm256_set1_epi64x((long long)(round + 1u))));
    k3 = _mm256_xor_si256(k3, prg_keymix_avx2(k1, k0));
  }
}

static void prg_ballet256_enc1_4keys_avx2_ctr(const __m256i* rk_pack, uint64_t a0, __m256i b_init,
                                               __m256i c_init, __m256i d_init,
                                               uint8_t out[4][PRG_BLOCK_SIZE]) {
  __m256i a = _mm256_set1_epi64x((long long)a0);
  __m256i b = b_init;
  __m256i c = c_init;
  __m256i d = d_init;

#if defined(__GNUC__)
#pragma GCC unroll 37
#endif
  for (size_t i = 0; i < (size_t)(RoundBallet256256 * 2); i += 4) {
    const __m256i rk0 = rk_pack[i + 0];
    const __m256i rk1 = rk_pack[i + 1];
    const __m256i rk2 = rk_pack[i + 2];
    const __m256i rk3 = rk_pack[i + 3];
    prg_ballet256_round_avx2(rk0, rk1, rk2, rk3, &a, &b, &c, &d);
  }

  uint64_t a_lane[4], b_lane[4], c_lane[4], d_lane[4];
  _mm256_storeu_si256((__m256i*)a_lane, a);
  _mm256_storeu_si256((__m256i*)b_lane, b);
  _mm256_storeu_si256((__m256i*)c_lane, c);
  _mm256_storeu_si256((__m256i*)d_lane, d);
  for (size_t lane = 0; lane < 4; ++lane) {
    prg_store64_swapped(out[lane] + 0, b_lane[lane]);
    prg_store64_swapped(out[lane] + 8, a_lane[lane]);
    prg_store64_swapped(out[lane] + 16, d_lane[lane]);
    prg_store64_swapped(out[lane] + 24, c_lane[lane]);
  }
}

static void prg_ballet256_enc2_4keys_avx2_ctr(const __m256i* rk_pack, uint64_t a0_lo,
                                               uint64_t a0_hi, __m256i b_init, __m256i c_init,
                                               __m256i d_init,
                                               uint8_t out_lo[4][PRG_BLOCK_SIZE],
                                               uint8_t out_hi[4][PRG_BLOCK_SIZE]) {
  __m256i a_lo = _mm256_set1_epi64x((long long)a0_lo);
  __m256i b_lo = b_init;
  __m256i c_lo = c_init;
  __m256i d_lo = d_init;

  __m256i a_hi = _mm256_set1_epi64x((long long)a0_hi);
  __m256i b_hi = b_init;
  __m256i c_hi = c_init;
  __m256i d_hi = d_init;

#if defined(__GNUC__)
#pragma GCC unroll 37
#endif
  for (size_t i = 0; i < (size_t)(RoundBallet256256 * 2); i += 4) {
    const __m256i rk0 = rk_pack[i + 0];
    const __m256i rk1 = rk_pack[i + 1];
    const __m256i rk2 = rk_pack[i + 2];
    const __m256i rk3 = rk_pack[i + 3];
    prg_ballet256_round_avx2(rk0, rk1, rk2, rk3, &a_lo, &b_lo, &c_lo, &d_lo);
    prg_ballet256_round_avx2(rk0, rk1, rk2, rk3, &a_hi, &b_hi, &c_hi, &d_hi);
  }

  uint64_t a_lane[4], b_lane[4], c_lane[4], d_lane[4];
  _mm256_storeu_si256((__m256i*)a_lane, a_lo);
  _mm256_storeu_si256((__m256i*)b_lane, b_lo);
  _mm256_storeu_si256((__m256i*)c_lane, c_lo);
  _mm256_storeu_si256((__m256i*)d_lane, d_lo);
  for (size_t lane = 0; lane < 4; ++lane) {
    prg_store64_swapped(out_lo[lane] + 0, b_lane[lane]);
    prg_store64_swapped(out_lo[lane] + 8, a_lane[lane]);
    prg_store64_swapped(out_lo[lane] + 16, d_lane[lane]);
    prg_store64_swapped(out_lo[lane] + 24, c_lane[lane]);
  }

  _mm256_storeu_si256((__m256i*)a_lane, a_hi);
  _mm256_storeu_si256((__m256i*)b_lane, b_hi);
  _mm256_storeu_si256((__m256i*)c_lane, c_hi);
  _mm256_storeu_si256((__m256i*)d_lane, d_hi);
  for (size_t lane = 0; lane < 4; ++lane) {
    prg_store64_swapped(out_hi[lane] + 0, b_lane[lane]);
    prg_store64_swapped(out_hi[lane] + 8, a_lane[lane]);
    prg_store64_swapped(out_hi[lane] + 16, d_lane[lane]);
    prg_store64_swapped(out_hi[lane] + 24, c_lane[lane]);
  }
}

static void prg_ballet256_emit_fixed_4keys_avx2(const __m256i* rk_pack, const uint8_t* iv,
                                                uint32_t tweak, uint8_t* const out_ptrs[4],
                                                size_t outlen) {
  const size_t full_blocks = outlen / PRG_BLOCK_SIZE;
  const size_t rem = outlen % PRG_BLOCK_SIZE;
  uint32_t ctr32_host;
  uint64_t ctr_hi;
  __m256i b_init;
  __m256i c_init;
  __m256i d_init;
  prg_fixed_tweak_init_4keys_avx2(iv, tweak, &ctr32_host, &ctr_hi, &b_init, &c_init, &d_init);

  size_t blk = 0;
  for (; blk + 1u < full_blocks; blk += 2u) {
    const uint32_t ctr_blk0_le = htole32(ctr32_host + (uint32_t)blk);
    const uint32_t ctr_blk1_le = htole32(ctr32_host + (uint32_t)(blk + 1u));
    const uint64_t a0 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk0_le);
    const uint64_t a1 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk1_le);
    uint8_t stream0[4][PRG_BLOCK_SIZE];
    uint8_t stream1[4][PRG_BLOCK_SIZE];
    prg_ballet256_enc2_4keys_avx2_ctr(rk_pack, a0, a1, b_init, c_init, d_init, stream0,
                                       stream1);
    for (size_t lane = 0; lane < 4u; ++lane) {
      if (out_ptrs[lane]) {
        uint8_t* out_lane = out_ptrs[lane] + blk * PRG_BLOCK_SIZE;
        memcpy(out_lane, stream0[lane], PRG_BLOCK_SIZE);
        memcpy(out_lane + PRG_BLOCK_SIZE, stream1[lane], PRG_BLOCK_SIZE);
      }
    }
  }

  for (; blk < full_blocks; ++blk) {
    const uint32_t ctr_blk_le = htole32(ctr32_host + (uint32_t)blk);
    const uint64_t a0 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk_le);
    uint8_t stream[4][PRG_BLOCK_SIZE];
    prg_ballet256_enc1_4keys_avx2_ctr(rk_pack, a0, b_init, c_init, d_init, stream);
    for (size_t lane = 0; lane < 4u; ++lane) {
      if (out_ptrs[lane]) {
        memcpy(out_ptrs[lane] + blk * PRG_BLOCK_SIZE, stream[lane], PRG_BLOCK_SIZE);
      }
    }
  }

  if (rem) {
    const uint32_t ctr_blk_le = htole32(ctr32_host + (uint32_t)full_blocks);
    const uint64_t a0 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk_le);
    uint8_t stream[4][PRG_BLOCK_SIZE];
    prg_ballet256_enc1_4keys_avx2_ctr(rk_pack, a0, b_init, c_init, d_init, stream);
    for (size_t lane = 0; lane < 4u; ++lane) {
      if (out_ptrs[lane]) {
        memcpy(out_ptrs[lane] + full_blocks * PRG_BLOCK_SIZE, stream[lane], rem);
      }
    }
  }
}

static void prg_2lambda_and_170_batch4_256_avx2(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_170, uint8_t* const out_2lambda_ptrs[4], size_t out_2lambda_len,
    uint8_t* const out_170_ptrs[4]) {
  __m256i rk_pack[RoundBallet256256 * 2];
  prg_ballet256_genrk4_pack_avx2(keys, key_stride, rk_pack);
  prg_ballet256_emit_fixed_4keys_avx2(rk_pack, iv, tweak_2lambda, out_2lambda_ptrs,
                                      out_2lambda_len);
  prg_ballet256_emit_fixed_4keys_avx2(rk_pack, iv, tweak_170, out_170_ptrs, PRG_170_BYTES);
}

static void prg_256_ballet_avx2(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                                size_t outlen) {
  uint64_t rk[RoundBallet256256 * 4];
  BalletGenRK_256_256_ENC((u8i*)rk, (u8i*)key);

  while (outlen >= 4u * PRG_BLOCK_SIZE) {
    uint8_t ctr[4][PRG_BLOCK_SIZE] = {{0}};
    uint8_t stream[4][PRG_BLOCK_SIZE];
    for (size_t lane = 0; lane < 4; ++lane) {
      memcpy(ctr[lane], internal_iv, IV_SIZE);
      prg_increment_iv(internal_iv);
    }
    prg_ballet256_enc4_avx2(rk, (const uint8_t(*)[PRG_BLOCK_SIZE])ctr, stream);
    memcpy(out, stream, 4u * PRG_BLOCK_SIZE);
    out += 4u * PRG_BLOCK_SIZE;
    outlen -= 4u * PRG_BLOCK_SIZE;
  }

  if (outlen) {
    const size_t needed_blocks = (outlen + PRG_BLOCK_SIZE - 1u) / PRG_BLOCK_SIZE;
    uint8_t ctr[4][PRG_BLOCK_SIZE] = {{0}};
    uint8_t stream[4][PRG_BLOCK_SIZE];
    for (size_t lane = 0; lane < needed_blocks; ++lane) {
      memcpy(ctr[lane], internal_iv, IV_SIZE);
      prg_increment_iv(internal_iv);
    }
    prg_ballet256_enc4_avx2(rk, (const uint8_t(*)[PRG_BLOCK_SIZE])ctr, stream);
    memcpy(out, stream, outlen);
  }
}
#endif

static void prg_256_ballet(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                           size_t outlen) {
#if PRG_HAS_BALLET_AVX2
  if (sm4th_x86_has_avx2()) {
    prg_256_ballet_avx2(key, internal_iv, out, outlen);
    return;
  }
#endif
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
#if PRG_HAS_BALLET_AVX2
  if (PRG_HAS_4KEY_FIXED_TWEAK_AVX2 && seclvl == 256u && sm4th_x86_has_avx2() &&
      out_stride >= outlen && count >= 4u) {
    for (; i + 4u <= count; i += 4u) {
      uint32_t ctr32_host;
      uint64_t ctr_hi;
      __m256i b_init;
      __m256i c_init;
      __m256i d_init;
      __m256i rk_pack[RoundBallet256256 * 2];
      uint8_t stream0[4][PRG_BLOCK_SIZE];
      uint8_t stream1[4][PRG_BLOCK_SIZE];
      prg_independent_tweak_init_4keys_avx2(iv, tweak_base, i, &ctr32_host, &ctr_hi, &b_init,
                                            &c_init, &d_init);
      prg_ballet256_genrk4_pack_avx2(keys + i * key_stride, key_stride, rk_pack);

      const uint64_t a0 =
          (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)htole32(ctr32_host));
      const uint64_t a1 =
          (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)htole32(ctr32_host + 1u));
      prg_ballet256_enc2_4keys_avx2_ctr(rk_pack, a0, a1, b_init, c_init, d_init, stream0,
                                         stream1);

      for (size_t lane = 0; lane < 4u; ++lane) {
        uint8_t* out_lane = out + (i + lane) * out_stride;
        memcpy(out_lane, stream0[lane], PRG_BLOCK_SIZE);
        memcpy(out_lane + PRG_BLOCK_SIZE, stream1[lane], PRG_BLOCK_SIZE);
      }
    }
  }
#endif
  for (; i < count; ++i) {
    prg(keys + i * key_stride, iv, tweak_base + (uint32_t)i, out + i * out_stride, seclvl, outlen);
  }
}

void prg_2_lambda_and_170_fixed_tweak_independent_batch_with_sd(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_170, uint8_t* sd_out, size_t sd_stride, uint8_t* out_2lambda,
    size_t out_2lambda_stride, uint8_t* out_170, size_t out_170_stride, unsigned int seclvl,
    size_t count) {
  const size_t key_bytes = prg_key_bytes(seclvl);
  const size_t out_2lambda_len = out_2lambda_stride;
  size_t i = 0;

#if PRG_HAS_BALLET_AVX2
  if (PRG_HAS_4KEY_FIXED_TWEAK_AVX2 && seclvl == 256u && sm4th_x86_has_avx2() &&
      out_170_stride >= PRG_170_BYTES && count >= 4u) {
    const size_t n4 = count & ~(size_t)3u;
    for (; i < n4; i += 4u) {
      uint8_t* out2_ptrs[4];
      uint8_t* out170_ptrs[4];
      for (size_t lane = 0; lane < 4u; ++lane) {
        const size_t idx = i + lane;
        const uint8_t* key_i = keys + idx * key_stride;
        uint8_t* sd_i = sd_out + idx * sd_stride;
        if (sd_i != key_i) {
          memcpy(sd_i, key_i, key_bytes);
        }
        out2_ptrs[lane] = out_2lambda + idx * out_2lambda_stride;
        out170_ptrs[lane] = out_170 + idx * out_170_stride;
      }
      prg_2lambda_and_170_batch4_256_avx2(keys + i * key_stride, key_stride, iv,
                                          tweak_2lambda, tweak_170, out2_ptrs,
                                          out_2lambda_len, out170_ptrs);
    }
  }
#endif

  for (; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    if (sd_out + i * sd_stride != key_i) {
      memcpy(sd_out + i * sd_stride, key_i, key_bytes);
    }
    prg(key_i, iv, tweak_2lambda, out_2lambda + i * out_2lambda_stride, seclvl,
        out_2lambda_len);
    prg(key_i, iv, tweak_170, out_170 + i * out_170_stride, seclvl, PRG_170_BYTES);
  }
}

void prg_2_lambda_and_170_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_170, uint8_t* sd_out, size_t sd_stride, size_t sd_index_base,
    size_t sd_index_xor, uint8_t* out_2lambda, size_t out_2lambda_stride, uint8_t* out_170,
    size_t out_170_stride, unsigned int seclvl, size_t count) {
  const size_t key_bytes = prg_key_bytes(seclvl);
  const size_t out_2lambda_len = out_2lambda_stride;
  size_t i = 0;

#if PRG_HAS_BALLET_AVX2
  if (PRG_HAS_4KEY_FIXED_TWEAK_AVX2 && seclvl == 256u && sm4th_x86_has_avx2() &&
      out_170_stride >= PRG_170_BYTES && count >= 4u) {
    const size_t n4 = count & ~(size_t)3u;
    for (; i < n4; i += 4u) {
      uint8_t* out2_ptrs[4];
      uint8_t* out170_ptrs[4];
      for (size_t lane = 0; lane < 4u; ++lane) {
        const size_t idx = i + lane;
        const uint8_t* key_i = keys + idx * key_stride;
        uint8_t* sd_i = prg_sd_xor_map_slot(sd_out, sd_stride, idx, sd_index_base,
                                            sd_index_xor);
        if (sd_i) {
          memcpy(sd_i, key_i, key_bytes);
        }
        out2_ptrs[lane] = out_2lambda + idx * out_2lambda_stride;
        out170_ptrs[lane] = prg_sd_xor_map_slot(out_170, out_170_stride, idx, sd_index_base,
                                                sd_index_xor);
      }
      prg_2lambda_and_170_batch4_256_avx2(keys + i * key_stride, key_stride, iv,
                                          tweak_2lambda, tweak_170, out2_ptrs,
                                          out_2lambda_len, out170_ptrs);
    }
  }
#endif

  for (; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* sd_i = prg_sd_xor_map_slot(sd_out, sd_stride, i, sd_index_base, sd_index_xor);
    uint8_t* out170_i = prg_sd_xor_map_slot(out_170, out_170_stride, i, sd_index_base,
                                            sd_index_xor);

    if (sd_i) {
      memcpy(sd_i, key_i, key_bytes);
    }
    prg(key_i, iv, tweak_2lambda, out_2lambda + i * out_2lambda_stride, seclvl,
        out_2lambda_len);
    if (out170_i) {
      prg(key_i, iv, tweak_170, out170_i, seclvl, PRG_170_BYTES);
    }
  }
}
