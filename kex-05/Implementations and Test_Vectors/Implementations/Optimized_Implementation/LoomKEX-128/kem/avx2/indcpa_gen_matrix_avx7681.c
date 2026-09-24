/* AVX2 gen_matrix for q=7681, k=4: shake128x4 + Lemire rejection (matches indcpa.c). */
#define WEAVER_USE_KECCAK4X 1
#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "params.h"

#if defined(WEAVER_AVX_GEN_MATRIX7681) && (WEAVER_Q == 7681) && (WEAVER_K == 4) && \
    (WEAVER_N == 256 || WEAVER_N == 512)

#include "indcpa.h"
#include "polyvec.h"
#include "symmetric.h"
#include "fips202.h"
#include "keccak4x/fips202x4.h"

#define LEMIRE_REJ_THRESHOLD ((uint32_t)((1ULL << 16) % WEAVER_Q))
#define GEN_MATRIX_NBLOCKS ((((uint32_t)2 * WEAVER_N * 65536u + (65536u - LEMIRE_REJ_THRESHOLD - 1)) / (65536u - LEMIRE_REJ_THRESHOLD) + XOF_BLOCKBYTES - 1) / XOF_BLOCKBYTES)

#include "rej_uniform_idx.inc"

static unsigned int rej_uniform7681_scalar(int16_t *r,
                                           unsigned int len,
                                           const uint8_t *buf,
                                           unsigned int buflen)
{
  const uint32_t threshold = LEMIRE_REJ_THRESHOLD;
  unsigned int ctr = 0;
  unsigned int pos = 0;

  while(ctr < len && pos + 1 < buflen) {
    uint32_t val = (uint32_t)buf[pos] | ((uint32_t)buf[pos + 1] << 8);
    uint32_t prod;
    uint16_t low;

    pos += 2;
    prod = val * (uint32_t)WEAVER_Q;
    low = (uint16_t)prod;

    if(low < threshold)
      continue;

    r[ctr++] = (int16_t)(prod >> 16);
  }

  return ctr;
}

static inline void lemire7681_store8(int16_t *r,
                                     unsigned int *ctr,
                                     __m256i out,
                                     __m256i ok)
{
  __m128i packed;
  __m128i shuflo;
  __m128i shufhi;
  unsigned int good;
  const __m128i ones = _mm_set1_epi8(1);

  packed = _mm_packus_epi32(_mm256_castsi256_si128(out),
                            _mm256_extracti128_si256(out, 1));
  good = (unsigned int)_mm256_movemask_ps(_mm256_castsi256_ps(ok));

  shuflo = _mm_loadl_epi64((__m128i *)&lemire7681_idx[good][0]);
  shufhi = _mm_add_epi8(shuflo, ones);
  shuflo = _mm_unpacklo_epi8(shuflo, shufhi);
  packed = _mm_shuffle_epi8(packed, shuflo);
  _mm_storeu_si128((__m128i *)&r[*ctr], packed);
  *ctr += (unsigned int)_mm_popcnt_u32(good);
}

static inline void lemire7681_x8(__m128i raw16,
                                 __m256i vq,
                                 __m256i vthm1,
                                 __m256i *out,
                                 __m256i *ok)
{
  __m256i val = _mm256_cvtepu16_epi32(raw16);
  __m256i prod = _mm256_mullo_epi32(val, vq);
  __m256i low = _mm256_and_si256(prod, _mm256_set1_epi32(0xFFFF));

  *out = _mm256_srli_epi32(prod, 16);
  *ok = _mm256_cmpgt_epi32(low, vthm1);
}

static unsigned int rej_uniform7681(int16_t *r,
                                    unsigned int len,
                                    const uint8_t *buf,
                                    unsigned int buflen,
                                    __m256i vq,
                                    __m256i vthm1)
{
  unsigned int ctr = 0;
  unsigned int pos = 0;
  __m256i out;
  __m256i ok;

  while(ctr + 16 <= len && pos + 32 <= buflen) {
    __m256i raw = _mm256_loadu_si256((const __m256i *)&buf[pos]);
    __m128i lo = _mm256_castsi256_si128(raw);
    __m128i hi = _mm256_extracti128_si256(raw, 1);

    lemire7681_x8(lo, vq, vthm1, &out, &ok);
    lemire7681_store8(r, &ctr, out, ok);

    lemire7681_x8(hi, vq, vthm1, &out, &ok);
    lemire7681_store8(r, &ctr, out, ok);
    pos += 32;
  }

  while(ctr + 8 <= len && pos + 16 <= buflen) {
    __m128i raw = _mm_loadu_si128((const __m128i *)&buf[pos]);
    lemire7681_x8(raw, vq, vthm1, &out, &ok);
    lemire7681_store8(r, &ctr, out, ok);
    pos += 16;
  }

  return ctr + rej_uniform7681_scalar(r + ctr, len - ctr, buf + pos, buflen - pos);
}

#ifdef TEST_REJ7681_EXPORT
unsigned int rej_uniform7681_test(int16_t *r, unsigned int len,
                                  const uint8_t *buf, unsigned int buflen)
{
  const __m256i vq = _mm256_set1_epi32(WEAVER_Q);
  const __m256i vthm1 = _mm256_set1_epi32((int32_t)LEMIRE_REJ_THRESHOLD - 1);
  return rej_uniform7681(r, len, buf, buflen, vq, vthm1);
}
#else

static void fill_x4_seeds(uint8_t buf[4][WEAVER_SYMBYTES + 2],
                          const uint8_t seed[WEAVER_SYMBYTES],
                          int transposed,
                          unsigned i0, unsigned j0,
                          unsigned i1, unsigned j1,
                          unsigned i2, unsigned j2,
                          unsigned i3, unsigned j3)
{
  unsigned b;
  unsigned coords[4][2] = {{i0, j0}, {i1, j1}, {i2, j2}, {i3, j3}};

  for(b = 0; b < 4; b++) {
    memcpy(buf[b], seed, WEAVER_SYMBYTES);
    buf[b][WEAVER_SYMBYTES + 0] = (uint8_t)coords[b][0];
    buf[b][WEAVER_SYMBYTES + 1] = (uint8_t)coords[b][1];
  }
  (void)transposed;
}

static void gen_matrix_x4(poly *p0, poly *p1, poly *p2, poly *p3,
                          const uint8_t seed[WEAVER_SYMBYTES],
                          int transposed,
                          unsigned i0, unsigned j0,
                          unsigned i1, unsigned j1,
                          unsigned i2, unsigned j2,
                          unsigned i3, unsigned j3)
{
  unsigned int ctr0, ctr1, ctr2, ctr3;
  uint8_t xseed[4][WEAVER_SYMBYTES + 2];
  __attribute__((aligned(32)))
  uint8_t buf[4][GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES + 32];
  keccakx4_state state;

  const __m256i vq = _mm256_set1_epi32(WEAVER_Q);
  const __m256i vthm1 = _mm256_set1_epi32((int32_t)LEMIRE_REJ_THRESHOLD - 1);

  fill_x4_seeds(xseed, seed, transposed, i0, j0, i1, j1, i2, j2, i3, j3);
#if WEAVER_MODE == 1
  shake128x4_absorb(&state, xseed[0], xseed[1], xseed[2], xseed[3], WEAVER_SYMBYTES + 2);
  shake128x4_squeezeblocks(buf[0], buf[1], buf[2], buf[3], GEN_MATRIX_NBLOCKS, &state);
#else
  /* LOOM Table 4 modes 3/5: scalar xof_absorb uses shake256_absorb_once but
   * xof_squeezeblocks remains shake128_squeezeblocks (see symmetric-shake.c). */
  shake256x4_absorb(&state, xseed[0], xseed[1], xseed[2], xseed[3], WEAVER_SYMBYTES + 2);
  shake128x4_squeezeblocks(buf[0], buf[1], buf[2], buf[3], GEN_MATRIX_NBLOCKS, &state);
#endif

  ctr0 = rej_uniform7681(p0->coeffs, WEAVER_N, buf[0], GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES, vq, vthm1);
  ctr1 = rej_uniform7681(p1->coeffs, WEAVER_N, buf[1], GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES, vq, vthm1);
  ctr2 = rej_uniform7681(p2->coeffs, WEAVER_N, buf[2], GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES, vq, vthm1);
  ctr3 = rej_uniform7681(p3->coeffs, WEAVER_N, buf[3], GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES, vq, vthm1);

  while(ctr0 < WEAVER_N || ctr1 < WEAVER_N || ctr2 < WEAVER_N || ctr3 < WEAVER_N) {
    shake128x4_squeezeblocks(buf[0], buf[1], buf[2], buf[3], 1, &state);

    if(ctr0 < WEAVER_N)
      ctr0 += rej_uniform7681(p0->coeffs + ctr0, WEAVER_N - ctr0, buf[0], XOF_BLOCKBYTES, vq, vthm1);
    if(ctr1 < WEAVER_N)
      ctr1 += rej_uniform7681(p1->coeffs + ctr1, WEAVER_N - ctr1, buf[1], XOF_BLOCKBYTES, vq, vthm1);
    if(ctr2 < WEAVER_N)
      ctr2 += rej_uniform7681(p2->coeffs + ctr2, WEAVER_N - ctr2, buf[2], XOF_BLOCKBYTES, vq, vthm1);
    if(ctr3 < WEAVER_N)
      ctr3 += rej_uniform7681(p3->coeffs + ctr3, WEAVER_N - ctr3, buf[3], XOF_BLOCKBYTES, vq, vthm1);
  }
}

void gen_matrix(polyvec *a, const uint8_t seed[WEAVER_SYMBYTES], int transposed)
{
  unsigned i;

  for(i = 0; i < WEAVER_K; i++) {
    gen_matrix_x4(&a[i].vec[0], &a[i].vec[1], &a[i].vec[2], &a[i].vec[3],
                  seed, transposed,
                  transposed ? i : 0, transposed ? 0 : i,
                  transposed ? i : 1, transposed ? 1 : i,
                  transposed ? i : 2, transposed ? 2 : i,
                  transposed ? i : 3, transposed ? 3 : i);
  }
}

#endif /* TEST_REJ7681_EXPORT */

#endif
