/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_Duet-1024.h"

#include <immintrin.h>
#include <stdint.h>
#include <string.h>

enum {
  RATE_BITS = 768,
  CAPACITY_BITS = 1152,
  WORD_BITS = 64,

  WORDS_R = RATE_BITS / WORD_BITS,
  WORDS_C = CAPACITY_BITS / WORD_BITS,
  WORDS_PERM = WORDS_R + WORDS_C,

  R_BYTES = WORDS_R * 8,
  ROUNDS = 18,
  MAX_DIGEST_BITS = 8192
};

#if defined(__GNUC__) || defined(__clang__)
#define DUET_FORCE_INLINE static inline __attribute__((always_inline))
#define DUET_NOINLINE __attribute__((noinline))
#define DUET_UNUSED __attribute__((unused))
#else
#define DUET_FORCE_INLINE static inline
#define DUET_NOINLINE
#define DUET_UNUSED
#endif

static const uint64_t E[2 * ROUNDS] = {
    UINT64_C(0xb7e151628aed2a6a), UINT64_C(0xbf7158809cf4f3c7), UINT64_C(0x62e7160f38b4da56),
    UINT64_C(0xa784d9045190cfef), UINT64_C(0x324e7738926cfbe5), UINT64_C(0xf4bf8d8d8c31d763),
    UINT64_C(0xda06c80abb1185eb), UINT64_C(0x4f7c7b5757f59584), UINT64_C(0x90cfd47d7c19bb42),
    UINT64_C(0x158d9554f7b46bce), UINT64_C(0xd55c4d79fd5f24d6), UINT64_C(0x613c31c3839a2ddf),
    UINT64_C(0x8a9a276bcfbfa1c8), UINT64_C(0x77c56284dab79cd4), UINT64_C(0xc2b3293d20e9e5ea),
    UINT64_C(0xf02ac60acc93ed87), UINT64_C(0x4422a52ecb238fee), UINT64_C(0xe5ab6add835fd1a0),
    UINT64_C(0x753d0a8f78e537d2), UINT64_C(0xb95bb79d8dcaec64), UINT64_C(0x2c1e9f23b829b5c2),
    UINT64_C(0x780bf38737df8bb3), UINT64_C(0x00d01334a0d0bd86), UINT64_C(0x45cbfa73a6160ffe),
    UINT64_C(0x393c48cbbbca060f), UINT64_C(0x0ff8ec6d31beb5cc), UINT64_C(0xeed7f2f0bb088017),
    UINT64_C(0x163bc60df45a0ecb), UINT64_C(0x1bcd289b06cbbfea), UINT64_C(0x21ad08e1847f3f73),
    UINT64_C(0x78d56ced94640d6e), UINT64_C(0xf0d3d37be67008e1), UINT64_C(0x86d1bf275b9b241d),
    UINT64_C(0xeb64749a47dfdfb9), UINT64_C(0x6632c3eb061b6472), UINT64_C(0xbbf84c26144e49c2),
};

#define DUET_ROTL64(x, n)                                                     \
  (((x) << ((unsigned int)(n) & 63U)) |                                       \
   ((x) >> ((64U - ((unsigned int)(n) & 63U)) & 63U)))

#define DUET_L64(x, i)                                                       \
  (DUET_ROTL64((x), (i)) ^ DUET_ROTL64((x), (i) + 21U) ^                     \
   DUET_ROTL64((x), (i) + 43U))

static inline uint64_t load64_be(const unsigned char in[8]) {
  return ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48) |
         ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32) |
         ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16) |
         ((uint64_t)in[6] << 8) | (uint64_t)in[7];
}

static inline void store64_be(unsigned char out[8], uint64_t x) {
  out[0] = (unsigned char)(x >> 56);
  out[1] = (unsigned char)(x >> 48);
  out[2] = (unsigned char)(x >> 40);
  out[3] = (unsigned char)(x >> 32);
  out[4] = (unsigned char)(x >> 24);
  out[5] = (unsigned char)(x >> 16);
  out[6] = (unsigned char)(x >> 8);
  out[7] = (unsigned char)x;
}

static inline void permutation1920_round(uint64_t s[WORDS_PERM], uint64_t rc) {
  uint64_t a0 = s[0];
  uint64_t a1 = s[1];
  uint64_t a2 = s[2];
  uint64_t a3 = s[3];
  uint64_t a4 = s[4];
  uint64_t x0 = ((a0 ^ a1) & a2) ^ a3;
  uint64_t x1 = ((a1 ^ a2) & a3) ^ a4;
  uint64_t x2 = ((a2 ^ a3) & a4) ^ a0;
  uint64_t x3 = ((a3 ^ a4) & a0) ^ a1;
  uint64_t x4 = ((a0 ^ a4) & a1) ^ a2;

  a0 = s[5];
  a1 = s[6];
  a2 = s[7];
  a3 = s[8];
  a4 = s[9];
  uint64_t x5 = ((a0 ^ a1) & a2) ^ a3;
  uint64_t x6 = ((a1 ^ a2) & a3) ^ a4;
  uint64_t x7 = ((a2 ^ a3) & a4) ^ a0;
  uint64_t x8 = ((a3 ^ a4) & a0) ^ a1;
  uint64_t x9 = ((a0 ^ a4) & a1) ^ a2;

  a0 = s[10];
  a1 = s[11];
  a2 = s[12];
  a3 = s[13];
  a4 = s[14];
  uint64_t x10 = ((a0 ^ a1) & a2) ^ a3;
  uint64_t x11 = ((a1 ^ a2) & a3) ^ a4;
  uint64_t x12 = ((a2 ^ a3) & a4) ^ a0;
  uint64_t x13 = ((a3 ^ a4) & a0) ^ a1;
  uint64_t x14 = ((a0 ^ a4) & a1) ^ a2;

  a0 = s[15];
  a1 = s[16];
  a2 = s[17];
  a3 = s[18];
  a4 = s[19];
  uint64_t x15 = ((a0 ^ a1) & a2) ^ a3;
  uint64_t x16 = ((a1 ^ a2) & a3) ^ a4;
  uint64_t x17 = ((a2 ^ a3) & a4) ^ a0;
  uint64_t x18 = ((a3 ^ a4) & a0) ^ a1;
  uint64_t x19 = ((a0 ^ a4) & a1) ^ a2;

  a0 = s[20];
  a1 = s[21];
  a2 = s[22];
  a3 = s[23];
  a4 = s[24];
  uint64_t x20 = ((a0 ^ a1) & a2) ^ a3;
  uint64_t x21 = ((a1 ^ a2) & a3) ^ a4;
  uint64_t x22 = ((a2 ^ a3) & a4) ^ a0;
  uint64_t x23 = ((a3 ^ a4) & a0) ^ a1;
  uint64_t x24 = ((a0 ^ a4) & a1) ^ a2;

  a0 = s[25];
  a1 = s[26];
  a2 = s[27];
  a3 = s[28];
  a4 = s[29];
  uint64_t x25 = ((a0 ^ a1) & a2) ^ a3;
  uint64_t x26 = ((a1 ^ a2) & a3) ^ a4;
  uint64_t x27 = ((a2 ^ a3) & a4) ^ a0;
  uint64_t x28 = ((a3 ^ a4) & a0) ^ a1;
  uint64_t x29 = ((a0 ^ a4) & a1) ^ a2;

  uint64_t y0 =
      x0 ^ x3 ^ x10 ^ x13 ^ x14 ^ x15 ^ x16 ^ x18 ^ x21 ^ x26 ^ x28;
  uint64_t y1 =
      x1 ^ x4 ^ x11 ^ x14 ^ x15 ^ x16 ^ x17 ^ x19 ^ x22 ^ x27 ^ x29;
  uint64_t y2 =
      x2 ^ x5 ^ x12 ^ x15 ^ x16 ^ x17 ^ x18 ^ x20 ^ x23 ^ x28 ^ x0;
  uint64_t y3 =
      x3 ^ x6 ^ x13 ^ x16 ^ x17 ^ x18 ^ x19 ^ x21 ^ x24 ^ x29 ^ x1;
  uint64_t y4 =
      x4 ^ x7 ^ x14 ^ x17 ^ x18 ^ x19 ^ x20 ^ x22 ^ x25 ^ x0 ^ x2;
  uint64_t y5 =
      x5 ^ x8 ^ x15 ^ x18 ^ x19 ^ x20 ^ x21 ^ x23 ^ x26 ^ x1 ^ x3;
  uint64_t y6 =
      x6 ^ x9 ^ x16 ^ x19 ^ x20 ^ x21 ^ x22 ^ x24 ^ x27 ^ x2 ^ x4;
  uint64_t y7 =
      x7 ^ x10 ^ x17 ^ x20 ^ x21 ^ x22 ^ x23 ^ x25 ^ x28 ^ x3 ^ x5;
  uint64_t y8 =
      x8 ^ x11 ^ x18 ^ x21 ^ x22 ^ x23 ^ x24 ^ x26 ^ x29 ^ x4 ^ x6;
  uint64_t y9 =
      x9 ^ x12 ^ x19 ^ x22 ^ x23 ^ x24 ^ x25 ^ x27 ^ x0 ^ x5 ^ x7;
  uint64_t y10 =
      x10 ^ x13 ^ x20 ^ x23 ^ x24 ^ x25 ^ x26 ^ x28 ^ x1 ^ x6 ^ x8;
  uint64_t y11 =
      x11 ^ x14 ^ x21 ^ x24 ^ x25 ^ x26 ^ x27 ^ x29 ^ x2 ^ x7 ^ x9;
  uint64_t y12 =
      x12 ^ x15 ^ x22 ^ x25 ^ x26 ^ x27 ^ x28 ^ x0 ^ x3 ^ x8 ^ x10;
  uint64_t y13 =
      x13 ^ x16 ^ x23 ^ x26 ^ x27 ^ x28 ^ x29 ^ x1 ^ x4 ^ x9 ^ x11;
  uint64_t y14 =
      x14 ^ x17 ^ x24 ^ x27 ^ x28 ^ x29 ^ x0 ^ x2 ^ x5 ^ x10 ^ x12;
  uint64_t y15 =
      x15 ^ x18 ^ x25 ^ x28 ^ x29 ^ x0 ^ x1 ^ x3 ^ x6 ^ x11 ^ x13;
  uint64_t y16 =
      x16 ^ x19 ^ x26 ^ x29 ^ x0 ^ x1 ^ x2 ^ x4 ^ x7 ^ x12 ^ x14;
  uint64_t y17 =
      x17 ^ x20 ^ x27 ^ x0 ^ x1 ^ x2 ^ x3 ^ x5 ^ x8 ^ x13 ^ x15;
  uint64_t y18 =
      x18 ^ x21 ^ x28 ^ x1 ^ x2 ^ x3 ^ x4 ^ x6 ^ x9 ^ x14 ^ x16;
  uint64_t y19 =
      x19 ^ x22 ^ x29 ^ x2 ^ x3 ^ x4 ^ x5 ^ x7 ^ x10 ^ x15 ^ x17;
  uint64_t y20 =
      x20 ^ x23 ^ x0 ^ x3 ^ x4 ^ x5 ^ x6 ^ x8 ^ x11 ^ x16 ^ x18;
  uint64_t y21 =
      x21 ^ x24 ^ x1 ^ x4 ^ x5 ^ x6 ^ x7 ^ x9 ^ x12 ^ x17 ^ x19;
  uint64_t y22 =
      x22 ^ x25 ^ x2 ^ x5 ^ x6 ^ x7 ^ x8 ^ x10 ^ x13 ^ x18 ^ x20;
  uint64_t y23 =
      x23 ^ x26 ^ x3 ^ x6 ^ x7 ^ x8 ^ x9 ^ x11 ^ x14 ^ x19 ^ x21;
  uint64_t y24 =
      x24 ^ x27 ^ x4 ^ x7 ^ x8 ^ x9 ^ x10 ^ x12 ^ x15 ^ x20 ^ x22;
  uint64_t y25 =
      x25 ^ x28 ^ x5 ^ x8 ^ x9 ^ x10 ^ x11 ^ x13 ^ x16 ^ x21 ^ x23;
  uint64_t y26 =
      x26 ^ x29 ^ x6 ^ x9 ^ x10 ^ x11 ^ x12 ^ x14 ^ x17 ^ x22 ^ x24;
  uint64_t y27 =
      x27 ^ x0 ^ x7 ^ x10 ^ x11 ^ x12 ^ x13 ^ x15 ^ x18 ^ x23 ^ x25;
  uint64_t y28 =
      x28 ^ x1 ^ x8 ^ x11 ^ x12 ^ x13 ^ x14 ^ x16 ^ x19 ^ x24 ^ x26;
  uint64_t y29 =
      x29 ^ x2 ^ x9 ^ x12 ^ x13 ^ x14 ^ x15 ^ x17 ^ x20 ^ x25 ^ x27;

  s[0] = DUET_L64((y0 ^ rc), 0);
  s[1] = DUET_L64(y1, 1);
  s[2] = DUET_L64(y2, 2);
  s[3] = DUET_L64(y3, 3);
  s[4] = DUET_L64(y4, 4);
  s[5] = DUET_L64(y5, 5);
  s[6] = DUET_L64(y6, 6);
  s[7] = DUET_L64(y7, 7);
  s[8] = DUET_L64(y8, 8);
  s[9] = DUET_L64(y9, 9);
  s[10] = DUET_L64(y10, 10);
  s[11] = DUET_L64(y11, 11);
  s[12] = DUET_L64(y12, 12);
  s[13] = DUET_L64(y13, 13);
  s[14] = DUET_L64(y14, 14);
  s[15] = DUET_L64(y15, 15);
  s[16] = DUET_L64(y16, 16);
  s[17] = DUET_L64(y17, 17);
  s[18] = DUET_L64(y18, 18);
  s[19] = DUET_L64(y19, 19);
  s[20] = DUET_L64(y20, 20);
  s[21] = DUET_L64(y21, 21);
  s[22] = DUET_L64(y22, 22);
  s[23] = DUET_L64(y23, 23);
  s[24] = DUET_L64(y24, 24);
  s[25] = DUET_L64(y25, 25);
  s[26] = DUET_L64(y26, 26);
  s[27] = DUET_L64(y27, 27);
  s[28] = DUET_L64(y28, 28);
  s[29] = DUET_L64(y29, 29);
}

static inline DUET_UNUSED void permutation1920(uint64_t s[WORDS_PERM],
                                               const uint64_t rc[ROUNDS]) {
  unsigned int round;

  for (round = 0; round < ROUNDS; ++round) {
    permutation1920_round(s, rc[round]);
  }
}

typedef struct {
  __m256i lo;
  __m256i hi;
} duet_avx2_6_t;

static const uint64_t DUET_AVX2_6_L64_COUNTS[3][5][2][4] = {
    {
        {{0, 5, 10, 15}, {20, 25, 0, 0}},
        {{1, 6, 11, 16}, {21, 26, 0, 0}},
        {{2, 7, 12, 17}, {22, 27, 0, 0}},
        {{3, 8, 13, 18}, {23, 28, 0, 0}},
        {{4, 9, 14, 19}, {24, 29, 0, 0}},
    },
    {
        {{21, 26, 31, 36}, {41, 46, 21, 21}},
        {{22, 27, 32, 37}, {42, 47, 21, 21}},
        {{23, 28, 33, 38}, {43, 48, 21, 21}},
        {{24, 29, 34, 39}, {44, 49, 21, 21}},
        {{25, 30, 35, 40}, {45, 50, 21, 21}},
    },
    {
        {{43, 48, 53, 58}, {63, 4, 43, 43}},
        {{44, 49, 54, 59}, {0, 5, 43, 43}},
        {{45, 50, 55, 60}, {1, 6, 43, 43}},
        {{46, 51, 56, 61}, {2, 7, 43, 43}},
        {{47, 52, 57, 62}, {3, 8, 43, 43}},
    },
};

DUET_FORCE_INLINE __m256i duet_avx2_rolv64(__m256i x, __m256i n) {
  const __m256i mask = _mm256_set1_epi64x(63);
  const __m256i zero = _mm256_setzero_si256();
  __m256i rn = _mm256_and_si256(_mm256_sub_epi64(zero, n), mask);
  return _mm256_or_si256(_mm256_sllv_epi64(x, n), _mm256_srlv_epi64(x, rn));
}

DUET_FORCE_INLINE __m256i duet_avx2_l64_part(__m256i x, unsigned int row,
                                              unsigned int part) {
  __m256i n0 = _mm256_loadu_si256(
      (const __m256i *)(const void *)DUET_AVX2_6_L64_COUNTS[0][row][part]);
  __m256i n1 = _mm256_loadu_si256(
      (const __m256i *)(const void *)DUET_AVX2_6_L64_COUNTS[1][row][part]);
  __m256i n2 = _mm256_loadu_si256(
      (const __m256i *)(const void *)DUET_AVX2_6_L64_COUNTS[2][row][part]);
  return _mm256_xor_si256(_mm256_xor_si256(duet_avx2_rolv64(x, n0),
                                           duet_avx2_rolv64(x, n1)),
                          duet_avx2_rolv64(x, n2));
}

DUET_FORCE_INLINE duet_avx2_6_t duet_avx2_6_xor(duet_avx2_6_t a,
                                                 duet_avx2_6_t b) {
  duet_avx2_6_t r;
  r.lo = _mm256_xor_si256(a.lo, b.lo);
  r.hi = _mm256_xor_si256(a.hi, b.hi);
  return r;
}

DUET_FORCE_INLINE duet_avx2_6_t duet_avx2_6_and(duet_avx2_6_t a,
                                                 duet_avx2_6_t b) {
  duet_avx2_6_t r;
  r.lo = _mm256_and_si256(a.lo, b.lo);
  r.hi = _mm256_and_si256(a.hi, b.hi);
  return r;
}

DUET_FORCE_INLINE duet_avx2_6_t duet_avx2_6_not(duet_avx2_6_t a) {
  const __m256i ones = _mm256_cmpeq_epi64(a.lo, a.lo);
  duet_avx2_6_t r;
  r.lo = _mm256_xor_si256(a.lo, ones);
  r.hi = _mm256_xor_si256(a.hi, ones);
  return r;
}

DUET_FORCE_INLINE void duet_avx2_6_sbox5(
    duet_avx2_6_t r0, duet_avx2_6_t r1, duet_avx2_6_t r2,
    duet_avx2_6_t r3, duet_avx2_6_t r4, duet_avx2_6_t x[5]) {
  duet_avx2_6_t t0 = duet_avx2_6_and(r3, r4);
  duet_avx2_6_t t1 = duet_avx2_6_and(r2, r4);
  duet_avx2_6_t t2 = duet_avx2_6_and(r2, r3);
  duet_avx2_6_t t3 = duet_avx2_6_and(r1, r4);
  duet_avx2_6_t t4 = duet_avx2_6_and(r1, r2);
  duet_avx2_6_t t5 = duet_avx2_6_and(r0, r1);
  duet_avx2_6_t t6 = duet_avx2_6_xor(r4, r2);
  duet_avx2_6_t t7 = duet_avx2_6_xor(r0, t0);
  duet_avx2_6_t t8 = duet_avx2_6_xor(t1, t2);
  duet_avx2_6_t t9 = duet_avx2_6_xor(t3, t4);
  duet_avx2_6_t t10 = duet_avx2_6_xor(t5, t6);
  duet_avx2_6_t t11 = duet_avx2_6_xor(t7, t8);
  duet_avx2_6_t t12 = duet_avx2_6_xor(t9, t10);
  duet_avx2_6_t t13 = duet_avx2_6_xor(t11, t12);
  duet_avx2_6_t t15 = duet_avx2_6_and(r1, r3);
  duet_avx2_6_t t16 = duet_avx2_6_and(r0, r2);
  duet_avx2_6_t t17 = duet_avx2_6_xor(r3, r2);
  duet_avx2_6_t t18 = duet_avx2_6_xor(r0, t1);
  duet_avx2_6_t t19 = duet_avx2_6_xor(t2, t15);
  duet_avx2_6_t t20 = duet_avx2_6_xor(t4, t16);
  duet_avx2_6_t t21 = duet_avx2_6_xor(t17, t18);
  duet_avx2_6_t t22 = duet_avx2_6_xor(t19, t20);
  duet_avx2_6_t t23 = duet_avx2_6_xor(t21, t22);
  duet_avx2_6_t t25 = duet_avx2_6_and(r0, r4);
  duet_avx2_6_t t26 = duet_avx2_6_xor(r2, r0);
  duet_avx2_6_t t27 = duet_avx2_6_xor(t0, t1);
  duet_avx2_6_t t28 = duet_avx2_6_xor(t3, t15);
  duet_avx2_6_t t29 = duet_avx2_6_xor(t4, t25);
  duet_avx2_6_t t30 = duet_avx2_6_xor(t16, t5);
  duet_avx2_6_t t31 = duet_avx2_6_xor(t26, t27);
  duet_avx2_6_t t32 = duet_avx2_6_xor(t28, t29);
  duet_avx2_6_t t33 = duet_avx2_6_xor(t30, t31);
  duet_avx2_6_t t35 = duet_avx2_6_and(r0, r3);
  duet_avx2_6_t t36 = duet_avx2_6_xor(r4, r3);
  duet_avx2_6_t t37 = duet_avx2_6_xor(r1, r0);
  duet_avx2_6_t t38 = duet_avx2_6_xor(t4, t35);
  duet_avx2_6_t t39 = duet_avx2_6_xor(t16, t5);
  duet_avx2_6_t t40 = duet_avx2_6_xor(t36, t37);
  duet_avx2_6_t t41 = duet_avx2_6_xor(t38, t39);
  duet_avx2_6_t t42 = duet_avx2_6_xor(t40, t41);
  duet_avx2_6_t t44 = duet_avx2_6_xor(r4, r2);
  duet_avx2_6_t t45 = duet_avx2_6_xor(r1, r0);
  duet_avx2_6_t t46 = duet_avx2_6_xor(t3, t15);
  duet_avx2_6_t t47 = duet_avx2_6_xor(t4, t25);
  duet_avx2_6_t t48 = duet_avx2_6_xor(t5, t44);
  duet_avx2_6_t t49 = duet_avx2_6_xor(t45, t46);
  duet_avx2_6_t t50 = duet_avx2_6_xor(t47, t48);
  x[0] = duet_avx2_6_not(t13);
  x[1] = duet_avx2_6_not(t23);
  x[2] = duet_avx2_6_xor(t32, t33);
  x[3] = duet_avx2_6_not(t42);
  x[4] = duet_avx2_6_xor(t49, t50);
}

DUET_FORCE_INLINE duet_avx2_6_t duet_avx2_6_rot(duet_avx2_6_t x,
                                                 unsigned int k) {
  duet_avx2_6_t r;
  const __m256i zero = _mm256_setzero_si256();
  __m256i t0, t1;

  switch (k) {
  case 0:
    return x;
  case 1:
    t0 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(0, 3, 2, 1));
    t1 = _mm256_permute4x64_epi64(x.hi, _MM_SHUFFLE(0, 0, 0, 0));
    r.lo = _mm256_blend_epi32(t0, t1, 0xc0);
    t0 = _mm256_permute4x64_epi64(x.hi, _MM_SHUFFLE(3, 2, 0, 1));
    t1 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(0, 0, 0, 0));
    r.hi = _mm256_blend_epi32(t0, t1, 0x0c);
    return r;
  case 2:
    t0 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(1, 0, 3, 2));
    t1 = _mm256_permute4x64_epi64(x.hi, _MM_SHUFFLE(1, 0, 1, 0));
    r.lo = _mm256_blend_epi32(t0, t1, 0xf0);
    r.hi = _mm256_blend_epi32(x.lo, zero, 0xf0);
    return r;
  case 3:
    t0 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(0, 1, 0, 3));
    t1 = _mm256_permute4x64_epi64(x.hi, _MM_SHUFFLE(0, 1, 0, 0));
    r.lo = _mm256_blend_epi32(t0, t1, 0x3c);
    t0 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(0, 0, 2, 1));
    r.hi = _mm256_blend_epi32(t0, zero, 0xf0);
    return r;
  case 4:
    t0 = _mm256_permute4x64_epi64(x.hi, _MM_SHUFFLE(1, 0, 1, 0));
    t1 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(1, 0, 1, 0));
    r.lo = _mm256_blend_epi32(t0, t1, 0xf0);
    t0 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(0, 0, 3, 2));
    r.hi = _mm256_blend_epi32(t0, zero, 0xf0);
    return r;
  default:
    t0 = _mm256_permute4x64_epi64(x.hi, _MM_SHUFFLE(1, 1, 1, 1));
    t1 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(2, 1, 0, 0));
    r.lo = _mm256_blend_epi32(t0, t1, 0xfc);
    t0 = _mm256_permute4x64_epi64(x.lo, _MM_SHUFFLE(3, 3, 3, 3));
    r.hi = _mm256_blend_epi32(zero, t0, 0x03);
    t1 = _mm256_permute4x64_epi64(x.hi, _MM_SHUFFLE(0, 0, 0, 0));
    r.hi = _mm256_blend_epi32(r.hi, t1, 0x0c);
    return r;
  }
}

#define DUET_AVX2_6_L30_TERM(row, off)                                       \
  duet_avx2_6_rot(x[((row) + (off)) % 5], (((row) + (off)) / 5) % 6)

#define DUET_AVX2_6_BUILD_L30(row, dst)                                      \
  do {                                                                       \
    (dst) = DUET_AVX2_6_L30_TERM((row), 0);                                  \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 3));          \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 10));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 13));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 14));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 15));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 16));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 18));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 21));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 26));         \
    (dst) = duet_avx2_6_xor((dst), DUET_AVX2_6_L30_TERM((row), 28));         \
  } while (0)

DUET_FORCE_INLINE duet_avx2_6_t duet_avx2_6_l64(duet_avx2_6_t y,
                                                 unsigned int row) {
  duet_avx2_6_t r;
  r.lo = duet_avx2_l64_part(y.lo, row, 0);
  r.hi = duet_avx2_l64_part(y.hi, row, 1);
  return r;
}

DUET_FORCE_INLINE duet_avx2_6_t duet_avx2_6_load_row(const uint64_t *s,
                                                      unsigned int row) {
  duet_avx2_6_t r;
  r.lo = _mm256_set_epi64x((long long)s[15 + row], (long long)s[10 + row],
                           (long long)s[5 + row], (long long)s[row]);
  r.hi = _mm256_set_epi64x(0, 0, (long long)s[25 + row],
                           (long long)s[20 + row]);
  return r;
}

DUET_FORCE_INLINE void duet_avx2_6_store_row(uint64_t *s, unsigned int row,
                                              duet_avx2_6_t r) {
  uint64_t lo[4];
  uint64_t hi[4];
  _mm256_storeu_si256((__m256i *)(void *)lo, r.lo);
  _mm256_storeu_si256((__m256i *)(void *)hi, r.hi);
  s[row] = lo[0];
  s[5 + row] = lo[1];
  s[10 + row] = lo[2];
  s[15 + row] = lo[3];
  s[20 + row] = hi[0];
  s[25 + row] = hi[1];
}

DUET_FORCE_INLINE void permutation1920_avx2_round(
    duet_avx2_6_t *r0, duet_avx2_6_t *r1, duet_avx2_6_t *r2,
    duet_avx2_6_t *r3, duet_avx2_6_t *r4, uint64_t rc) {
  duet_avx2_6_t x[5];
  duet_avx2_6_t y0, y1, y2, y3, y4;

  duet_avx2_6_sbox5(*r0, *r1, *r2, *r3, *r4, x);

  DUET_AVX2_6_BUILD_L30(0, y0);
  DUET_AVX2_6_BUILD_L30(1, y1);
  DUET_AVX2_6_BUILD_L30(2, y2);
  DUET_AVX2_6_BUILD_L30(3, y3);
  DUET_AVX2_6_BUILD_L30(4, y4);

  y0.lo = _mm256_xor_si256(y0.lo, _mm256_set_epi64x(0, 0, 0, (long long)rc));

  *r0 = duet_avx2_6_l64(y0, 0);
  *r1 = duet_avx2_6_l64(y1, 1);
  *r2 = duet_avx2_6_l64(y2, 2);
  *r3 = duet_avx2_6_l64(y3, 3);
  *r4 = duet_avx2_6_l64(y4, 4);
}

static DUET_NOINLINE void permutation1920_avx2(uint64_t s[WORDS_PERM],
                                               const uint64_t rc[ROUNDS]) {
  duet_avx2_6_t r0 = duet_avx2_6_load_row(s, 0);
  duet_avx2_6_t r1 = duet_avx2_6_load_row(s, 1);
  duet_avx2_6_t r2 = duet_avx2_6_load_row(s, 2);
  duet_avx2_6_t r3 = duet_avx2_6_load_row(s, 3);
  duet_avx2_6_t r4 = duet_avx2_6_load_row(s, 4);
  unsigned int round;

  for (round = 0; round < ROUNDS; ++round) {
    permutation1920_avx2_round(&r0, &r1, &r2, &r3, &r4, rc[round]);
  }

  duet_avx2_6_store_row(s, 0, r0);
  duet_avx2_6_store_row(s, 1, r1);
  duet_avx2_6_store_row(s, 2, r2);
  duet_avx2_6_store_row(s, 3, r3);
  duet_avx2_6_store_row(s, 4, r4);
}

#define permutation1920 permutation1920_avx2

static inline void load_full_block(uint64_t block[WORDS_R],
                                   const unsigned char *msg) {
  unsigned int i;

  for (i = 0; i < WORDS_R; ++i) {
    block[i] = load64_be(msg + 8U * i);
  }
}

static void pad_message_block(uint64_t block[WORDS_R],
                              const unsigned char *msg,
                              unsigned long long msg_len_bits) {
  unsigned long long full_bytes = msg_len_bits / 8U;
  unsigned int rem_bits = (unsigned int)(msg_len_bits & 7U);
  unsigned int word = 0;
  unsigned int i;

  memset(block, 0, sizeof(uint64_t) * WORDS_R);

  while (full_bytes >= 8U) {
    block[word++] = load64_be(msg);
    msg += 8;
    full_bytes -= 8U;
  }

  if (rem_bits == 0U && full_bytes == 0U) {
    block[word] = UINT64_C(0x8000000000000000);
    return;
  }

  for (i = 0; i < (unsigned int)full_bytes; ++i) {
    block[word] |= (uint64_t)msg[i] << (56U - 8U * i);
  }

  if (rem_bits == 0U) {
    block[word] |= UINT64_C(0x80) << (56U - 8U * (unsigned int)full_bytes);
  } else {
    unsigned char last =
        (unsigned char)(msg[full_bytes] & (0xffU << (8U - rem_bits)));
    last |= (unsigned char)(0x80U >> rem_bits);
    block[word] |= (uint64_t)last << (56U - 8U * (unsigned int)full_bytes);
  }
}

static inline void absorb_block(uint64_t A[WORDS_R], uint64_t B[WORDS_R],
                                uint64_t C[WORDS_C],
                                const uint64_t Mi[WORDS_R]) {
  uint64_t perm[WORDS_PERM];
  unsigned int i;

  for (i = 0; i < WORDS_R; ++i) {
    perm[i] = A[i] ^ Mi[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    perm[WORDS_R + i] = C[i];
  }
  permutation1920(perm, E);
  for (i = 0; i < WORDS_R; ++i) {
    A[i] = perm[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    perm[WORDS_R + i] ^= C[i];
    C[i] = perm[WORDS_R + i];
  }

  perm[0] = B[0] ^ Mi[8];
  perm[1] = B[1] ^ Mi[2];
  perm[2] = B[2] ^ Mi[11];
  perm[3] = B[3] ^ Mi[0];
  perm[4] = B[4] ^ Mi[5];
  perm[5] = B[5] ^ Mi[9];
  perm[6] = B[6] ^ Mi[3];
  perm[7] = B[7] ^ Mi[10];
  perm[8] = B[8] ^ Mi[1];
  perm[9] = B[9] ^ Mi[7];
  perm[10] = B[10] ^ Mi[4];
  perm[11] = B[11] ^ Mi[6];
  permutation1920(perm, E + ROUNDS);
  for (i = 0; i < WORDS_R; ++i) {
    B[i] = perm[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    C[i] ^= perm[WORDS_R + i];
  }
}

static inline void apply_empty_f1(uint64_t A[WORDS_R], uint64_t C[WORDS_C]) {
  uint64_t perm[WORDS_PERM];
  unsigned int i;

  for (i = 0; i < WORDS_R; ++i) {
    perm[i] = A[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    perm[WORDS_R + i] = C[i];
  }
  permutation1920(perm, E);
  for (i = 0; i < WORDS_R; ++i) {
    A[i] = perm[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    C[i] ^= perm[WORDS_R + i];
  }
}

static inline void apply_empty_f2(uint64_t B[WORDS_R], uint64_t C[WORDS_C]) {
  uint64_t perm[WORDS_PERM];
  unsigned int i;

  for (i = 0; i < WORDS_R; ++i) {
    perm[i] = B[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    perm[WORDS_R + i] = C[i];
  }
  permutation1920(perm, E + ROUNDS);
  for (i = 0; i < WORDS_R; ++i) {
    B[i] = perm[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    C[i] ^= perm[WORDS_R + i];
  }
}

static void squeeze_copy_capacity(unsigned char *digest, int *out_offset,
                                  int *remaining_digest_bits,
                                  const uint64_t C[WORDS_C]) {
  int take_bits = *remaining_digest_bits > CAPACITY_BITS
                      ? CAPACITY_BITS
                      : *remaining_digest_bits;
  int take_bytes = (take_bits + 7) / 8;
  int full_words = take_bytes / 8;
  int rem_bytes = take_bytes & 7;
  int i;

  for (i = 0; i < full_words; ++i) {
    store64_be(digest + *out_offset + 8 * i, C[i]);
  }
  if (rem_bytes != 0) {
    unsigned char tmp[8];

    store64_be(tmp, C[full_words]);
    memcpy(digest + *out_offset + 8 * full_words, tmp, (size_t)rem_bytes);
  }

  *out_offset += take_bytes;
  *remaining_digest_bits -= take_bits;
}

static inline void clear_unused_digest_bits(unsigned char *digest,
                                            int digest_len_bits) {
  unsigned int rem_bits = (unsigned int)digest_len_bits & 7U;

  if (rem_bits != 0U) {
    digest[digest_len_bits / 8] &= (unsigned char)(0xffU << (8U - rem_bits));
  }
}

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
  uint64_t A[WORDS_R] = {0};
  uint64_t B[WORDS_R] = {0};
  uint64_t C[WORDS_C] = {0};
  uint64_t Mi[WORDS_R];
  unsigned long long full_blocks;
  unsigned long long rem_bits;
  const unsigned char *p;
  unsigned long long blk;
  int remaining_digest_bits;
  int out_offset;

  if (digest_len_bits <= 0 || digest_len_bits > MAX_DIGEST_BITS) {
    return -1;
  }
  if (digest == NULL) {
    return -2;
  }
  if (msg == NULL && msg_len_bits != 0) {
    return -3;
  }

  full_blocks = msg_len_bits / RATE_BITS;
  rem_bits = msg_len_bits % RATE_BITS;
  p = msg;

  for (blk = 0; blk < full_blocks; ++blk) {
    load_full_block(Mi, p);
    absorb_block(A, B, C, Mi);
    p += R_BYTES;
  }

  pad_message_block(Mi, p, rem_bits);
  absorb_block(A, B, C, Mi);

  remaining_digest_bits = digest_len_bits;
  out_offset = 0;
  squeeze_copy_capacity(digest, &out_offset, &remaining_digest_bits, C);

  while (remaining_digest_bits > 0) {
    apply_empty_f1(A, C);
    squeeze_copy_capacity(digest, &out_offset, &remaining_digest_bits, C);
    if (remaining_digest_bits == 0) {
      break;
    }

    apply_empty_f2(B, C);
    squeeze_copy_capacity(digest, &out_offset, &remaining_digest_bits, C);
  }

  clear_unused_digest_bits(digest, digest_len_bits);

  return 0;
}
