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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <arm_neon.h>

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
#else
#define DUET_FORCE_INLINE static inline
#define DUET_NOINLINE
#endif

static const uint64_t PI[2 * ROUNDS] = {
    UINT64_C(0x243f6a8885a308d3), UINT64_C(0x13198a2e03707344), UINT64_C(0xa4093822299f31d0),
    UINT64_C(0x082efa98ec4e6c89), UINT64_C(0x452821e638d01377), UINT64_C(0xbe5466cf34e90c6c),
    UINT64_C(0xc0ac29b7c97c50dd), UINT64_C(0x3f84d5b5b5470917), UINT64_C(0x9216d5d98979fb1b),
    UINT64_C(0xd1310ba698dfb5ac), UINT64_C(0x2ffd72dbd01adfb7), UINT64_C(0xb8e1afed6a267e96),
    UINT64_C(0xba7c9045f12c7f99), UINT64_C(0x24a19947b3916cf7), UINT64_C(0x0801f2e2858efc16),
    UINT64_C(0x636920d871574e69), UINT64_C(0xa458fea3f4933d7e), UINT64_C(0x0d95748f728eb658),
    UINT64_C(0x718bcd5882154aee), UINT64_C(0x7b54a41dc25a59b5), UINT64_C(0x9c30d5392af26013),
    UINT64_C(0xc5d1b023286085f0), UINT64_C(0xca417918b8db38ef), UINT64_C(0x8e79dcb0603a180e),
};

static const unsigned char DUET_SIGMA[WORDS_R] = {3U, 0U, 5U, 1U, 4U, 2U};

#define DUET_ROTL64(x, n)                                                       (((x) << ((unsigned int)(n) & 63U)) |                                          ((x) >> ((64U - ((unsigned int)(n) & 63U)) & 63U)))

#define DUET_L64(x, i)                                                         (DUET_ROTL64((x), (i)) ^ DUET_ROTL64((x), (i) + 21U) ^                        DUET_ROTL64((x), (i) + 43U))

DUET_FORCE_INLINE uint64_t load64_be(const unsigned char *in) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) &&        (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
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
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) &&        (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
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

#define DUET_SBOX5_ASSIGN(o0, o1, o2, o3, o4, x0, x1, x2, x3, x4)              do {                                                                           uint64_t t0 = (x3) & (x4);                                                   uint64_t t1 = (x2) & (x4);                                                   uint64_t t2 = (x2) & (x3);                                                   uint64_t t3 = (x1) & (x4);                                                   uint64_t t4 = (x1) & (x2);                                                   uint64_t t5 = (x0) & (x1);                                                   uint64_t t6 = (x4) ^ (x2);                                                   uint64_t t7 = (x0) ^ t0;                                                     uint64_t t8 = t1 ^ t2;                                                       uint64_t t9 = t3 ^ t4;                                                       uint64_t t10 = t5 ^ t6;                                                      uint64_t t11 = t7 ^ t8;                                                      uint64_t t12 = t9 ^ t10;                                                     uint64_t t13 = t11 ^ t12;                                                    uint64_t t15 = (x1) & (x3);                                                  uint64_t t16 = (x0) & (x2);                                                  uint64_t t17 = (x3) ^ (x2);                                                  uint64_t t18 = (x0) ^ t1;                                                    uint64_t t19 = t2 ^ t15;                                                     uint64_t t20 = t4 ^ t16;                                                     uint64_t t21 = t17 ^ t18;                                                    uint64_t t22 = t19 ^ t20;                                                    uint64_t t23 = t21 ^ t22;                                                    uint64_t t25 = (x0) & (x4);                                                  uint64_t t26 = (x2) ^ (x0);                                                  uint64_t t27 = t0 ^ t1;                                                      uint64_t t28 = t3 ^ t15;                                                     uint64_t t29 = t4 ^ t25;                                                     uint64_t t30 = t16 ^ t5;                                                     uint64_t t31 = t26 ^ t27;                                                    uint64_t t32 = t28 ^ t29;                                                    uint64_t t33 = t30 ^ t31;                                                    uint64_t t35 = (x0) & (x3);                                                  uint64_t t36 = (x4) ^ (x3);                                                  uint64_t t37 = (x1) ^ (x0);                                                  uint64_t t38 = t4 ^ t35;                                                     uint64_t t39 = t16 ^ t5;                                                     uint64_t t40 = t36 ^ t37;                                                    uint64_t t41 = t38 ^ t39;                                                    uint64_t t42 = t40 ^ t41;                                                    uint64_t t44 = (x4) ^ (x2);                                                  uint64_t t45 = (x1) ^ (x0);                                                  uint64_t t46 = t3 ^ t15;                                                     uint64_t t47 = t4 ^ t25;                                                     uint64_t t48 = t5 ^ t44;                                                     uint64_t t49 = t45 ^ t46;                                                    uint64_t t50 = t47 ^ t48;                                                    (o0) = ~t13;                                                                 (o1) = ~t23;                                                                 (o2) = t32 ^ t33;                                                            (o3) = ~t42;                                                                 (o4) = t49 ^ t50;                                                          } while (0)



typedef struct {
  uint64x2_t v01;
  uint64_t v2;
} duet_neon3_t;

DUET_FORCE_INLINE uint64x2_t duet_neon_set2(uint64_t a, uint64_t b) {
  uint64x2_t v = vdupq_n_u64(0);
  v = vsetq_lane_u64(a, v, 0);
  v = vsetq_lane_u64(b, v, 1);
  return v;
}

DUET_FORCE_INLINE duet_neon3_t duet_neon3_xor(duet_neon3_t a,
                                               duet_neon3_t b) {
  duet_neon3_t r;
  r.v01 = veorq_u64(a.v01, b.v01);
  r.v2 = a.v2 ^ b.v2;
  return r;
}

DUET_FORCE_INLINE duet_neon3_t duet_neon3_and(duet_neon3_t a,
                                               duet_neon3_t b) {
  duet_neon3_t r;
  r.v01 = vandq_u64(a.v01, b.v01);
  r.v2 = a.v2 & b.v2;
  return r;
}

DUET_FORCE_INLINE duet_neon3_t duet_neon3_not(duet_neon3_t a) {
  const uint64x2_t ones = vdupq_n_u64(~UINT64_C(0));
  duet_neon3_t r;
  r.v01 = veorq_u64(a.v01, ones);
  r.v2 = ~a.v2;
  return r;
}

DUET_FORCE_INLINE duet_neon3_t duet_neon3_rot(duet_neon3_t x,
                                               unsigned int k) {
  duet_neon3_t r;
  uint64_t c0 = vgetq_lane_u64(x.v01, 0);
  uint64_t c1 = vgetq_lane_u64(x.v01, 1);
  switch (k) {
  case 0:
    return x;
  case 1:
    r.v01 = duet_neon_set2(c1, x.v2);
    r.v2 = c0;
    return r;
  default:
    r.v01 = duet_neon_set2(x.v2, c0);
    r.v2 = c1;
    return r;
  }
}

#define DUET_NEON3_L15_TERM(row, off)                                        \
  duet_neon3_rot(x[((row) + (off)) % 5], (((row) + (off)) / 5) % 3)

#define DUET_NEON3_BUILD_L15(row, dst)                                       \
  do {                                                                       \
    (dst) = DUET_NEON3_L15_TERM((row), 0);                                   \
    (dst) = duet_neon3_xor((dst), DUET_NEON3_L15_TERM((row), 3));            \
    (dst) = duet_neon3_xor((dst), DUET_NEON3_L15_TERM((row), 4));            \
    (dst) = duet_neon3_xor((dst), DUET_NEON3_L15_TERM((row), 5));            \
    (dst) = duet_neon3_xor((dst), DUET_NEON3_L15_TERM((row), 7));            \
    (dst) = duet_neon3_xor((dst), DUET_NEON3_L15_TERM((row), 8));            \
    (dst) = duet_neon3_xor((dst), DUET_NEON3_L15_TERM((row), 13));           \
  } while (0)

DUET_FORCE_INLINE void duet_neon3_sbox5(
    duet_neon3_t r0, duet_neon3_t r1, duet_neon3_t r2, duet_neon3_t r3,
    duet_neon3_t r4, duet_neon3_t x[5]) {
  duet_neon3_t t0 = duet_neon3_and(r3, r4);
  duet_neon3_t t1 = duet_neon3_and(r2, r4);
  duet_neon3_t t2 = duet_neon3_and(r2, r3);
  duet_neon3_t t3 = duet_neon3_and(r1, r4);
  duet_neon3_t t4 = duet_neon3_and(r1, r2);
  duet_neon3_t t5 = duet_neon3_and(r0, r1);
  duet_neon3_t t6 = duet_neon3_xor(r4, r2);
  duet_neon3_t t7 = duet_neon3_xor(r0, t0);
  duet_neon3_t t8 = duet_neon3_xor(t1, t2);
  duet_neon3_t t9 = duet_neon3_xor(t3, t4);
  duet_neon3_t t10 = duet_neon3_xor(t5, t6);
  duet_neon3_t t11 = duet_neon3_xor(t7, t8);
  duet_neon3_t t12 = duet_neon3_xor(t9, t10);
  duet_neon3_t t13 = duet_neon3_xor(t11, t12);
  duet_neon3_t t15 = duet_neon3_and(r1, r3);
  duet_neon3_t t16 = duet_neon3_and(r0, r2);
  duet_neon3_t t17 = duet_neon3_xor(r3, r2);
  duet_neon3_t t18 = duet_neon3_xor(r0, t1);
  duet_neon3_t t19 = duet_neon3_xor(t2, t15);
  duet_neon3_t t20 = duet_neon3_xor(t4, t16);
  duet_neon3_t t21 = duet_neon3_xor(t17, t18);
  duet_neon3_t t22 = duet_neon3_xor(t19, t20);
  duet_neon3_t t23 = duet_neon3_xor(t21, t22);
  duet_neon3_t t25 = duet_neon3_and(r0, r4);
  duet_neon3_t t26 = duet_neon3_xor(r2, r0);
  duet_neon3_t t27 = duet_neon3_xor(t0, t1);
  duet_neon3_t t28 = duet_neon3_xor(t3, t15);
  duet_neon3_t t29 = duet_neon3_xor(t4, t25);
  duet_neon3_t t30 = duet_neon3_xor(t16, t5);
  duet_neon3_t t31 = duet_neon3_xor(t26, t27);
  duet_neon3_t t32 = duet_neon3_xor(t28, t29);
  duet_neon3_t t33 = duet_neon3_xor(t30, t31);
  duet_neon3_t t35 = duet_neon3_and(r0, r3);
  duet_neon3_t t36 = duet_neon3_xor(r4, r3);
  duet_neon3_t t37 = duet_neon3_xor(r1, r0);
  duet_neon3_t t38 = duet_neon3_xor(t4, t35);
  duet_neon3_t t39 = duet_neon3_xor(t16, t5);
  duet_neon3_t t40 = duet_neon3_xor(t36, t37);
  duet_neon3_t t41 = duet_neon3_xor(t38, t39);
  duet_neon3_t t42 = duet_neon3_xor(t40, t41);
  duet_neon3_t t44 = duet_neon3_xor(r4, r2);
  duet_neon3_t t45 = duet_neon3_xor(r1, r0);
  duet_neon3_t t46 = duet_neon3_xor(t3, t15);
  duet_neon3_t t47 = duet_neon3_xor(t4, t25);
  duet_neon3_t t48 = duet_neon3_xor(t5, t44);
  duet_neon3_t t49 = duet_neon3_xor(t45, t46);
  duet_neon3_t t50 = duet_neon3_xor(t47, t48);
  x[0] = duet_neon3_not(t13);
  x[1] = duet_neon3_not(t23);
  x[2] = duet_neon3_xor(t32, t33);
  x[3] = duet_neon3_not(t42);
  x[4] = duet_neon3_xor(t49, t50);
}

DUET_FORCE_INLINE duet_neon3_t duet_neon3_l64(duet_neon3_t y,
                                               unsigned int row) {
  duet_neon3_t r;
  r.v01 = duet_neon_set2(DUET_L64(vgetq_lane_u64(y.v01, 0), row),
                         DUET_L64(vgetq_lane_u64(y.v01, 1), row + 5U));
  r.v2 = DUET_L64(y.v2, row + 10U);
  return r;
}

DUET_FORCE_INLINE duet_neon3_t duet_neon3_load_row(const uint64_t *s,
                                                    unsigned int row) {
  duet_neon3_t r;
  r.v01 = duet_neon_set2(s[row], s[5 + row]);
  r.v2 = s[10 + row];
  return r;
}

DUET_FORCE_INLINE void duet_neon3_store_row(uint64_t *s, unsigned int row,
                                             duet_neon3_t r) {
  s[row] = vgetq_lane_u64(r.v01, 0);
  s[5 + row] = vgetq_lane_u64(r.v01, 1);
  s[10 + row] = r.v2;
}

DUET_FORCE_INLINE void permutation_neon_round(
    duet_neon3_t *r0, duet_neon3_t *r1, duet_neon3_t *r2, duet_neon3_t *r3,
    duet_neon3_t *r4, uint64_t rc) {
  duet_neon3_t x[5];
  duet_neon3_t y0, y1, y2, y3, y4;

  duet_neon3_sbox5(*r0, *r1, *r2, *r3, *r4, x);
  DUET_NEON3_BUILD_L15(0, y0);
  DUET_NEON3_BUILD_L15(1, y1);
  DUET_NEON3_BUILD_L15(2, y2);
  DUET_NEON3_BUILD_L15(3, y3);
  DUET_NEON3_BUILD_L15(4, y4);

  y0.v01 = veorq_u64(y0.v01, vsetq_lane_u64(rc, vdupq_n_u64(0), 0));
  *r0 = duet_neon3_l64(y0, 0);
  *r1 = duet_neon3_l64(y1, 1);
  *r2 = duet_neon3_l64(y2, 2);
  *r3 = duet_neon3_l64(y3, 3);
  *r4 = duet_neon3_l64(y4, 4);
}

static DUET_NOINLINE void permutation(uint64_t s[restrict WORDS_PERM],
                                      const uint64_t rc[restrict ROUNDS]) {
  duet_neon3_t r0 = duet_neon3_load_row(s, 0);
  duet_neon3_t r1 = duet_neon3_load_row(s, 1);
  duet_neon3_t r2 = duet_neon3_load_row(s, 2);
  duet_neon3_t r3 = duet_neon3_load_row(s, 3);
  duet_neon3_t r4 = duet_neon3_load_row(s, 4);
  unsigned int round;

  for (round = 0; round < ROUNDS; ++round) {
    permutation_neon_round(&r0, &r1, &r2, &r3, &r4, rc[round]);
  }

  duet_neon3_store_row(s, 0, r0);
  duet_neon3_store_row(s, 1, r1);
  duet_neon3_store_row(s, 2, r2);
  duet_neon3_store_row(s, 3, r3);
  duet_neon3_store_row(s, 4, r4);
}

DUET_FORCE_INLINE void load_full_block(uint64_t block[restrict WORDS_R],
                                       const unsigned char *msg) {
  unsigned int i;
  for (i = 0; i < WORDS_R; ++i) {
    block[i] = load64_be(msg + 8U * i);
  }
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

  for (i = 0; i < WORDS_R; ++i) {
    p[i] = A[i] ^ Mi[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    p[WORDS_R + i] = C[i];
  }
  permutation(p, PI);
  for (i = 0; i < WORDS_R; ++i) {
    A[i] = p[i];
  }
  for (i = 0; i < WORDS_C; ++i) {
    C[i] ^= p[WORDS_R + i];
  }

  for (i = 0; i < WORDS_R; ++i) {
    p[i] = B[i] ^ Mi[DUET_SIGMA[i]];
  }
  for (i = 0; i < WORDS_C; ++i) {
    p[WORDS_R + i] = C[i];
  }
  permutation(p, PI + ROUNDS);
  for (i = 0; i < WORDS_R; ++i) {
    B[i] = p[i];
  }
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
  permutation(p, rc);
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
