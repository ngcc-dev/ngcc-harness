#ifndef BLOCK_IMPL_AVX2_H
#define BLOCK_IMPL_AVX2_H

#include <immintrin.h>
#include <wmmintrin.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

typedef __m128i block128;
typedef __m256i block256;

typedef struct
{
	block128 data[3];
} block384;
typedef struct
{
	block256 data[2];
} block512;
typedef struct
{
	block256 data[4];
} block1024;

#define shuffle_2x4xepi32(x, y, i) \
	_mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), i))
#define permute_8xepi32(x, i) \
	_mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(x), i))
#define shuffle_2x4xepi64(x, y, i) \
	_mm256_castpd_si256(_mm256_shuffle_pd(_mm256_castsi256_pd(x), _mm256_castsi256_pd(y), i))




inline __m256i mm256_compute_mask_2(const uint64_t idx, const size_t bit) {
  const uint64_t m1 = -((idx >> bit) & 1);
  const uint64_t m2 = -((idx >> (bit + 1)) & 1);
  return _mm256_set_epi64x(m2, m2, m1, m1);
}
inline __m256i mm256_compute_mask(const uint64_t idx, const size_t bit) {
  return _mm256_set1_epi64x(-((idx >> bit) & 1));
}

inline __m128i* block128_as_m128i(block128* block) {
  return (__m128i*)block;
}
inline void clmul_schoolbook128(__m128i out[2], const __m128i a, const __m128i b) {
  __m128i tmp[3];
  out[0] = _mm_clmulepi64_si128(a, b, 0x00);
  out[1] = _mm_clmulepi64_si128(a, b, 0x11);
  tmp[0] = _mm_clmulepi64_si128(a, b, 0x01);
  tmp[1] = _mm_clmulepi64_si128(a, b, 0x10);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);
  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);
}
inline void reduce_clmul128(__m128i out[1], const __m128i in[2]) {
  __m128i p = _mm_set_epi64x(0x0, 0x87);
  __m128i t0, t1, t2;
  t0 = _mm_clmulepi64_si128(in[1], p, 0x01);
  t1 = _mm_slli_si128(t0, 8);
  t2 = _mm_srli_si128(t0, 8);
  t2 = _mm_xor_si128(t2, in[1]);
  t0 = _mm_clmulepi64_si128(t2, p, 0x00);
  out[0] = _mm_xor_si128(t0, in[0]);
  out[0] = _mm_xor_si128(out[0], t1);
}
inline void sqr128(__m128i out[2], const __m128i a) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a, mask);
  tmp[1] = _mm_srli_epi64(a, 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);
}
inline void gf128sqr(__m128i *out, const __m128i in) {
  __m128i tmp[2];
  sqr128(tmp, in);
  reduce_clmul128(out, tmp);
}
inline void gf128mul(__m128i *out, const __m128i in1, const __m128i in2) {
  __m128i tmp[2];
  clmul_schoolbook128(tmp, in1, in2);
  reduce_clmul128(out, tmp);
}
inline void block128_inverse(block128* block) {
  const size_t u[11] = {1, 2, 3, 6, 12, 24, 48, 51, 63, 126, 127};
  const size_t q_index[10] = {0, 0, 2, 3, 4, 5, 2, 4, 8, 0};
  __m128i b[11];
  memcpy(b, block, 16);
  for (size_t i = 1; i < 11; ++i) {
    __m128i b_p = b[i - 1];
    __m128i b_q = b[q_index[i - 1]];
    for (size_t m = u[q_index[i - 1]]; m; --m) {
      gf128sqr(&b_p, b_p);
    }
    gf128mul(&b[i], b_p, b_q);
  }
  gf128sqr(block, b[10]);
}
inline void block128_mersenne_inverse(block128* block) {

  __m128i in[4];
  memcpy(in, block, 16);

  __m128i tmp, tmp2;
  gf128sqr(&tmp, in[0]);
  gf128sqr(&in[1], tmp);

  gf128sqr(&tmp, in[1]);
  for(size_t i = 1 ; i < 3; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[1]);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 6; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&in[2], tmp, tmp2);


  gf128sqr(&tmp, in[2]);
  for(size_t i = 1 ; i < 12; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[2]);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 24; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, tmp2);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 12; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[2]);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 3; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&in[3], tmp, in[1]);

  
  gf128sqr(&tmp, in[3]);
  for(size_t i = 1 ; i < 63; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[3]);

  gf128sqr(&tmp2, tmp2);

  gf128mul(block, tmp2, in[0]);

}
inline void block128_multiply_with_GF2_matrix(block128* block, const uint64_t* matrix) {
  block128 tmp;
  for (size_t j = 0; j < 2; j++) {
    uint64_t t = 0;
    for (size_t i = 0; i < 64; i++) {
      const uint64_t *A = &matrix[(j*64*2) + (i*2)];
      uint64_t bit = _mm_popcnt_u64((((uint64_t*)block)[0] & A[0]) ^ (((uint64_t*)block)[1] & A[1])) & 1;
      t ^= (bit << i);
    }
    ((uint64_t*)&tmp)[j] = t;
  }
  *block = tmp;
}
inline void block128_multiply_with_transposed_GF2_matrix(block128* block, const uint64_t* matrix) {
  const uint64_t *vptr = (uint64_t*)block;
  const __m256i *Ablock = (const __m256i*)matrix;

  __m256i cval[2] = {_mm256_setzero_si256(), _mm256_setzero_si256()};
  for (unsigned int w = 2; w; --w, ++vptr) {
    uint64_t idx = *vptr;
    for (unsigned int i = sizeof(uint64_t) * 8; i;
         i -= 8, idx >>= 8, Ablock += 4) {
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[0], mm256_compute_mask_2(idx, 0)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[1], mm256_compute_mask_2(idx, 2)));
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[2], mm256_compute_mask_2(idx, 4)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[3], mm256_compute_mask_2(idx, 6)));
    }
  }
  cval[0] = _mm256_xor_si256(cval[0], cval[1]);
  *block = _mm_xor_si128(_mm256_extracti128_si256(cval[0], 0),
                                     _mm256_extracti128_si256(cval[0], 1));
}
inline __m128i* block137_as_m128i(block137* block) {
  return (__m128i*)block;
}
inline void clmul_schoolbook137(__m128i out[3], const __m128i a[2], const __m128i b[2]) {
  __m128i tmp[3];
  out[0] = _mm_clmulepi64_si128(a[0], b[0], 0x00);
  out[1] = _mm_clmulepi64_si128(a[0], b[0], 0x11);
  out[2] = _mm_clmulepi64_si128(a[1], b[1], 0x00);
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[0], b[1], 0x00));
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[1], b[0], 0x00));

  tmp[0] = _mm_clmulepi64_si128(a[0], b[0], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[0], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[0], 0x10);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[1], 0x01);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[1] = _mm_xor_si128(out[1], tmp[1]);
  out[2] = _mm_xor_si128(out[2], tmp[2]);
}
static inline void reduce_clmul137(__m128i out[2], const __m128i in[3]) {
  uint64_t t[6] = {0};

  memcpy(&t[0], &in[0], 16);
  memcpy(&t[2], &in[1], 16);
  memcpy(&t[4], &in[2], 16);

  for (int bit = 383; bit >= 137; --bit) {
    int word = bit >> 6;
    int off  = bit & 63;
    uint64_t mask = 1ULL << off;
    if (t[word] & mask) {
      t[word] ^= mask;
      {
        int b = bit - 137;
        t[b >> 6] ^= (1ULL << (b & 63));
      }
      {
        int b = bit - 116;
        t[b >> 6] ^= (1ULL << (b & 63));
      }
    }
  }
  t[2] &= 0x1FFULL;
  t[3] = 0;
  t[4] = 0;
  t[5] = 0;
  memcpy(&out[0], &t[0], 16);
  out[1] = _mm_set_epi64x(0, t[2] & 0x1FFULL);
}

static inline void sqr137(__m128i out[3], const __m128i a[2]) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a[0], mask);
  tmp[1] = _mm_srli_epi64(a[0], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

  tmp[0] = _mm_and_si128(a[1], mask);
  tmp[1] = _mm_srli_epi64(a[1], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
}
static inline void gf137sqr(__m128i *out, const __m128i *in) {
  __m128i tmp[3];
  sqr137(tmp, in);
  reduce_clmul137(out, tmp);
}
static inline void gf137mul(__m128i *out, const __m128i *in1, const __m128i *in2) {
  __m128i tmp[3];
  clmul_schoolbook137(tmp, in1, in2);
  reduce_clmul137(out, tmp);
}
static inline void block137_sqr(block137* out, const block137* in)
{
    gf137sqr((__m128i*)out->data, (const __m128i*)in->data);
    out->data[2] &= 0x1FFULL;
}

static inline void block137_mul(block137* out, const block137* a, const block137* b)
{
    gf137mul((__m128i*)out->data, (const __m128i*)a->data, (const __m128i*)b->data);
    out->data[2] &= 0x1FFULL;
}
static inline block137 block137_set_one(void)
{
    block137 r = {{0, 0, 0}};
    r.data[0] = 1;
    return r;
}

static inline void block137_mask(block137* x)
{
    x->data[2] &= 0x1FFULL;
}

static void block137_pow_bin(block137* block, const char* exp_bits)
{
    block137 base = *block;
    block137 result = block137_set_one();
    for (const char* p = exp_bits; *p; ++p) {
        if (*p != '0' && *p != '1')
            continue;

        block137 tmp;

        block137_sqr(&tmp, &result);
        result = tmp;

        if (*p == '1') {
            block137_mul(&result, &result, &base);
        }
    }

    block137_mask(&result);
    *block = result;
}

static inline void block137_mersenne_inverse1(block137* block) {
    static const char EXP_INV_1[] =
        "11011011011011011011011011011011011011011011011011011011011011011011101101101101101101101101101101101101101101101101101101101101101101101";

    block137_pow_bin(block, EXP_INV_1);
}

static inline void block137_mersenne_inverse2(block137* block) {
    static const char EXP_INV_1[] =
        "11011011011101101101101110110110110111011011011011101101101101110110110111011011011011101101101101110110110110111011011011011101101101101";

    block137_pow_bin(block, EXP_INV_1);
}
static inline void block137_multiply_with_GF2_matrix(block137* block, const uint64_t matrix[137][3]) {
    block137 tmp;
    tmp.data[0] = 0;
    tmp.data[1] = 0;
    tmp.data[2] = 0;

    const uint64_t x0 = block->data[0];
    const uint64_t x1 = block->data[1];
    const uint64_t x2 = block->data[2] & 0x1FFULL;

    for (size_t row = 0; row < 137; ++row) {
        const uint64_t* A = matrix[row];

        uint64_t bit =
            (_mm_popcnt_u64(x0 & A[0]) ^
             _mm_popcnt_u64(x1 & A[1]) ^
             _mm_popcnt_u64(x2 & A[2])) & 1ULL;

        tmp.data[row >> 6] ^= bit << (row & 63);
    }

    tmp.data[2] &= 0x1FFULL;
    *block = tmp;
}


inline __m128i* block197_as_m128i(block197* block) {
  return (__m128i*)block;
}

static inline void block197_mask(block197* x)
{
    x->data[3] &= 0x1FULL;
}

static inline block197 block197_set_one(void)
{
    block197 r = {{0, 0, 0, 0}};
    r.data[0] = 1;
    return r;
}

static inline void clmul_schoolbook197(__m128i out[4],
                                       const __m128i a[2],
                                       const __m128i b[2])
{
    uint64_t aw[4], bw[4];
    uint64_t t[8] = {0};

    memcpy(aw, a, 32);
    memcpy(bw, b, 32);

    aw[3] &= 0x1FULL;
    bw[3] &= 0x1FULL;

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            __m128i aa = _mm_set_epi64x(0, aw[i]);
            __m128i bb = _mm_set_epi64x(0, bw[j]);
            __m128i p = _mm_clmulepi64_si128(aa, bb, 0x00);

            uint64_t pp[2];
            _mm_storeu_si128((__m128i*)pp, p);

            t[i + j]     ^= pp[0];
            t[i + j + 1] ^= pp[1];
        }
    }

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3], t[2]);
    out[2] = _mm_set_epi64x(t[5], t[4]);
    out[3] = _mm_set_epi64x(t[7], t[6]);
}

static inline void reduce_clmul197(__m128i out[2], const __m128i in[4])
{
    uint64_t t[8] = {0};

    memcpy(&t[0], &in[0], 16);
    memcpy(&t[2], &in[1], 16);
    memcpy(&t[4], &in[2], 16);
    memcpy(&t[6], &in[3], 16);

    for (int bit = 511; bit >= 197; --bit) {
        int word = bit >> 6;
        int off  = bit & 63;
        uint64_t mask = 1ULL << off;

        if (t[word] & mask) {
            t[word] ^= mask;

            {
                int b = bit - 197;
                t[b >> 6] ^= (1ULL << (b & 63));
            }

            {
                int b = bit - 176;
                t[b >> 6] ^= (1ULL << (b & 63));
            }
            {
                int b = bit - 195;
                t[b >> 6] ^= (1ULL << (b & 63));
            }
            {
                int b = bit - 196;
                t[b >> 6] ^= (1ULL << (b & 63));
            }
        }
    }

    t[3] &= 0x1FULL;
    t[4] = 0;
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3] & 0x1FULL, t[2]);
}

static inline void sqr197(__m128i out[4], const __m128i a[2])
{
    __m128i tmp[2];

    __m128i sqrT = _mm_set_epi64x(
        0x5554515045444140,
        0x1514111005040100
    );

    __m128i mask = _mm_set_epi64x(
        0x0F0F0F0F0F0F0F0F,
        0x0F0F0F0F0F0F0F0F
    );

    tmp[0] = _mm_and_si128(a[0], mask);
    tmp[1] = _mm_srli_epi64(a[0], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

    tmp[0] = _mm_and_si128(a[1], mask);
    tmp[1] = _mm_srli_epi64(a[1], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[3] = _mm_unpackhi_epi8(tmp[0], tmp[1]);
}

static inline void gf197sqr(__m128i* out, const __m128i* in)
{
    __m128i tmp[4];
    sqr197(tmp, in);
    reduce_clmul197(out, tmp);
}

static inline void gf197mul(__m128i* out, const __m128i* in1, const __m128i* in2)
{
    __m128i tmp[4];
    clmul_schoolbook197(tmp, in1, in2);
    reduce_clmul197(out, tmp);
}

static inline void block197_sqr(block197* out, const block197* in)
{
    gf197sqr((__m128i*)out->data, (const __m128i*)in->data);
    out->data[3] &= 0x1FULL;
}

static inline void block197_mul(block197* out, const block197* a, const block197* b)
{
    gf197mul((__m128i*)out->data,
             (const __m128i*)a->data,
             (const __m128i*)b->data);
    out->data[3] &= 0x1FULL;
}

static void block197_pow_bin(block197* block, const char* exp_bits)
{
    block197 base = *block;
    block197 result = block197_set_one();

    for (const char* p = exp_bits; *p; ++p) {
        if (*p != '0' && *p != '1')
            continue;

        block197 tmp;

        block197_sqr(&tmp, &result);
        result = tmp;

        if (*p == '1') {
            block197_mul(&result, &result, &base);
        }
    }

    block197_mask(&result);
    *block = result;
}

static inline void block197_mersenne_inverse1(block197* block)
{
    static const char EXP_INV_1[] =
        "11101110111101110111101110111101110111101110111101110111101110111101110111101110111101110111101110111011110111011110111011110111011110111011110111011110111011110111011110111011110111011110111011101";

    block197_pow_bin(block, EXP_INV_1);
}

static inline void block197_mersenne_inverse2(block197* block)
{
    static const char EXP_INV_2[] =
        "11111111111101111111111110111111111111011111111111101111111111110111111111111011111111111101111111111111011111111111101111111111110111111111111011111111111101111111111110111111111111011111111111101";

    block197_pow_bin(block, EXP_INV_2);
}

static inline void block197_multiply_with_GF2_matrix(
    block197* block,
    const uint64_t matrix[197][4]
) {
    block197 tmp;
    tmp.data[0] = 0;
    tmp.data[1] = 0;
    tmp.data[2] = 0;
    tmp.data[3] = 0;

    const uint64_t x0 = block->data[0];
    const uint64_t x1 = block->data[1];
    const uint64_t x2 = block->data[2];
    const uint64_t x3 = block->data[3] & 0x1FULL;

    for (size_t row = 0; row < 197; ++row) {
        const uint64_t* A = matrix[row];

        uint64_t bit =
            (_mm_popcnt_u64(x0 & A[0]) ^
             _mm_popcnt_u64(x1 & A[1]) ^
             _mm_popcnt_u64(x2 & A[2]) ^
             _mm_popcnt_u64(x3 & (A[3] & 0x1FULL))) & 1ULL;

        tmp.data[row >> 6] |= bit << (row & 63);
    }

    tmp.data[3] &= 0x1FULL;
    *block = tmp;
}

static inline block521 block521_set_one(void)
{
    block521 out = {{1, 0, 0, 0, 0, 0, 0, 0, 0}};
    return out;
}


static inline void block521_mul(block521* out, const block521* a, const block521* b)
{
    uint64_t product[17] = {0};

    for (size_t word = 0; word < BLOCK521_WORDS; ++word) {
        const unsigned int limit = word == 8 ? 9 : 64;
        for (unsigned int bit = 0; bit < limit; ++bit) {
            const uint64_t select = UINT64_C(0) - ((b->data[word] >> bit) & 1);
            const size_t shift = word * 64 + bit;
            const size_t word_shift = shift >> 6;
            const unsigned int bit_shift = shift & 63;
            for (size_t i = 0; i < BLOCK521_WORDS; ++i) {
                product[word_shift + i] ^= (a->data[i] << bit_shift) & select;
                if (bit_shift && word_shift + i + 1 < 17)
                    product[word_shift + i + 1] ^= (a->data[i] >> (64 - bit_shift)) & select;
            }
        }
    }

    for (int bit = 1040; bit >= 521; --bit) {
        const uint64_t mask = UINT64_C(1) << (bit & 63);
        const uint64_t select = UINT64_C(0) - ((product[bit >> 6] >> (bit & 63)) & 1);
        product[bit >> 6] ^= mask & select;
        const int low = bit - 521;
        product[low >> 6] ^= (UINT64_C(1) << (low & 63)) & select;
        const int mid = low + 32;
        product[mid >> 6] ^= (UINT64_C(1) << (mid & 63)) & select;
    }

    memcpy(out->data, product, sizeof(out->data));
    block521_canonicalize(out);
}

static inline void block521_sqr(block521* out, const block521* in)
{
    block521_mul(out, in, in);
}

static inline void block521_pow_words(block521* block, const uint64_t exponent[9])
{
    const block521 base = *block;
    block521 result = block521_set_one();
    for (int bit = 520; bit >= 0; --bit) {
        block521_sqr(&result, &result);
        if ((exponent[bit >> 6] >> (bit & 63)) & 1)
            block521_mul(&result, &result, &base);
    }
    *block = result;
}

static inline void block521_mersenne_inverse1(block521* block)
{
    static const uint64_t exponent[9] = {
        UINT64_C(0xaaa9555554aaaaa9), UINT64_C(0x4aaaaaa5555552aa),
        UINT64_C(0x55552aaaaa955555), UINT64_C(0xa9555554aaaaaa55),
        UINT64_C(0xaaaaa9555554aaaa), UINT64_C(0x554aaaaaa5555552),
        UINT64_C(0x5555552aaaaa9555), UINT64_C(0xaaa9555554aaaaaa),
        UINT64_C(0xaa)};
    block521_pow_words(block, exponent);
}

static inline void block521_mersenne_inverse2(block521* block)
{
    static const uint64_t exponent[9] = {
        UINT64_C(0x5556aaaaab555555), UINT64_C(0xb555555aaaaaad55),
        UINT64_C(0xaaaad555556aaaaa), UINT64_C(0x56aaaaab555555aa),
        UINT64_C(0x555556aaaaab5555), UINT64_C(0xaab555555aaaaaad),
        UINT64_C(0xaaaaaad555556aaa), UINT64_C(0x5556aaaaab555555),
        UINT64_C(0x155)};
    block521_pow_words(block, exponent);
}

static inline void block521_multiply_with_GF2_matrix(
    block521* block, const uint64_t matrix[521][BLOCK521_WORDS])
{
    block521 out = {{0}};
    for (size_t row = 0; row < 521; ++row) {
        unsigned int parity = 0;
        for (size_t word = 0; word < BLOCK521_WORDS; ++word)
            parity ^= (unsigned int)__builtin_parityll(block->data[word] & matrix[row][word]);
        out.data[row >> 6] |= (uint64_t)(parity & 1) << (row & 63);
    }
    *block = out;
}


inline __m128i* block263_as_m128i(block263* block) {
  return (__m128i*)block;
}

static inline void block263_mask(block263* x)
{
    x->data[4] &= 0x7FULL;
}

static inline block263 block263_set_one(void)
{
    block263 r = {{0, 0, 0, 0, 0}};
    r.data[0] = 1;
    return r;
}

static inline void clmul_schoolbook263(__m128i out[5],
                                       const __m128i a[3],
                                       const __m128i b[3])
{
    uint64_t aw[6], bw[6];
    uint64_t t[10] = {0};

    memcpy(aw, a, 48);
    memcpy(bw, b, 48);

    aw[4] &= 0x7FULL;
    bw[4] &= 0x7FULL;
    aw[5] = 0;
    bw[5] = 0;

    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 5; j++) {
            __m128i aa = _mm_set_epi64x(0, aw[i]);
            __m128i bb = _mm_set_epi64x(0, bw[j]);
            __m128i p = _mm_clmulepi64_si128(aa, bb, 0x00);

            uint64_t pp[2];
            _mm_storeu_si128((__m128i*)pp, p);

            t[i + j]     ^= pp[0];
            t[i + j + 1] ^= pp[1];
        }
    }

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3], t[2]);
    out[2] = _mm_set_epi64x(t[5], t[4]);
    out[3] = _mm_set_epi64x(t[7], t[6]);
    out[4] = _mm_set_epi64x(t[9], t[8]);
}

static inline void reduce_clmul263(__m128i out[3], const __m128i in[5])
{
    uint64_t t[10] = {0};

    memcpy(&t[0], &in[0], 16);
    memcpy(&t[2], &in[1], 16);
    memcpy(&t[4], &in[2], 16);
    memcpy(&t[6], &in[3], 16);
    memcpy(&t[8], &in[4], 16);

    for (int bit = 639; bit >= 263; --bit) {
        int word = bit >> 6;
        int off  = bit & 63;
        uint64_t mask = 1ULL << off;

        if (t[word] & mask) {
            t[word] ^= mask;

            {
                int b = bit - 263;
                t[b >> 6] ^= (1ULL << (b & 63));
            }

            {
                int b = bit - 170;
                t[b >> 6] ^= (1ULL << (b & 63));
            }
        }
    }

    t[4] &= 0x7FULL;
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;
    t[8] = 0;
    t[9] = 0;

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3], t[2]);
    out[2] = _mm_set_epi64x(0, t[4] & 0x7FULL);
}

static inline void sqr263(__m128i out[5], const __m128i a[3])
{
    __m128i tmp[2];

    __m128i sqrT = _mm_set_epi64x(
        0x5554515045444140,
        0x1514111005040100
    );

    __m128i mask = _mm_set_epi64x(
        0x0F0F0F0F0F0F0F0F,
        0x0F0F0F0F0F0F0F0F
    );

    tmp[0] = _mm_and_si128(a[0], mask);
    tmp[1] = _mm_srli_epi64(a[0], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

    tmp[0] = _mm_and_si128(a[1], mask);
    tmp[1] = _mm_srli_epi64(a[1], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[3] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

    tmp[0] = _mm_and_si128(a[2], mask);
    tmp[1] = _mm_srli_epi64(a[2], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[4] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
}

static inline void gf263sqr(__m128i* out, const __m128i* in)
{
    __m128i tmp[5];
    sqr263(tmp, in);
    reduce_clmul263(out, tmp);
}

static inline void gf263mul(__m128i* out, const __m128i* in1, const __m128i* in2)
{
    __m128i tmp[5];
    clmul_schoolbook263(tmp, in1, in2);
    reduce_clmul263(out, tmp);
}

static inline void block263_sqr(block263* out, const block263* in)
{
    __m128i in128[3] = {
        _mm_set_epi64x(in->data[1], in->data[0]),
        _mm_set_epi64x(in->data[3], in->data[2]),
        _mm_set_epi64x(0, in->data[4] & 0x7FULL)
    };

    __m128i out128[3];
    gf263sqr(out128, in128);

    uint64_t words[6];
    _mm_storeu_si128((__m128i*)&words[0], out128[0]);
    _mm_storeu_si128((__m128i*)&words[2], out128[1]);
    _mm_storeu_si128((__m128i*)&words[4], out128[2]);

    out->data[0] = words[0];
    out->data[1] = words[1];
    out->data[2] = words[2];
    out->data[3] = words[3];
    out->data[4] = words[4] & 0x7FULL;
}

static inline void block263_mul(block263* out, const block263* a, const block263* b)
{
    __m128i a128[3] = {
        _mm_set_epi64x(a->data[1], a->data[0]),
        _mm_set_epi64x(a->data[3], a->data[2]),
        _mm_set_epi64x(0, a->data[4] & 0x7FULL)
    };

    __m128i b128[3] = {
        _mm_set_epi64x(b->data[1], b->data[0]),
        _mm_set_epi64x(b->data[3], b->data[2]),
        _mm_set_epi64x(0, b->data[4] & 0x7FULL)
    };

    __m128i out128[3];
    gf263mul(out128, a128, b128);

    uint64_t words[6];
    _mm_storeu_si128((__m128i*)&words[0], out128[0]);
    _mm_storeu_si128((__m128i*)&words[2], out128[1]);
    _mm_storeu_si128((__m128i*)&words[4], out128[2]);

    out->data[0] = words[0];
    out->data[1] = words[1];
    out->data[2] = words[2];
    out->data[3] = words[3];
    out->data[4] = words[4] & 0x7FULL;
}

static void block263_pow_bin(block263* block, const char* exp_bits)
{
    block263 base = *block;
    block263 result = block263_set_one();

    for (const char* p = exp_bits; *p; ++p) {
        if (*p != '0' && *p != '1')
            continue;

        block263 tmp;

        block263_sqr(&tmp, &result);
        result = tmp;

        if (*p == '1') {
            block263_mul(&result, &result, &base);
        }
    }

    block263_mask(&result);
    *block = result;
}

static inline void block263_mersenne_inverse1(block263* block)
{
    static const char EXP_INV_1[] =
        "11101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011101";

    block263_pow_bin(block, EXP_INV_1);
}

static inline void block263_mersenne_inverse2(block263* block)
{
    static const char EXP_INV_2[] =
        "11111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101";

    block263_pow_bin(block, EXP_INV_2);
}

static inline void block263_multiply_with_GF2_matrix(
    block263* block,
    const uint64_t matrix[263][5]
) {
    block263 tmp;
    tmp.data[0] = 0;
    tmp.data[1] = 0;
    tmp.data[2] = 0;
    tmp.data[3] = 0;
    tmp.data[4] = 0;

    const uint64_t x0 = block->data[0];
    const uint64_t x1 = block->data[1];
    const uint64_t x2 = block->data[2];
    const uint64_t x3 = block->data[3];
    const uint64_t x4 = block->data[4] & 0x7FULL;

    for (size_t row = 0; row < 263; ++row) {
        const uint64_t* A = matrix[row];

        uint64_t bit =
            (_mm_popcnt_u64(x0 & A[0]) ^
             _mm_popcnt_u64(x1 & A[1]) ^
             _mm_popcnt_u64(x2 & A[2]) ^
             _mm_popcnt_u64(x3 & A[3]) ^
             _mm_popcnt_u64(x4 & (A[4] & 0x7FULL))) & 1ULL;

        tmp.data[row >> 6] |= bit << (row & 63);
    }

    tmp.data[4] &= 0x7FULL;
    *block = tmp;
}

inline __m128i* block192_as_m128i(block192* block) {
  return (__m128i*)block;
}
inline void clmul_schoolbook192(__m128i out[3], const __m128i a[2], const __m128i b[2]) {
  __m128i tmp[3];
  out[0] = _mm_clmulepi64_si128(a[0], b[0], 0x00);
  out[1] = _mm_clmulepi64_si128(a[0], b[0], 0x11);
  out[2] = _mm_clmulepi64_si128(a[1], b[1], 0x00);
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[0], b[1], 0x00));
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[1], b[0], 0x00));

  tmp[0] = _mm_clmulepi64_si128(a[0], b[0], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[0], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[0], 0x10);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[1], 0x01);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[1] = _mm_xor_si128(out[1], tmp[1]);
  out[2] = _mm_xor_si128(out[2], tmp[2]);
}
inline void reduce_clmul192(__m128i out[2], const __m128i in[3]) {
  __m128i p = _mm_set_epi64x(0x0, 0x87);
  __m128i t0, t1, t2, t3;
  t0 = _mm_clmulepi64_si128(in[2], p, 0x01);
  t3 = _mm_xor_si128(in[1], t0);

  t0 = _mm_clmulepi64_si128(in[2], p, 0x00);
  t1 = _mm_slli_si128(t0, 8);
  t2 = _mm_srli_si128(t0, 8);
  t3 = _mm_xor_si128(t3, t2);

  t0 = _mm_clmulepi64_si128(t3, p, 0x01);
  out[0] = _mm_xor_si128(t0, in[0]);
  out[0] = _mm_xor_si128(out[0], t1);
  out[1] = _mm_and_si128(t3, _mm_set_epi64x(0x0, 0xFFFFFFFFFFFFFFFF));
}
inline void sqr192(__m128i out[3], const __m128i a[2]) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a[0], mask);
  tmp[1] = _mm_srli_epi64(a[0], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

  tmp[0] = _mm_and_si128(a[1], mask);
  tmp[1] = _mm_srli_epi64(a[1], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
}
inline void gf192sqr(__m128i *out, const __m128i *in) {
  __m128i tmp[3];
  sqr192(tmp, in);
  reduce_clmul192(out, tmp);
}
inline void gf192mul(__m128i *out, const __m128i *in1, const __m128i *in2) {
  __m128i tmp[3];
  clmul_schoolbook192(tmp, in1, in2);
  reduce_clmul192(out, tmp);
}
inline void block192_inverse(block192* block) {
  const size_t u[12] = {1, 2, 3, 5, 10, 20, 23, 46, 92, 95, 190, 191};
  const size_t q_index[11] = {0, 0, 1, 3, 4, 2, 6, 7, 2, 9, 0};
  __m128i b[12][2];

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 8);

  for (size_t i = 1; i < 12; ++i) {

    __m128i b_p[2] = {b[i - 1][0], b[i - 1][1]};
    __m128i b_q[2] = {b[q_index[i - 1]][0], b[q_index[i - 1]][1]};

    for (size_t m = u[q_index[i - 1]]; m; --m) {
      gf192sqr(b_p, b_p);
    }
    gf192mul(b[i], b_p, b_q);
  }
  __m128i* tmp;
  tmp = (__m128i*)malloc(32);
  gf192sqr(tmp, b[11]);
  memcpy(block, tmp, 24);
  free(tmp);
}
inline void block192_mersenne_inverse(block192* block) {

  __m128i b[5][2];

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 8);

  __m128i tmp[2], tmp2[2];

  gf192sqr(tmp,b[0]);
  gf192sqr(tmp, tmp);
  gf192mul(tmp, tmp, b[0]);
  gf192sqr(b[1], tmp);

  gf192sqr(tmp, b[1]);
  for(size_t i = 1 ; i < 5; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(b[2], tmp, b[1]);

  gf192sqr(tmp, b[2]);
  for(size_t i = 1 ; i < 10; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(b[3], tmp, b[2]);

  gf192sqr(tmp, b[3]);
  for(size_t i = 1 ; i < 20; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, b[3]);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 40; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, tmp2);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 80; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, tmp2);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 20; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, b[3]);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 10; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(b[4], tmp, b[2]);

  gf192sqr(tmp, b[4]);
  gf192sqr(tmp, tmp);

  __m128i* tmp3;
  tmp3 = (__m128i*)malloc(32);
  gf192mul(tmp3, tmp, b[0]);
  memcpy(block, tmp3, 24);
  free(tmp3);
}
inline void block192_multiply_with_GF2_matrix(block192* block, const uint64_t* matrix) {
  block192 tmp;
  for (size_t j = 0; j < 3; j++) {
    uint64_t t = 0;
    for (size_t i = 0; i < 64; i++) {
      const uint64_t *A = &matrix[(j*64*4) + (i*4)];
      uint64_t bit =
          _mm_popcnt_u64((((uint64_t*)block)[0] & A[0]) ^ (((uint64_t*)block)[1] & A[1]) ^
                         (((uint64_t*)block)[2] & A[2])) &  1;
      t ^= (bit << i);
    }
    ((uint64_t*)&tmp)[j] = t;
  }
  *block = tmp;
}
inline void block192_multiply_with_transposed_GF2_matrix(block192* block, const uint64_t* matrix) {
  const uint64_t *vptr = (uint64_t*)block;
  const __m256i *Ablock = (const __m256i*)matrix;

  __m256i cval[2] = {_mm256_setzero_si256(), _mm256_setzero_si256()};
  for (unsigned int w = 3; w; --w, ++vptr) {
    uint64_t idx = *vptr;
    for (unsigned int i = sizeof(uint64_t) * 8; i;
         i -= 4, idx >>= 4, Ablock += 4) {
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[0], mm256_compute_mask(idx, 0)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[1], mm256_compute_mask(idx, 1)));
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[2], mm256_compute_mask(idx, 2)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[3], mm256_compute_mask(idx, 3)));
    }
  }
  block256 tmp = _mm256_xor_si256(cval[0], cval[1]);
  memcpy(block, &tmp, 24);
}

inline __m128i* block256_as_m128i(block256* block) {
  return (__m128i*)(block);
}
inline void clmul_schoolbook256(__m128i out[4], const __m128i a[2], const __m128i b[2]) {
  __m128i tmp[4];
  out[0] = _mm_clmulepi64_si128(a[0], b[0], 0x00);

  out[1] = _mm_clmulepi64_si128(a[0], b[0], 0x11);
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[0], b[1], 0x00));
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[1], b[0], 0x00));

  out[2] = _mm_clmulepi64_si128(a[1], b[1], 0x00);
  out[2] = _mm_xor_si128(out[2], _mm_clmulepi64_si128(a[0], b[1], 0x11));
  out[2] = _mm_xor_si128(out[2], _mm_clmulepi64_si128(a[1], b[0], 0x11));

  out[3] = _mm_clmulepi64_si128(a[1], b[1], 0x11);

  tmp[0] = _mm_clmulepi64_si128(a[0], b[0], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[0], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[0], 0x10);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[1], 0x01);
  tmp[2] = _mm_clmulepi64_si128(a[0], b[1], 0x10);
  tmp[3] = _mm_clmulepi64_si128(a[1], b[0], 0x01);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[2] = _mm_xor_si128(tmp[2], tmp[3]);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[2]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[1] = _mm_xor_si128(out[1], tmp[1]);
  out[2] = _mm_xor_si128(out[2], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[1], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[1], b[1], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[2] = _mm_xor_si128(out[2], tmp[1]);
  out[3] = _mm_xor_si128(out[3], tmp[2]);
}
inline void reduce_clmul256(__m128i out[2], const __m128i in[4]) {
  __m128i p = _mm_set_epi64x(0x0, 0x425);
  __m128i t0, t1, t2, t3;
  t0 = _mm_clmulepi64_si128(in[3], p, 0x01);
  t1 = _mm_slli_si128(t0, 8);
  t2 = _mm_srli_si128(t0, 8);
  t3 = _mm_xor_si128(in[2], t2);
  out[1] = _mm_xor_si128(in[1], t1);

  t0 = _mm_clmulepi64_si128(in[3], p, 0x00);
  out[1] = _mm_xor_si128(out[1], t0);

  t0 = _mm_clmulepi64_si128(in[2], p, 0x01);
  t1 = _mm_slli_si128(t0, 8);
  t2 = _mm_srli_si128(t0, 8);
  out[1] = _mm_xor_si128(out[1], t2);
  out[0] = _mm_xor_si128(t1, in[0]);
  t0 = _mm_clmulepi64_si128(t3, p, 0x00);
  out[0] = _mm_xor_si128(t0, out[0]);
}
inline void sqr256(__m128i out[4], const __m128i a[2]) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a[0], mask);
  tmp[1] = _mm_srli_epi64(a[0], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

  tmp[0] = _mm_and_si128(a[1], mask);
  tmp[1] = _mm_srli_epi64(a[1], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[3] = _mm_unpackhi_epi8(tmp[0], tmp[1]);
}
inline void gf256sqr(__m128i *out, const __m128i *in) {
  __m128i tmp[4];
  sqr256(tmp, in);
  reduce_clmul256(out, tmp);
}
inline void gf256mul(__m128i *out, const __m128i *in1, const __m128i *in2) {
  __m128i tmp[4];
  clmul_schoolbook256(tmp, in1, in2);
  reduce_clmul256(out, tmp);
}
inline void block256_inverse(block256* block) {
  const size_t u[11] = {1, 2, 3, 6, 12, 15, 30, 60, 120, 240, 255};
  const size_t q_index[10] = {0, 0, 2, 3, 2, 5, 6, 7, 8, 5};
  __m128i b[11][2];

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 16);

  for (size_t i = 1; i < 11; ++i) {

    __m128i b_p[2] = {b[i - 1][0], b[i - 1][1]};
    __m128i b_q[2] = {b[q_index[i - 1]][0], b[q_index[i - 1]][1]};

    for (size_t m = u[q_index[i - 1]]; m; --m) {
      gf256sqr(b_p, b_p);
    }

    gf256mul(b[i], b_p, b_q);
  }
  gf256sqr((__m128i*)block, b[10]);
}
inline void block256_mersenne_inverse(block256* block) {

  __m128i b[4][2];

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 16);
  
  __m128i tmp[2], tmp2[2];

  gf256sqr(tmp, b[0]);
  gf256mul(tmp, tmp, b[0]);
  gf256sqr(b[1], tmp);

  gf256sqr(tmp, b[1]);
  for(size_t i = 1 ; i < 3; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, b[1]);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 6; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 3; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(b[2], tmp, b[1]);

  gf256sqr(tmp, b[2]);
  for(size_t i = 1 ; i < 15; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, b[2]);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 30; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 60; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 120; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 15; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(b[3], tmp, b[2]);
  
  gf256sqr(tmp, b[3]);
  
  gf256mul((__m128i*)block, tmp, b[0]);
  
}
inline void block256_multiply_with_GF2_matrix(block256* block, const uint64_t* matrix) {
  block256 tmp;
  for (size_t j = 0; j < 4; j++) {
    uint64_t t = 0;
    for (size_t i = 0; i < 64; i++) {
      const uint64_t *A = &matrix[(j*64*4) + (i*4)];
      uint64_t bit =
          _mm_popcnt_u64((((uint64_t*)block)[0] & A[0]) ^ (((uint64_t*)block)[1] & A[1]) ^
                         (((uint64_t*)block)[2] & A[2]) ^ (((uint64_t*)block)[3] & A[3])) &   1;
      t ^= (bit << i);
    }
    ((uint64_t*)&tmp)[j] = t;
  }
  *block = tmp;
}
inline void block256_multiply_with_transposed_GF2_matrix(block256* block, const uint64_t* matrix) {
  const uint64_t *vptr = (uint64_t*)block;
  const __m256i *Ablock = (const __m256i*)matrix;

  __m256i cval[2] = {_mm256_setzero_si256(), _mm256_setzero_si256()};
  for (unsigned int w = 4; w; --w, ++vptr) {
    uint64_t idx = *vptr;
    for (unsigned int i = sizeof(uint64_t) * 8; i;
         i -= 4, idx >>= 4, Ablock += 4) {
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[0], mm256_compute_mask(idx, 0)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[1], mm256_compute_mask(idx, 1)));
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[2], mm256_compute_mask(idx, 2)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[3], mm256_compute_mask(idx, 3)));
    }
  }
  block256 tmp = _mm256_xor_si256(cval[0], cval[1]);
  memcpy(block, &tmp, 24);
}




inline block128 block128_xor(block128 x, block128 y) { return _mm_xor_si128(x, y); }
inline block256 block256_xor(block256 x, block256 y) { return _mm256_xor_si256(x, y); }
inline block128 block128_and(block128 x, block128 y) { return _mm_and_si128(x, y); }
inline block256 block256_and(block256 x, block256 y) { return _mm256_and_si256(x, y); }
inline block128 block128_set_zero() { return _mm_setzero_si128(); }
inline block256 block256_set_zero() { return _mm256_setzero_si256(); }
inline block128 block128_set_all_8(uint8_t x) { return _mm_set1_epi8(x); }
inline block256 block256_set_all_8(uint8_t x) { return _mm256_set1_epi8(x); }
inline block128 block128_set_low32(uint32_t x) { return _mm_setr_epi32(x, 0, 0, 0); }
inline block256 block256_set_low32(uint32_t x) { return _mm256_setr_epi32(x, 0, 0, 0, 0, 0, 0, 0); }
inline block128 block128_set_low64(uint64_t x) { return _mm_set_epi64x(0, x); }
inline block256 block256_set_low64(uint64_t x) { return _mm256_setr_epi64x(x, 0, 0, 0); }
inline block256 block256_set_128(block128 x0, block128 x1) { return _mm256_setr_m128i(x0, x1); }

inline block256 block256_set_low128(block128 x)
{
	return _mm256_inserti128_si256(_mm256_setzero_si256(), x, 0);
}

inline block384 block384_xor(block384 x, block384 y)
{
	block384 out;
	out.data[0] = block128_xor(x.data[0], y.data[0]);
	out.data[1] = block128_xor(x.data[1], y.data[1]);
	out.data[2] = block128_xor(x.data[2], y.data[2]);
	return out;
}
inline block512 block512_xor(block512 x, block512 y)
{
	block512 out;
	out.data[0] = block256_xor(x.data[0], y.data[0]);
	out.data[1] = block256_xor(x.data[1], y.data[1]);
	return out;
}
inline block1024 block1024_xor(block1024 x, block1024 y)
{
	block1024 out;
	for (size_t i=0;i<4;i++) out.data[i]=block256_xor(x.data[i],y.data[i]);
	return out;
}

inline block384 block384_and(block384 x, block384 y)
{
	block384 out;
	out.data[0] = block128_and(x.data[0], y.data[0]);
	out.data[1] = block128_and(x.data[1], y.data[1]);
	out.data[2] = block128_and(x.data[2], y.data[2]);
	return out;
}
inline block512 block512_and(block512 x, block512 y)
{
	block512 out;
	out.data[0] = block256_and(x.data[0], y.data[0]);
	out.data[1] = block256_and(x.data[1], y.data[1]);
	return out;
}
inline block1024 block1024_and(block1024 x, block1024 y)
{
	block1024 out;
	for (size_t i=0;i<4;i++) out.data[i]=block256_and(x.data[i],y.data[i]);
	return out;
}

inline block384 block384_set_zero()
{
	block384 out;
	out.data[0] = block128_set_zero();
	out.data[1] = block128_set_zero();
	out.data[2] = block128_set_zero();
	return out;
}
inline block512 block512_set_zero()
{
	block512 out;
	out.data[0] = block256_set_zero();
	out.data[1] = block256_set_zero();
	return out;
}
inline block1024 block1024_set_zero() { block1024 out; for(size_t i=0;i<4;i++)out.data[i]=block256_set_zero(); return out; }

inline block384 block384_set_all_8(uint8_t x)
{
	block384 out;
	out.data[0] = block128_set_all_8(x);
	out.data[1] = block128_set_all_8(x);
	out.data[2] = block128_set_all_8(x);
	return out;
}
inline block512 block512_set_all_8(uint8_t x)
{
	block512 out;
	out.data[0] = block256_set_all_8(x);
	out.data[1] = block256_set_all_8(x);
	return out;
}
inline block1024 block1024_set_all_8(uint8_t x) { block1024 out; for(size_t i=0;i<4;i++)out.data[i]=block256_set_all_8(x); return out; }

inline block384 block384_set_low32(uint32_t x)
{
	block384 out;
	out.data[0] = block128_set_low32(x);
	return out;
}
inline block512 block512_set_low32(uint32_t x)
{
	block512 out = block512_set_zero();
	out.data[0] = block256_set_low32(x);
	return out;
}
inline block1024 block1024_set_low32(uint32_t x) { block1024 out=block1024_set_zero(); out.data[0]=block256_set_low32(x); return out; }
inline block384 block384_set_low64(uint64_t x)
{
	block384 out;
	out.data[0] = block128_set_low64(x);
	return out;
}
inline block512 block512_set_low64(uint64_t x)
{
	block512 out = block512_set_zero();
	out.data[0] = block256_set_low64(x);
	return out;
}
inline block1024 block1024_set_low64(uint64_t x) { block1024 out=block1024_set_zero(); out.data[0]=block256_set_low64(x); return out; }

inline bool block512_any_zeros(block512 x)
{
	const uint8_t* p=(const uint8_t*)&x;
	for(size_t i=0;i<sizeof(x);i++) if(p[i]==0) return true;
	return false;
}

inline bool block128_any_zeros(block128 x)
{
	return _mm_movemask_epi8(_mm_cmpeq_epi8(x, _mm_setzero_si128()));
}

inline bool block256_any_zeros(block256 x)
{
	return _mm256_movemask_epi8(_mm256_cmpeq_epi8(x, _mm256_setzero_si256()));
}

inline bool block192_any_zeros(block192 x)
{
	block256 b = block256_set_zero();
	memcpy(&b, &x, sizeof(x));
	return _mm256_movemask_epi8(_mm256_cmpeq_epi8(b, _mm256_setzero_si256())) & 0x00ffffff;
}

inline block128 block128_byte_reverse(block128 x)
{
	block128 shuffle = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
	return _mm_shuffle_epi8(x, shuffle);
}

inline block256 block256_from_2_block128(block128 x, block128 y)
{
	return _mm256_setr_m128i(x, y);
}

#define VOLE_BLOCK_SHIFT 0
typedef block128 vole_block;
inline vole_block vole_block_set_zero() { return block128_set_zero(); }
inline vole_block vole_block_xor(vole_block x, vole_block y) { return block128_xor(x, y); }
inline vole_block vole_block_and(vole_block x, vole_block y) { return block128_and(x, y); }
inline vole_block vole_block_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline vole_block vole_block_set_low32(uint32_t x) { return block128_set_low32(x); }
inline vole_block vole_block_set_low64(uint64_t x) { return block128_set_low64(x); }

#ifndef HAVE_VCLMUL
#define POLY_VEC_LEN_SHIFT 0
typedef block128 clmul_block;
inline clmul_block clmul_block_xor(clmul_block x, clmul_block y) { return block128_xor(x, y); }
inline clmul_block clmul_block_and(clmul_block x, clmul_block y) { return block128_and(x, y); }
inline clmul_block clmul_block_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline clmul_block clmul_block_set_zero() { return block128_set_zero(); }

inline clmul_block clmul_block_clmul_ll(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x00);
}
inline clmul_block clmul_block_clmul_lh(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x10);
}
inline clmul_block clmul_block_clmul_hl(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x01);
}
inline clmul_block clmul_block_clmul_hh(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x11);
}

inline clmul_block clmul_block_shift_left_64(clmul_block x)
{
	return _mm_slli_si128(x, 8);
}
inline clmul_block clmul_block_shift_right_64(clmul_block x)
{
	return _mm_srli_si128(x, 8);
}
inline clmul_block clmul_block_mix_64(clmul_block x, clmul_block y)
{
	return _mm_alignr_epi8(x, y, 8);
}
inline clmul_block clmul_block_broadcast_low64(clmul_block x)
{
	return _mm_broadcastq_epi64(x);
}

#else
#define POLY_VEC_LEN_SHIFT 1
typedef block256 clmul_block;
inline clmul_block clmul_block_xor(clmul_block x, clmul_block y) { return block256_xor(x, y); }
inline clmul_block clmul_block_and(clmul_block x, clmul_block y) { return block256_and(x, y); }
inline clmul_block clmul_block_set_all_8(uint8_t x) { return block256_set_all_8(x); }
inline clmul_block clmul_block_set_zero() { return block256_set_zero(); }

inline clmul_block clmul_block_clmul_ll(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x00);
}
inline clmul_block clmul_block_clmul_lh(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x10);
}
inline clmul_block clmul_block_clmul_hl(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x01);
}
inline clmul_block clmul_block_clmul_hh(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x11);
}

inline clmul_block clmul_block_shift_left_64(clmul_block x)
{
	return _mm256_slli_si256(x, 8);
}
inline clmul_block clmul_block_shift_right_64(clmul_block x)
{
	return _mm256_srli_si256(x, 8);
}
inline clmul_block clmul_block_mix_64(clmul_block x, clmul_block y)
{
	return _mm256_alignr_epi8(x, y, 8);
}
inline clmul_block clmul_block_broadcast_low64(clmul_block x)
{
	return _mm256_shuffle_epi32(x, 0x44);
}
#endif

#endif
