/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_Duet-512.h"

#include <immintrin.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum {
  RATE_BITS = 384,
  CAPACITY_BITS = 576,
  WORD_BITS = 64,

  WORDS_R = RATE_BITS / WORD_BITS,
  WORDS_C = CAPACITY_BITS / WORD_BITS,
  WORDS_PERM = WORDS_R + WORDS_C,

  R_BYTES = WORDS_R * 8,
  C_BYTES = WORDS_C * 8,
  ROUNDS = 12,
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

static const uint64_t PI[2 * ROUNDS] = {
    UINT64_C(0x243f6a8885a308d3), UINT64_C(0x13198a2e03707344),
    UINT64_C(0xa4093822299f31d0), UINT64_C(0x082efa98ec4e6c89),
    UINT64_C(0x452821e638d01377), UINT64_C(0xbe5466cf34e90c6c),
    UINT64_C(0xc0ac29b7c97c50dd), UINT64_C(0x3f84d5b5b5470917),
    UINT64_C(0x9216d5d98979fb1b), UINT64_C(0xd1310ba698dfb5ac),
    UINT64_C(0x2ffd72dbd01adfb7), UINT64_C(0xb8e1afed6a267e96),
    UINT64_C(0xba7c9045f12c7f99), UINT64_C(0x24a19947b3916cf7),
    UINT64_C(0x0801f2e2858efc16), UINT64_C(0x636920d871574e69),
    UINT64_C(0xa458fea3f4933d7e), UINT64_C(0x0d95748f728eb658),
    UINT64_C(0x718bcd5882154aee), UINT64_C(0x7b54a41dc25a59b5),
    UINT64_C(0x9c30d5392af26013), UINT64_C(0xc5d1b023286085f0),
    UINT64_C(0xca417918b8db38ef), UINT64_C(0x8e79dcb0603a180e),
};

#define DUET_ROTL64(x, n)                                                     \
  (((x) << ((unsigned int)(n) & 63U)) |                                       \
   ((x) >> ((64U - ((unsigned int)(n) & 63U)) & 63U)))

#define DUET_L64(x, i)                                                        \
  (DUET_ROTL64((x), (i)) ^ DUET_ROTL64((x), (i) + 21U) ^                     \
   DUET_ROTL64((x), (i) + 43U))

/* memcpy keeps unaligned access and strict aliasing fully defined. */
DUET_FORCE_INLINE uint64_t load64_be(const unsigned char *in) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) &&    \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  uint64_t x;
  memcpy(&x, in, sizeof(x));
  return __builtin_bswap64(x);
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  uint64_t x;
  memcpy(&x, in, sizeof(x));
  return x;
#else
  return ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48) |
         ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32) |
         ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16) |
         ((uint64_t)in[6] << 8) | (uint64_t)in[7];
#endif
}

DUET_FORCE_INLINE void store64_be(unsigned char *out, uint64_t x) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) &&    \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  x = __builtin_bswap64(x);
  memcpy(out, &x, sizeof(x));
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  memcpy(out, &x, sizeof(x));
#else
  out[0] = (unsigned char)(x >> 56);
  out[1] = (unsigned char)(x >> 48);
  out[2] = (unsigned char)(x >> 40);
  out[3] = (unsigned char)(x >> 32);
  out[4] = (unsigned char)(x >> 24);
  out[5] = (unsigned char)(x >> 16);
  out[6] = (unsigned char)(x >> 8);
  out[7] = (unsigned char)x;
#endif
}

/*
 * One round.  The 15x15 linear diffusion layer is implemented by a
 * scheduled 50-XOR straight-line circuit (the direct equations need 90 XORs).
 * Only eight intermediate values are live at once, limiting register pressure.
 */
DUET_FORCE_INLINE void permutation960_round(uint64_t s[restrict WORDS_PERM],
                                             uint64_t rc) {
  uint64_t x[WORDS_PERM];
  uint64_t a0, a1, a2, a3, a4;
  uint64_t v0, v1, v2, v3, v4, v5, v6, v7;

#define DUET_SBOX5(base)                                                      \
  do {                                                                        \
    a0 = s[(base) + 0];                                                       \
    a1 = s[(base) + 1];                                                       \
    a2 = s[(base) + 2];                                                       \
    a3 = s[(base) + 3];                                                       \
    a4 = s[(base) + 4];                                                       \
    x[(base) + 0] = ((a0 ^ a1) & a2) ^ a3;                                  \
    x[(base) + 1] = ((a1 ^ a2) & a3) ^ a4;                                  \
    x[(base) + 2] = ((a2 ^ a3) & a4) ^ a0;                                  \
    x[(base) + 3] = ((a3 ^ a4) & a0) ^ a1;                                  \
    x[(base) + 4] = ((a0 ^ a4) & a1) ^ a2;                                  \
  } while (0)

  DUET_SBOX5(0);
  DUET_SBOX5(5);
  DUET_SBOX5(10);
#undef DUET_SBOX5

  /* 50-XOR linear circuit, topologically scheduled for <= 8 live temps. */
  v0 = x[9] ^ x[14];
  v0 = x[4] ^ v0;
  v1 = x[5] ^ x[8];
  v2 = x[3] ^ x[13];
  v2 = v2 ^ v1;
  v1 = x[6] ^ v1;
  v2 = x[2] ^ v2;
  v3 = x[1] ^ v0;
  s[1] = DUET_L64(v3 ^ v1, 1);
  v0 = x[3] ^ v0;
  v1 = x[13] ^ v3;
  v3 = x[2] ^ x[9];
  v4 = x[1] ^ x[11];
  v5 = x[6] ^ v4;
  v6 = x[8] ^ v5;
  v5 = x[10] ^ v5;
  s[6] = DUET_L64(v1 ^ v5, 6);
  s[13] = DUET_L64(v6 ^ v2, 13);
  v5 = x[0] ^ x[12];
  v7 = x[7] ^ v6;
  v6 = v6 ^ v5;
  s[8] = DUET_L64(x[13] ^ v6, 8);
  v6 = x[10] ^ v7;
  s[3] = DUET_L64(x[3] ^ v6, 3);
  v5 = v2 ^ v5;
  v6 = x[0] ^ v0;
  s[11] = DUET_L64(v4 ^ v6, 11);
  v4 = x[0] ^ x[10];
  v6 = x[5] ^ v4;
  v7 = v3 ^ v5;
  s[5] = DUET_L64(v4 ^ v7, 5);
  v3 = x[6] ^ v3;
  v4 = x[7] ^ x[12];
  v7 = x[2] ^ x[4];
  v7 = v4 ^ v7;
  s[0] = DUET_L64((v7 ^ v5) ^ rc, 0);
  s[9] = DUET_L64(v7 ^ v1, 9);
  v1 = v4 ^ v3;
  s[14] = DUET_L64(v0 ^ v1, 14);
  v0 = x[11] ^ v4;
  v1 = x[14] ^ v6;
  s[10] = DUET_L64(v1 ^ v2, 10);
  s[7] = DUET_L64(v1 ^ v0, 7);
  v0 = x[7] ^ v6;
  s[2] = DUET_L64(v0 ^ v3, 2);
  v1 = x[1] ^ v7;
  s[12] = DUET_L64(v0 ^ v1, 12);
  v0 = x[8] ^ v7;
  v1 = x[9] ^ x[11];
  s[4] = DUET_L64(v0 ^ v1, 4);
}

/* Keep the large permutation out of CryptHash/absorb_block to cap code size. */
static DUET_NOINLINE DUET_UNUSED void permutation960(
    uint64_t s[restrict WORDS_PERM], const uint64_t rc[restrict ROUNDS]) {
  unsigned int round;
  for (round = 0; round < ROUNDS; ++round) {
    permutation960_round(s, rc[round]);
  }
}

static const uint64_t DUET_AVX2_L64_COUNTS[3][5][4] = {
    {
        {0, 5, 10, 0}, {1, 6, 11, 0}, {2, 7, 12, 0},
        {3, 8, 13, 0}, {4, 9, 14, 0},
    },
    {
        {21, 26, 31, 21}, {22, 27, 32, 21}, {23, 28, 33, 21},
        {24, 29, 34, 21}, {25, 30, 35, 21},
    },
    {
        {43, 48, 53, 43}, {44, 49, 54, 43}, {45, 50, 55, 43},
        {46, 51, 56, 43}, {47, 52, 57, 43},
    },
};

DUET_FORCE_INLINE __m256i duet_avx2_xor3(__m256i a, __m256i b, __m256i c) {
  return _mm256_ternarylogic_epi64(a, b, c, 0x96);
}

DUET_FORCE_INLINE __m256i duet_avx2_rolv64(__m256i x, __m256i n) {
  return _mm256_rolv_epi64(x, n);
}

DUET_FORCE_INLINE __m256i duet_avx2_l64_row(__m256i x, unsigned int row) {
  __m256i n0 = _mm256_loadu_si256(
      (const __m256i *)(const void *)DUET_AVX2_L64_COUNTS[0][row]);
  __m256i n1 = _mm256_loadu_si256(
      (const __m256i *)(const void *)DUET_AVX2_L64_COUNTS[1][row]);
  __m256i n2 = _mm256_loadu_si256(
      (const __m256i *)(const void *)DUET_AVX2_L64_COUNTS[2][row]);
  return duet_avx2_xor3(duet_avx2_rolv64(x, n0), duet_avx2_rolv64(x, n1),
                        duet_avx2_rolv64(x, n2));
}

DUET_FORCE_INLINE __m256i duet_avx2_not(__m256i a) {
  const __m256i ones = _mm256_cmpeq_epi64(a, a);
  return _mm256_xor_si256(a, ones);
}

DUET_FORCE_INLINE void duet_avx2_sbox5(__m256i r0, __m256i r1, __m256i r2,
                                       __m256i r3, __m256i r4,
                                       __m256i x[5]) {
  __m256i t0 = _mm256_and_si256(r3, r4);
  __m256i t1 = _mm256_and_si256(r2, r4);
  __m256i t2 = _mm256_and_si256(r2, r3);
  __m256i t3 = _mm256_and_si256(r1, r4);
  __m256i t4 = _mm256_and_si256(r1, r2);
  __m256i t5 = _mm256_and_si256(r0, r1);
  __m256i t6 = _mm256_xor_si256(r4, r2);
  __m256i t7 = _mm256_xor_si256(r0, t0);
  __m256i t8 = _mm256_xor_si256(t1, t2);
  __m256i t9 = _mm256_xor_si256(t3, t4);
  __m256i t10 = _mm256_xor_si256(t5, t6);
  __m256i t11 = _mm256_xor_si256(t7, t8);
  __m256i t12 = _mm256_xor_si256(t9, t10);
  __m256i t13 = _mm256_xor_si256(t11, t12);
  __m256i t15 = _mm256_and_si256(r1, r3);
  __m256i t16 = _mm256_and_si256(r0, r2);
  __m256i t17 = _mm256_xor_si256(r3, r2);
  __m256i t18 = _mm256_xor_si256(r0, t1);
  __m256i t19 = _mm256_xor_si256(t2, t15);
  __m256i t20 = _mm256_xor_si256(t4, t16);
  __m256i t21 = _mm256_xor_si256(t17, t18);
  __m256i t22 = _mm256_xor_si256(t19, t20);
  __m256i t23 = _mm256_xor_si256(t21, t22);
  __m256i t25 = _mm256_and_si256(r0, r4);
  __m256i t26 = _mm256_xor_si256(r2, r0);
  __m256i t27 = _mm256_xor_si256(t0, t1);
  __m256i t28 = _mm256_xor_si256(t3, t15);
  __m256i t29 = _mm256_xor_si256(t4, t25);
  __m256i t30 = _mm256_xor_si256(t16, t5);
  __m256i t31 = _mm256_xor_si256(t26, t27);
  __m256i t32 = _mm256_xor_si256(t28, t29);
  __m256i t33 = _mm256_xor_si256(t30, t31);
  __m256i t35 = _mm256_and_si256(r0, r3);
  __m256i t36 = _mm256_xor_si256(r4, r3);
  __m256i t37 = _mm256_xor_si256(r1, r0);
  __m256i t38 = _mm256_xor_si256(t4, t35);
  __m256i t39 = _mm256_xor_si256(t16, t5);
  __m256i t40 = _mm256_xor_si256(t36, t37);
  __m256i t41 = _mm256_xor_si256(t38, t39);
  __m256i t42 = _mm256_xor_si256(t40, t41);
  __m256i t44 = _mm256_xor_si256(r4, r2);
  __m256i t45 = _mm256_xor_si256(r1, r0);
  __m256i t46 = _mm256_xor_si256(t3, t15);
  __m256i t47 = _mm256_xor_si256(t4, t25);
  __m256i t48 = _mm256_xor_si256(t5, t44);
  __m256i t49 = _mm256_xor_si256(t45, t46);
  __m256i t50 = _mm256_xor_si256(t47, t48);
  x[0] = duet_avx2_not(t13);
  x[1] = duet_avx2_not(t23);
  x[2] = _mm256_xor_si256(t32, t33);
  x[3] = duet_avx2_not(t42);
  x[4] = _mm256_xor_si256(t49, t50);
}

#define DUET_AVX2_ROT3(v, k)                                                 \
  ((k) == 0 ? (v)                                                            \
            : ((k) == 1 ? _mm256_permute4x64_epi64(                         \
                                (v), _MM_SHUFFLE(3, 0, 2, 1))               \
                          : _mm256_permute4x64_epi64(                       \
                                (v), _MM_SHUFFLE(3, 1, 0, 2))))

#define DUET_AVX2_L15_TERM(row, off)                                         \
  DUET_AVX2_ROT3(x[((row) + (off)) % 5], (((row) + (off)) / 5) % 3)

#define DUET_AVX2_BUILD_L15(row, dst)                                        \
  do {                                                                       \
    (dst) = DUET_AVX2_L15_TERM((row), 0);                                    \
    (dst) = _mm256_xor_si256((dst), DUET_AVX2_L15_TERM((row), 3));           \
    (dst) = _mm256_xor_si256((dst), DUET_AVX2_L15_TERM((row), 4));           \
    (dst) = _mm256_xor_si256((dst), DUET_AVX2_L15_TERM((row), 5));           \
    (dst) = _mm256_xor_si256((dst), DUET_AVX2_L15_TERM((row), 7));           \
    (dst) = _mm256_xor_si256((dst), DUET_AVX2_L15_TERM((row), 8));           \
    (dst) = _mm256_xor_si256((dst), DUET_AVX2_L15_TERM((row), 13));          \
  } while (0)

DUET_FORCE_INLINE __m256i duet_avx2_load_row3(const uint64_t *s,
                                               unsigned int row) {
  return _mm256_set_epi64x(0, (long long)s[10 + row],
                           (long long)s[5 + row], (long long)s[row]);
}

DUET_FORCE_INLINE void duet_avx2_store_row3(uint64_t *s, unsigned int row,
                                             __m256i v) {
  uint64_t tmp[4];
  _mm256_storeu_si256((__m256i *)(void *)tmp, v);
  s[row] = tmp[0];
  s[5 + row] = tmp[1];
  s[10 + row] = tmp[2];
}

DUET_FORCE_INLINE void permutation960_avx2_round(
    __m256i *r0, __m256i *r1, __m256i *r2, __m256i *r3, __m256i *r4,
    uint64_t rc) {
  __m256i x[5];
  __m256i y0, y1, y2, y3, y4;

  duet_avx2_sbox5(*r0, *r1, *r2, *r3, *r4, x);

  DUET_AVX2_BUILD_L15(0, y0);
  DUET_AVX2_BUILD_L15(1, y1);
  DUET_AVX2_BUILD_L15(2, y2);
  DUET_AVX2_BUILD_L15(3, y3);
  DUET_AVX2_BUILD_L15(4, y4);

  y0 = _mm256_xor_si256(y0, _mm256_set_epi64x(0, 0, 0, (long long)rc));

  *r0 = duet_avx2_l64_row(y0, 0);
  *r1 = duet_avx2_l64_row(y1, 1);
  *r2 = duet_avx2_l64_row(y2, 2);
  *r3 = duet_avx2_l64_row(y3, 3);
  *r4 = duet_avx2_l64_row(y4, 4);
}

static DUET_NOINLINE void permutation960_avx2(
    uint64_t s[restrict WORDS_PERM], const uint64_t rc[restrict ROUNDS]) {
  __m256i r0 = duet_avx2_load_row3(s, 0);
  __m256i r1 = duet_avx2_load_row3(s, 1);
  __m256i r2 = duet_avx2_load_row3(s, 2);
  __m256i r3 = duet_avx2_load_row3(s, 3);
  __m256i r4 = duet_avx2_load_row3(s, 4);
  unsigned int round;

  for (round = 0; round < ROUNDS; ++round) {
    permutation960_avx2_round(&r0, &r1, &r2, &r3, &r4, rc[round]);
  }

  duet_avx2_store_row3(s, 0, r0);
  duet_avx2_store_row3(s, 1, r1);
  duet_avx2_store_row3(s, 2, r2);
  duet_avx2_store_row3(s, 3, r3);
  duet_avx2_store_row3(s, 4, r4);
}

#define permutation960 permutation960_avx2

DUET_FORCE_INLINE void load_full_block(uint64_t block[restrict WORDS_R],
                                       const unsigned char *msg) {
  const __m256i bswap256 =
      _mm256_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7,
                      8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
  const __m128i bswap128 =
      _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
  __m256i lo =
      _mm256_loadu_si256((const __m256i *)(const void *)(msg + 0));
  __m128i hi = _mm_loadu_si128((const __m128i *)(const void *)(msg + 32));

  lo = _mm256_shuffle_epi8(lo, bswap256);
  hi = _mm_shuffle_epi8(hi, bswap128);
  _mm256_storeu_si256((__m256i *)(void *)(block + 0), lo);
  _mm_storeu_si128((__m128i *)(void *)(block + 4), hi);
}

static void pad_message_block(uint64_t block[restrict WORDS_R],
                              const unsigned char *msg,
                              unsigned long long msg_len_bits) {
  unsigned int full_bytes = (unsigned int)(msg_len_bits >> 3);
  unsigned int rem_bits = (unsigned int)msg_len_bits & 7U;
  unsigned int full_words = full_bytes >> 3;
  unsigned int tail_bytes = full_bytes & 7U;
  unsigned int i;

  memset(block, 0, sizeof(uint64_t) * WORDS_R);

  for (i = 0; i < full_words; ++i) {
    block[i] = load64_be(msg + 8U * i);
  }

  if (rem_bits == 0U && tail_bytes == 0U) {
    block[full_words] = UINT64_C(0x8000000000000000);
    return;
  }

  msg += 8U * full_words;
  for (i = 0; i < tail_bytes; ++i) {
    block[full_words] |= (uint64_t)msg[i] << (56U - 8U * i);
  }

  if (rem_bits == 0U) {
    block[full_words] |= UINT64_C(0x80) << (56U - 8U * tail_bytes);
  } else {
    unsigned char last =
        (unsigned char)(msg[tail_bytes] & (0xffU << (8U - rem_bits)));
    last |= (unsigned char)(0x80U >> rem_bits);
    block[full_words] |= (uint64_t)last << (56U - 8U * tail_bytes);
  }
}

static DUET_NOINLINE void absorb_block(
    uint64_t A[restrict WORDS_R], uint64_t B[restrict WORDS_R],
    uint64_t C[restrict WORDS_C], const uint64_t Mi[restrict WORDS_R]) {
  uint64_t p[WORDS_PERM];
  unsigned int i;

  p[0] = A[0] ^ Mi[0];
  p[1] = A[1] ^ Mi[1];
  p[2] = A[2] ^ Mi[2];
  p[3] = A[3] ^ Mi[3];
  p[4] = A[4] ^ Mi[4];
  p[5] = A[5] ^ Mi[5];
  for (i = 0; i < WORDS_C; ++i) {
    p[WORDS_R + i] = C[i];
  }

  permutation960(p, PI);

  A[0] = p[0];
  A[1] = p[1];
  A[2] = p[2];
  A[3] = p[3];
  A[4] = p[4];
  A[5] = p[5];
  for (i = 0; i < WORDS_C; ++i) {
    p[WORDS_R + i] ^= C[i];
    C[i] = p[WORDS_R + i];
  }

  p[0] = B[0] ^ Mi[3];
  p[1] = B[1] ^ Mi[0];
  p[2] = B[2] ^ Mi[5];
  p[3] = B[3] ^ Mi[1];
  p[4] = B[4] ^ Mi[4];
  p[5] = B[5] ^ Mi[2];

  permutation960(p, PI + ROUNDS);

  B[0] = p[0];
  B[1] = p[1];
  B[2] = p[2];
  B[3] = p[3];
  B[4] = p[4];
  B[5] = p[5];
  for (i = 0; i < WORDS_C; ++i) {
    C[i] ^= p[WORDS_R + i];
  }
}

static DUET_NOINLINE void apply_empty(
    uint64_t R[restrict WORDS_R], uint64_t C[restrict WORDS_C],
    const uint64_t rc[restrict ROUNDS]) {
  uint64_t p[WORDS_PERM];
  unsigned int i;

  for (i = 0; i < WORDS_R; ++i) {
    p[i] = R[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    p[WORDS_R + i] = C[i];
  }

  permutation960(p, rc);

  for (i = 0; i < WORDS_R; ++i) {
    R[i] = p[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    C[i] ^= p[WORDS_R + i];
  }
}

DUET_FORCE_INLINE void copy_capacity_bytes(unsigned char *out,
                                           const uint64_t C[restrict WORDS_C],
                                           size_t count) {
  size_t full_words = count >> 3;
  size_t rem_bytes = count & 7U;
  size_t i;

  for (i = 0; i < full_words; ++i) {
    store64_be(out + 8U * i, C[i]);
  }
  if (rem_bytes != 0U) {
    uint64_t x = C[full_words];
    unsigned int shift = 56U;
    for (i = 0; i < rem_bytes; ++i, shift -= 8U) {
      out[8U * full_words + i] = (unsigned char)(x >> shift);
    }
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
  unsigned long long blk;
  const unsigned char *p;
  unsigned char *out;
  size_t remaining_bytes;

  if (digest_len_bits <= 0 || digest_len_bits > MAX_DIGEST_BITS) {
    return -1;
  }
  if (digest == NULL) {
    return -2;
  }
  if (msg == NULL && msg_len_bits != 0U) {
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

  remaining_bytes = ((size_t)(unsigned int)digest_len_bits + 7U) >> 3;
  out = digest;

  for (;;) {
    size_t take = remaining_bytes < C_BYTES ? remaining_bytes : C_BYTES;
    copy_capacity_bytes(out, C, take);
    out += take;
    remaining_bytes -= take;
    if (remaining_bytes == 0U) {
      break;
    }

    apply_empty(A, C, PI);
    take = remaining_bytes < C_BYTES ? remaining_bytes : C_BYTES;
    copy_capacity_bytes(out, C, take);
    out += take;
    remaining_bytes -= take;
    if (remaining_bytes == 0U) {
      break;
    }

    apply_empty(B, C, PI + ROUNDS);
  }

  if (((unsigned int)digest_len_bits & 7U) != 0U) {
    digest[(unsigned int)digest_len_bits >> 3] &=
        (unsigned char)(0xffU << (8U - ((unsigned int)digest_len_bits & 7U)));
  }

  return 0;
}
