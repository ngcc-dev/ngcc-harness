/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

AMD/Zen AVX-512VL/VKS optimized Cuishen-768 implementation.
*/

#ifndef CUISHEN768_AVX512VL_STREAM_STATE
#define CUISHEN768_AVX512VL_STREAM_STATE 1
#endif

#include "CryptHash_Cuishen-768.h"
#include <immintrin.h>
#include <stdint.h>

#if !defined(__AVX2__) || !defined(__AVX512F__) || !defined(__AVX512VL__)
#error "This file requires AVX2, AVX-512F, and AVX-512VL."
#endif

#ifndef CUISHEN768_AVX512VL_ALIGNED_STACK
#if defined(__GNUC__) && !defined(__clang__)
#define CUISHEN768_AVX512VL_ALIGNED_STACK 1
#else
#define CUISHEN768_AVX512VL_ALIGNED_STACK 0
#endif
#endif

#if CUISHEN768_AVX512VL_ALIGNED_STACK && (defined(__GNUC__) || defined(__clang__))
#define CUISHEN768_ALIGN32 __attribute__((aligned(32)))
#else
#define CUISHEN768_ALIGN32
#endif

#if CUISHEN768_AVX512VL_ALIGNED_STACK
#define CUISHEN768_STORE_U64X4(P, V)                                       \
  _mm256_store_si256((__m256i *)(void *)(P), (V))
#define CUISHEN768_LOAD_U64X4(P)                                           \
  _mm256_load_si256((const __m256i *)(const void *)(P))
#else
#define CUISHEN768_STORE_U64X4(P, V)                                       \
  _mm256_storeu_si256((__m256i *)(void *)(P), (V))
#define CUISHEN768_LOAD_U64X4(P)                                           \
  _mm256_loadu_si256((const __m256i *)(const void *)(P))
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

#ifndef CUISHEN768_AVX512VL_E_VOLATILE
#if defined(__clang__)
#define CUISHEN768_AVX512VL_E_VOLATILE 1
#else
#define CUISHEN768_AVX512VL_E_VOLATILE 0
#endif
#endif

#if CUISHEN768_AVX512VL_E_VOLATILE
#define CUISHEN768_AVX512VL_E_QUAL volatile
#else
#define CUISHEN768_AVX512VL_E_QUAL
#endif

#ifndef CUISHEN768_AVX512VL_STREAM_STATE
#if defined(__GNUC__) && !defined(__clang__)
#define CUISHEN768_AVX512VL_STREAM_STATE 1
#else
#define CUISHEN768_AVX512VL_STREAM_STATE 0
#endif
#endif

#ifndef CUISHEN768_AVX512VL_ROUNDS_PER_LOOP
#if defined(__GNUC__) && !defined(__clang__)
#define CUISHEN768_AVX512VL_ROUNDS_PER_LOOP 8
#else
#define CUISHEN768_AVX512VL_ROUNDS_PER_LOOP 4
#endif
#endif

#if CUISHEN768_AVX512VL_ROUNDS_PER_LOOP != 1 &&                                \
    CUISHEN768_AVX512VL_ROUNDS_PER_LOOP != 2 &&                                \
    CUISHEN768_AVX512VL_ROUNDS_PER_LOOP != 4 &&                                \
    CUISHEN768_AVX512VL_ROUNDS_PER_LOOP != 8 &&                                \
    CUISHEN768_AVX512VL_ROUNDS_PER_LOOP != 16 &&                               \
    CUISHEN768_AVX512VL_ROUNDS_PER_LOOP != 32
#error "CUISHEN768_AVX512VL_ROUNDS_PER_LOOP must be 1, 2, 4, 8, 16, or 32."
#endif

static const CUISHEN768_AVX512VL_E_QUAL uint64_t E_CONST[ROUNDS] = {
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

static inline __m256i make_u64x4(uint64_t lane0, uint64_t lane1,
                                 uint64_t lane2, uint64_t lane3) {
  return _mm256_setr_epi64x((long long)lane0, (long long)lane1,
                            (long long)lane2, (long long)lane3);
}

static inline uint64_t load64_be(const unsigned char *in) {
  return ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48) |
         ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32) |
         ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16) |
         ((uint64_t)in[6] << 8) | (uint64_t)in[7];
}

static inline __m256i reverse_u64_bytes(__m256i v) {
  const __m256i reverse_bytes =
      _mm256_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9,
                       8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9,
                       8);
  return _mm256_shuffle_epi8(v, reverse_bytes);
}

static inline __m256i load_u64x4_be(const unsigned char *in) {
  return reverse_u64_bytes(
      _mm256_loadu_si256((const __m256i *)(const void *)in));
}

static inline void store_u64x4_be(unsigned char *out, __m256i v) {
  _mm256_storeu_si256((__m256i *)(void *)out, reverse_u64_bytes(v));
}

static inline void load1024_be(const unsigned char *in,
                               uint64_t out[MESSAGE_WORDS]) {
  CUISHEN768_STORE_U64X4(out + 0, load_u64x4_be(in + 0));
  CUISHEN768_STORE_U64X4(out + 4, load_u64x4_be(in + 32));
  CUISHEN768_STORE_U64X4(out + 8, load_u64x4_be(in + 64));
  CUISHEN768_STORE_U64X4(out + 12, load_u64x4_be(in + 96));
}

__attribute__((always_inline)) static inline void
zero_words(uint64_t *out, unsigned int first_word, unsigned int out_words) {
  const __m256i zero = _mm256_setzero_si256();
  unsigned int word = first_word;

#if CUISHEN768_AVX512VL_ALIGNED_STACK
  for (; (word & 3U) != 0U && word < out_words; ++word) {
    out[word] = 0;
  }
#endif
  for (; word + 3U < out_words; word += 4U) {
    CUISHEN768_STORE_U64X4(out + word, zero);
  }
  for (; word < out_words; ++word) {
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

  for (; word + 3U < full_words; word += 4U) {
    CUISHEN768_STORE_U64X4(out + word, load_u64x4_be(tail + 8 * word));
  }
  for (; word < full_words; ++word) {
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
    zero_from = full_words + 1U;
  }

  zero_words(out, zero_from, out_words);
}

#define ROTL64X4(X, COUNTS) _mm256_rolv_epi64((X), (COUNTS))

#define SWAP128(X) _mm256_permute2x128_si256((X), (X), 0x01)

#if defined(__GNUC__) && !defined(__clang__)
#define CUISHEN768_VT_XOR(V45, V67)                                        \
  _mm256_ternarylogic_epi64(                                                \
      _mm256_xor_si256((V45), SWAP128(V45)), (V67), SWAP128(V67), 0x96)
#else
#define CUISHEN768_VT_XOR(V45, V67)                                        \
  _mm256_xor_si256(_mm256_xor_si256((V45), SWAP128(V45)),                   \
                   _mm256_xor_si256((V67), SWAP128(V67)))
#endif

#define CUISHEN768_PAIR_ROUND_AVX512VL_VKS()                                \
  do {                                                                       \
    const uint64_t ec = *ec_ptr++;                                           \
    const uint64_t rk4 = bk4;                                                \
    const uint64_t rk5 = bk5;                                                \
    const uint64_t rk6 = bk6;                                                \
    const uint64_t rk7 = bk7;                                                \
    const uint64_t c0e = c0 ^ ec;                                           \
    const __m256i qbk01 = make_u64x4(bk0 ^ c0e, bk0 ^ c0e, bk1 ^ c1,        \
                                     bk1 ^ c1);                             \
    const __m256i qbk23 = make_u64x4(bk2, bk2, bk3, bk3);                   \
    const __m256i vt = CUISHEN768_VT_XOR(v45, v67);                        \
    const __m256i n45 = _mm256_add_epi64(                                  \
        ROTL64X4(_mm256_ternarylogic_epi64(v01, qm01, qbk01, 0x96),         \
                 rot_ab_left),                                              \
        ROTL64X4(_mm256_xor_si256(vt, make_u64x4(rk7, rk7, rk6, rk6)),      \
                 rot_hg_left));                                             \
    const __m256i n67 = _mm256_add_epi64(                                  \
        ROTL64X4(_mm256_ternarylogic_epi64(v23, qm89, qbk23, 0x96),         \
                 rot_cd_left),                                              \
        ROTL64X4(_mm256_xor_si256(vt, make_u64x4(rk5, rk5, rk4, rk4)),      \
                 rot_fe_left));                                             \
    const __m256i pm23 = qm23;                                               \
    const __m256i pm45 = qm45;                                               \
    const __m256i qx23 = _mm256_xor_si256(qm23, SWAP128(qm23));             \
    const __m256i qx1011 = _mm256_xor_si256(qm1011, SWAP128(qm1011));       \
    const __m256i qnm23 = _mm256_add_epi64(                                \
        ROTL64X4(qm89, rot_m89_left),                                      \
        ROTL64X4(qx1011, rot_m1011_left));                                 \
    const __m256i qnm1011 = _mm256_add_epi64(                              \
        ROTL64X4(qm01, rot_m01_left),                                      \
        ROTL64X4(qx23, rot_m23_left));                                     \
    const uint64_t nb7 = rotl64(bk0, BA) ^ bk1;                              \
    const uint64_t nc1 = rotl64(c0, 2);                                      \
    v01 = v45;                                                               \
    v23 = v67;                                                               \
    v45 = n45;                                                               \
    v67 = n67;                                                               \
    qm01 = qm67;                                                             \
    qm23 = qnm23;                                                            \
    qm45 = qm1011;                                                           \
    qm67 = qm1213;                                                           \
    qm89 = qm1415;                                                           \
    qm1011 = qnm1011;                                                        \
    qm1213 = pm23;                                                           \
    qm1415 = pm45;                                                           \
    bk0 = bk1;                                                               \
    bk1 = bk2;                                                               \
    bk2 = bk3;                                                               \
    bk3 = bk4;                                                               \
    bk4 = bk5;                                                               \
    bk5 = bk6;                                                               \
    bk6 = bk7;                                                               \
    bk7 = nb7;                                                               \
    c0 = c1;                                                                 \
    c1 = nc1;                                                                \
  } while (0)

static __attribute__((always_inline)) inline void encrypt_pair_same_key(
    __m256i v01, __m256i v23, __m256i v45, __m256i v67,
    const uint64_t message_key[MESSAGE_WORDS],
    const uint64_t block_key[LINK_WORDS], uint64_t counter1, __m256i out[4]) {
  const __m256i rot_ab_left = make_u64x4(A, A, B, B);
  const __m256i rot_cd_left = make_u64x4(C, C, D, D);
  const __m256i rot_hg_left = make_u64x4(H, H, G, G);
  const __m256i rot_fe_left = make_u64x4(F, F, E, E);
  const __m256i rot_m89_left = make_u64x4(ME, ME, MF, MF);
  const __m256i rot_m1011_left = make_u64x4(MH, MH, MG, MG);
  const __m256i rot_m01_left = make_u64x4(MA, MA, MB, MB);
  const __m256i rot_m23_left = make_u64x4(MD, MD, MC, MC);
  __m256i qm01 = make_u64x4(message_key[0], message_key[0], message_key[1],
                            message_key[1]);
  __m256i qm23 = make_u64x4(message_key[2], message_key[2], message_key[3],
                            message_key[3]);
  __m256i qm45 = make_u64x4(message_key[4], message_key[4], message_key[5],
                            message_key[5]);
  __m256i qm67 = make_u64x4(message_key[6], message_key[6], message_key[7],
                            message_key[7]);
  __m256i qm89 = make_u64x4(message_key[8], message_key[8], message_key[9],
                            message_key[9]);
  __m256i qm1011 = make_u64x4(message_key[10], message_key[10],
                              message_key[11], message_key[11]);
  __m256i qm1213 = make_u64x4(message_key[12], message_key[12],
                              message_key[13], message_key[13]);
  __m256i qm1415 = make_u64x4(message_key[14], message_key[14],
                              message_key[15], message_key[15]);
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
  const CUISHEN768_AVX512VL_E_QUAL uint64_t *ec_ptr = E_CONST;
  const CUISHEN768_AVX512VL_E_QUAL uint64_t *const ec_end = E_CONST + ROUNDS;

  for (; ec_ptr != ec_end;) {
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
#if CUISHEN768_AVX512VL_ROUNDS_PER_LOOP >= 2
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
#endif
#if CUISHEN768_AVX512VL_ROUNDS_PER_LOOP >= 4
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
#endif
#if CUISHEN768_AVX512VL_ROUNDS_PER_LOOP >= 8
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
#endif
#if CUISHEN768_AVX512VL_ROUNDS_PER_LOOP >= 16
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
#endif
#if CUISHEN768_AVX512VL_ROUNDS_PER_LOOP >= 32
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
    CUISHEN768_PAIR_ROUND_AVX512VL_VKS();
#endif
  }

  out[0] = v01;
  out[1] = v23;
  out[2] = v45;
  out[3] = v67;
}

#undef CUISHEN768_PAIR_ROUND_AVX512VL_VKS
#undef CUISHEN768_VT_XOR
#undef SWAP128
#undef ROTL64X4

static inline void split_pair_state(const __m256i state[4], __m256i *top_lo,
                                    __m256i *top_hi, __m256i *bot_lo,
                                    __m256i *bot_hi) {
  *top_lo = _mm256_permute4x64_epi64(
      _mm256_unpacklo_epi64(state[0], state[1]), 0xD8);
  *bot_lo = _mm256_permute4x64_epi64(
      _mm256_unpackhi_epi64(state[0], state[1]), 0xD8);
  *top_hi = _mm256_permute4x64_epi64(
      _mm256_unpacklo_epi64(state[2], state[3]), 0xD8);
  *bot_hi = _mm256_permute4x64_epi64(
      _mm256_unpackhi_epi64(state[2], state[3]), 0xD8);
}

#if !CUISHEN768_AVX512VL_STREAM_STATE
static inline void store_pair_outputs(const __m256i state[4], uint64_t top[8],
                                      uint64_t bottom[8]) {
  __m256i top_lo;
  __m256i top_hi;
  __m256i bot_lo;
  __m256i bot_hi;

  split_pair_state(state, &top_lo, &top_hi, &bot_lo, &bot_hi);

  CUISHEN768_STORE_U64X4(top + 0, top_lo);
  CUISHEN768_STORE_U64X4(top + 4, top_hi);
  CUISHEN768_STORE_U64X4(bottom + 0, bot_lo);
  CUISHEN768_STORE_U64X4(bottom + 4, bot_hi);
}
#endif

#if CUISHEN768_AVX512VL_STREAM_STATE
static inline void init_compress_state_from_top(__m256i top_lo, __m256i top_hi,
                                                __m256i state[4]) {
  const __m256i domain_xor = make_u64x4(0, 0, 0, 1);

  state[0] = _mm256_permute4x64_epi64(top_lo, 0x50);
  state[1] = _mm256_permute4x64_epi64(top_lo, 0xFA);
  state[2] = _mm256_permute4x64_epi64(top_hi, 0x50);
  state[3] =
      _mm256_xor_si256(_mm256_permute4x64_epi64(top_hi, 0xFA), domain_xor);
}

static inline void init_initial_pair_state(__m256i state[4]) {
  state[0] = make_u64x4(IV_1[0], IV_2[0], IV_1[1], IV_2[1]);
  state[1] = make_u64x4(IV_1[2], IV_2[2], IV_1[3], IV_2[3]);
  state[2] = make_u64x4(IV_1[4], IV_2[4], IV_1[5], IV_2[5]);
  state[3] = make_u64x4(IV_1[6], IV_2[6], IV_1[7], IV_2[7]);
}
#endif

static inline void store_pair_digest_be(const __m256i state[4],
                                        unsigned char digest[96]) {
  __m256i top_lo;
  __m256i top_hi;
  __m256i bot_lo;
  __m256i bot_hi;

  split_pair_state(state, &top_lo, &top_hi, &bot_lo, &bot_hi);

  store_u64x4_be(digest + 0, top_lo);
  store_u64x4_be(digest + 32, top_hi);
  store_u64x4_be(digest + 64, bot_lo);
}

#if CUISHEN768_AVX512VL_STREAM_STATE
__attribute__((always_inline)) static inline void
compress_pair_state(__m256i pair_state[4], const uint64_t m[MESSAGE_WORDS],
                    unsigned long long msg_bits_processed) {
  uint64_t b[LINK_WORDS] CUISHEN768_ALIGN32;
  __m256i top_lo;
  __m256i top_hi;
  __m256i bot_lo;
  __m256i bot_hi;
  __m256i state[4];

  split_pair_state(pair_state, &top_lo, &top_hi, &bot_lo, &bot_hi);
  CUISHEN768_STORE_U64X4(b + 0, bot_lo);
  CUISHEN768_STORE_U64X4(b + 4, bot_hi);
  init_compress_state_from_top(top_lo, top_hi, state);

  encrypt_pair_same_key(state[0], state[1], state[2], state[3], m, b,
                        (uint64_t)msg_bits_processed, pair_state);
  pair_state[0] = _mm256_xor_si256(pair_state[0], state[0]);
  pair_state[1] = _mm256_xor_si256(pair_state[1], state[1]);
  pair_state[2] = _mm256_xor_si256(pair_state[2], state[2]);
  pair_state[3] = _mm256_xor_si256(pair_state[3], state[3]);
}

static void finalization(const __m256i pair_state[4],
                         const unsigned char *tail_msg,
                         unsigned long long tail_msg_bits,
                         unsigned long long counter_bits,
                         unsigned char digest[96]) {
  uint64_t final_key[MASTER_KEY_WORDS] CUISHEN768_ALIGN32;
  __m256i top_lo;
  __m256i top_hi;
  __m256i bot_lo;
  __m256i bot_hi;
  __m256i state[4];

  split_pair_state(pair_state, &top_lo, &top_hi, &bot_lo, &bot_hi);
  CUISHEN768_STORE_U64X4(final_key + 0, top_lo);
  CUISHEN768_STORE_U64X4(final_key + 4, top_hi);
  CUISHEN768_STORE_U64X4(final_key + 8, bot_lo);
  CUISHEN768_STORE_U64X4(final_key + 12, bot_hi);
  load_partial_be(tail_msg, tail_msg_bits, final_key + FINAL_PREFIX_WORDS,
                  MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);

  state[0] = _mm256_setzero_si256();
  state[1] = _mm256_setzero_si256();
  state[2] = _mm256_setzero_si256();
  state[3] = make_u64x4(0, 0, 2, 3);
  encrypt_pair_same_key(state[0], state[1], state[2], state[3], final_key,
                        final_key + MESSAGE_WORDS, (uint64_t)counter_bits,
                        state);
  store_pair_digest_be(state, digest);
}
#else
__attribute__((always_inline)) static inline void
compress_one_block(uint64_t t[8], uint64_t b[8],
                   const uint64_t m[MESSAGE_WORDS],
                   unsigned long long msg_bits_processed) {
  __m256i state[4];
  const __m256i input0 = make_u64x4(t[0], t[0], t[1], t[1]);
  const __m256i input1 = make_u64x4(t[2], t[2], t[3], t[3]);
  const __m256i input2 = make_u64x4(t[4], t[4], t[5], t[5]);
  const __m256i input3 = make_u64x4(t[6], t[6], t[7],
                                    t[7] ^ 1ULL);

  encrypt_pair_same_key(input0, input1, input2, input3, m, b,
                        (uint64_t)msg_bits_processed, state);
  state[0] = _mm256_xor_si256(state[0], input0);
  state[1] = _mm256_xor_si256(state[1], input1);
  state[2] = _mm256_xor_si256(state[2], input2);
  state[3] = _mm256_xor_si256(state[3], input3);
  store_pair_outputs(state, t, b);
}

static void compress_full_block_be(uint64_t t[8], uint64_t b[8],
                                   const unsigned char *msg,
                                   unsigned long long msg_bits_processed) {
  uint64_t m[MESSAGE_WORDS] CUISHEN768_ALIGN32;

  load1024_be(msg, m);
  compress_one_block(t, b, m, msg_bits_processed);
}

static void finalization(const uint64_t t[8], const uint64_t b[8],
                         const unsigned char *tail_msg,
                         unsigned long long tail_msg_bits,
                         unsigned long long counter_bits,
                         unsigned char digest[96]) {
  uint64_t final_key[MASTER_KEY_WORDS] CUISHEN768_ALIGN32;
  __m256i state[4];

  CUISHEN768_STORE_U64X4(final_key + 0, CUISHEN768_LOAD_U64X4(t + 0));
  CUISHEN768_STORE_U64X4(final_key + 4, CUISHEN768_LOAD_U64X4(t + 4));
  CUISHEN768_STORE_U64X4(final_key + 8, CUISHEN768_LOAD_U64X4(b + 0));
  CUISHEN768_STORE_U64X4(final_key + 12, CUISHEN768_LOAD_U64X4(b + 4));
  load_partial_be(tail_msg, tail_msg_bits, final_key + FINAL_PREFIX_WORDS,
                  MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);

  state[0] = _mm256_setzero_si256();
  state[1] = _mm256_setzero_si256();
  state[2] = _mm256_setzero_si256();
  state[3] = make_u64x4(0, 0, 2, 3);
  encrypt_pair_same_key(state[0], state[1], state[2], state[3], final_key,
                        final_key + MESSAGE_WORDS, (uint64_t)counter_bits,
                        state);
  store_pair_digest_be(state, digest);
}

static void finalization_initial(const unsigned char *tail_msg,
                                 unsigned long long tail_msg_bits,
                                 unsigned long long counter_bits,
                                 unsigned char digest[96]) {
  uint64_t final_key[MASTER_KEY_WORDS] CUISHEN768_ALIGN32;
  __m256i state[4];

  CUISHEN768_STORE_U64X4(
      final_key + 0,
      _mm256_loadu_si256((const __m256i *)(const void *)(IV_1 + 0)));
  CUISHEN768_STORE_U64X4(
      final_key + 4,
      _mm256_loadu_si256((const __m256i *)(const void *)(IV_1 + 4)));
  CUISHEN768_STORE_U64X4(
      final_key + 8,
      _mm256_loadu_si256((const __m256i *)(const void *)(IV_2 + 0)));
  CUISHEN768_STORE_U64X4(
      final_key + 12,
      _mm256_loadu_si256((const __m256i *)(const void *)(IV_2 + 4)));
  load_partial_be(tail_msg, tail_msg_bits, final_key + FINAL_PREFIX_WORDS,
                  MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);

  state[0] = _mm256_setzero_si256();
  state[1] = _mm256_setzero_si256();
  state[2] = _mm256_setzero_si256();
  state[3] = make_u64x4(0, 0, 2, 3);
  encrypt_pair_same_key(state[0], state[1], state[2], state[3], final_key,
                        final_key + MESSAGE_WORDS, (uint64_t)counter_bits,
                        state);
  store_pair_digest_be(state, digest);
}
#endif

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
#if CUISHEN768_AVX512VL_STREAM_STATE
  __m256i pair_state[4];
#else
  uint64_t t[8] CUISHEN768_ALIGN32;
  uint64_t b[8] CUISHEN768_ALIGN32;
#endif
  uint64_t m[MESSAGE_WORDS] CUISHEN768_ALIGN32;
  unsigned long long full_blocks;
  unsigned long long rem_bits;
  unsigned long long block;
  unsigned long long processed_bits;
  const unsigned char *p;
#if !CUISHEN768_AVX512VL_STREAM_STATE
  int i;
#endif

  if (digest_len_bits != DIGEST_BIT_LENGTH || digest == 0) {
    return -1;
  }
  if (msg_len_bits != 0ULL && msg == 0) {
    return -1;
  }

#if !CUISHEN768_AVX512VL_STREAM_STATE
#if defined(__GNUC__) && !defined(__clang__)
  if (msg_len_bits != 0ULL && msg_len_bits <= FINAL_TAIL_BITS) {
    finalization_initial(msg, msg_len_bits, msg_len_bits, digest);
    return 0;
  }
#else
  if (msg_len_bits <= FINAL_TAIL_BITS) {
    finalization_initial(msg, msg_len_bits, msg_len_bits, digest);
    return 0;
  }
#endif
#endif

#if CUISHEN768_AVX512VL_STREAM_STATE
  init_initial_pair_state(pair_state);
#else
  for (i = 0; i < 8; ++i) {
    t[i] = IV_1[i];
    b[i] = IV_2[i];
  }
#endif

  full_blocks = msg_len_bits >> 10;
  rem_bits = msg_len_bits & 1023ULL;
  p = msg;
  processed_bits = MESSAGE_BLOCK_BITS;

  for (block = 0ULL; block < full_blocks; ++block) {
#if CUISHEN768_AVX512VL_STREAM_STATE
    load1024_be(p, m);
    compress_pair_state(pair_state, m, processed_bits);
#else
    compress_full_block_be(t, b, p, processed_bits);
#endif
    p += 128;
    processed_bits += MESSAGE_BLOCK_BITS;
  }

  if (rem_bits > FINAL_TAIL_BITS) {
    load_partial_be(p, rem_bits, m, MESSAGE_WORDS);
#if CUISHEN768_AVX512VL_STREAM_STATE
    compress_pair_state(pair_state, m, msg_len_bits);
#else
    compress_one_block(t, b, m, msg_len_bits);
#endif
    p = 0;
    rem_bits = 0;
  }

#if CUISHEN768_AVX512VL_STREAM_STATE
  finalization(pair_state, p, rem_bits, msg_len_bits, digest);
#else
  finalization(t, b, p, rem_bits, msg_len_bits, digest);
#endif

  return 0;
}
