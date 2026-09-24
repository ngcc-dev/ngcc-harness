/* AVX2 gen_matrix for q=3329, n=128, k=5 (WEAVER-640): shake128x4 + rej_uniform_avx. */
#define WEAVER_USE_KECCAK4X 1
#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "params.h"

#if defined(WEAVER_AVX_GEN_MATRIX128) && (WEAVER_Q == 3329) && (WEAVER_N == 128) && \
    (WEAVER_K == 5)

#include "indcpa.h"
#include "polyvec.h"
#include "poly.h"
#include "symmetric.h"
#include "fips202.h"
#include "keccak4x/fips202x4.h"
#include "rejsample.h"

#define GEN_MATRIX_NBLOCKS (AVX_REJ_UNIFORM_BUFLEN / XOF_BLOCKBYTES)

static unsigned int rej_uniform_tail(int16_t *r,
                                     unsigned int len,
                                     const uint8_t *buf,
                                     unsigned int buflen)
{
  unsigned int ctr, pos;
  uint16_t val0, val1;

  ctr = pos = 0;
  while(ctr < len && pos + 3 <= buflen) {
    val0 = ((buf[pos + 0] >> 0) | ((uint16_t)buf[pos + 1] << 8)) & 0xFFF;
    val1 = ((buf[pos + 1] >> 4) | ((uint16_t)buf[pos + 2] << 4)) & 0xFFF;
    pos += 3;

    if(val0 < WEAVER_Q)
      r[ctr++] = val0;
    if(ctr < len && val1 < WEAVER_Q)
      r[ctr++] = val1;
  }

  return ctr;
}

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
    if(transposed) {
      buf[b][WEAVER_SYMBYTES + 0] = (uint8_t)coords[b][0];
      buf[b][WEAVER_SYMBYTES + 1] = (uint8_t)coords[b][1];
    } else {
      buf[b][WEAVER_SYMBYTES + 0] = (uint8_t)coords[b][1];
      buf[b][WEAVER_SYMBYTES + 1] = (uint8_t)coords[b][0];
    }
  }
}

static void gen_poly_x4(poly *p0, poly *p1, poly *p2, poly *p3,
                      const uint8_t seed[WEAVER_SYMBYTES], int transposed,
                      unsigned i0, unsigned j0,
                      unsigned i1, unsigned j1,
                      unsigned i2, unsigned j2,
                      unsigned i3, unsigned j3)
{
  unsigned ctr0, ctr1, ctr2, ctr3;
  uint8_t xseed[4][WEAVER_SYMBYTES + 2];
  __attribute__((aligned(32)))
  uint8_t buf[4][GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES + 32];
  keccakx4_state state;

  fill_x4_seeds(xseed, seed, transposed, i0, j0, i1, j1, i2, j2, i3, j3);
  shake128x4_absorb(&state, xseed[0], xseed[1], xseed[2], xseed[3], WEAVER_SYMBYTES + 2);
  shake128x4_squeezeblocks(buf[0], buf[1], buf[2], buf[3], GEN_MATRIX_NBLOCKS, &state);

  ctr0 = rej_uniform_avx(p0->coeffs, buf[0]);
  ctr1 = rej_uniform_avx(p1->coeffs, buf[1]);
  ctr2 = rej_uniform_avx(p2->coeffs, buf[2]);
  ctr3 = rej_uniform_avx(p3->coeffs, buf[3]);

  while(ctr0 < WEAVER_N || ctr1 < WEAVER_N || ctr2 < WEAVER_N || ctr3 < WEAVER_N) {
    shake128x4_squeezeblocks(buf[0], buf[1], buf[2], buf[3], 1, &state);
    if(ctr0 < WEAVER_N)
      ctr0 += rej_uniform_tail(p0->coeffs + ctr0, WEAVER_N - ctr0, buf[0], XOF_BLOCKBYTES);
    if(ctr1 < WEAVER_N)
      ctr1 += rej_uniform_tail(p1->coeffs + ctr1, WEAVER_N - ctr1, buf[1], XOF_BLOCKBYTES);
    if(ctr2 < WEAVER_N)
      ctr2 += rej_uniform_tail(p2->coeffs + ctr2, WEAVER_N - ctr2, buf[2], XOF_BLOCKBYTES);
    if(ctr3 < WEAVER_N)
      ctr3 += rej_uniform_tail(p3->coeffs + ctr3, WEAVER_N - ctr3, buf[3], XOF_BLOCKBYTES);
  }
}

static void gen_poly_x1(poly *p,
                        const uint8_t seed[WEAVER_SYMBYTES], int transposed,
                        unsigned i, unsigned j)
{
  unsigned ctr;
  __attribute__((aligned(32)))
  uint8_t buf[GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES + 32];
  xof_state state;

  if(transposed)
    xof_absorb(&state, seed, (uint8_t)i, (uint8_t)j);
  else
    xof_absorb(&state, seed, (uint8_t)j, (uint8_t)i);

  xof_squeezeblocks(buf, GEN_MATRIX_NBLOCKS, &state);
  ctr = rej_uniform_avx(p->coeffs, buf);

  while(ctr < WEAVER_N) {
    xof_squeezeblocks(buf, 1, &state);
    ctr += rej_uniform_tail(p->coeffs + ctr, WEAVER_N - ctr, buf, XOF_BLOCKBYTES);
  }
}

void gen_matrix(polyvec *a, const uint8_t seed[WEAVER_SYMBYTES], int transposed)
{
  /* 25 = 6*4 + 1: six shake128x4 batches across rows, one scalar tail. */
  gen_poly_x4(&a[0].vec[0], &a[0].vec[1], &a[0].vec[2], &a[0].vec[3],
              seed, transposed, 0, 0, 0, 1, 0, 2, 0, 3);
  gen_poly_x4(&a[0].vec[4], &a[1].vec[0], &a[1].vec[1], &a[1].vec[2],
              seed, transposed, 0, 4, 1, 0, 1, 1, 1, 2);
  gen_poly_x4(&a[1].vec[3], &a[1].vec[4], &a[2].vec[0], &a[2].vec[1],
              seed, transposed, 1, 3, 1, 4, 2, 0, 2, 1);
  gen_poly_x4(&a[2].vec[2], &a[2].vec[3], &a[2].vec[4], &a[3].vec[0],
              seed, transposed, 2, 2, 2, 3, 2, 4, 3, 0);
  gen_poly_x4(&a[3].vec[1], &a[3].vec[2], &a[3].vec[3], &a[3].vec[4],
              seed, transposed, 3, 1, 3, 2, 3, 3, 3, 4);
  gen_poly_x4(&a[4].vec[0], &a[4].vec[1], &a[4].vec[2], &a[4].vec[3],
              seed, transposed, 4, 0, 4, 1, 4, 2, 4, 3);
  gen_poly_x1(&a[4].vec[4], seed, transposed, 4, 4);
}

#endif
