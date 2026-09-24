/*
 * SM3 x8 AVX2 backend for fixed-length ICCS KDF inputs.
 * The vector round structure follows the public GmSSL sm3_avx2.c design
 * (Apache-2.0), rewritten here as a self-contained one-block/fallback helper.
 */
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "auxfunc.h"
#include "sm3x8_avx2.h"

#define ROLT(x,n) _mm256_or_si256(_mm256_slli_epi32((x), (n)), _mm256_srli_epi32((x), (32 - (n))))
#define P0(x) _mm256_xor_si256((x), _mm256_xor_si256(ROLT((x), 9), ROLT((x), 17)))
#define P1(x) _mm256_xor_si256((x), _mm256_xor_si256(ROLT((x), 15), ROLT((x), 23)))
#define FF00(x,y,z) _mm256_xor_si256((x), _mm256_xor_si256((y), (z)))
#define FF16(x,y,z) _mm256_or_si256(_mm256_and_si256((x), (y)), _mm256_or_si256(_mm256_and_si256((x), (z)), _mm256_and_si256((y), (z))))
#define GG00(x,y,z) _mm256_xor_si256((x), _mm256_xor_si256((y), (z)))
#define GG16(x,y,z) _mm256_xor_si256(_mm256_and_si256(_mm256_xor_si256((y), (z)), (x)), (z))

static const uint32_t K[64] = {
  0x79cc4519U, 0xf3988a32U, 0xe7311465U, 0xce6228cbU,
  0x9cc45197U, 0x3988a32fU, 0x7311465eU, 0xe6228cbcU,
  0xcc451979U, 0x988a32f3U, 0x311465e7U, 0x6228cbceU,
  0xc451979cU, 0x88a32f39U, 0x11465e73U, 0x228cbce6U,
  0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
  0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
  0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
  0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
  0x7a879d8aU, 0xf50f3b14U, 0xea1e7629U, 0xd43cec53U,
  0xa879d8a7U, 0x50f3b14fU, 0xa1e7629eU, 0x43cec53dU,
  0x879d8a7aU, 0x0f3b14f5U, 0x1e7629eaU, 0x3cec53d4U,
  0x79d8a7a8U, 0xf3b14f50U, 0xe7629ea1U, 0xcec53d43U,
  0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
  0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
  0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
  0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
};

static uint32_t load_be32(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void store_be32(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}

static __m256i load_lane_words(uint8_t block[8][64], int word)
{
  return _mm256_setr_epi32(
    (int)load_be32(block[0] + 4 * word),
    (int)load_be32(block[1] + 4 * word),
    (int)load_be32(block[2] + 4 * word),
    (int)load_be32(block[3] + 4 * word),
    (int)load_be32(block[4] + 4 * word),
    (int)load_be32(block[5] + 4 * word),
    (int)load_be32(block[6] + 4 * word),
    (int)load_be32(block[7] + 4 * word));
}

static void sm3_x8_compress_block(uint8_t block[8][64], __m256i state[8])
{
  __m256i W[68];
  __m256i A = state[0];
  __m256i B = state[1];
  __m256i C = state[2];
  __m256i D = state[3];
  __m256i E = state[4];
  __m256i F = state[5];
  __m256i G = state[6];
  __m256i H = state[7];
  int j;

  for(j = 0; j < 16; j++)
    W[j] = load_lane_words(block, j);
  for(; j < 68; j++)
    W[j] = _mm256_xor_si256(P1(_mm256_xor_si256(_mm256_xor_si256(W[j - 16], W[j - 9]), ROLT(W[j - 3], 15))), _mm256_xor_si256(ROLT(W[j - 13], 7), W[j - 6]));

  for(j = 0; j < 64; j++) {
    __m256i SS1 = ROLT(_mm256_add_epi32(_mm256_add_epi32(ROLT(A, 12), E), _mm256_set1_epi32((int)K[j])), 7);
    __m256i SS2 = _mm256_xor_si256(SS1, ROLT(A, 12));
    __m256i W1 = _mm256_xor_si256(W[j], W[j + 4]);
    __m256i TT1;
    __m256i TT2;

    if(j < 16) {
      TT1 = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(FF00(A, B, C), D), SS2), W1);
      TT2 = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(GG00(E, F, G), H), SS1), W[j]);
    } else {
      TT1 = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(FF16(A, B, C), D), SS2), W1);
      TT2 = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(GG16(E, F, G), H), SS1), W[j]);
    }

    D = C;
    C = ROLT(B, 9);
    B = A;
    A = TT1;
    H = G;
    G = ROLT(F, 19);
    F = E;
    E = P0(TT2);
  }

  state[0] = _mm256_xor_si256(A, state[0]);
  state[1] = _mm256_xor_si256(B, state[1]);
  state[2] = _mm256_xor_si256(C, state[2]);
  state[3] = _mm256_xor_si256(D, state[3]);
  state[4] = _mm256_xor_si256(E, state[4]);
  state[5] = _mm256_xor_si256(F, state[5]);
  state[6] = _mm256_xor_si256(G, state[6]);
  state[7] = _mm256_xor_si256(H, state[7]);
}

static void sm3_x8_store_digest(__m256i state[8], uint8_t dgst[8][32])
{
  uint32_t lanes[8];
  int j;

  for(j = 0; j < 8; j++) {
    int lane;
    _mm256_storeu_si256((__m256i *)lanes, state[j]);
    for(lane = 0; lane < 8; lane++)
      store_be32(dgst[lane] + 4 * j, lanes[lane]);
  }
}

void sm3_x8_digest(const uint8_t *data, size_t datalen, uint8_t dgst[8][32])
{
  uint8_t block0[8][64];
  uint8_t block1[8][64];
  __m256i state[8];
  int lane;

  if(datalen > 119) {
    for(lane = 0; lane < 8; lane++)
      sm3hash(256, data + (size_t)lane * datalen, (unsigned long long)datalen * 8ULL, dgst[lane]);
    return;
  }

  state[0] = _mm256_set1_epi32(0x7380166F);
  state[1] = _mm256_set1_epi32(0x4914B2B9);
  state[2] = _mm256_set1_epi32(0x172442D7);
  state[3] = _mm256_set1_epi32(0xDA8A0600);
  state[4] = _mm256_set1_epi32(0xA96F30BC);
  state[5] = _mm256_set1_epi32(0x163138AA);
  state[6] = _mm256_set1_epi32(0xE38DEE4D);
  state[7] = _mm256_set1_epi32(0xB0FB0E4E);

  memset(block0, 0, sizeof(block0));
  memset(block1, 0, sizeof(block1));

  if(datalen <= 55) {
    for(lane = 0; lane < 8; lane++) {
      memcpy(block0[lane], data + (size_t)lane * datalen, datalen);
      block0[lane][datalen] = 0x80;
      store_be32(block0[lane] + 60, (uint32_t)datalen * 8U);
    }
    sm3_x8_compress_block(block0, state);
  } else {
    for(lane = 0; lane < 8; lane++) {
      const uint8_t *lane_data = data + (size_t)lane * datalen;
      size_t first = datalen < 64 ? datalen : 64;
      size_t tail = datalen - first;

      memcpy(block0[lane], lane_data, first);
      if(tail != 0)
        memcpy(block1[lane], lane_data + first, tail);
      if(datalen < 64)
        block0[lane][datalen] = 0x80;
      else
        block1[lane][tail] = 0x80;
      store_be32(block1[lane] + 60, (uint32_t)datalen * 8U);
    }
    sm3_x8_compress_block(block0, state);
    sm3_x8_compress_block(block1, state);
  }

  sm3_x8_store_digest(state, dgst);
}

void sm3_x4_digest(const uint8_t *data, size_t datalen, uint8_t dgst[4][32])
{
  uint8_t out[8][32];
  size_t lane;

  if(datalen > 119) {
    for(lane = 0; lane < 4; lane++)
      sm3hash(256, data + lane * datalen, (unsigned long long)datalen * 8ULL, dgst[lane]);
    return;
  }

  {
    uint8_t tmp[8 * (datalen == 0 ? 1 : datalen)];

    memset(tmp, 0, sizeof(tmp));
    for(lane = 0; lane < 4; lane++)
      memcpy(tmp + lane * datalen, data + lane * datalen, datalen);
    sm3_x8_digest(tmp, datalen, out);
    for(lane = 0; lane < 4; lane++)
      memcpy(dgst[lane], out[lane], 32);
  }
}
