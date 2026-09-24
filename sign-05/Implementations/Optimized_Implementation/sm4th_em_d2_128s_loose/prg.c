/*
 * SPDX-License-Identifier: MIT
 *
 * Pseudorandom generator (PRG) selection and wrappers.
 */
#include "prg.h"

#include "utils_sm4/sm4.h"
#include "x86_caps.h"

#ifndef SM4TH_ENABLE_HYGON_CIS
#define SM4TH_ENABLE_HYGON_CIS 0
#endif

#if SM4TH_ENABLE_HYGON_CIS && defined(__x86_64__)
#include "utils_sm4/hygon_cis_sm4.h"
#endif
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

#include <assert.h>
#include <string.h>

#ifndef PRG_BLOCK_SIZE
#define PRG_BLOCK_SIZE SM4_BLOCK_SIZE
#endif

#ifndef PRG_BATCH_BLOCKS
#define PRG_BATCH_BLOCKS 32
#endif

#if SM4TH_ENABLE_SM4_ACCE && (defined(__x86_64__) || defined(__i386__))
#define PRG_HAS_SM4_FASTPATH 1
#else
#define PRG_HAS_SM4_FASTPATH 0
#endif

#if SM4TH_ENABLE_HYGON_CIS && defined(__x86_64__)
#define PRG_HAS_HYGON_CIS_FASTPATH 1
#else
#define PRG_HAS_HYGON_CIS_FASTPATH 0
#endif

#if PRG_HAS_SM4_FASTPATH
void sm4_encrypt4_opt(const uint32_t *rk, const void *src, const void *dst);

void yuchen_sm4_key_schedule_impl(const uint8_t *key, uint32_t *rk);
void yuchen_sm4_encrypt_impl(const uint32_t *rk, const uint8_t *plaintext, uint8_t *ciphertext);
#endif

static inline void prg_increment_iv(uint8_t* iv) {
  uint32_t iv0;
  memcpy(&iv0, iv, sizeof(uint32_t));
  iv0 = htole32(le32toh(iv0) + 1);
  memcpy(iv, &iv0, sizeof(uint32_t));
}

static inline void prg_increment_iv_by_n(uint8_t* iv, uint32_t n) {
  uint32_t iv0;
  memcpy(&iv0, iv, sizeof(uint32_t));
  iv0 = htole32(le32toh(iv0) + n);
  memcpy(iv, &iv0, sizeof(uint32_t));
}

static void add_to_upper_word(uint8_t* iv, uint32_t tweak) {
  uint32_t iv3;
  memcpy(&iv3, iv + PRG_BLOCK_SIZE - sizeof(uint32_t), sizeof(uint32_t));
  iv3 = htole32(le32toh(iv3) + tweak);
  memcpy(iv + PRG_BLOCK_SIZE - sizeof(uint32_t), &iv3, sizeof(uint32_t));
}

static inline int prg_has_sm4_fastpath_runtime(void) {
#if PRG_HAS_SM4_FASTPATH
  return sm4th_x86_has_aes_ssse3() && sm4th_x86_has_avx2();
#else
  return 0;
#endif
}

static inline int prg_has_sm4_fastpath_runtime_cached(void) {
#if PRG_HAS_SM4_FASTPATH
  static int cached = -1;
  if (cached < 0) {
    cached = prg_has_sm4_fastpath_runtime() ? 1 : 0;
  }
  return cached;
#else
  return 0;
#endif
}

#if PRG_HAS_HYGON_CIS_FASTPATH
static inline int prg_has_hygon_cis_fastpath_runtime_cached(void) {
  static int cached = -1;
  if (cached < 0) {
    cached = hygon_cis_sm4_is_supported() ? 1 : 0;
  }
  return cached;
}

static inline void prg_prepare_ctr2(const uint8_t* iv, uint32_t tweak,
                                    uint8_t ctr2[2 * PRG_BLOCK_SIZE]) {
  memcpy(ctr2, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(ctr2, tweak);
  memcpy(ctr2 + PRG_BLOCK_SIZE, ctr2, PRG_BLOCK_SIZE);
  prg_increment_iv(ctr2 + PRG_BLOCK_SIZE);
}

static inline void prg_prepare_ctr2x4(const uint8_t* iv, uint32_t tweak_base,
                                      uint32_t tweak_step, size_t index_base,
                                      uint8_t ctr2x4[4 * 2 * PRG_BLOCK_SIZE]) {
  for (size_t lane = 0; lane < 4; ++lane) {
    prg_prepare_ctr2(iv, tweak_base + tweak_step * (uint32_t)(index_base + lane),
                     ctr2x4 + lane * 2 * PRG_BLOCK_SIZE);
  }
}

static inline void prg_hygon_ctr2_from_key(const uint8_t* key, const uint8_t* iv,
                                           uint32_t tweak, uint8_t* out) {
  uint8_t ctr2[2 * PRG_BLOCK_SIZE];
  prg_prepare_ctr2(iv, tweak, ctr2);
  hygon_cis_sm4_prg_2lambda_from_key(key, ctr2, out);
}

#endif

#if PRG_HAS_SM4_FASTPATH
static inline uint32_t prg_load_u32_be(const uint8_t* p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
         (uint32_t)p[3];
}

static inline uint32_t sm4_key_sub_table(uint32_t x) {
  static const uint8_t sm4_sbox_local[256] = {
      0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2, 0x28, 0xFB,
      0x2C, 0x05, 0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3, 0xAA, 0x44, 0x13, 0x26,
      0x49, 0x86, 0x06, 0x99, 0x9C, 0x42, 0x50, 0xF4, 0x91, 0xEF, 0x98, 0x7A, 0x33, 0x54,
      0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62, 0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95,
      0x80, 0xDF, 0x94, 0xFA, 0x75, 0x8F, 0x3F, 0xA6, 0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73,
      0x17, 0xBA, 0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8, 0x68, 0x6B, 0x81, 0xB2,
      0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35, 0x1E, 0x24,
      0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B, 0x01, 0x21, 0x78, 0x87,
      0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52, 0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4,
      0xC8, 0x9E, 0xEA, 0xBF, 0x8A, 0xD2, 0x40, 0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE,
      0xF9, 0x61, 0x15, 0xA1, 0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55, 0xAD, 0x93,
      0x32, 0x30, 0xF5, 0x8C, 0xB1, 0xE3, 0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60,
      0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F, 0xD5, 0xDB, 0x37, 0x45, 0xDE, 0xFD,
      0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51, 0x8D, 0x1B, 0xAF, 0x92,
      0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41, 0x1F, 0x10, 0x5A, 0xD8, 0x0A, 0xC1,
      0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD, 0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0,
      0x89, 0x69, 0x97, 0x4A, 0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E,
      0xC6, 0x84, 0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20, 0x79, 0xEE, 0x5F, 0x3E,
      0xD7, 0xCB, 0x39, 0x48};
  uint32_t t = ((uint32_t)sm4_sbox_local[(uint8_t)(x >> 24)] << 24) |
               ((uint32_t)sm4_sbox_local[(uint8_t)(x >> 16)] << 16) |
               ((uint32_t)sm4_sbox_local[(uint8_t)(x >> 8)] << 8) |
               (uint32_t)sm4_sbox_local[(uint8_t)x];
  return t ^ rotl32_u(t, 13) ^ rotl32_u(t, 23);
}

static void sm4_key_schedule_interleaved_4(const uint8_t* keys, size_t key_stride,
                                           uint32_t rks[4][SM4_KEY_SCHEDULE]) {
  uint32_t k0[4] = {0}, k1[4] = {0}, k2[4] = {0}, k3[4] = {0};

  for (size_t lane = 0; lane < 4; ++lane) {
    const uint8_t* key = keys + lane * key_stride;
    k0[lane] = prg_load_u32_be(key) ^ FK[0];
    k1[lane] = prg_load_u32_be(key + 4) ^ FK[1];
    k2[lane] = prg_load_u32_be(key + 8) ^ FK[2];
    k3[lane] = prg_load_u32_be(key + 12) ^ FK[3];
  }

  for (size_t i = 0; i < SM4_KEY_SCHEDULE; i += 4) {
    for (size_t lane = 0; lane < 4; ++lane) {
      k0[lane] ^= sm4_key_sub_table(k1[lane] ^ k2[lane] ^ k3[lane] ^ CK[i]);
      rks[lane][i] = k0[lane];
    }
    for (size_t lane = 0; lane < 4; ++lane) {
      k1[lane] ^= sm4_key_sub_table(k2[lane] ^ k3[lane] ^ k0[lane] ^ CK[i + 1]);
      rks[lane][i + 1] = k1[lane];
    }
    for (size_t lane = 0; lane < 4; ++lane) {
      k2[lane] ^= sm4_key_sub_table(k3[lane] ^ k0[lane] ^ k1[lane] ^ CK[i + 2]);
      rks[lane][i + 2] = k2[lane];
    }
    for (size_t lane = 0; lane < 4; ++lane) {
      k3[lane] ^= sm4_key_sub_table(k0[lane] ^ k1[lane] ^ k2[lane] ^ CK[i + 3]);
      rks[lane][i + 3] = k3[lane];
    }
  }
}

// adapted from sm4_encrypt4_opt in sm4ni.c to encrypt 4 blocks with 4 different keys in parallel
// dst[lane] = SM4_Encrypt(rks[lane], src[lane])，lane=0..3。
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("ssse3,aes"))) static void
sm4_encrypt4_diffkeys_1blk(uint32_t rks[4][SM4_KEY_SCHEDULE], const uint8_t* src, uint8_t* dst) {
  const __m128i c0f __attribute__((aligned(0x10))) = {0x0F0F0F0F0F0F0F0FULL,
                                                       0x0F0F0F0F0F0F0F0FULL};
  const __m128i flp __attribute__((aligned(0x10))) = {0x0405060700010203ULL,
                                                       0x0C0D0E0F08090A0BULL};
  const __m128i shr __attribute__((aligned(0x10))) = {0x0B0E0104070A0D00ULL,
                                                       0x0306090C0F020508ULL};
  const __m128i m1l __attribute__((aligned(0x10))) = {0x9197E2E474720701ULL,
                                                       0xC7C1B4B222245157ULL};
  const __m128i m1h __attribute__((aligned(0x10))) = {0xE240AB09EB49A200ULL,
                                                       0xF052B91BF95BB012ULL};
  const __m128i m2l __attribute__((aligned(0x10))) = {0x5B67F2CEA19D0834ULL,
                                                       0xEDD14478172BBE82ULL};
  const __m128i m2h __attribute__((aligned(0x10))) = {0xAE7201DD73AFDC00ULL,
                                                       0x11CDBE62CC1063BFULL};
  const __m128i r08 __attribute__((aligned(0x10))) = {0x0605040702010003ULL,
                                                       0x0E0D0C0F0A09080BULL};
  const __m128i r16 __attribute__((aligned(0x10))) = {0x0504070601000302ULL,
                                                       0x0D0C0F0E09080B0AULL};
  const __m128i r24 __attribute__((aligned(0x10))) = {0x0407060500030201ULL,
                                                       0x0C0F0E0D080B0A09ULL};

  __m128i x, y, t0, t1, t2, t3;
  uint32_t v[4] __attribute__((aligned(0x10)));
  const uint32_t* p32 = (const uint32_t*)src;

  t0 = _mm_set_epi32(p32[12], p32[8], p32[4], p32[0]);
  t0 = _mm_shuffle_epi8(t0, flp);
  t1 = _mm_set_epi32(p32[13], p32[9], p32[5], p32[1]);
  t1 = _mm_shuffle_epi8(t1, flp);
  t2 = _mm_set_epi32(p32[14], p32[10], p32[6], p32[2]);
  t2 = _mm_shuffle_epi8(t2, flp);
  t3 = _mm_set_epi32(p32[15], p32[11], p32[7], p32[3]);
  t3 = _mm_shuffle_epi8(t3, flp);

  for (int i = 0; i < 32; ++i) {
    const __m128i rk = _mm_set_epi32((int)rks[3][i], (int)rks[2][i], (int)rks[1][i],
                                     (int)rks[0][i]);
    x                = t1 ^ t2 ^ t3 ^ rk;

    y = _mm_and_si128(x, c0f);
    y = _mm_shuffle_epi8(m1l, y);
    x = _mm_srli_epi64(x, 4);
    x = _mm_and_si128(x, c0f);
    x = _mm_shuffle_epi8(m1h, x) ^ y;

    x = _mm_shuffle_epi8(x, shr);
    x = _mm_aesenclast_si128(x, c0f);

    y = _mm_andnot_si128(x, c0f);
    y = _mm_shuffle_epi8(m2l, y);
    x = _mm_srli_epi64(x, 4);
    x = _mm_and_si128(x, c0f);
    x = _mm_shuffle_epi8(m2h, x) ^ y;

    y = x ^ _mm_shuffle_epi8(x, r08) ^ _mm_shuffle_epi8(x, r16);
    y = _mm_slli_epi32(y, 2) ^ _mm_srli_epi32(y, 30);
    x = x ^ y ^ _mm_shuffle_epi8(x, r24);

    x ^= t0;
    t0 = t1;
    t1 = t2;
    t2 = t3;
    t3 = x;
  }

  uint32_t* out32 = (uint32_t*)dst;
  _mm_store_si128((__m128i*)v, _mm_shuffle_epi8(t3, flp));
  out32[0] = v[0];
  out32[4] = v[1];
  out32[8] = v[2];
  out32[12] = v[3];
  _mm_store_si128((__m128i*)v, _mm_shuffle_epi8(t2, flp));
  out32[1] = v[0];
  out32[5] = v[1];
  out32[9] = v[2];
  out32[13] = v[3];
  _mm_store_si128((__m128i*)v, _mm_shuffle_epi8(t1, flp));
  out32[2] = v[0];
  out32[6] = v[1];
  out32[10] = v[2];
  out32[14] = v[3];
  _mm_store_si128((__m128i*)v, _mm_shuffle_epi8(t0, flp));
  out32[3] = v[0];
  out32[7] = v[1];
  out32[11] = v[2];
  out32[15] = v[3];
}

__attribute__((target("ssse3,aes"))) static void
sm4_pack_round_keys4(uint32_t rks[4][SM4_KEY_SCHEDULE], __m128i rk4[SM4_KEY_SCHEDULE]) {
  for (int i = 0; i < SM4_KEY_SCHEDULE; ++i) {
    rk4[i] = _mm_set_epi32((int)rks[3][i], (int)rks[2][i], (int)rks[1][i], (int)rks[0][i]);
  }
}

__attribute__((target("ssse3,aes"))) static void
sm4_encrypt4_diffkeys_1blk_same_input(const __m128i rk4[SM4_KEY_SCHEDULE], const uint8_t* src_blk,
                                      uint8_t* dst_4blk) {
  const __m128i c0f __attribute__((aligned(0x10))) = {0x0F0F0F0F0F0F0F0FULL,
                                                       0x0F0F0F0F0F0F0F0FULL};
  const __m128i flp __attribute__((aligned(0x10))) = {0x0405060700010203ULL,
                                                       0x0C0D0E0F08090A0BULL};
  const __m128i shr __attribute__((aligned(0x10))) = {0x0B0E0104070A0D00ULL,
                                                       0x0306090C0F020508ULL};
  const __m128i m1l __attribute__((aligned(0x10))) = {0x9197E2E474720701ULL,
                                                       0xC7C1B4B222245157ULL};
  const __m128i m1h __attribute__((aligned(0x10))) = {0xE240AB09EB49A200ULL,
                                                       0xF052B91BF95BB012ULL};
  const __m128i m2l __attribute__((aligned(0x10))) = {0x5B67F2CEA19D0834ULL,
                                                       0xEDD14478172BBE82ULL};
  const __m128i m2h __attribute__((aligned(0x10))) = {0xAE7201DD73AFDC00ULL,
                                                       0x11CDBE62CC1063BFULL};
  const __m128i r08 __attribute__((aligned(0x10))) = {0x0605040702010003ULL,
                                                       0x0E0D0C0F0A09080BULL};
  const __m128i r16 __attribute__((aligned(0x10))) = {0x0504070601000302ULL,
                                                       0x0D0C0F0E09080B0AULL};
  const __m128i r24 __attribute__((aligned(0x10))) = {0x0407060500030201ULL,
                                                       0x0C0F0E0D080B0A09ULL};

  __m128i x, y, t0, t1, t2, t3;
  uint32_t v0[4] __attribute__((aligned(0x10)));
  uint32_t v1[4] __attribute__((aligned(0x10)));
  uint32_t v2[4] __attribute__((aligned(0x10)));
  uint32_t v3[4] __attribute__((aligned(0x10)));
  const uint32_t* p32 = (const uint32_t*)src_blk;

  t0 = _mm_shuffle_epi8(_mm_set1_epi32((int)p32[0]), flp);
  t1 = _mm_shuffle_epi8(_mm_set1_epi32((int)p32[1]), flp);
  t2 = _mm_shuffle_epi8(_mm_set1_epi32((int)p32[2]), flp);
  t3 = _mm_shuffle_epi8(_mm_set1_epi32((int)p32[3]), flp);

  for (int i = 0; i < 32; ++i) {
    x = t1 ^ t2 ^ t3 ^ rk4[i];

    y = _mm_and_si128(x, c0f);
    y = _mm_shuffle_epi8(m1l, y);
    x = _mm_srli_epi64(x, 4);
    x = _mm_and_si128(x, c0f);
    x = _mm_shuffle_epi8(m1h, x) ^ y;

    x = _mm_shuffle_epi8(x, shr);
    x = _mm_aesenclast_si128(x, c0f);

    y = _mm_andnot_si128(x, c0f);
    y = _mm_shuffle_epi8(m2l, y);
    x = _mm_srli_epi64(x, 4);
    x = _mm_and_si128(x, c0f);
    x = _mm_shuffle_epi8(m2h, x) ^ y;

    y = x ^ _mm_shuffle_epi8(x, r08) ^ _mm_shuffle_epi8(x, r16);
    y = _mm_slli_epi32(y, 2) ^ _mm_srli_epi32(y, 30);
    x = x ^ y ^ _mm_shuffle_epi8(x, r24);

    x ^= t0;
    t0 = t1;
    t1 = t2;
    t2 = t3;
    t3 = x;
  }

  _mm_store_si128((__m128i*)v0, _mm_shuffle_epi8(t3, flp));
  _mm_store_si128((__m128i*)v1, _mm_shuffle_epi8(t2, flp));
  _mm_store_si128((__m128i*)v2, _mm_shuffle_epi8(t1, flp));
  _mm_store_si128((__m128i*)v3, _mm_shuffle_epi8(t0, flp));

  uint32_t* out0 = (uint32_t*)(dst_4blk + 0 * PRG_BLOCK_SIZE);
  uint32_t* out1 = (uint32_t*)(dst_4blk + 1 * PRG_BLOCK_SIZE);
  uint32_t* out2 = (uint32_t*)(dst_4blk + 2 * PRG_BLOCK_SIZE);
  uint32_t* out3 = (uint32_t*)(dst_4blk + 3 * PRG_BLOCK_SIZE);

  out0[0] = v0[0];
  out0[1] = v1[0];
  out0[2] = v2[0];
  out0[3] = v3[0];
  out1[0] = v0[1];
  out1[1] = v1[1];
  out1[2] = v2[1];
  out1[3] = v3[1];
  out2[0] = v0[2];
  out2[1] = v1[2];
  out2[2] = v2[2];
  out2[3] = v3[2];
  out3[0] = v0[3];
  out3[1] = v1[3];
  out3[2] = v2[3];
  out3[3] = v3[3];
}
#endif

static inline void prg_sm4_ctr2_from_rk(const uint32_t* rk, const uint8_t* iv, uint32_t tweak,
                                        uint8_t* out) {
  uint8_t ctr[4 * PRG_BLOCK_SIZE];
  uint8_t enc[4 * PRG_BLOCK_SIZE];
  memcpy(ctr, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(ctr, tweak);
  memcpy(ctr + PRG_BLOCK_SIZE, ctr, PRG_BLOCK_SIZE);
  prg_increment_iv(ctr + PRG_BLOCK_SIZE);
  // Fill the unused lanes to keep one 4-block SM4NI call.
  memcpy(ctr + 2 * PRG_BLOCK_SIZE, ctr, PRG_BLOCK_SIZE);
  memcpy(ctr + 3 * PRG_BLOCK_SIZE, ctr + PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
  sm4_encrypt4_opt(rk, ctr, enc);
  memcpy(out, enc, 2 * PRG_BLOCK_SIZE);
}
#endif

static void generic_prg(const uint8_t* key, uint8_t* internal_iv, uint8_t* out,
                        unsigned int seclvl, size_t outlen) {
  assert(seclvl == 128 || seclvl == 256);
  /* SM4-CTR PRG only. */
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

static inline void prg_2_lambda_single_impl(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                                             uint8_t* out, unsigned int seclvl,
                                             int use_sm4_fastpath) {
#if PRG_HAS_HYGON_CIS_FASTPATH
  if (seclvl == 128u && prg_has_hygon_cis_fastpath_runtime_cached()) {
    prg_hygon_ctr2_from_key(key, iv, tweak, out);
    return;
  }
#endif
#if PRG_HAS_SM4_FASTPATH
  if (use_sm4_fastpath) {
    uint32_t rk[SM4_KEY_SCHEDULE];
    yuchen_sm4_key_schedule_impl(key, rk);
    prg_sm4_ctr2_from_rk(rk, iv, tweak, out);
    return;
  }
#else
  (void)use_sm4_fastpath;
#endif

  uint8_t internal_iv[PRG_BLOCK_SIZE];
  memcpy(internal_iv, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(internal_iv, tweak);
  generic_prg(key, internal_iv, out, seclvl, seclvl * 2 / 8);
}

// - run sm4_key_schedule_interleaved_4 for 4 lane round keys (different keys).
// - prepare two counter blocks for each lane: ctr0 and ctr1=ctr0+1.
// - call sm4_encrypt4_diffkeys_1blk twice to encrypt ctr0/ctr1.
// - concatenate the results for each lane to form 32B output (2 SM4 blocks).
static void prg_2_lambda_independent_batch_impl(const uint8_t* keys, size_t key_stride,
                                                const uint8_t* iv, uint32_t tweak_base,
                                                uint32_t tweak_step, uint8_t* out,
                                                size_t out_stride, unsigned int seclvl,
                                                size_t count) {
  if (count == 0) {
    return;
  }

#if PRG_HAS_HYGON_CIS_FASTPATH
  if (prg_has_hygon_cis_fastpath_runtime_cached() && seclvl == 128 && out_stride == 32) {
    size_t i = 0;
    while (i + 4 <= count) {
      uint8_t ctr2x4[4 * 2 * PRG_BLOCK_SIZE];
      prg_prepare_ctr2x4(iv, tweak_base, tweak_step, i, ctr2x4);
      hygon_cis_sm4_prg_2lambda_4way_from_key(keys + i * key_stride, key_stride, ctr2x4,
                                              out + i * out_stride, out_stride);
      i += 4;
    }

    for (; i < count; ++i) {
      prg_hygon_ctr2_from_key(keys + i * key_stride, iv,
                              tweak_base + tweak_step * (uint32_t)i, out + i * out_stride);
    }
    return;
  }
#endif

#if PRG_HAS_SM4_FASTPATH
  if (prg_has_sm4_fastpath_runtime_cached() && seclvl == 128 && out_stride == 32) {
    size_t i = 0;
    while (i + 4 <= count) {
      uint32_t rks[4][SM4_KEY_SCHEDULE];
      uint8_t ctr0[4 * PRG_BLOCK_SIZE];
      uint8_t ctr1[4 * PRG_BLOCK_SIZE];
      uint8_t enc0[4 * PRG_BLOCK_SIZE];
      uint8_t enc1[4 * PRG_BLOCK_SIZE];

      sm4_key_schedule_interleaved_4(keys + i * key_stride, key_stride, rks);

      for (size_t lane = 0; lane < 4; ++lane) {
        uint8_t* ctr0_lane = ctr0 + lane * PRG_BLOCK_SIZE;
        uint8_t* ctr1_lane = ctr1 + lane * PRG_BLOCK_SIZE;
        memcpy(ctr0_lane, iv, PRG_BLOCK_SIZE);
        add_to_upper_word(ctr0_lane, tweak_base + tweak_step * (uint32_t)(i + lane));
        memcpy(ctr1_lane, ctr0_lane, PRG_BLOCK_SIZE);
        prg_increment_iv(ctr1_lane);
      }

#if defined(__x86_64__) || defined(__i386__)
      sm4_encrypt4_diffkeys_1blk(rks, ctr0, enc0);
      sm4_encrypt4_diffkeys_1blk(rks, ctr1, enc1);
      for (size_t lane = 0; lane < 4; ++lane) {
        uint8_t* out_lane = out + (i + lane) * out_stride;
        memcpy(out_lane, enc0 + lane * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
        memcpy(out_lane + PRG_BLOCK_SIZE, enc1 + lane * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
      }
#else
      for (size_t lane = 0; lane < 4; ++lane) {
        prg_sm4_ctr2_from_rk(rks[lane], iv, tweak_base + (uint32_t)(i + lane),
                             out + (i + lane) * out_stride);
      }
#endif
      i += 4;
    }

    for (; i < count; ++i) {
      uint32_t rk[SM4_KEY_SCHEDULE];
      yuchen_sm4_key_schedule_impl(keys + i * key_stride, rk);
      prg_sm4_ctr2_from_rk(rk, iv, tweak_base + tweak_step * (uint32_t)i, out + i * out_stride);
    }
    return;
  }
#endif

  const int use_sm4_fastpath = (seclvl == 128) && prg_has_sm4_fastpath_runtime_cached();
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* out_i = out + i * out_stride;
    prg_2_lambda_single_impl(key_i, iv, tweak_base + tweak_step * (uint32_t)i, out_i, seclvl,
                             use_sm4_fastpath);
  }
}

void prg_2_lambda_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                    uint32_t tweak_base, uint8_t* out, size_t out_stride,
                                    unsigned int seclvl, size_t count) {
  prg_2_lambda_independent_batch_impl(keys, key_stride, iv, tweak_base, 1, out, out_stride,
                                      seclvl, count);
}

static inline uint8_t* prg_sd_xor_map_slot(uint8_t* sd_out, size_t sd_stride, size_t index,
                                           size_t sd_index_base, size_t sd_index_xor) {
  const size_t mapped = (sd_index_base + index) ^ sd_index_xor;
  if (mapped == 0) {
    return NULL;
  }
  return sd_out + mapped * sd_stride;
}

#define PRG_162_BYTES 162u
#define PRG_162_FULL_BLOCKS 10u
#define PRG_162_TOTAL_BLOCKS (PRG_162_FULL_BLOCKS + 1u)

#if PRG_HAS_HYGON_CIS_FASTPATH
static inline void prg_prepare_counters_162(const uint8_t* iv, uint32_t tweak,
                                            uint8_t ctr162[PRG_162_TOTAL_BLOCKS *
                                                           PRG_BLOCK_SIZE]) {
  uint8_t internal_iv[PRG_BLOCK_SIZE];
  uint8_t suffix[PRG_BLOCK_SIZE - sizeof(uint32_t)];
  uint32_t ctr_base_le;
  uint32_t ctr_base;

  memcpy(internal_iv, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(internal_iv, tweak);
  memcpy(&ctr_base_le, internal_iv, sizeof(uint32_t));
  ctr_base = le32toh(ctr_base_le);
  memcpy(suffix, internal_iv + sizeof(uint32_t), sizeof(suffix));

  for (uint32_t blk = 0; blk < PRG_162_TOTAL_BLOCKS; ++blk) {
    const size_t off = (size_t)blk * PRG_BLOCK_SIZE;
    const uint32_t ctr = htole32(ctr_base + blk);
    memcpy(ctr162 + off, &ctr, sizeof(uint32_t));
    memcpy(ctr162 + off + sizeof(uint32_t), suffix, sizeof(suffix));
  }
}

static inline void prg_hygon_162_from_rk_precomp(
    const uint32_t* rk, const uint8_t ctr162[PRG_162_TOTAL_BLOCKS * PRG_BLOCK_SIZE],
    uint8_t* out) {
  uint8_t tail[PRG_BLOCK_SIZE];

  hygon_cis_sm4_encrypt_blocks(rk, ctr162, out, PRG_162_FULL_BLOCKS);
  hygon_cis_sm4_encrypt_blocks(rk, ctr162 + PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE, tail, 1);
  memcpy(out + PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE, tail,
         PRG_162_BYTES - PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE);
}

static inline void prg_hygon_162_from_rk(const uint32_t* rk, const uint8_t* iv,
                                         uint32_t tweak, uint8_t* out) {
  uint8_t ctr162[PRG_162_TOTAL_BLOCKS * PRG_BLOCK_SIZE];
  prg_prepare_counters_162(iv, tweak, ctr162);
  prg_hygon_162_from_rk_precomp(rk, ctr162, out);
}

static inline void prg_hygon_2lambda_162_from_key_precomp(
    const uint8_t* key, const uint8_t ctr2[2 * PRG_BLOCK_SIZE],
    const uint8_t ctr162[PRG_162_TOTAL_BLOCKS * PRG_BLOCK_SIZE], uint8_t* out2,
    uint8_t* out162) {
  uint32_t rk[SM4_KEY_SCHEDULE];

  hygon_cis_sm4_set_key(key, rk);
  hygon_cis_sm4_encrypt_blocks(rk, ctr2, out2, 2);
  prg_hygon_162_from_rk_precomp(rk, ctr162, out162);
}
#endif

#if PRG_HAS_SM4_FASTPATH
static inline void prg_162_from_rk(const uint32_t* rk, const uint8_t* iv, uint32_t tweak,
                                   uint8_t* out) {
  uint8_t ctr0[PRG_BLOCK_SIZE];
  uint8_t ctr[PRG_BLOCK_SIZE];
  uint8_t enc[PRG_BLOCK_SIZE];

  memcpy(ctr0, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(ctr0, tweak);

  for (uint32_t blk = 0; blk < PRG_162_FULL_BLOCKS; ++blk) {
    memcpy(ctr, ctr0, PRG_BLOCK_SIZE);
    prg_increment_iv_by_n(ctr, blk);
    yuchen_sm4_encrypt_impl(rk, ctr, enc);
    memcpy(out + (size_t)blk * PRG_BLOCK_SIZE, enc, PRG_BLOCK_SIZE);
  }

  memcpy(ctr, ctr0, PRG_BLOCK_SIZE);
  prg_increment_iv_by_n(ctr, PRG_162_FULL_BLOCKS);
  yuchen_sm4_encrypt_impl(rk, ctr, enc);
  memcpy(out + (size_t)PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE, enc, PRG_162_BYTES - PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE);
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("ssse3,aes"))) static void
prg_sm4_ctr2_and_162_from_rks4_fixed_tweaks(
    uint32_t rks[4][SM4_KEY_SCHEDULE], const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_162, uint8_t* out2_0, uint8_t* out2_1, uint8_t* out2_2, uint8_t* out2_3,
    uint8_t* out162_0, uint8_t* out162_1, uint8_t* out162_2, uint8_t* out162_3) {
  __m128i rk4[SM4_KEY_SCHEDULE];
  uint8_t ctr[PRG_BLOCK_SIZE];
  uint8_t enc[4 * PRG_BLOCK_SIZE];

  sm4_pack_round_keys4(rks, rk4);

  memcpy(ctr, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(ctr, tweak_2lambda);

  sm4_encrypt4_diffkeys_1blk_same_input(rk4, ctr, enc);
  memcpy(out2_0, enc + 0 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
  memcpy(out2_1, enc + 1 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
  memcpy(out2_2, enc + 2 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
  memcpy(out2_3, enc + 3 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);

  prg_increment_iv(ctr);
  sm4_encrypt4_diffkeys_1blk_same_input(rk4, ctr, enc);
  memcpy(out2_0 + PRG_BLOCK_SIZE, enc + 0 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
  memcpy(out2_1 + PRG_BLOCK_SIZE, enc + 1 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
  memcpy(out2_2 + PRG_BLOCK_SIZE, enc + 2 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
  memcpy(out2_3 + PRG_BLOCK_SIZE, enc + 3 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);

  memcpy(ctr, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(ctr, tweak_162);

  for (uint32_t blk = 0; blk < PRG_162_FULL_BLOCKS; ++blk) {
    sm4_encrypt4_diffkeys_1blk_same_input(rk4, ctr, enc);
    memcpy(out162_0 + (size_t)blk * PRG_BLOCK_SIZE, enc + 0 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
    memcpy(out162_1 + (size_t)blk * PRG_BLOCK_SIZE, enc + 1 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
    memcpy(out162_2 + (size_t)blk * PRG_BLOCK_SIZE, enc + 2 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
    memcpy(out162_3 + (size_t)blk * PRG_BLOCK_SIZE, enc + 3 * PRG_BLOCK_SIZE, PRG_BLOCK_SIZE);
    prg_increment_iv(ctr);
  }

  sm4_encrypt4_diffkeys_1blk_same_input(rk4, ctr, enc);
  memcpy(out162_0 + (size_t)PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE, enc + 0 * PRG_BLOCK_SIZE,
         PRG_162_BYTES - PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE);
  memcpy(out162_1 + (size_t)PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE, enc + 1 * PRG_BLOCK_SIZE,
         PRG_162_BYTES - PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE);
  memcpy(out162_2 + (size_t)PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE, enc + 2 * PRG_BLOCK_SIZE,
         PRG_162_BYTES - PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE);
  memcpy(out162_3 + (size_t)PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE, enc + 3 * PRG_BLOCK_SIZE,
         PRG_162_BYTES - PRG_162_FULL_BLOCKS * PRG_BLOCK_SIZE);
}
#endif
#endif

static void prg_162(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out) {
#if PRG_HAS_HYGON_CIS_FASTPATH
  if (prg_has_hygon_cis_fastpath_runtime_cached()) {
    uint32_t rk[SM4_KEY_SCHEDULE];
    hygon_cis_sm4_set_key(key, rk);
    prg_hygon_162_from_rk(rk, iv, tweak, out);
    return;
  }
#endif

#if PRG_HAS_SM4_FASTPATH
  if (!prg_has_sm4_fastpath_runtime_cached()) {
    uint8_t internal_iv[PRG_BLOCK_SIZE];
    memcpy(internal_iv, iv, PRG_BLOCK_SIZE);
    add_to_upper_word(internal_iv, tweak);
    generic_prg(key, internal_iv, out, 128, PRG_162_BYTES);
    return;
  }

  uint32_t rk[SM4_KEY_SCHEDULE];
  yuchen_sm4_key_schedule_impl(key, rk);
  prg_162_from_rk(rk, iv, tweak, out);
#else
  uint8_t internal_iv[PRG_BLOCK_SIZE];
  memcpy(internal_iv, iv, PRG_BLOCK_SIZE);
  add_to_upper_word(internal_iv, tweak);
  generic_prg(key, internal_iv, out, 128, PRG_162_BYTES);
#endif
}

static inline void prg_2lambda_and_162_single_impl(const uint8_t* key, const uint8_t* iv,
                                                    uint32_t tweak_2lambda, uint32_t tweak_162,
                                                    uint8_t* out2, uint8_t* out162,
                                                    unsigned int seclvl,
                                                    int use_sm4_fastpath) {
#if PRG_HAS_HYGON_CIS_FASTPATH
  if (seclvl == 128u && prg_has_hygon_cis_fastpath_runtime_cached()) {
    uint8_t ctr2[2 * PRG_BLOCK_SIZE];
    uint8_t ctr162[PRG_162_TOTAL_BLOCKS * PRG_BLOCK_SIZE];
    prg_prepare_ctr2(iv, tweak_2lambda, ctr2);
    prg_prepare_counters_162(iv, tweak_162, ctr162);
    prg_hygon_2lambda_162_from_key_precomp(key, ctr2, ctr162, out2, out162);
    return;
  }
#endif

#if PRG_HAS_SM4_FASTPATH
  if (use_sm4_fastpath && seclvl == 128u) {
    uint32_t rk[SM4_KEY_SCHEDULE];
    yuchen_sm4_key_schedule_impl(key, rk);
    prg_sm4_ctr2_from_rk(rk, iv, tweak_2lambda, out2);
    prg_162_from_rk(rk, iv, tweak_162, out162);
    return;
  }
#endif
  prg_2_lambda_single_impl(key, iv, tweak_2lambda, out2, seclvl, use_sm4_fastpath);
  prg_162(key, iv, tweak_162, out162);
}

void prg_2_lambda_and_162_fixed_tweak_independent_batch_with_sd(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_162, uint8_t* sd_out, size_t sd_stride, uint8_t* out_2lambda,
    size_t out_2lambda_stride, uint8_t* out_162, size_t out_162_stride, unsigned int seclvl,
    size_t count) {
  if (count == 0) {
    return;
  }

  const size_t lambda_bytes = seclvl / 8;

#if PRG_HAS_HYGON_CIS_FASTPATH
  if (prg_has_hygon_cis_fastpath_runtime_cached() && seclvl == 128 &&
      out_2lambda_stride == 32 && out_162_stride == PRG_162_BYTES) {
    uint8_t ctr2[2 * PRG_BLOCK_SIZE];
    uint8_t ctr162[PRG_162_TOTAL_BLOCKS * PRG_BLOCK_SIZE];

    prg_prepare_ctr2(iv, tweak_2lambda, ctr2);
    prg_prepare_counters_162(iv, tweak_162, ctr162);

    for (size_t i = 0; i < count; ++i) {
      const uint8_t* key_i = keys + i * key_stride;
      uint8_t* sd_i = sd_out + i * sd_stride;
      uint8_t* out2_i = out_2lambda + i * out_2lambda_stride;
      uint8_t* out162_i = out_162 + i * out_162_stride;
      if (sd_i != key_i) {
        memcpy(sd_i, key_i, lambda_bytes);
      }
      prg_hygon_2lambda_162_from_key_precomp(key_i, ctr2, ctr162, out2_i, out162_i);
    }
    return;
  }
#endif

#if PRG_HAS_SM4_FASTPATH
  if (prg_has_sm4_fastpath_runtime_cached() && seclvl == 128 && out_2lambda_stride == 32 &&
      out_162_stride == PRG_162_BYTES) {
    size_t i = 0;
    while (i + 4 <= count) {
      uint32_t rks[4][SM4_KEY_SCHEDULE];

      sm4_key_schedule_interleaved_4(keys + i * key_stride, key_stride, rks);

#if defined(__x86_64__) || defined(__i386__)
      uint8_t* out2_0 = out_2lambda + (i + 0) * out_2lambda_stride;
      uint8_t* out2_1 = out_2lambda + (i + 1) * out_2lambda_stride;
      uint8_t* out2_2 = out_2lambda + (i + 2) * out_2lambda_stride;
      uint8_t* out2_3 = out_2lambda + (i + 3) * out_2lambda_stride;
      uint8_t* out162_0 = out_162 + (i + 0) * out_162_stride;
      uint8_t* out162_1 = out_162 + (i + 1) * out_162_stride;
      uint8_t* out162_2 = out_162 + (i + 2) * out_162_stride;
      uint8_t* out162_3 = out_162 + (i + 3) * out_162_stride;

      prg_sm4_ctr2_and_162_from_rks4_fixed_tweaks(
          rks, iv, tweak_2lambda, tweak_162, out2_0, out2_1, out2_2, out2_3, out162_0, out162_1,
          out162_2, out162_3);

      for (size_t lane = 0; lane < 4; ++lane) {
        const size_t idx = i + lane;
        const uint8_t* key_lane = keys + idx * key_stride;
        uint8_t* sd_lane = sd_out + idx * sd_stride;
        memcpy(sd_lane, key_lane, lambda_bytes);
      }
#else
      for (size_t lane = 0; lane < 4; ++lane) {
        const size_t idx = i + lane;
        const uint8_t* key_lane = keys + idx * key_stride;
        uint8_t* sd_lane = sd_out + idx * sd_stride;
        uint8_t* out2_lane = out_2lambda + idx * out_2lambda_stride;
        uint8_t* out162_lane = out_162 + idx * out_162_stride;
        memcpy(sd_lane, key_lane, lambda_bytes);
        prg_sm4_ctr2_from_rk(rks[lane], iv, tweak_2lambda, out2_lane);
        prg_162_from_rk(rks[lane], iv, tweak_162, out162_lane);
      }
#endif
      i += 4;
    }

    for (; i < count; ++i) {
      uint32_t rk[SM4_KEY_SCHEDULE];
      const uint8_t* key_i = keys + i * key_stride;
      uint8_t* sd_i = sd_out + i * sd_stride;
      uint8_t* out2_i = out_2lambda + i * out_2lambda_stride;
      uint8_t* out162_i = out_162 + i * out_162_stride;
      yuchen_sm4_key_schedule_impl(key_i, rk);
      memcpy(sd_i, key_i, lambda_bytes);
      prg_sm4_ctr2_from_rk(rk, iv, tweak_2lambda, out2_i);
      prg_162_from_rk(rk, iv, tweak_162, out162_i);
    }
    return;
  }
#endif

  const int use_sm4_fastpath = (seclvl == 128) && prg_has_sm4_fastpath_runtime_cached();
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* sd_i = sd_out + i * sd_stride;
    uint8_t* out2_i = out_2lambda + i * out_2lambda_stride;
    uint8_t* out162_i = out_162 + i * out_162_stride;
    memcpy(sd_i, key_i, lambda_bytes);
    prg_2lambda_and_162_single_impl(key_i, iv, tweak_2lambda, tweak_162, out2_i, out162_i,
                                    seclvl, use_sm4_fastpath);
  }
}

void prg_2_lambda_and_162_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_162, uint8_t* sd_out, size_t sd_stride, size_t sd_index_base,
    size_t sd_index_xor, uint8_t* out_2lambda, size_t out_2lambda_stride, uint8_t* out_162,
    size_t out_162_stride, unsigned int seclvl, size_t count) {
  if (count == 0) {
    return;
  }

  const size_t lambda_bytes = seclvl / 8;

#if PRG_HAS_HYGON_CIS_FASTPATH
  if (prg_has_hygon_cis_fastpath_runtime_cached() && seclvl == 128 &&
      out_2lambda_stride == 32 && out_162_stride == PRG_162_BYTES) {
    uint8_t ctr2[2 * PRG_BLOCK_SIZE];
    uint8_t ctr162[PRG_162_TOTAL_BLOCKS * PRG_BLOCK_SIZE];

    prg_prepare_ctr2(iv, tweak_2lambda, ctr2);
    prg_prepare_counters_162(iv, tweak_162, ctr162);

    for (size_t i = 0; i < count; ++i) {
      const uint8_t* key_i = keys + i * key_stride;
      uint8_t* out2_i = out_2lambda + i * out_2lambda_stride;
      uint8_t* sd_i =
          prg_sd_xor_map_slot(sd_out, sd_stride, i, sd_index_base, sd_index_xor);
      uint8_t* out162_i =
          prg_sd_xor_map_slot(out_162, out_162_stride, i, sd_index_base, sd_index_xor);
      if (sd_i) {
        memcpy(sd_i, key_i, lambda_bytes);
      }
      if (out162_i) {
        prg_hygon_2lambda_162_from_key_precomp(key_i, ctr2, ctr162, out2_i, out162_i);
      } else {
        hygon_cis_sm4_prg_2lambda_from_key(key_i, ctr2, out2_i);
      }
    }
    return;
  }
#endif

#if PRG_HAS_SM4_FASTPATH
  if (prg_has_sm4_fastpath_runtime_cached() && seclvl == 128 && out_2lambda_stride == 32 &&
      out_162_stride == PRG_162_BYTES) {
    size_t i = 0;
    while (i + 4 <= count) {
      uint32_t rks[4][SM4_KEY_SCHEDULE];

      sm4_key_schedule_interleaved_4(keys + i * key_stride, key_stride, rks);

#if defined(__x86_64__) || defined(__i386__)
      uint8_t* out2_0 = out_2lambda + (i + 0) * out_2lambda_stride;
      uint8_t* out2_1 = out_2lambda + (i + 1) * out_2lambda_stride;
      uint8_t* out2_2 = out_2lambda + (i + 2) * out_2lambda_stride;
      uint8_t* out2_3 = out_2lambda + (i + 3) * out_2lambda_stride;
      uint8_t* out162_0 =
          prg_sd_xor_map_slot(out_162, out_162_stride, i + 0, sd_index_base, sd_index_xor);
      uint8_t* out162_1 =
          prg_sd_xor_map_slot(out_162, out_162_stride, i + 1, sd_index_base, sd_index_xor);
      uint8_t* out162_2 =
          prg_sd_xor_map_slot(out_162, out_162_stride, i + 2, sd_index_base, sd_index_xor);
      uint8_t* out162_3 =
          prg_sd_xor_map_slot(out_162, out_162_stride, i + 3, sd_index_base, sd_index_xor);

      if (out162_0 && out162_1 && out162_2 && out162_3) {
        prg_sm4_ctr2_and_162_from_rks4_fixed_tweaks(
            rks, iv, tweak_2lambda, tweak_162, out2_0, out2_1, out2_2, out2_3, out162_0, out162_1,
            out162_2, out162_3);
      } else {
        uint8_t out162_tmp[4 * PRG_162_BYTES];
        prg_sm4_ctr2_and_162_from_rks4_fixed_tweaks(
            rks, iv, tweak_2lambda, tweak_162, out2_0, out2_1, out2_2, out2_3,
            out162_tmp + 0 * PRG_162_BYTES, out162_tmp + 1 * PRG_162_BYTES,
            out162_tmp + 2 * PRG_162_BYTES, out162_tmp + 3 * PRG_162_BYTES);
        if (out162_0) {
          memcpy(out162_0, out162_tmp + 0 * PRG_162_BYTES, PRG_162_BYTES);
        }
        if (out162_1) {
          memcpy(out162_1, out162_tmp + 1 * PRG_162_BYTES, PRG_162_BYTES);
        }
        if (out162_2) {
          memcpy(out162_2, out162_tmp + 2 * PRG_162_BYTES, PRG_162_BYTES);
        }
        if (out162_3) {
          memcpy(out162_3, out162_tmp + 3 * PRG_162_BYTES, PRG_162_BYTES);
        }
      }

      for (size_t lane = 0; lane < 4; ++lane) {
        const size_t idx = i + lane;
        const uint8_t* key_lane = keys + idx * key_stride;
        uint8_t* sd_lane =
            prg_sd_xor_map_slot(sd_out, sd_stride, idx, sd_index_base, sd_index_xor);
        if (sd_lane) {
          memcpy(sd_lane, key_lane, lambda_bytes);
        }
      }
#else
      for (size_t lane = 0; lane < 4; ++lane) {
        const size_t idx = i + lane;
        const uint8_t* key_lane = keys + idx * key_stride;
        uint8_t* out2_lane = out_2lambda + idx * out_2lambda_stride;
        uint8_t* sd_lane =
            prg_sd_xor_map_slot(sd_out, sd_stride, idx, sd_index_base, sd_index_xor);
        uint8_t* out162_lane =
            prg_sd_xor_map_slot(out_162, out_162_stride, idx, sd_index_base, sd_index_xor);
        if (sd_lane) {
          memcpy(sd_lane, key_lane, lambda_bytes);
        }
        prg_sm4_ctr2_from_rk(rks[lane], iv, tweak_2lambda, out2_lane);
        if (out162_lane) {
          prg_162_from_rk(rks[lane], iv, tweak_162, out162_lane);
        }
      }
#endif
      i += 4;
    }

    for (; i < count; ++i) {
      uint32_t rk[SM4_KEY_SCHEDULE];
      const uint8_t* key_i = keys + i * key_stride;
      uint8_t* out2_i = out_2lambda + i * out_2lambda_stride;
      uint8_t* sd_i =
          prg_sd_xor_map_slot(sd_out, sd_stride, i, sd_index_base, sd_index_xor);
      uint8_t* out162_i =
          prg_sd_xor_map_slot(out_162, out_162_stride, i, sd_index_base, sd_index_xor);
      yuchen_sm4_key_schedule_impl(key_i, rk);
      if (sd_i) {
        memcpy(sd_i, key_i, lambda_bytes);
      }
      prg_sm4_ctr2_from_rk(rk, iv, tweak_2lambda, out2_i);
      if (out162_i) {
        prg_162_from_rk(rk, iv, tweak_162, out162_i);
      }
    }
    return;
  }
#endif

  const int use_sm4_fastpath = (seclvl == 128) && prg_has_sm4_fastpath_runtime_cached();
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* key_i = keys + i * key_stride;
    uint8_t* out2_i = out_2lambda + i * out_2lambda_stride;
    uint8_t* sd_i =
        prg_sd_xor_map_slot(sd_out, sd_stride, i, sd_index_base, sd_index_xor);
    uint8_t* out162_i =
        prg_sd_xor_map_slot(out_162, out_162_stride, i, sd_index_base, sd_index_xor);
    if (sd_i) {
      memcpy(sd_i, key_i, lambda_bytes);
    }
    if (out162_i) {
      prg_2lambda_and_162_single_impl(key_i, iv, tweak_2lambda, tweak_162, out2_i, out162_i,
                                      seclvl, use_sm4_fastpath);
    } else {
      prg_2_lambda_single_impl(key_i, iv, tweak_2lambda, out2_i, seclvl, use_sm4_fastpath);
    }
  }
}
