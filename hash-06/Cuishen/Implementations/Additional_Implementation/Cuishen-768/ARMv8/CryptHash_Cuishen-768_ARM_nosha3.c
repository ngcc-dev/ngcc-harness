/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

Cuishen-768 implementation for ARM targets without SHA3/XAR.

The hot ARX round core is scalar because AArch64 scalar rotate is cheaper than
synthesizing each 64-bit vector rotate from base NEON shifts. NEON is used for
endian loads, zeroing small word ranges, and digest stores only.

Compile with -mcpu=apple-m1+nosha3 so Apple clang cannot fold base NEON
operations back into crypto-extension instructions.
*/

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC target("+nosha3")
#endif

#include "CryptHash_Cuishen-768.h"
#include <arm_neon.h>
#include <stdint.h>

#if defined(__ARM_FEATURE_SHA3)
#error "CryptHash_Cuishen-768-c-neon-nosha3.c must be built without SHA3"
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
  LINK_WORDS = 8,
  MASTER_KEY_WORDS = MESSAGE_WORDS + LINK_WORDS,
  FINAL_PREFIX_WORDS = 16,
  FINAL_TAIL_BITS = 512,
  A = 18,
  B = 33,
  C = 47,
  D = 60,
  E = 49,
  F = 20,
  G = 16,
  H = 21,
  MA = 14,
  MB = 43,
  MC = 28,
  MD = 19,
  ME = 29,
  MF = 63,
  MG = 12,
  MH = 6,
  BA = 47
};

static const uint64_t IV_1[8] = {
    0x34e0d42e61a33f99ULL, 0x87abb9f2087207edULL,
    0xec3fc3f38a10ea02ULL, 0x610bebf29db2faf5ULL,
    0x7420b49edc5a21eeULL, 0xd1fd8a3396bdeee8ULL,
    0x092197f60194adc1ULL, 0x1b530c95f8b3def8ULL};

static const uint64_t IV_2[8] = {
    0x869d6342f6d22822ULL, 0x11076689f6aff6b0ULL,
    0x43ab9fb62162bb7fULL, 0x75a9f91d5813e9e8ULL,
    0xd7cd8173f479197aULL, 0x07fe00ff606fac41ULL,
    0x379f513f856fc7a9ULL, 0x66b651a8ab0e883bULL};

static const uint64_t E_CONST[ROUNDS] = {
    0xb7e151628aed2a6aULL, 0xbf7158809cf4f3c7ULL,
    0x62e7160f38b4da56ULL, 0xa784d9045190cfefULL,
    0x324e7738926cfbe5ULL, 0xf4bf8d8d8c31d763ULL,
    0xda06c80abb1185ebULL, 0x4f7c7b5757f59584ULL,
    0x90cfd47d7c19bb42ULL, 0x158d9554f7b46bceULL,
    0xd55c4d79fd5f24d6ULL, 0x613c31c3839a2ddfULL,
    0x8a9a276bcfbfa1c8ULL, 0x77c56284dab79cd4ULL,
    0xc2b3293d20e9e5eaULL, 0xf02ac60acc93ed87ULL,
    0x4422a52ecb238feeULL, 0xe5ab6add835fd1a0ULL,
    0x753d0a8f78e537d2ULL, 0xb95bb79d8dcaec64ULL,
    0x2c1e9f23b829b5c2ULL, 0x780bf38737df8bb3ULL,
    0x00d01334a0d0bd86ULL, 0x45cbfa73a6160ffeULL,
    0x393c48cbbbca060fULL, 0x0ff8ec6d31beb5ccULL,
    0xeed7f2f0bb088017ULL, 0x163bc60df45a0ecbULL,
    0x1bcd289b06cbbfeaULL, 0x21ad08e1847f3f73ULL,
    0x78d56ced94640d6eULL, 0xf0d3d37be67008e1ULL,
    0x86d1bf275b9b241dULL, 0xeb64749a47dfdfb9ULL,
    0x6632c3eb061b6472ULL, 0xbbf84c26144e49c2ULL,
    0x0d04c324ef10de513ULL, 0xd3f5114b8b5d374dULL,
    0x93cb8879c7d52ffdULL, 0x72ba0aae7277da7bULL,
    0xa1b4af1488d8e836ULL, 0xaf14865e6c37ab68ULL,
    0x76fe690b57112138ULL, 0x2af341afe94f77bcULL,
    0xf06c83b8ff5675f0ULL, 0x979074ad9a787bc5ULL,
    0xb9bd4b0c5937d3edULL, 0xe4c3a79396215edaULL,
    0xb1f57d0b5a7db461ULL, 0xdd8f3c75540d0012ULL,
    0x1fd56e95f8c731e9ULL, 0xc4d7221bbed0c62bULL,
    0xb5a87804b679a0caULL, 0xa41d802a4604c311ULL,
    0xb71de3e5c6b400e0ULL, 0x24a6668ccf2e2de8ULL,
    0x6876e4f5c50000f0ULL, 0xa93b3aa7e6342b30ULL,
    0x2a0a47373b25f73eULL, 0x3b26d569fe2291adULL,
    0x36d6a147d1060b87ULL, 0x1a2801f978376408ULL,
    0x2ff592d9140db1e9ULL, 0x399df4b0e14ca8e8ULL};

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
zero_words(uint64_t *out, unsigned int first_word, unsigned int out_words) {
  const uint64x2_t zero = vdupq_n_u64(0);
  unsigned int word = first_word;

  for (; word + 1U < out_words; word += 2U) {
    vst1q_u64(out + word, zero);
  }
  if (word < out_words) {
    out[word] = 0;
  }
}

__attribute__((always_inline)) static inline void
load_partial_be(const unsigned char *tail, unsigned long long rem_bits,
                uint64_t *out, unsigned int out_words) {
  const unsigned long long rem_bytes = rem_bits >> 3;
  const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
  const unsigned int partial_bytes = (unsigned int)(rem_bytes & 7ULL);
  const unsigned int rem_tail_bits = (unsigned int)(rem_bits & 7ULL);
  unsigned int word = 0;
  unsigned int zero_from = full_words;

  for (; word + 1U < full_words; word += 2U) {
    vst1q_u64(out + word, load_u64x2_be(tail + 8U * word));
  }
  if (word < full_words) {
    out[word] = load64_be(tail + 8U * word);
  }

  if (partial_bytes != 0U || rem_tail_bits != 0U) {
    const unsigned long long byte_offset = (unsigned long long)full_words << 3;
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
    zero_from = full_words + 1U;
  }

  zero_words(out, zero_from, out_words);
}

__attribute__((always_inline)) static inline void
load_aligned_tail_be(const unsigned char *tail, unsigned int words,
                     uint64_t *out, unsigned int out_words) {
  unsigned int word = 0;

  for (; word + 1U < words; word += 2U) {
    vst1q_u64(out + word, load_u64x2_be(tail + 8U * word));
  }
  if (word < words) {
    out[word] = load64_be(tail + 8U * word);
    ++word;
  }
  zero_words(out, word, out_words);
}

/*
 * Without SHA3/XAR, scalar 64-bit rotate is cheaper than synthesizing each
 * vector rotate from base NEON shifts. Keep top/bottom schedule sharing, but
 * run the ARX state words as scalars.
 */
#define CUISHEN768_PAIR_ROUND(I)                                           \
  do {                                                                       \
    const uint64_t rk0 = m0 ^ bk0 ^ cnt0;                                    \
    const uint64_t rk1 = m1 ^ bk1 ^ cnt1;                                    \
    const uint64_t rk2 = m8 ^ bk2;                                           \
    const uint64_t rk3 = m9 ^ bk3;                                           \
    const uint64_t rk4 = bk4;                                                \
    const uint64_t rk5 = bk5;                                                \
    const uint64_t rk6 = bk6;                                                \
    const uint64_t rk7 = bk7;                                                \
    const uint64_t xt = x4 ^ x5 ^ x6 ^ x7;                                   \
    const uint64_t yt = y4 ^ y5 ^ y6 ^ y7;                                   \
    const uint64_t nx0 = x4;                                                 \
    const uint64_t nx1 = x5;                                                 \
    const uint64_t nx2 = x6;                                                 \
    const uint64_t nx3 = x7;                                                 \
    const uint64_t nx4 =                                                     \
        rotl64(x0 ^ rk0 ^ E_CONST[(I)], A) + rotl64(xt ^ rk7, H);            \
    const uint64_t nx5 = rotl64(x1 ^ rk1, B) + rotl64(xt ^ rk6, G);          \
    const uint64_t nx6 = rotl64(x2 ^ rk2, C) + rotl64(xt ^ rk5, F);          \
    const uint64_t nx7 = rotl64(x3 ^ rk3, D) + rotl64(xt ^ rk4, E);          \
    const uint64_t ny0 = y4;                                                 \
    const uint64_t ny1 = y5;                                                 \
    const uint64_t ny2 = y6;                                                 \
    const uint64_t ny3 = y7;                                                 \
    const uint64_t ny4 =                                                     \
        rotl64(y0 ^ rk0 ^ E_CONST[(I)], A) + rotl64(yt ^ rk7, H);            \
    const uint64_t ny5 = rotl64(y1 ^ rk1, B) + rotl64(yt ^ rk6, G);          \
    const uint64_t ny6 = rotl64(y2 ^ rk2, C) + rotl64(yt ^ rk5, F);          \
    const uint64_t ny7 = rotl64(y3 ^ rk3, D) + rotl64(yt ^ rk4, E);          \
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
    const uint64_t nb7 = rotl64(bk0, BA) ^ bk1;                              \
    const uint64_t ncnt1 = rotl64(cnt0, 2);                                  \
    x0 = nx0;                                                                \
    x1 = nx1;                                                                \
    x2 = nx2;                                                                \
    x3 = nx3;                                                                \
    x4 = nx4;                                                                \
    x5 = nx5;                                                                \
    x6 = nx6;                                                                \
    x7 = nx7;                                                                \
    y0 = ny0;                                                                \
    y1 = ny1;                                                                \
    y2 = ny2;                                                                \
    y3 = ny3;                                                                \
    y4 = ny4;                                                                \
    y5 = ny5;                                                                \
    y6 = ny6;                                                                \
    y7 = ny7;                                                                \
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
    bk3 = bk4;                                                               \
    bk4 = bk5;                                                               \
    bk5 = bk6;                                                               \
    bk6 = bk7;                                                               \
    bk7 = nb7;                                                               \
    cnt0 = cnt1;                                                             \
    cnt1 = ncnt1;                                                            \
  } while (0)

__attribute__((always_inline)) static inline void encrypt_pair_same_key(
    uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4,
    uint64_t x5, uint64_t x6, uint64_t x7, uint64_t y0, uint64_t y1,
    uint64_t y2, uint64_t y3, uint64_t y4, uint64_t y5, uint64_t y6,
    uint64_t y7, const uint64_t message_key[MESSAGE_WORDS],
    const uint64_t block_key[LINK_WORDS], uint64_t counter0,
    uint64_t counter1, uint64_t out_x[8], uint64_t out_y[8]) {
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
  uint64_t bk4 = block_key[4];
  uint64_t bk5 = block_key[5];
  uint64_t bk6 = block_key[6];
  uint64_t bk7 = block_key[7];
  uint64_t cnt0 = counter0;
  uint64_t cnt1 = counter1;
  int round;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (round = 0; round < ROUNDS; round += 8) {
    CUISHEN768_PAIR_ROUND(round);
    CUISHEN768_PAIR_ROUND(round + 1);
    CUISHEN768_PAIR_ROUND(round + 2);
    CUISHEN768_PAIR_ROUND(round + 3);
    CUISHEN768_PAIR_ROUND(round + 4);
    CUISHEN768_PAIR_ROUND(round + 5);
    CUISHEN768_PAIR_ROUND(round + 6);
    CUISHEN768_PAIR_ROUND(round + 7);
  }

  out_x[0] = x0;
  out_x[1] = x1;
  out_x[2] = x2;
  out_x[3] = x3;
  out_x[4] = x4;
  out_x[5] = x5;
  out_x[6] = x6;
  out_x[7] = x7;
  out_y[0] = y0;
  out_y[1] = y1;
  out_y[2] = y2;
  out_y[3] = y3;
  out_y[4] = y4;
  out_y[5] = y5;
  out_y[6] = y6;
  out_y[7] = y7;
}

#undef CUISHEN768_PAIR_ROUND

static inline void store_digest_be(unsigned char digest[96],
                                   const uint64_t top[8],
                                   const uint64_t bottom[8]) {
  store_u64x2_be(digest + 0, make_u64x2(top[0], top[1]));
  store_u64x2_be(digest + 16, make_u64x2(top[2], top[3]));
  store_u64x2_be(digest + 32, make_u64x2(top[4], top[5]));
  store_u64x2_be(digest + 48, make_u64x2(top[6], top[7]));
  store_u64x2_be(digest + 64, make_u64x2(bottom[0], bottom[1]));
  store_u64x2_be(digest + 80, make_u64x2(bottom[2], bottom[3]));
}

__attribute__((always_inline)) static inline void
compress_one_block(uint64_t t[8], uint64_t b[8],
                   const uint64_t m[MESSAGE_WORDS],
                   unsigned long long msg_bits_processed) {
  const uint64_t t0 = t[0];
  const uint64_t t1 = t[1];
  const uint64_t t2 = t[2];
  const uint64_t t3 = t[3];
  const uint64_t t4 = t[4];
  const uint64_t t5 = t[5];
  const uint64_t t6 = t[6];
  const uint64_t t7 = t[7];

  encrypt_pair_same_key(t0, t1, t2, t3, t4, t5, t6,
                        t7, t0, t1, t2, t3,
                        t4, t5, t6,
                        t7 ^ 1ULL, m, b, 0,
                        (uint64_t)msg_bits_processed, t, b);
  t[0] ^= t0;
  t[1] ^= t1;
  t[2] ^= t2;
  t[3] ^= t3;
  t[4] ^= t4;
  t[5] ^= t5;
  t[6] ^= t6;
  t[7] ^= t7;
  b[0] ^= t0;
  b[1] ^= t1;
  b[2] ^= t2;
  b[3] ^= t3;
  b[4] ^= t4;
  b[5] ^= t5;
  b[6] ^= t6;
  b[7] ^= t7 ^ 1ULL;
}

static void encrypt_final_key(const uint64_t final_key[MASTER_KEY_WORDS],
                              unsigned long long counter_bits,
                              unsigned char digest[96]) {
  uint64_t top[8];
  uint64_t bottom[8];

  encrypt_pair_same_key(0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 3,
                        final_key, final_key + MESSAGE_WORDS, 0,
                        (uint64_t)counter_bits, top, bottom);
  store_digest_be(digest, top, bottom);
}

static void encrypt_final_empty_tail(const uint64_t t[8], const uint64_t b[8],
                                     unsigned long long counter_bits,
                                     unsigned char digest[96]) {
  uint64_t final_key[MESSAGE_WORDS];
  uint64_t zero_key[LINK_WORDS] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint64_t top[8];
  uint64_t bottom[8];
  int i;

  for (i = 0; i < 8; ++i) {
    final_key[i] = t[i];
    final_key[8 + i] = b[i];
  }
  encrypt_pair_same_key(0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 3,
                        final_key, zero_key, 0, (uint64_t)counter_bits, top,
                        bottom);
  store_digest_be(digest, top, bottom);
}

static void finalization(const uint64_t t[8], const uint64_t b[8],
                         const unsigned char *tail_msg,
                         unsigned long long tail_msg_bits,
                         unsigned long long counter_bits,
                         unsigned char digest[96]) {
  uint64_t final_key[MASTER_KEY_WORDS];

  if (tail_msg_bits == 0ULL) {
    encrypt_final_empty_tail(t, b, counter_bits, digest);
    return;
  }

  vst1q_u64(final_key + 0, vld1q_u64(t + 0));
  vst1q_u64(final_key + 2, vld1q_u64(t + 2));
  vst1q_u64(final_key + 4, vld1q_u64(t + 4));
  vst1q_u64(final_key + 6, vld1q_u64(t + 6));
  vst1q_u64(final_key + 8, vld1q_u64(b + 0));
  vst1q_u64(final_key + 10, vld1q_u64(b + 2));
  vst1q_u64(final_key + 12, vld1q_u64(b + 4));
  vst1q_u64(final_key + 14, vld1q_u64(b + 6));

  if ((tail_msg_bits & 63ULL) == 0ULL) {
    load_aligned_tail_be(tail_msg, (unsigned int)(tail_msg_bits >> 6),
                         final_key + FINAL_PREFIX_WORDS,
                         MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);
  } else {
    load_partial_be(tail_msg, tail_msg_bits, final_key + FINAL_PREFIX_WORDS,
                    MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);
  }

  encrypt_final_key(final_key, counter_bits, digest);
}

static void finalization_initial(const unsigned char *tail_msg,
                                 unsigned long long tail_msg_bits,
                                 unsigned long long counter_bits,
                                 unsigned char digest[96]) {
  uint64_t final_key[MASTER_KEY_WORDS];

  if (tail_msg_bits == 0ULL) {
    encrypt_final_empty_tail(IV_1, IV_2, counter_bits, digest);
    return;
  }

  vst1q_u64(final_key + 0, vld1q_u64(IV_1 + 0));
  vst1q_u64(final_key + 2, vld1q_u64(IV_1 + 2));
  vst1q_u64(final_key + 4, vld1q_u64(IV_1 + 4));
  vst1q_u64(final_key + 6, vld1q_u64(IV_1 + 6));
  vst1q_u64(final_key + 8, vld1q_u64(IV_2 + 0));
  vst1q_u64(final_key + 10, vld1q_u64(IV_2 + 2));
  vst1q_u64(final_key + 12, vld1q_u64(IV_2 + 4));
  vst1q_u64(final_key + 14, vld1q_u64(IV_2 + 6));

  if ((tail_msg_bits & 63ULL) == 0ULL) {
    load_aligned_tail_be(tail_msg, (unsigned int)(tail_msg_bits >> 6),
                         final_key + FINAL_PREFIX_WORDS,
                         MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);
  } else {
    load_partial_be(tail_msg, tail_msg_bits, final_key + FINAL_PREFIX_WORDS,
                    MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);
  }

  encrypt_final_key(final_key, counter_bits, digest);
}

CUISHEN_ARM_IMPL_ATTR("+nosha3")
int cuishen768_arm_nosha3_impl(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
  uint64_t t[8];
  uint64_t b[8];
  uint64_t m[MESSAGE_WORDS];
  unsigned long long full_blocks;
  unsigned long long rem_bits;
  unsigned long long block;
  unsigned long long processed_bits;
  const unsigned char *p;
  int i;

  if (digest_len_bits != DIGEST_BIT_LENGTH || digest == 0) {
    return -1;
  }
  if (msg_len_bits != 0ULL && msg == 0) {
    return -1;
  }

  if (msg_len_bits <= FINAL_TAIL_BITS) {
    finalization_initial(msg, msg_len_bits, msg_len_bits, digest);
    return 0;
  }

  for (i = 0; i < 8; ++i) {
    t[i] = IV_1[i];
    b[i] = IV_2[i];
  }

  full_blocks = msg_len_bits >> 10;
  rem_bits = msg_len_bits & 1023ULL;
  p = msg;
  processed_bits = MESSAGE_BLOCK_BITS;

  for (block = 0ULL; block < full_blocks; ++block) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(p + 128, 0, 3);
#endif
    load1024_be(p, m);
    compress_one_block(t, b, m, processed_bits);
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
