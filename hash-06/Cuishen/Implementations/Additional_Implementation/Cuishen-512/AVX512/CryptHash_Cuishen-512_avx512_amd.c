/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

AVX-512VL optimized Cuishen-512 implementation.
Experimental variant: vector-register key schedule.

Build with GCC/Clang flags similar to:
  -O3 -mavx512f -mavx512vl -mssse3
*/

#include "CryptHash_Cuishen-512.h"
#include <immintrin.h>
#include <stdint.h>

#if !defined(__AVX512F__) || !defined(__AVX512VL__)
#error "This file requires AVX-512F and AVX-512VL. Build with -mavx512f -mavx512vl."
#endif

enum {
  ROUNDS = 64,
  ROUNDS_PER_LOOP = 4,
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

#ifndef CUISHEN_AVX512_PI_VOLATILE
#if defined(__clang__)
#define CUISHEN_AVX512_PI_VOLATILE 1
#else
#define CUISHEN_AVX512_PI_VOLATILE 0
#endif
#endif

#if CUISHEN_AVX512_PI_VOLATILE
#define CUISHEN_AVX512_PI_QUAL volatile
#else
#define CUISHEN_AVX512_PI_QUAL
#endif

#ifndef CUISHEN_AVX512_DIRECT_FINAL_KEYS
#if defined(__clang__)
#define CUISHEN_AVX512_DIRECT_FINAL_KEYS 0
#else
#define CUISHEN_AVX512_DIRECT_FINAL_KEYS 1
#endif
#endif

/*
 * Keep Clang from eagerly hoisting all per-round PI broadcasts when this loop
 * is fully unrolled.  GCC normally handles the plain const table well.
 */
#define CUISHEN_PI_VALUES(X)                                             \
  X(0x243f6a8885a308d3ULL), X(0x13198a2e03707344ULL),                       \
      X(0xa4093822299f31d0ULL), X(0x082efa98ec4e6c89ULL),                   \
      X(0x452821e638d01377ULL), X(0xbe5466cf34e90c6cULL),                   \
      X(0xc0ac29b7c97c50ddULL), X(0x3f84d5b5b5470917ULL),                   \
      X(0x9216d5d98979fb1bULL), X(0xd1310ba698dfb5acULL),                   \
      X(0x2ffd72dbd01adfb7ULL), X(0xb8e1afed6a267e96ULL),                   \
      X(0xba7c9045f12c7f99ULL), X(0x24a19947b3916cf7ULL),                   \
      X(0x0801f2e2858efc16ULL), X(0x636920d871574e69ULL),                   \
      X(0xa458fea3f4933d7eULL), X(0x0d95748f728eb658ULL),                   \
      X(0x718bcd5882154aeeULL), X(0x7b54a41dc25a59b5ULL),                   \
      X(0x9c30d5392af26013ULL), X(0xc5d1b023286085f0ULL),                   \
      X(0xca417918b8db38efULL), X(0x8e79dcb0603a180eULL),                   \
      X(0x6c9e0e8bb01e8a3eULL), X(0xd71577c1bd314b27ULL),                   \
      X(0x78af2fda55605c60ULL), X(0xe65525f3aa55ab94ULL),                   \
      X(0x5748986263e81440ULL), X(0x55ca396a2aab10b6ULL),                   \
      X(0xb4cc5c341141e8ceULL), X(0xa15486af7c72e993ULL),                   \
      X(0xb3ee1411636fbc2aULL), X(0x2ba9c55d741831f6ULL),                   \
      X(0xce5c3e169b87931eULL), X(0xafd6ba336c24cf5cULL),                   \
      X(0x7a32538128958677ULL), X(0x3b8f48986b4bb9afULL),                   \
      X(0xc4bfe81b66282193ULL), X(0x61d809ccfb21a991ULL),                   \
      X(0x487cac605dec8032ULL), X(0xef845d5de98575b1ULL),                   \
      X(0xdc262302eb651b88ULL), X(0x23893e81d396acc5ULL),                   \
      X(0x0f6d6ff383f44239ULL), X(0x2e0b4482a4842004ULL),                   \
      X(0x69c8f04a9e1f9b5eULL), X(0x21c66842f6e96c9aULL),                   \
      X(0x670c9c61abd388f0ULL), X(0x6a51a0d2d8542f68ULL),                   \
      X(0x960fa728ab5133a3ULL), X(0x6eef0b6c137a3be4ULL),                   \
      X(0xba3bf0507efb2a98ULL), X(0xa1f1651d39af0176ULL),                   \
      X(0x66ca593e82430e88ULL), X(0x8cee8619456f9fb4ULL),                   \
      X(0x7d84a5c33b8b5ebeULL), X(0xe06f75d885c12073ULL),                   \
      X(0x401a449f56c16aa6ULL), X(0x4ed3aa62363f7706ULL),                   \
      X(0x1bfedf72429b023dULL), X(0x37d0d724d00a1248ULL),                   \
      X(0xdb0fead349f1c09bULL), X(0x075372c980991b7bULL)

#if defined(__clang__)
#define CUISHEN_PI_SCALAR(X) X
static const CUISHEN_AVX512_PI_QUAL uint64_t PI[ROUNDS] = {
    CUISHEN_PI_VALUES(CUISHEN_PI_SCALAR)};
#define CUISHEN_LOAD_PI(I) set1_u64x2(PI[(I)])
#else
typedef uint64_t cuishen_u64x2 __attribute__((vector_size(16)));
#define CUISHEN_PI_VECTOR(X) { (X), (X) }
static const CUISHEN_AVX512_PI_QUAL cuishen_u64x2 PI[ROUNDS] = {
    CUISHEN_PI_VALUES(CUISHEN_PI_VECTOR)};
#define CUISHEN_LOAD_PI(I) ((__m128i)PI[(I)])
#endif

static inline __m128i make_u64x2(uint64_t lane0, uint64_t lane1) {
  return _mm_set_epi64x((long long)lane1, (long long)lane0);
}

static inline __m128i set1_u64x2(uint64_t x) {
  return _mm_set1_epi64x((long long)x);
}

static inline __m128i dup_lo_u64x2(__m128i x) {
  return _mm_shuffle_epi32(x, 0x44);
}

static inline __m128i dup_hi_u64x2(__m128i x) {
  return _mm_shuffle_epi32(x, 0xee);
}

static inline uint64_t load64_be(const unsigned char *in) {
  return ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48) |
         ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32) |
         ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16) |
         ((uint64_t)in[6] << 8) | (uint64_t)in[7];
}

static inline __m128i reverse_u64_bytes(__m128i v) {
  const __m128i reverse_bytes =
      _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15,
                   0, 1, 2, 3, 4, 5, 6, 7);
  return _mm_shuffle_epi8(v, reverse_bytes);
}

static inline __m128i load_u64x2_be(const unsigned char *in) {
  return reverse_u64_bytes(_mm_loadu_si128((const __m128i *)(const void *)in));
}

static inline void store_u64x2_be(unsigned char *out, __m128i v) {
  _mm_storeu_si128((__m128i *)(void *)out, reverse_u64_bytes(v));
}

static inline void load1024_be(const unsigned char *in,
                               uint64_t out[MESSAGE_WORDS]) {
  _mm_storeu_si128((__m128i *)(void *)(out + 0), load_u64x2_be(in + 0));
  _mm_storeu_si128((__m128i *)(void *)(out + 2), load_u64x2_be(in + 16));
  _mm_storeu_si128((__m128i *)(void *)(out + 4), load_u64x2_be(in + 32));
  _mm_storeu_si128((__m128i *)(void *)(out + 6), load_u64x2_be(in + 48));
  _mm_storeu_si128((__m128i *)(void *)(out + 8), load_u64x2_be(in + 64));
  _mm_storeu_si128((__m128i *)(void *)(out + 10), load_u64x2_be(in + 80));
  _mm_storeu_si128((__m128i *)(void *)(out + 12), load_u64x2_be(in + 96));
  _mm_storeu_si128((__m128i *)(void *)(out + 14), load_u64x2_be(in + 112));
}

__attribute__((always_inline)) static inline void
zero_partial_block_tail(uint64_t out[MESSAGE_WORDS],
                        unsigned int first_word) {
  const __m128i zero = _mm_setzero_si128();
  const unsigned int count = MESSAGE_WORDS - first_word;

  switch (count) {
  case 0U:
    return;
  case 1U:
    out[first_word] = 0;
    return;
  case 2U:
    _mm_storeu_si128((__m128i *)(void *)(out + first_word), zero);
    return;
  case 3U:
    _mm_storeu_si128((__m128i *)(void *)(out + first_word), zero);
    out[first_word + 2U] = 0;
    return;
  case 4U:
    _mm_storeu_si128((__m128i *)(void *)(out + first_word), zero);
    _mm_storeu_si128((__m128i *)(void *)(out + first_word + 2U), zero);
    return;
  default:
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#else
    return;
#endif
  }
}

__attribute__((always_inline)) static inline uint64_t
load_partial_word_be(const unsigned char *tail, unsigned long long byte_offset,
                     unsigned int partial_bytes,
                     unsigned int rem_tail_bits) {
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

  return w;
}

/* Used only for the tail-spill path where rem_bits > FINAL_TAIL_BITS. */
__attribute__((always_inline)) static inline void
load_tail_spill_block_be(const unsigned char *tail,
                         unsigned long long rem_bits,
                         uint64_t out[MESSAGE_WORDS]) {
  const unsigned long long rem_bytes = rem_bits >> 3;
  const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
  const unsigned int partial_bytes = (unsigned int)(rem_bytes & 7ULL);
  const unsigned int rem_tail_bits = (unsigned int)(rem_bits & 7ULL);
  unsigned int word = 0;
  unsigned int zero_from = full_words;

#if defined(__GNUC__) || defined(__clang__)
  if (full_words < 12U) {
    __builtin_unreachable();
  }
#endif

  for (; word + 1U < full_words; word += 2U) {
    _mm_storeu_si128((__m128i *)(void *)(out + word),
                     load_u64x2_be(tail + 8 * word));
  }
  if (word < full_words) {
    out[word] = load64_be(tail + 8 * word);
  }

  if (partial_bytes != 0U || rem_tail_bits != 0U) {
    const unsigned long long byte_offset = full_words << 3;

    out[full_words] =
        load_partial_word_be(tail, byte_offset, partial_bytes, rem_tail_bits);
    zero_from = full_words + 1U;
  }

#if defined(__GNUC__) || defined(__clang__)
  if (zero_from < 12U) {
    __builtin_unreachable();
  }
#endif

  zero_partial_block_tail(out, zero_from);
}

#if CUISHEN_AVX512_DIRECT_FINAL_KEYS
__attribute__((always_inline)) static inline __m128i
load_tail_pair_be(const unsigned char *tail, unsigned int full_words,
                  unsigned int has_partial_word, uint64_t partial_word,
                  unsigned int first_word) {
  if (full_words >= first_word + 2U) {
    return load_u64x2_be(tail + 8U * first_word);
  }
  if (full_words == first_word + 1U) {
    return make_u64x2(load64_be(tail + 8U * first_word),
                      has_partial_word != 0U ? partial_word : 0);
  }
  if (full_words == first_word && has_partial_word != 0U) {
    return make_u64x2(partial_word, 0);
  }
  return _mm_setzero_si128();
}

__attribute__((always_inline)) static inline void
load_tail12_be_keys(const unsigned char *tail, unsigned long long rem_bits,
                    __m128i *vm89, __m128i *vm1011, __m128i *vm1213,
                    __m128i *vm1415, __m128i *vbk01, __m128i *vbk23) {
  const unsigned long long rem_bytes = rem_bits >> 3;
  const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
  const unsigned int partial_bytes = (unsigned int)(rem_bytes & 7ULL);
  const unsigned int rem_tail_bits = (unsigned int)(rem_bits & 7ULL);
  const unsigned int has_partial_word =
      (partial_bytes != 0U || rem_tail_bits != 0U);
  const uint64_t partial_word =
      has_partial_word != 0U
          ? load_partial_word_be(tail, (unsigned long long)full_words << 3,
                                 partial_bytes, rem_tail_bits)
          : 0;

  *vm89 = load_tail_pair_be(tail, full_words, has_partial_word, partial_word,
                            0U);
  *vm1011 = load_tail_pair_be(tail, full_words, has_partial_word, partial_word,
                              2U);
  *vm1213 = load_tail_pair_be(tail, full_words, has_partial_word, partial_word,
                              4U);
  *vm1415 = load_tail_pair_be(tail, full_words, has_partial_word, partial_word,
                              6U);
  *vbk01 = load_tail_pair_be(tail, full_words, has_partial_word, partial_word,
                             8U);
  *vbk23 = load_tail_pair_be(tail, full_words, has_partial_word, partial_word,
                             10U);
}
#else
__attribute__((always_inline)) static inline void
load_final_tail_be(const unsigned char *tail, unsigned long long rem_bits,
                   uint64_t *out, unsigned int out_words) {
  const unsigned long long rem_bytes = rem_bits >> 3;
  const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
  const unsigned int partial_bytes = (unsigned int)(rem_bytes & 7ULL);
  const unsigned int rem_tail_bits = (unsigned int)(rem_bits & 7ULL);
  unsigned int word = 0;

  __builtin_memset(out, 0, (size_t)out_words * sizeof(*out));

  for (; word + 1U < full_words; word += 2U) {
    _mm_storeu_si128((__m128i *)(void *)(out + word),
                     load_u64x2_be(tail + 8U * word));
  }
  if (word < full_words) {
    out[word] = load64_be(tail + 8U * word);
  }

  if (partial_bytes != 0U || rem_tail_bits != 0U) {
    out[full_words] =
        load_partial_word_be(tail, (unsigned long long)full_words << 3,
                             partial_bytes, rem_tail_bits);
  }
}
#endif

#define ROTL64X2(X, N) _mm_rol_epi64((X), (N))
#define XOR3X2(A0, A1, A2) _mm_ternarylogic_epi64((A0), (A1), (A2), 0x96)

/*
 * AVX-512VL version of the v6 paired encryption.
 *
 * The two active 64-bit lanes match the NEON implementation:
 *   lane 0 = top encryption, lane 1 = bottom encryption.
 *
 * The v6 message, chain-word, and counter key schedule is duplicated into the same two
 * vector lanes, so each key-schedule rotate uses vprolq and no scalar
 * round-key broadcast is needed in the hot loop.
 */
#define CUISHEN_PAIR_ROUND_AVX512(I)                                     \
  do {                                                                       \
    const __m128i piv = CUISHEN_LOAD_PI(I);                               \
    const __m128i rk0 = XOR3X2(vm0, vbk0, vc0);                              \
    const __m128i rk1 = XOR3X2(vm1, vbk1, vc1);                              \
    const __m128i rk2 = _mm_xor_si128(vm8, vbk2);                            \
    const __m128i rk3 = _mm_xor_si128(vm9, vbk3);                            \
    const __m128i xt = _mm_xor_si128(vx2, vx3);                              \
    const __m128i nx2 = _mm_add_epi64(                                      \
        ROTL64X2(XOR3X2(vx0, rk0, piv), A),                                 \
        ROTL64X2(_mm_xor_si128(xt, rk3), D));                               \
    const __m128i nx3 = _mm_add_epi64(                                      \
        ROTL64X2(_mm_xor_si128(vx1, rk1), B),                               \
        ROTL64X2(_mm_xor_si128(xt, rk2), C));                               \
    const __m128i vm23 = _mm_xor_si128(vm2, vm3);                            \
    const __m128i vm1011 = _mm_xor_si128(vm10, vm11);                        \
    const __m128i pm2 = vm2;                                                 \
    const __m128i pm3 = vm3;                                                 \
    const __m128i pm4 = vm4;                                                 \
    const __m128i pm5 = vm5;                                                 \
    const __m128i nm2 = _mm_add_epi64(ROTL64X2(vm8, ME),                    \
                                      ROTL64X2(vm1011, MH));                \
    const __m128i nm3 = _mm_add_epi64(ROTL64X2(vm9, MF),                    \
                                      ROTL64X2(vm1011, MG));                \
    const __m128i nm10 = _mm_add_epi64(ROTL64X2(vm0, MA),                   \
                                       ROTL64X2(vm23, MD));                 \
    const __m128i nm11 = _mm_add_epi64(ROTL64X2(vm1, MB),                   \
                                       ROTL64X2(vm23, MC));                 \
    const __m128i nb3 = _mm_xor_si128(ROTL64X2(vbk0, BA), vbk1);             \
    const __m128i nc1 = ROTL64X2(vc0, 2);                                    \
    vx0 = vx2;                                                               \
    vx1 = vx3;                                                               \
    vx2 = nx2;                                                               \
    vx3 = nx3;                                                               \
    vm0 = vm6;                                                               \
    vm1 = vm7;                                                               \
    vm2 = nm2;                                                               \
    vm3 = nm3;                                                               \
    vm4 = vm10;                                                              \
    vm5 = vm11;                                                              \
    vm6 = vm12;                                                              \
    vm7 = vm13;                                                              \
    vm8 = vm14;                                                              \
    vm9 = vm15;                                                              \
    vm10 = nm10;                                                             \
    vm11 = nm11;                                                             \
    vm12 = pm2;                                                              \
    vm13 = pm3;                                                              \
    vm14 = pm4;                                                              \
    vm15 = pm5;                                                              \
    vbk0 = vbk1;                                                             \
    vbk1 = vbk2;                                                             \
    vbk2 = vbk3;                                                             \
    vbk3 = nb3;                                                              \
    vc0 = vc1;                                                               \
    vc1 = nc1;                                                               \
  } while (0)

#define CUISHEN_PAIR_ROUND_GROUP_AVX512(I)                               \
  do {                                                                       \
    CUISHEN_PAIR_ROUND_AVX512(I);                                         \
    CUISHEN_PAIR_ROUND_AVX512((I) + 1);                                   \
    CUISHEN_PAIR_ROUND_AVX512((I) + 2);                                   \
    CUISHEN_PAIR_ROUND_AVX512((I) + 3);                                   \
  } while (0)

static __attribute__((always_inline)) inline void encrypt_pair_same_key_vregs(
    __m128i vx0, __m128i vx1, __m128i vx2, __m128i vx3,
    __m128i vm01, __m128i vm23, __m128i vm45, __m128i vm67, __m128i vm89,
    __m128i vm1011, __m128i vm1213, __m128i vm1415, __m128i vbk01,
    __m128i vbk23, uint64_t counter1, __m128i out[4]) {
  __m128i vm0 = dup_lo_u64x2(vm01);
  __m128i vm1 = dup_hi_u64x2(vm01);
  __m128i vm2 = dup_lo_u64x2(vm23);
  __m128i vm3 = dup_hi_u64x2(vm23);
  __m128i vm4 = dup_lo_u64x2(vm45);
  __m128i vm5 = dup_hi_u64x2(vm45);
  __m128i vm6 = dup_lo_u64x2(vm67);
  __m128i vm7 = dup_hi_u64x2(vm67);
  __m128i vm8 = dup_lo_u64x2(vm89);
  __m128i vm9 = dup_hi_u64x2(vm89);
  __m128i vm10 = dup_lo_u64x2(vm1011);
  __m128i vm11 = dup_hi_u64x2(vm1011);
  __m128i vm12 = dup_lo_u64x2(vm1213);
  __m128i vm13 = dup_hi_u64x2(vm1213);
  __m128i vm14 = dup_lo_u64x2(vm1415);
  __m128i vm15 = dup_hi_u64x2(vm1415);
  __m128i vbk0 = dup_lo_u64x2(vbk01);
  __m128i vbk1 = dup_hi_u64x2(vbk01);
  __m128i vbk2 = dup_lo_u64x2(vbk23);
  __m128i vbk3 = dup_hi_u64x2(vbk23);
  __m128i vc0 = _mm_setzero_si128();
  __m128i vc1 = set1_u64x2(counter1);
  int round;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (round = 0; round < ROUNDS; round += ROUNDS_PER_LOOP) {
    CUISHEN_PAIR_ROUND_GROUP_AVX512(round);
  }

  out[0] = vx0;
  out[1] = vx1;
  out[2] = vx2;
  out[3] = vx3;
}

static __attribute__((always_inline)) inline void
encrypt_pair_same_key_vregs_decode_next(
    __m128i vx0, __m128i vx1, __m128i vx2, __m128i vx3,
    __m128i vm01, __m128i vm23, __m128i vm45, __m128i vm67, __m128i vm89,
    __m128i vm1011, __m128i vm1213, __m128i vm1415, __m128i vbk01,
    __m128i vbk23, uint64_t counter1, const unsigned char *next_msg,
    uint64_t next_m[MESSAGE_WORDS], __m128i out[4]) {
  __m128i vm0 = dup_lo_u64x2(vm01);
  __m128i vm1 = dup_hi_u64x2(vm01);
  __m128i vm2 = dup_lo_u64x2(vm23);
  __m128i vm3 = dup_hi_u64x2(vm23);
  __m128i vm4 = dup_lo_u64x2(vm45);
  __m128i vm5 = dup_hi_u64x2(vm45);
  __m128i vm6 = dup_lo_u64x2(vm67);
  __m128i vm7 = dup_hi_u64x2(vm67);
  __m128i vm8 = dup_lo_u64x2(vm89);
  __m128i vm9 = dup_hi_u64x2(vm89);
  __m128i vm10 = dup_lo_u64x2(vm1011);
  __m128i vm11 = dup_hi_u64x2(vm1011);
  __m128i vm12 = dup_lo_u64x2(vm1213);
  __m128i vm13 = dup_hi_u64x2(vm1213);
  __m128i vm14 = dup_lo_u64x2(vm1415);
  __m128i vm15 = dup_hi_u64x2(vm1415);
  __m128i vbk0 = dup_lo_u64x2(vbk01);
  __m128i vbk1 = dup_hi_u64x2(vbk01);
  __m128i vbk2 = dup_lo_u64x2(vbk23);
  __m128i vbk3 = dup_hi_u64x2(vbk23);
  __m128i vc0 = _mm_setzero_si128();
  __m128i vc1 = set1_u64x2(counter1);
  int round;

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (round = 0; round < ROUNDS / 2; round += ROUNDS_PER_LOOP) {
    const unsigned int pair = (unsigned int)round >> 2;

    CUISHEN_PAIR_ROUND_GROUP_AVX512(round);
    _mm_storeu_si128((__m128i *)(void *)(next_m + 2U * pair),
                     load_u64x2_be(next_msg + 16U * pair));
  }

#if defined(__clang__)
#pragma clang loop unroll(disable)
#endif
  for (; round < ROUNDS; round += ROUNDS_PER_LOOP) {
    CUISHEN_PAIR_ROUND_GROUP_AVX512(round);
  }

  out[0] = vx0;
  out[1] = vx1;
  out[2] = vx2;
  out[3] = vx3;
}

static __attribute__((always_inline)) inline void encrypt_pair_same_key(
    __m128i vx0, __m128i vx1, __m128i vx2, __m128i vx3,
    const uint64_t message_key[MESSAGE_WORDS],
    const uint64_t block_key[LINK_WORDS], uint64_t counter1, __m128i out[4]) {
  encrypt_pair_same_key_vregs(
      vx0, vx1, vx2, vx3,
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 0)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 2)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 4)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 6)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 8)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 10)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 12)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 14)),
      _mm_loadu_si128((const __m128i *)(const void *)(block_key + 0)),
      _mm_loadu_si128((const __m128i *)(const void *)(block_key + 2)),
      counter1, out);
}

static __attribute__((always_inline)) inline void
encrypt_pair_same_key_decode_next(
    __m128i vx0, __m128i vx1, __m128i vx2, __m128i vx3,
    const uint64_t message_key[MESSAGE_WORDS],
    const uint64_t block_key[LINK_WORDS], uint64_t counter1,
    const unsigned char *next_msg, uint64_t next_m[MESSAGE_WORDS],
    __m128i out[4]) {
  encrypt_pair_same_key_vregs_decode_next(
      vx0, vx1, vx2, vx3,
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 0)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 2)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 4)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 6)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 8)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 10)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 12)),
      _mm_loadu_si128((const __m128i *)(const void *)(message_key + 14)),
      _mm_loadu_si128((const __m128i *)(const void *)(block_key + 0)),
      _mm_loadu_si128((const __m128i *)(const void *)(block_key + 2)),
      counter1, next_msg, next_m, out);
}

#undef CUISHEN_PAIR_ROUND_GROUP_AVX512
#undef CUISHEN_PAIR_ROUND_AVX512
#undef XOR3X2
#undef ROTL64X2
#undef CUISHEN_LOAD_PI
#if defined(__clang__)
#undef CUISHEN_PI_SCALAR
#else
#undef CUISHEN_PI_VECTOR
#endif
#undef CUISHEN_PI_VALUES

__attribute__((always_inline)) static inline void
compress_one_block(uint64_t t[4], uint64_t b[4],
                   const uint64_t m[MESSAGE_WORDS],
                   unsigned long long msg_bits_processed) {
  __m128i state[4];
  const __m128i input0 = set1_u64x2(t[0]);
  const __m128i input1 = set1_u64x2(t[1]);
  const __m128i input2 = set1_u64x2(t[2]);
  const __m128i input3 = make_u64x2(t[3],
                                    t[3] ^ 1ULL);

  encrypt_pair_same_key(input0, input1, input2, input3, m, b,
                        (uint64_t)msg_bits_processed, state);
  state[0] = _mm_xor_si128(state[0], input0);
  state[1] = _mm_xor_si128(state[1], input1);
  state[2] = _mm_xor_si128(state[2], input2);
  state[3] = _mm_xor_si128(state[3], input3);

  _mm_storeu_si128((__m128i *)(void *)(t + 0),
                   _mm_unpacklo_epi64(state[0], state[1]));
  _mm_storeu_si128((__m128i *)(void *)(t + 2),
                   _mm_unpacklo_epi64(state[2], state[3]));
  _mm_storeu_si128((__m128i *)(void *)(b + 0),
                   _mm_unpackhi_epi64(state[0], state[1]));
  _mm_storeu_si128((__m128i *)(void *)(b + 2),
                   _mm_unpackhi_epi64(state[2], state[3]));
}

__attribute__((always_inline)) static inline void
compress_one_block_decode_next(uint64_t t[4], uint64_t b[4],
                               const uint64_t m[MESSAGE_WORDS],
                               unsigned long long msg_bits_processed,
                               const unsigned char *next_msg,
                               uint64_t next_m[MESSAGE_WORDS]) {
  /* Current compression is chain-dependent; only next-block decode can overlap. */
  __m128i state[4];
  const __m128i input0 = set1_u64x2(t[0]);
  const __m128i input1 = set1_u64x2(t[1]);
  const __m128i input2 = set1_u64x2(t[2]);
  const __m128i input3 = make_u64x2(t[3],
                                    t[3] ^ 1ULL);

  encrypt_pair_same_key_decode_next(input0, input1, input2, input3, m,
                                    b, (uint64_t)msg_bits_processed, next_msg,
                                    next_m, state);
  state[0] = _mm_xor_si128(state[0], input0);
  state[1] = _mm_xor_si128(state[1], input1);
  state[2] = _mm_xor_si128(state[2], input2);
  state[3] = _mm_xor_si128(state[3], input3);

  _mm_storeu_si128((__m128i *)(void *)(t + 0),
                   _mm_unpacklo_epi64(state[0], state[1]));
  _mm_storeu_si128((__m128i *)(void *)(t + 2),
                   _mm_unpacklo_epi64(state[2], state[3]));
  _mm_storeu_si128((__m128i *)(void *)(b + 0),
                   _mm_unpackhi_epi64(state[0], state[1]));
  _mm_storeu_si128((__m128i *)(void *)(b + 2),
                   _mm_unpackhi_epi64(state[2], state[3]));
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
#if CUISHEN_AVX512_DIRECT_FINAL_KEYS
  const __m128i zero = _mm_setzero_si128();
  __m128i vm89;
  __m128i vm1011;
  __m128i vm1213;
  __m128i vm1415;
  __m128i vbk01;
  __m128i vbk23;
  __m128i state[4];

  if (tail_msg_bits != 0ULL) {
    load_tail12_be_keys(tail_msg, tail_msg_bits, &vm89, &vm1011, &vm1213,
                        &vm1415, &vbk01, &vbk23);
  } else {
    vm89 = zero;
    vm1011 = zero;
    vm1213 = zero;
    vm1415 = zero;
    vbk01 = zero;
    vbk23 = zero;
  }

  state[0] = zero;
  state[1] = zero;
  state[2] = zero;
  state[3] = make_u64x2(2, 3);
  encrypt_pair_same_key_vregs(
      state[0], state[1], state[2], state[3],
      _mm_loadu_si128((const __m128i *)(const void *)(t + 0)),
      _mm_loadu_si128((const __m128i *)(const void *)(t + 2)),
      _mm_loadu_si128((const __m128i *)(const void *)(b + 0)),
      _mm_loadu_si128((const __m128i *)(const void *)(b + 2)), vm89, vm1011,
      vm1213, vm1415, vbk01, vbk23, (uint64_t)counter_bits, state);
#else
  uint64_t final_key[MASTER_KEY_WORDS];
  __m128i state[4];

  _mm_storeu_si128((__m128i *)(void *)(final_key + 0),
                   _mm_loadu_si128((const __m128i *)(const void *)(t + 0)));
  _mm_storeu_si128((__m128i *)(void *)(final_key + 2),
                   _mm_loadu_si128((const __m128i *)(const void *)(t + 2)));
  _mm_storeu_si128((__m128i *)(void *)(final_key + 4),
                   _mm_loadu_si128((const __m128i *)(const void *)(b + 0)));
  _mm_storeu_si128((__m128i *)(void *)(final_key + 6),
                   _mm_loadu_si128((const __m128i *)(const void *)(b + 2)));
  load_final_tail_be(tail_msg, tail_msg_bits, final_key + FINAL_PREFIX_WORDS,
                     MASTER_KEY_WORDS - FINAL_PREFIX_WORDS);

  state[0] = _mm_setzero_si128();
  state[1] = _mm_setzero_si128();
  state[2] = _mm_setzero_si128();
  state[3] = make_u64x2(2, 3);
  encrypt_pair_same_key(state[0], state[1], state[2], state[3], final_key,
                        final_key + MESSAGE_WORDS, (uint64_t)counter_bits,
                        state);
#endif

  store_u64x2_be(digest + 0, _mm_unpacklo_epi64(state[0], state[1]));
  store_u64x2_be(digest + 16, _mm_unpacklo_epi64(state[2], state[3]));
  store_u64x2_be(digest + 32, _mm_unpackhi_epi64(state[0], state[1]));
  store_u64x2_be(digest + 48, _mm_unpackhi_epi64(state[2], state[3]));
}

int CryptHash(int digest_len_bits, const unsigned char *msg,
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

  if (full_blocks == 1ULL) {
    compress_full_block_be(t, b, p, processed_bits);
    p += 128;
    processed_bits += MESSAGE_BLOCK_BITS;
  } else if (full_blocks != 0ULL) {
    uint64_t next_m[MESSAGE_WORDS];
    uint64_t *cur_m = m;
    uint64_t *decode_m = next_m;

    load1024_be(p, m);
    for (block = 0ULL; block < full_blocks; ++block) {
      if (block + 1ULL < full_blocks) {
        compress_one_block_decode_next(t, b, cur_m, processed_bits, p + 128,
                                       decode_m);
      } else {
        compress_one_block(t, b, cur_m, processed_bits);
      }

      p += 128;
      processed_bits += MESSAGE_BLOCK_BITS;

      if (block + 1ULL < full_blocks) {
        uint64_t *const done_m = cur_m;
        cur_m = decode_m;
        decode_m = done_m;
      }
    }
  }

  if (rem_bits > FINAL_TAIL_BITS) {
    load_tail_spill_block_be(p, rem_bits, m);
    compress_one_block(t, b, m, msg_len_bits);
    p = 0;
    rem_bits = 0;
  }

  finalization(t, b, p, rem_bits, msg_len_bits, digest);

  return 0;
}
