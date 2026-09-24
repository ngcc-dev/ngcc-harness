/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

ARM NEON/SHA3-XAR optimized Cuishen-768 implementation.
*/

#if defined(__GNUC__) && !defined(__clang__) && !defined(__ARM_FEATURE_SHA3)
#pragma GCC target("+sha3")
#endif

#include "CryptHash_Cuishen-768.h"
#include <arm_neon.h>
#include <stdint.h>

#if !defined(__ARM_FEATURE_SHA3)
#error "CryptHash_Cuishen-768-c-neon.c requires ARM SHA3/XAR support"
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

#define ROTL64X2_XOR_SCALAR_KEY(X, K, N)                                    \
  vxarq_u64((X), vdupq_n_u64((K)), 64U - (N))

/*
 * Each uint64x2_t lane carries one of the c-opt top/bottom encryptions.
 * The 1536-bit v3 key schedule stays scalar, while the eight state words use
 * SHA3/XAR for rotl64(state ^ key, n).
 */
#define CUISHEN768_PAIR_ROUND_NEON()                                        \
  do {                                                                       \
    const uint64_t ec = *ec_ptr++;                                           \
    const uint64_t rk0 = m0 ^ bk0 ^ c0;                                      \
    const uint64_t rk1 = m1 ^ bk1 ^ c1;                                      \
    const uint64_t rk2 = m8 ^ bk2;                                           \
    const uint64_t rk3 = m9 ^ bk3;                                           \
    const uint64_t rk4 = bk4;                                                \
    const uint64_t rk5 = bk5;                                                \
    const uint64_t rk6 = bk6;                                                \
    const uint64_t rk7 = bk7;                                                \
    const uint64x2_t v45 = veorq_u64(vx4, vx5);                              \
    const uint64x2_t v67 = veorq_u64(vx6, vx7);                              \
    const uint64x2_t vt = veorq_u64(v45, v67);                               \
    const uint64x2_t nx4 = vaddq_u64(                                        \
        ROTL64X2_XOR_SCALAR_KEY(vx0, rk0 ^ ec, A),                           \
        ROTL64X2_XOR_SCALAR_KEY(vt, rk7, H));                                \
    const uint64x2_t nx5 = vaddq_u64(                                        \
        ROTL64X2_XOR_SCALAR_KEY(vx1, rk1, B),                                \
        ROTL64X2_XOR_SCALAR_KEY(vt, rk6, G));                                \
    const uint64x2_t nx6 = vaddq_u64(                                        \
        ROTL64X2_XOR_SCALAR_KEY(vx2, rk2, C),                                \
        ROTL64X2_XOR_SCALAR_KEY(vt, rk5, F));                                \
    const uint64x2_t nx7 = vaddq_u64(                                        \
        ROTL64X2_XOR_SCALAR_KEY(vx3, rk3, D),                                \
        ROTL64X2_XOR_SCALAR_KEY(vt, rk4, E));                                \
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
    const uint64_t nc1 = rotl64(c0, 2);                                      \
    vx0 = vx4;                                                               \
    vx1 = vx5;                                                               \
    vx2 = vx6;                                                               \
    vx3 = vx7;                                                               \
    vx4 = nx4;                                                               \
    vx5 = nx5;                                                               \
    vx6 = nx6;                                                               \
    vx7 = nx7;                                                               \
    m0 = m6;                                                                 \
    m1 = m7;                                                                 \
    m2 = nm2;                                                                \
    m3 = nm3;                                                                \
    m4 = m10;                                                               \
    m5 = m11;                                                               \
    m6 = m12;                                                               \
    m7 = m13;                                                               \
    m8 = m14;                                                               \
    m9 = m15;                                                               \
    m10 = nm10;                                                             \
    m11 = nm11;                                                             \
    m12 = pm2;                                                              \
    m13 = pm3;                                                              \
    m14 = pm4;                                                              \
    m15 = pm5;                                                              \
    bk0 = bk1;                                                              \
    bk1 = bk2;                                                              \
    bk2 = bk3;                                                              \
    bk3 = bk4;                                                              \
    bk4 = bk5;                                                              \
    bk5 = bk6;                                                              \
    bk6 = bk7;                                                              \
    bk7 = nb7;                                                              \
    c0 = c1;                                                                \
    c1 = nc1;                                                               \
  } while (0)

__attribute__((always_inline)) static inline void encrypt_pair_same_key(
    uint64x2_t vx0, uint64x2_t vx1, uint64x2_t vx2, uint64x2_t vx3,
    uint64x2_t vx4, uint64x2_t vx5, uint64x2_t vx6, uint64x2_t vx7,
    const uint64_t message_key[MESSAGE_WORDS],
    const uint64_t block_key[LINK_WORDS], uint64_t counter1,
    uint64x2_t out[8]) {
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
  uint64_t c0 = 0;
  uint64_t c1 = counter1;
  const uint64_t *ec_ptr = E_CONST;
  const uint64_t *const ec_end = E_CONST + ROUNDS;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (; ec_ptr != ec_end;) {
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
  }

  out[0] = vx0;
  out[1] = vx1;
  out[2] = vx2;
  out[3] = vx3;
  out[4] = vx4;
  out[5] = vx5;
  out[6] = vx6;
  out[7] = vx7;
}

__attribute__((always_inline)) static inline void
encrypt_final_empty_tail(const uint64_t t[8], const uint64_t b[8],
                         uint64_t counter1, uint64x2_t out[8]) {
  uint64x2_t vx0 = vdupq_n_u64(0);
  uint64x2_t vx1 = vdupq_n_u64(0);
  uint64x2_t vx2 = vdupq_n_u64(0);
  uint64x2_t vx3 = vdupq_n_u64(0);
  uint64x2_t vx4 = vdupq_n_u64(0);
  uint64x2_t vx5 = vdupq_n_u64(0);
  uint64x2_t vx6 = vdupq_n_u64(0);
  uint64x2_t vx7 = make_u64x2(2, 3);
  uint64_t m0 = t[0];
  uint64_t m1 = t[1];
  uint64_t m2 = t[2];
  uint64_t m3 = t[3];
  uint64_t m4 = t[4];
  uint64_t m5 = t[5];
  uint64_t m6 = t[6];
  uint64_t m7 = t[7];
  uint64_t m8 = b[0];
  uint64_t m9 = b[1];
  uint64_t m10 = b[2];
  uint64_t m11 = b[3];
  uint64_t m12 = b[4];
  uint64_t m13 = b[5];
  uint64_t m14 = b[6];
  uint64_t m15 = b[7];
  uint64_t bk0 = 0;
  uint64_t bk1 = 0;
  uint64_t bk2 = 0;
  uint64_t bk3 = 0;
  uint64_t bk4 = 0;
  uint64_t bk5 = 0;
  uint64_t bk6 = 0;
  uint64_t bk7 = 0;
  uint64_t c0 = 0;
  uint64_t c1 = counter1;
  const uint64_t *ec_ptr = E_CONST;
  const uint64_t *const ec_end = E_CONST + ROUNDS;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (; ec_ptr != ec_end;) {
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
  }

  out[0] = vx0;
  out[1] = vx1;
  out[2] = vx2;
  out[3] = vx3;
  out[4] = vx4;
  out[5] = vx5;
  out[6] = vx6;
  out[7] = vx7;
}

__attribute__((always_inline)) static inline void
encrypt_final_empty_tail_pair(const uint64x2_t pair_state[8],
                              uint64_t counter1, uint64x2_t out[8]) {
  uint64x2_t vx0 = vdupq_n_u64(0);
  uint64x2_t vx1 = vdupq_n_u64(0);
  uint64x2_t vx2 = vdupq_n_u64(0);
  uint64x2_t vx3 = vdupq_n_u64(0);
  uint64x2_t vx4 = vdupq_n_u64(0);
  uint64x2_t vx5 = vdupq_n_u64(0);
  uint64x2_t vx6 = vdupq_n_u64(0);
  uint64x2_t vx7 = make_u64x2(2, 3);
  uint64_t m0 = vgetq_lane_u64(pair_state[0], 0);
  uint64_t m1 = vgetq_lane_u64(pair_state[1], 0);
  uint64_t m2 = vgetq_lane_u64(pair_state[2], 0);
  uint64_t m3 = vgetq_lane_u64(pair_state[3], 0);
  uint64_t m4 = vgetq_lane_u64(pair_state[4], 0);
  uint64_t m5 = vgetq_lane_u64(pair_state[5], 0);
  uint64_t m6 = vgetq_lane_u64(pair_state[6], 0);
  uint64_t m7 = vgetq_lane_u64(pair_state[7], 0);
  uint64_t m8 = vgetq_lane_u64(pair_state[0], 1);
  uint64_t m9 = vgetq_lane_u64(pair_state[1], 1);
  uint64_t m10 = vgetq_lane_u64(pair_state[2], 1);
  uint64_t m11 = vgetq_lane_u64(pair_state[3], 1);
  uint64_t m12 = vgetq_lane_u64(pair_state[4], 1);
  uint64_t m13 = vgetq_lane_u64(pair_state[5], 1);
  uint64_t m14 = vgetq_lane_u64(pair_state[6], 1);
  uint64_t m15 = vgetq_lane_u64(pair_state[7], 1);
  uint64_t bk0 = 0;
  uint64_t bk1 = 0;
  uint64_t bk2 = 0;
  uint64_t bk3 = 0;
  uint64_t bk4 = 0;
  uint64_t bk5 = 0;
  uint64_t bk6 = 0;
  uint64_t bk7 = 0;
  uint64_t c0 = 0;
  uint64_t c1 = counter1;
  const uint64_t *ec_ptr = E_CONST;
  const uint64_t *const ec_end = E_CONST + ROUNDS;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (; ec_ptr != ec_end;) {
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
    CUISHEN768_PAIR_ROUND_NEON();
  }

  out[0] = vx0;
  out[1] = vx1;
  out[2] = vx2;
  out[3] = vx3;
  out[4] = vx4;
  out[5] = vx5;
  out[6] = vx6;
  out[7] = vx7;
}

#undef CUISHEN768_PAIR_ROUND_NEON
#undef ROTL64X2_XOR_SCALAR_KEY

static inline void store_pair_digest_be(const uint64x2_t state[8],
                                        unsigned char digest[96]) {
  store_u64x2_be(digest + 0, vzip1q_u64(state[0], state[1]));
  store_u64x2_be(digest + 16, vzip1q_u64(state[2], state[3]));
  store_u64x2_be(digest + 32, vzip1q_u64(state[4], state[5]));
  store_u64x2_be(digest + 48, vzip1q_u64(state[6], state[7]));
  store_u64x2_be(digest + 64, vzip2q_u64(state[0], state[1]));
  store_u64x2_be(digest + 80, vzip2q_u64(state[2], state[3]));
}

static inline void store_pair_bottom_words(const uint64x2_t state[8],
                                           uint64_t bottom[8]) {
  vst1q_u64(bottom + 0, vzip2q_u64(state[0], state[1]));
  vst1q_u64(bottom + 2, vzip2q_u64(state[2], state[3]));
  vst1q_u64(bottom + 4, vzip2q_u64(state[4], state[5]));
  vst1q_u64(bottom + 6, vzip2q_u64(state[6], state[7]));
}

static inline void store_final_key_prefix_from_pair_state(
    const uint64x2_t pair_state[8], uint64_t final_key[MASTER_KEY_WORDS]) {
  vst1q_u64(final_key + 0, vzip1q_u64(pair_state[0], pair_state[1]));
  vst1q_u64(final_key + 2, vzip1q_u64(pair_state[2], pair_state[3]));
  vst1q_u64(final_key + 4, vzip1q_u64(pair_state[4], pair_state[5]));
  vst1q_u64(final_key + 6, vzip1q_u64(pair_state[6], pair_state[7]));
  vst1q_u64(final_key + 8, vzip2q_u64(pair_state[0], pair_state[1]));
  vst1q_u64(final_key + 10, vzip2q_u64(pair_state[2], pair_state[3]));
  vst1q_u64(final_key + 12, vzip2q_u64(pair_state[4], pair_state[5]));
  vst1q_u64(final_key + 14, vzip2q_u64(pair_state[6], pair_state[7]));
}

static inline void init_initial_pair_state(uint64x2_t state[8]) {
  state[0] = make_u64x2(IV_1[0], IV_2[0]);
  state[1] = make_u64x2(IV_1[1], IV_2[1]);
  state[2] = make_u64x2(IV_1[2], IV_2[2]);
  state[3] = make_u64x2(IV_1[3], IV_2[3]);
  state[4] = make_u64x2(IV_1[4], IV_2[4]);
  state[5] = make_u64x2(IV_1[5], IV_2[5]);
  state[6] = make_u64x2(IV_1[6], IV_2[6]);
  state[7] = make_u64x2(IV_1[7], IV_2[7]);
}

static inline void init_compress_state_from_pair(const uint64x2_t pair_state[8],
                                                 uint64x2_t state[8]) {
  const uint64_t t7 = vgetq_lane_u64(pair_state[7], 0);

  state[0] = vdupq_laneq_u64(pair_state[0], 0);
  state[1] = vdupq_laneq_u64(pair_state[1], 0);
  state[2] = vdupq_laneq_u64(pair_state[2], 0);
  state[3] = vdupq_laneq_u64(pair_state[3], 0);
  state[4] = vdupq_laneq_u64(pair_state[4], 0);
  state[5] = vdupq_laneq_u64(pair_state[5], 0);
  state[6] = vdupq_laneq_u64(pair_state[6], 0);
  state[7] = make_u64x2(t7,
                        t7 ^ 1ULL);
}

__attribute__((always_inline)) static inline void
compress_pair_state(uint64x2_t pair_state[8],
                    const uint64_t m[MESSAGE_WORDS],
                    unsigned long long msg_bits_processed) {
  uint64_t b[LINK_WORDS];
  uint64x2_t state[8];

  store_pair_bottom_words(pair_state, b);
  init_compress_state_from_pair(pair_state, state);
  encrypt_pair_same_key(state[0], state[1], state[2], state[3], state[4],
                        state[5], state[6], state[7], m, b,
                        (uint64_t)msg_bits_processed, pair_state);
  pair_state[0] = veorq_u64(pair_state[0], state[0]);
  pair_state[1] = veorq_u64(pair_state[1], state[1]);
  pair_state[2] = veorq_u64(pair_state[2], state[2]);
  pair_state[3] = veorq_u64(pair_state[3], state[3]);
  pair_state[4] = veorq_u64(pair_state[4], state[4]);
  pair_state[5] = veorq_u64(pair_state[5], state[5]);
  pair_state[6] = veorq_u64(pair_state[6], state[6]);
  pair_state[7] = veorq_u64(pair_state[7], state[7]);
}

static void finalization_pair_state(const uint64x2_t pair_state[8],
                                    const unsigned char *tail_msg,
                                    unsigned long long tail_msg_bits,
                                    unsigned long long counter_bits,
                                    unsigned char digest[96]) {
  uint64_t final_key[MASTER_KEY_WORDS];
  uint64x2_t state[8];

  if (tail_msg_bits == 0ULL) {
    encrypt_final_empty_tail_pair(pair_state, (uint64_t)counter_bits, state);
    store_pair_digest_be(state, digest);
    return;
  }

  store_final_key_prefix_from_pair_state(pair_state, final_key);
  if ((tail_msg_bits & 63ULL) == 0ULL) {
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
  state[3] = vdupq_n_u64(0);
  state[4] = vdupq_n_u64(0);
  state[5] = vdupq_n_u64(0);
  state[6] = vdupq_n_u64(0);
  state[7] = make_u64x2(2, 3);

  encrypt_pair_same_key(state[0], state[1], state[2], state[3], state[4],
                        state[5], state[6], state[7], final_key,
                        final_key + MESSAGE_WORDS, (uint64_t)counter_bits,
                        state);
  store_pair_digest_be(state, digest);
}

static void finalization_initial(const unsigned char *tail_msg,
                                 unsigned long long tail_msg_bits,
                                 unsigned long long counter_bits,
                                 unsigned char digest[96]) {
  uint64_t final_key[MASTER_KEY_WORDS];
  uint64x2_t state[8];

  if (tail_msg_bits == 0ULL) {
    encrypt_final_empty_tail(IV_1, IV_2, (uint64_t)counter_bits, state);
    store_pair_digest_be(state, digest);
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

  state[0] = vdupq_n_u64(0);
  state[1] = vdupq_n_u64(0);
  state[2] = vdupq_n_u64(0);
  state[3] = vdupq_n_u64(0);
  state[4] = vdupq_n_u64(0);
  state[5] = vdupq_n_u64(0);
  state[6] = vdupq_n_u64(0);
  state[7] = make_u64x2(2, 3);

  encrypt_pair_same_key(state[0], state[1], state[2], state[3], state[4],
                        state[5], state[6], state[7], final_key,
                        final_key + MESSAGE_WORDS, (uint64_t)counter_bits,
                        state);
  store_pair_digest_be(state, digest);
}

__attribute__((always_inline)) static inline int
crypt_hash_stream_path(const unsigned char *msg,
                       unsigned long long msg_len_bits,
                       unsigned long long full_blocks,
                       unsigned long long rem_bits, unsigned char *digest) {
  uint64x2_t pair_state[8];
  uint64_t m[MESSAGE_WORDS];
  unsigned long long block;
  unsigned long long processed_bits = MESSAGE_BLOCK_BITS;
  const unsigned char *p = msg;

  init_initial_pair_state(pair_state);
  for (block = 0ULL; block < full_blocks; ++block) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(p + 128, 0, 3);
#endif
    load1024_be(p, m);
    compress_pair_state(pair_state, m, processed_bits);
    p += 128;
    processed_bits += MESSAGE_BLOCK_BITS;
  }

  if (rem_bits > FINAL_TAIL_BITS) {
    load_partial_be(p, rem_bits, m, MESSAGE_WORDS);
    compress_pair_state(pair_state, m, msg_len_bits);
    p = 0;
    rem_bits = 0;
  }

  finalization_pair_state(pair_state, p, rem_bits, msg_len_bits, digest);
  return 0;
}

CUISHEN_ARM_IMPL_ATTR("+sha3")
int cuishen768_arm_sha3_impl(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
  unsigned long long full_blocks;
  unsigned long long rem_bits;

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

  full_blocks = msg_len_bits >> 10;
  rem_bits = msg_len_bits & 1023ULL;

  return crypt_hash_stream_path(msg, msg_len_bits, full_blocks, rem_bits,
                                digest);
}
