/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

ARM NEON/SHA3-XAR optimized Cuishen-512 implementation.
*/

#if defined(__GNUC__) && !defined(__clang__) && !defined(__ARM_FEATURE_SHA3)
#pragma GCC target("+sha3")
#endif

#include "CryptHash_Cuishen-512.h"
#include <arm_neon.h>
#include <stdint.h>
#include <string.h>

#if !defined(__ARM_FEATURE_SHA3)
#error "CryptHash_Cuishen-512-c-neon.c requires ARM SHA3/XAR support"
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define CUISHEN_ARM_IMPL_ATTR(feature)                                      \
  __attribute__((visibility("hidden"), noinline, noclone, target(feature)))
#elif defined(__clang__)
#define CUISHEN_ARM_IMPL_ATTR(feature)                                      \
  __attribute__((visibility("hidden"), noinline, target(feature)))
#else
#define CUISHEN_ARM_IMPL_ATTR(feature)
#endif

enum {
  ROUNDS = 64,
  MESSAGE_WORDS = 16,
  MESSAGE_BLOCK_BITS = 1024,
  LINK_WORDS = 4,
  MASTER_KEY_WORDS = MESSAGE_WORDS + LINK_WORDS,
  FINAL_PREFIX_WORDS = 8,
  FINAL_TAIL_BITS = 768,
  A = 7,
  B = 31,
  C = 56,
  D = 20,
  MA = 14,
  MB = 43,
  MC = 28,
  MD = 19,
  ME = 29,
  MF = 63,
  MG = 12,
  MH = 6,
  BA = 27
};

static const uint64_t IV_1[4] = {0x0cc4a61194f81760ULL,
                                 0x5815a7be0543c11cULL,
                                 0x70b7ed67fc9b5c42ULL,
                                 0xa1513c69681ad6d4ULL};

static const uint64_t IV_2[4] = {0x44f9363580e83d02ULL,
                                 0x720dcdfd9dba5b44ULL,
                                 0xb467369e08efd70eULL,
                                 0xca320b75e2b634f9ULL};

static const uint64_t PI[ROUNDS] = {
    0x243f6a8885a308d3ULL, 0x13198a2e03707344ULL, 0xa4093822299f31d0ULL,
    0x082efa98ec4e6c89ULL, 0x452821e638d01377ULL, 0xbe5466cf34e90c6cULL,
    0xc0ac29b7c97c50ddULL, 0x3f84d5b5b5470917ULL, 0x9216d5d98979fb1bULL,
    0xd1310ba698dfb5acULL, 0x2ffd72dbd01adfb7ULL, 0xb8e1afed6a267e96ULL,
    0xba7c9045f12c7f99ULL, 0x24a19947b3916cf7ULL, 0x0801f2e2858efc16ULL,
    0x636920d871574e69ULL, 0xa458fea3f4933d7eULL, 0x0d95748f728eb658ULL,
    0x718bcd5882154aeeULL, 0x7b54a41dc25a59b5ULL, 0x9c30d5392af26013ULL,
    0xc5d1b023286085f0ULL, 0xca417918b8db38efULL, 0x8e79dcb0603a180eULL,
    0x6c9e0e8bb01e8a3eULL, 0xd71577c1bd314b27ULL, 0x78af2fda55605c60ULL,
    0xe65525f3aa55ab94ULL, 0x5748986263e81440ULL, 0x55ca396a2aab10b6ULL,
    0xb4cc5c341141e8ceULL, 0xa15486af7c72e993ULL, 0xb3ee1411636fbc2aULL,
    0x2ba9c55d741831f6ULL, 0xce5c3e169b87931eULL, 0xafd6ba336c24cf5cULL,
    0x7a32538128958677ULL, 0x3b8f48986b4bb9afULL, 0xc4bfe81b66282193ULL,
    0x61d809ccfb21a991ULL, 0x487cac605dec8032ULL, 0xef845d5de98575b1ULL,
    0xdc262302eb651b88ULL, 0x23893e81d396acc5ULL, 0x0f6d6ff383f44239ULL,
    0x2e0b4482a4842004ULL, 0x69c8f04a9e1f9b5eULL, 0x21c66842f6e96c9aULL,
    0x670c9c61abd388f0ULL, 0x6a51a0d2d8542f68ULL, 0x960fa728ab5133a3ULL,
    0x6eef0b6c137a3be4ULL, 0xba3bf0507efb2a98ULL, 0xa1f1651d39af0176ULL,
    0x66ca593e82430e88ULL, 0x8cee8619456f9fb4ULL, 0x7d84a5c33b8b5ebeULL,
    0xe06f75d885c12073ULL, 0x401a449f56c16aa6ULL, 0x4ed3aa62363f7706ULL,
    0x1bfedf72429b023dULL, 0x37d0d724d00a1248ULL, 0xdb0fead349f1c09bULL,
    0x075372c980991b7bULL};

static inline uint64_t rotl64(uint64_t x, unsigned int n) {
  return (x << n) | (x >> (64U - n));
}

static inline uint64x2_t make_u64x2(uint64_t lane0, uint64_t lane1) {
  uint64x2_t v = vdupq_n_u64(lane0);
  return vsetq_lane_u64(lane1, v, 1);
}

static inline uint64_t load64_be(const unsigned char *in) {
  return ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48) |
         ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32) |
         ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16) |
         ((uint64_t)in[6] << 8) | (uint64_t)in[7];
}

static inline uint64x2_t load_u64x2_be(const unsigned char *in) {
  return vreinterpretq_u64_u8(vrev64q_u8(vld1q_u8(in)));
}

static inline void store_u64x2_be(unsigned char *out, uint64x2_t v) {
  vst1q_u8(out, vrev64q_u8(vreinterpretq_u8_u64(v)));
}

static inline void load1024_be(const unsigned char *in,
                               uint64_t out[MESSAGE_WORDS]) {
  vst1q_u64(out + 0, load_u64x2_be(in + 0));
  vst1q_u64(out + 2, load_u64x2_be(in + 16));
  vst1q_u64(out + 4, load_u64x2_be(in + 32));
  vst1q_u64(out + 6, load_u64x2_be(in + 48));
  vst1q_u64(out + 8, load_u64x2_be(in + 64));
  vst1q_u64(out + 10, load_u64x2_be(in + 80));
  vst1q_u64(out + 12, load_u64x2_be(in + 96));
  vst1q_u64(out + 14, load_u64x2_be(in + 112));
}

__attribute__((always_inline)) static inline void
load_partial_be(const unsigned char *tail, unsigned long long rem_bits,
                uint64_t *out, int out_words) {
  const unsigned long long rem_bytes = rem_bits >> 3;
  const unsigned long long full_words = rem_bytes >> 3;
  const unsigned int partial_bytes = (unsigned int)(rem_bytes & 7ULL);
  const unsigned int rem_tail_bits = (unsigned int)(rem_bits & 7ULL);
  unsigned long long word = 0;

  memset(out, 0, (size_t)out_words * sizeof(*out));

  for (; word + 1ULL < full_words; word += 2ULL) {
    vst1q_u64(out + word, load_u64x2_be(tail + 8 * word));
  }
  if (word < full_words) {
    out[word] = load64_be(tail + 8 * word);
  }

  if (partial_bytes != 0U || rem_tail_bits != 0U) {
    const unsigned long long byte_offset = full_words << 3;
    uint64_t w = 0;
    unsigned int byte;

    for (byte = 0; byte < partial_bytes; ++byte) {
      w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
    }
    if (rem_tail_bits != 0U) {
      const unsigned char tail_mask =
          (unsigned char)(0xFFU << (8U - rem_tail_bits));
      w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
           << (56U - 8U * partial_bytes);
    }

    out[full_words] = w;
  }
}

__attribute__((always_inline)) static inline void
load_aligned_tail_be(const unsigned char *tail, unsigned int words,
                     uint64_t *out, unsigned int out_words) {
  const uint64x2_t zero = vdupq_n_u64(0);
  unsigned int word = 0;

  for (; word + 1U < words; word += 2U) {
    vst1q_u64(out + word, load_u64x2_be(tail + 8U * word));
  }
  if (word < words) {
    out[word] = load64_be(tail + 8U * word);
    ++word;
  }
  for (; word + 1U < out_words; word += 2U) {
    vst1q_u64(out + word, zero);
  }
  if (word < out_words) {
    out[word] = 0;
  }
}

#define ROTL64X2_XOR_SCALAR_KEY(X, K, N)                                    \
  vxarq_u64((X), vdupq_n_u64((K)), 64U - (N))

/*
 * Each uint64x2_t lane carries one of the c-opt top/bottom encryptions.
 * The v6 1280-bit key schedule stays scalar, while state ARX rounds use XAR
 * so rotl64(state ^ key, n) maps to a single vector instruction.
 */
#define CUISHEN_PAIR_ROUND_NEON(I)                                        \
  do {                                                                       \
    const uint64_t rk0 = m0 ^ bk0 ^ c0;                                      \
    const uint64_t rk1 = m1 ^ bk1 ^ c1;                                      \
    const uint64_t rk2 = m8 ^ bk2;                                           \
    const uint64_t rk3 = m9 ^ bk3;                                           \
    const uint64x2_t xt = veorq_u64(vx2, vx3);                               \
    const uint64x2_t nx0 = vx2;                                              \
    const uint64x2_t nx1 = vx3;                                              \
    const uint64x2_t nx2 = vaddq_u64(                                        \
        ROTL64X2_XOR_SCALAR_KEY(vx0, rk0 ^ PI[(I)], A),                      \
        ROTL64X2_XOR_SCALAR_KEY(xt, rk3, D));                                \
    const uint64x2_t nx3 = vaddq_u64(                                        \
        ROTL64X2_XOR_SCALAR_KEY(vx1, rk1, B),                                \
        ROTL64X2_XOR_SCALAR_KEY(xt, rk2, C));                                \
    const uint64_t m23 = m2 ^ m3;                                            \
    const uint64_t m1011 = m10 ^ m11;                                        \
    const uint64_t pm2 = m2;                                                 \
    const uint64_t pm3 = m3;                                                 \
    const uint64_t pm4 = m4;                                                 \
    const uint64_t pm5 = m5;                                                 \
    const uint64_t nm2 = rotl64(m8, ME) + rotl64(m1011, MH);                 \
    const uint64_t nm3 = rotl64(m9, MF) + rotl64(m1011, MG);                 \
    const uint64_t nm10 = rotl64(m0, MA) + rotl64(m23, MD);                  \
    const uint64_t nm11 = rotl64(m1, MB) + rotl64(m23, MC);                  \
    const uint64_t nb3 = rotl64(bk0, BA) ^ bk1;                              \
    const uint64_t nc1 = rotl64(c0, 2);                                      \
    vx0 = nx0;                                                               \
    vx1 = nx1;                                                               \
    vx2 = nx2;                                                               \
    vx3 = nx3;                                                               \
    m0 = m6;                                                                 \
    m1 = m7;                                                                 \
    m2 = nm2;                                                                \
    m3 = nm3;                                                                \
    m4 = m10;                                                                \
    m5 = m11;                                                                \
    m6 = m12;                                                                \
    m7 = m13;                                                                \
    m8 = m14;                                                                \
    m9 = m15;                                                                \
    m10 = nm10;                                                              \
    m11 = nm11;                                                              \
    m12 = pm2;                                                               \
    m13 = pm3;                                                               \
    m14 = pm4;                                                               \
    m15 = pm5;                                                               \
    bk0 = bk1;                                                               \
    bk1 = bk2;                                                               \
    bk2 = bk3;                                                               \
    bk3 = nb3;                                                               \
    c0 = c1;                                                                 \
    c1 = nc1;                                                                \
  } while (0)

__attribute__((always_inline)) static inline void encrypt_pair_same_key(
    uint64x2_t vx0, uint64x2_t vx1, uint64x2_t vx2, uint64x2_t vx3,
    const uint64_t message_key[MESSAGE_WORDS], const uint64_t block_key[4],
    uint64_t counter1, uint64x2_t out[4]) {
  uint64_t m0 = message_key[0];
  uint64_t m1 = message_key[1];
  uint64_t m2 = message_key[2];
  uint64_t m3 = message_key[3];
  uint64_t m4 = message_key[4];
  uint64_t m5 = message_key[5];
  uint64_t m6 = message_key[6];
  uint64_t m7 = message_key[7];
  uint64_t m8 = message_key[8];
  uint64_t m9 = message_key[9];
  uint64_t m10 = message_key[10];
  uint64_t m11 = message_key[11];
  uint64_t m12 = message_key[12];
  uint64_t m13 = message_key[13];
  uint64_t m14 = message_key[14];
  uint64_t m15 = message_key[15];
  uint64_t bk0 = block_key[0];
  uint64_t bk1 = block_key[1];
  uint64_t bk2 = block_key[2];
  uint64_t bk3 = block_key[3];
  uint64_t c0 = 0;
  uint64_t c1 = counter1;
  int round;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (round = 0; round < ROUNDS; round += 8) {
    CUISHEN_PAIR_ROUND_NEON(round);
    CUISHEN_PAIR_ROUND_NEON(round + 1);
    CUISHEN_PAIR_ROUND_NEON(round + 2);
    CUISHEN_PAIR_ROUND_NEON(round + 3);
    CUISHEN_PAIR_ROUND_NEON(round + 4);
    CUISHEN_PAIR_ROUND_NEON(round + 5);
    CUISHEN_PAIR_ROUND_NEON(round + 6);
    CUISHEN_PAIR_ROUND_NEON(round + 7);
  }

  out[0] = vx0;
  out[1] = vx1;
  out[2] = vx2;
  out[3] = vx3;
}

__attribute__((always_inline)) static inline void
encrypt_final_empty_tail(const uint64_t t[4], const uint64_t b[4],
                         uint64_t counter1, uint64x2_t out[4]) {
  uint64x2_t vx0 = vdupq_n_u64(0);
  uint64x2_t vx1 = vdupq_n_u64(0);
  uint64x2_t vx2 = vdupq_n_u64(0);
  uint64x2_t vx3 = make_u64x2(2, 3);
  uint64_t m0 = t[0];
  uint64_t m1 = t[1];
  uint64_t m2 = t[2];
  uint64_t m3 = t[3];
  uint64_t m4 = b[0];
  uint64_t m5 = b[1];
  uint64_t m6 = b[2];
  uint64_t m7 = b[3];
  uint64_t m8 = 0;
  uint64_t m9 = 0;
  uint64_t m10 = 0;
  uint64_t m11 = 0;
  uint64_t m12 = 0;
  uint64_t m13 = 0;
  uint64_t m14 = 0;
  uint64_t m15 = 0;
  uint64_t bk0 = 0;
  uint64_t bk1 = 0;
  uint64_t bk2 = 0;
  uint64_t bk3 = 0;
  uint64_t c0 = 0;
  uint64_t c1 = counter1;
  int round;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (round = 0; round < ROUNDS; round += 8) {
    CUISHEN_PAIR_ROUND_NEON(round);
    CUISHEN_PAIR_ROUND_NEON(round + 1);
    CUISHEN_PAIR_ROUND_NEON(round + 2);
    CUISHEN_PAIR_ROUND_NEON(round + 3);
    CUISHEN_PAIR_ROUND_NEON(round + 4);
    CUISHEN_PAIR_ROUND_NEON(round + 5);
    CUISHEN_PAIR_ROUND_NEON(round + 6);
    CUISHEN_PAIR_ROUND_NEON(round + 7);
  }

  out[0] = vx0;
  out[1] = vx1;
  out[2] = vx2;
  out[3] = vx3;
}

#undef CUISHEN_PAIR_ROUND_NEON
#undef ROTL64X2_XOR_SCALAR_KEY

__attribute__((always_inline)) static inline void
compress_one_block(uint64_t t[4], uint64_t b[4],
                   const uint64_t m[MESSAGE_WORDS],
                   unsigned long long msg_bits_processed) {
  uint64x2_t state[4];
  uint64x2_t input[4];

  input[0] = vdupq_n_u64(t[0]);
  input[1] = vdupq_n_u64(t[1]);
  input[2] = vdupq_n_u64(t[2]);
  input[3] = make_u64x2(t[3],
                        t[3] ^ 1ULL);
  encrypt_pair_same_key(input[0], input[1], input[2], input[3], m, b,
                        (uint64_t)msg_bits_processed, state);
  state[0] = veorq_u64(state[0], input[0]);
  state[1] = veorq_u64(state[1], input[1]);
  state[2] = veorq_u64(state[2], input[2]);
  state[3] = veorq_u64(state[3], input[3]);

  vst1q_u64(t + 0, vzip1q_u64(state[0], state[1]));
  vst1q_u64(t + 2, vzip1q_u64(state[2], state[3]));
  vst1q_u64(b + 0, vzip2q_u64(state[0], state[1]));
  vst1q_u64(b + 2, vzip2q_u64(state[2], state[3]));
}

static void compress_full_block_be(uint64_t t[4], uint64_t b[4],
                                   const unsigned char *msg,
                                   unsigned long long msg_bits_processed) {
  uint64_t m[MESSAGE_WORDS];

  load1024_be(msg, m);
  compress_one_block(t, b, m, msg_bits_processed);
}

static void finalization(const uint64_t t[4], const uint64_t b[4],
                         const unsigned char *tail_msg,
                         unsigned long long tail_msg_bits,
                         unsigned long long counter_bits,
                         unsigned char digest[64]) {
  uint64_t final_key[MASTER_KEY_WORDS];
  uint64x2_t state[4];

  if (tail_msg_bits == 0ULL) {
    encrypt_final_empty_tail(t, b, (uint64_t)counter_bits, state);
  } else {
    /* Final key is t || b || tail message || zero padding. */
    final_key[0] = t[0];
    final_key[1] = t[1];
    final_key[2] = t[2];
    final_key[3] = t[3];
    final_key[4] = b[0];
    final_key[5] = b[1];
    final_key[6] = b[2];
    final_key[7] = b[3];
    if ((tail_msg_bits & 63ULL) == 0ULL && tail_msg_bits != 256ULL) {
      load_aligned_tail_be(tail_msg, (unsigned int)(tail_msg_bits >> 6),
                           final_key + FINAL_PREFIX_WORDS,
                           MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);
    } else {
      load_partial_be(tail_msg, tail_msg_bits, final_key + FINAL_PREFIX_WORDS,
                      MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);
    }

    state[0] = vdupq_n_u64(0);
    state[1] = vdupq_n_u64(0);
    state[2] = vdupq_n_u64(0);
    state[3] = make_u64x2(2, 3);
    encrypt_pair_same_key(state[0], state[1], state[2], state[3], final_key,
                          final_key + MESSAGE_WORDS, (uint64_t)counter_bits,
                          state);
  }

  store_u64x2_be(digest + 0, vzip1q_u64(state[0], state[1]));
  store_u64x2_be(digest + 16, vzip1q_u64(state[2], state[3]));
  store_u64x2_be(digest + 32, vzip2q_u64(state[0], state[1]));
  store_u64x2_be(digest + 48, vzip2q_u64(state[2], state[3]));
}

CUISHEN_ARM_IMPL_ATTR("+sha3")
int cuishen512_arm_sha3_impl(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
  uint64_t t[4];
  uint64_t b[4];
  uint64_t m[MESSAGE_WORDS];
  unsigned long long full_blocks;
  unsigned long long rem_bits;
  unsigned long long block;
  unsigned long long processed_bits;
  const unsigned char *p;

  if (digest_len_bits != DIGEST_BIT_LENGTH || digest == 0) {
    return -1;
  }
  if (msg_len_bits != 0ULL && msg == 0) {
    return -1;
  }

  t[0] = IV_1[0];
  t[1] = IV_1[1];
  t[2] = IV_1[2];
  t[3] = IV_1[3];
  b[0] = IV_2[0];
  b[1] = IV_2[1];
  b[2] = IV_2[2];
  b[3] = IV_2[3];

  full_blocks = msg_len_bits >> 10;
  rem_bits = msg_len_bits & 1023ULL;
  p = msg;
  processed_bits = MESSAGE_BLOCK_BITS;

  for (block = 0ULL; block < full_blocks; ++block) {
    compress_full_block_be(t, b, p, processed_bits);
    p += 128;
    processed_bits += MESSAGE_BLOCK_BITS;
  }

  if (rem_bits > FINAL_TAIL_BITS) {
    load_partial_be(p, rem_bits, m, MESSAGE_WORDS);
    compress_one_block(t, b, m, msg_len_bits);
    p = 0;
    rem_bits = 0;
  }

  finalization(t, b, p, rem_bits, msg_len_bits, digest);

  return 0;
}
