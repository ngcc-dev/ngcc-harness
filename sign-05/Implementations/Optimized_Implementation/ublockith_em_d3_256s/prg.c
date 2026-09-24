/*
 * SPDX-License-Identifier: MIT
 *
 * ublockith_em_d3_256s PRG layer:
 * - only 256-bit security path is kept
 * - PRG backend is Ballet-256/256 (coreE portable, AVX2 parallel-enc on x86 when PRG_ACCE=1)
 */

#include "prg.h"

#include "endian_compat.h"
#include "utils_ublock/ublock.h"
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

static void prg_ballet256_enc4_avx2(const uint64_t* rk, const uint8_t in[4][UBLOCK_BLOCK],
                                    uint8_t out[4][UBLOCK_BLOCK]) {
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
                                               uint8_t out[4][UBLOCK_BLOCK]) {
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
                                               uint8_t out_lo[4][UBLOCK_BLOCK],
                                               uint8_t out_hi[4][UBLOCK_BLOCK]) {
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

static void prg_fixed_tweak_independent_batch_256_avx2(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak, uint8_t* out,
    size_t out_stride, size_t outlen, size_t count) {
  const size_t full_blocks = outlen / UBLOCK_BLOCK;
  const size_t rem = outlen % UBLOCK_BLOCK;
  uint32_t ctr32_host;
  uint64_t ctr_hi;
  __m256i b_init;
  __m256i c_init;
  __m256i d_init;
  prg_fixed_tweak_init_4keys_avx2(iv, tweak, &ctr32_host, &ctr_hi, &b_init, &c_init, &d_init);

  for (size_t i = 0; i + 4 <= count; i += 4) {
    __m256i rk_pack[RoundBallet256256 * 2];
    prg_ballet256_genrk4_pack_avx2(keys + i * key_stride, key_stride, rk_pack);

    size_t blk = 0;
    for (; blk + 1u < full_blocks; blk += 2u) {
      const uint32_t ctr_blk0_le = htole32(ctr32_host + (uint32_t)blk);
      const uint32_t ctr_blk1_le = htole32(ctr32_host + (uint32_t)(blk + 1u));
      const uint64_t a0 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk0_le);
      const uint64_t a1 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk1_le);
      uint8_t stream0[4][UBLOCK_BLOCK];
      uint8_t stream1[4][UBLOCK_BLOCK];
      prg_ballet256_enc2_4keys_avx2_ctr(rk_pack, a0, a1, b_init, c_init, d_init, stream0,
                                         stream1);
      for (size_t lane = 0; lane < 4; ++lane) {
        uint8_t* out_lane = out + (i + lane) * out_stride + blk * UBLOCK_BLOCK;
        memcpy(out_lane, stream0[lane], UBLOCK_BLOCK);
        memcpy(out_lane + UBLOCK_BLOCK, stream1[lane], UBLOCK_BLOCK);
      }
    }

    for (; blk < full_blocks; ++blk) {
      const uint32_t ctr_blk_le = htole32(ctr32_host + (uint32_t)blk);
      const uint64_t a0 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk_le);
      uint8_t stream[4][UBLOCK_BLOCK];
      prg_ballet256_enc1_4keys_avx2_ctr(rk_pack, a0, b_init, c_init, d_init, stream);
      for (size_t lane = 0; lane < 4; ++lane) {
        memcpy(out + (i + lane) * out_stride + blk * UBLOCK_BLOCK, stream[lane], UBLOCK_BLOCK);
      }
    }

    if (rem) {
      const uint32_t ctr_blk_le = htole32(ctr32_host + (uint32_t)full_blocks);
      const uint64_t a0 = (uint64_t)__builtin_bswap64(ctr_hi | (uint64_t)ctr_blk_le);
      uint8_t stream[4][UBLOCK_BLOCK];
      prg_ballet256_enc1_4keys_avx2_ctr(rk_pack, a0, b_init, c_init, d_init, stream);
      for (size_t lane = 0; lane < 4; ++lane) {
        memcpy(out + (i + lane) * out_stride + full_blocks * UBLOCK_BLOCK, stream[lane], rem);
      }
    }
  }
}

static void prg_256_ballet_avx2(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                                size_t outlen) {
  uint64_t rk[RoundBallet256256 * 4];
  BalletGenRK_256_256_ENC((u8i*)rk, (u8i*)key);

  while (outlen >= 4u * UBLOCK_BLOCK) {
    uint8_t ctr[4][UBLOCK_BLOCK] = {{0}};
    uint8_t stream[4][UBLOCK_BLOCK];
    for (size_t lane = 0; lane < 4; ++lane) {
      memcpy(ctr[lane], internal_iv, IV_SIZE);
      prg_increment_iv(internal_iv);
    }
    prg_ballet256_enc4_avx2(rk, (const uint8_t(*)[UBLOCK_BLOCK])ctr, stream);
    memcpy(out, stream, 4u * UBLOCK_BLOCK);
    out += 4u * UBLOCK_BLOCK;
    outlen -= 4u * UBLOCK_BLOCK;
  }

  if (outlen) {
    const size_t needed_blocks = (outlen + UBLOCK_BLOCK - 1u) / UBLOCK_BLOCK;
    uint8_t ctr[4][UBLOCK_BLOCK] = {{0}};
    uint8_t stream[4][UBLOCK_BLOCK];
    for (size_t lane = 0; lane < needed_blocks; ++lane) {
      memcpy(ctr[lane], internal_iv, IV_SIZE);
      prg_increment_iv(internal_iv);
    }
    prg_ballet256_enc4_avx2(rk, (const uint8_t(*)[UBLOCK_BLOCK])ctr, stream);
    memcpy(out, stream, outlen);
  }
}
#endif

static void prg_256_ballet(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                           size_t outlen) {
#if PRG_HAS_BALLET_AVX2
  if (ublockith_x86_has_avx2()) {
    prg_256_ballet_avx2(key, internal_iv, out, outlen);
    return;
  }
#endif
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
#if PRG_HAS_BALLET_AVX2
  if (PRG_HAS_4KEY_FIXED_TWEAK_AVX2 && seclvl == 256u && ublockith_x86_has_avx2() &&
      out_stride >= outlen && count >= 4u) {
    for (; i + 4u <= count; i += 4u) {
      uint32_t ctr32_host;
      uint64_t ctr_hi;
      __m256i b_init;
      __m256i c_init;
      __m256i d_init;
      __m256i rk_pack[RoundBallet256256 * 2];
      uint8_t stream0[4][UBLOCK_BLOCK];
      uint8_t stream1[4][UBLOCK_BLOCK];
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
        memcpy(out_lane, stream0[lane], UBLOCK_BLOCK);
        memcpy(out_lane + UBLOCK_BLOCK, stream1[lane], UBLOCK_BLOCK);
      }
    }
  }
#endif
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

#if PRG_HAS_BALLET_AVX2
  if (PRG_HAS_4KEY_FIXED_TWEAK_AVX2 && seclvl == 256u && ublockith_x86_has_avx2() &&
      out_stride >= outlen && count >= 4u) {
    const size_t n4 = count & ~(size_t)3u;
    prg_fixed_tweak_independent_batch_256_avx2(keys, key_stride, iv, tweak, out, out_stride,
                                               outlen, n4);
    for (size_t j = 0; j < n4; ++j) {
      memcpy(sd_out + j * sd_stride, keys + j * key_stride, key_bytes);
    }
    i = n4;
  }
#endif

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

#if PRG_HAS_BALLET_AVX2
  if (PRG_HAS_4KEY_FIXED_TWEAK_AVX2 && seclvl == 256u && ublockith_x86_has_avx2() &&
      out_stride >= outlen && count >= 4u) {
    const size_t n4 = count & ~(size_t)3u;
    prg_fixed_tweak_independent_batch_256_avx2(keys, key_stride, iv, tweak, out, out_stride,
                                               outlen, n4);
    for (size_t j = 0; j < n4; ++j) {
      const uint8_t* key_j = keys + j * key_stride;
      uint8_t* sd_slot =
          prg_sd_xor_map_slot(sd_out, sd_stride, sd_index_base + j, sd_index_xor);
      memcpy(sd_slot, key_j, key_bytes);
    }
    i = n4;
  }
#endif

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
#if PRG_HAS_BALLET_AVX2
  if (PRG_HAS_4KEY_FIXED_TWEAK_AVX2 && seclvl == 256u && ublockith_x86_has_avx2() &&
      out_stride == outlen && count >= 4) {
    const size_t n4 = count & ~(size_t)3u;
    prg_fixed_tweak_independent_batch_256_avx2(keys, key_stride, iv, tweak, out, out_stride,
                                               outlen, n4);
    i = n4;
  }
#endif
  for (; i < count; ++i) {
    prg(keys + i * key_stride, iv, tweak, out + i * out_stride, seclvl, outlen);
  }
}

void prg_4_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
                  unsigned int seclvl) {
  const size_t outlen = 4u * (size_t)(seclvl / 8u);
  prg(key, iv, tweak, out, seclvl, outlen);
}
