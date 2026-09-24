#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include "auxfunc.h"
#include "params.h"
#include "reduce.h"
#include "rounding.h"
#include "ntt.h"
#include "poly.h"
#include "rejsample.h"
#include "consts.h"

#define _mm256_blendv_epi32(a,b,mask) \
  _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(a), \
                                       _mm256_castsi256_ps(b), \
                                       _mm256_castsi256_ps(mask)))

/* Initial XOF lengths derived from the current rejection probabilities. */
#define POLY_UNIFORM_BUFLEN \
  (4U * (unsigned int)((((uint64_t)N << 26) + Q - 1) / Q))

#if ETA == 2
#define POLY_UNIFORM_ETA_BUFLEN ((8U * N + 14U) / 15U)
#elif ETA == 3
#define POLY_UNIFORM_ETA_BUFLEN ((8U * N + 13U) / 14U)
#endif

static const ALIGNED_INT32(8) qdata8 = {{Q, Q, Q, Q, Q, Q, Q, Q}};

/* Reduce all coefficients of a polynomial modulo Q. */
void poly_reduce(poly *a) {
  unsigned int i;
  __m256i f, g;
  const __m256i q = _mm256_load_si256(&qdata8.vec[0]);
  const __m256i off = _mm256_set1_epi32(1 << 25);

  for(i = 0; i < N/8; i++) {
    f = _mm256_load_si256(&a->vec[i]);
    g = _mm256_add_epi32(f, off);
    g = _mm256_srai_epi32(g, 26);
    g = _mm256_mullo_epi32(g, q);
    f = _mm256_sub_epi32(f, g);
    _mm256_store_si256(&a->vec[i], f);
  }
}

/* Conditionally add Q to move coefficients into the standard nonnegative range. */
void poly_caddq(poly *a) {
  unsigned int i;
  __m256i f, g;
  const __m256i q = _mm256_load_si256(&qdata8.vec[0]);
  const __m256i zero = _mm256_setzero_si256();

  for(i = 0; i < N/8; i++) {
    f = _mm256_load_si256(&a->vec[i]);
    g = _mm256_blendv_epi32(zero, q, f);
    f = _mm256_add_epi32(f, g);
    _mm256_store_si256(&a->vec[i], f);
  }
}

/* Add two polynomials coefficient-wise. */
void poly_add(poly *c, const poly *a, const poly *b) {
  unsigned int i;
  __m256i f, g;

  for(i = 0; i < N/8; i++) {
    f = _mm256_load_si256(&a->vec[i]);
    g = _mm256_load_si256(&b->vec[i]);
    f = _mm256_add_epi32(f, g);
    _mm256_store_si256(&c->vec[i], f);
  }
}

/* Subtract two polynomials coefficient-wise. */
void poly_sub(poly *c, const poly *a, const poly *b) {
  unsigned int i;
  __m256i f, g;

  for(i = 0; i < N/8; i++) {
    f = _mm256_load_si256(&a->vec[i]);
    g = _mm256_load_si256(&b->vec[i]);
    f = _mm256_sub_epi32(f, g);
    _mm256_store_si256(&c->vec[i], f);
  }
}

/* Left-shift coefficients by D bits as required by the scheme. */
void poly_shiftl(poly *a) {
  unsigned int i;
  __m256i f;

  for(i = 0; i < N/8; i++) {
    f = _mm256_load_si256(&a->vec[i]);
    f = _mm256_slli_epi32(f, D);
    _mm256_store_si256(&a->vec[i], f);
  }
}

/* Map a polynomial into the NTT domain. */
void poly_ntt(poly *a) {
  ntt(a->coeffs);
}

/* Map a polynomial back from the NTT domain. */
void poly_invntt_tomont(poly *a) {
  invntt_tomont(a->coeffs);
}

void poly_nttunpack(poly *a) {
  (void)a;
}

/* Non-assembly fallback kept for staged migration. */
static void __attribute__((unused))
poly_pointwise_montgomery_c(poly *c, const poly *a, const poly *b) {
  unsigned int i;
  __m256i f, g, even_prod, odd_lhs, odd_rhs, odd_prod;
  union {
    int64_t coeffs[4];
    __m256i vec;
  } even, odd;

  for(i = 0; i < N/8; i++) {
    f = _mm256_load_si256(&a->vec[i]);
    g = _mm256_load_si256(&b->vec[i]);

    even_prod = _mm256_mul_epi32(f, g);
    odd_lhs = _mm256_srli_epi64(f, 32);
    odd_rhs = _mm256_srli_epi64(g, 32);
    odd_prod = _mm256_mul_epi32(odd_lhs, odd_rhs);

    even.vec = even_prod;
    odd.vec = odd_prod;

    c->coeffs[8*i + 0] = montgomery_reduce(even.coeffs[0]);
    c->coeffs[8*i + 1] = montgomery_reduce(odd.coeffs[0]);
    c->coeffs[8*i + 2] = montgomery_reduce(even.coeffs[1]);
    c->coeffs[8*i + 3] = montgomery_reduce(odd.coeffs[1]);
    c->coeffs[8*i + 4] = montgomery_reduce(even.coeffs[2]);
    c->coeffs[8*i + 5] = montgomery_reduce(odd.coeffs[2]);
    c->coeffs[8*i + 6] = montgomery_reduce(even.coeffs[3]);
    c->coeffs[8*i + 7] = montgomery_reduce(odd.coeffs[3]);
  }
}

/* Pointwise multiply two NTT-domain polynomials. */
void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b) {
  pointwise_avx(c->vec, a->vec, b->vec, qdata.vec);
}

/* Split coefficients into high and low parts for the public key. */
void poly_power2round(poly *a1, poly *a0, const poly *a) {
  power2round_avx(a1->vec, a0->vec, a->vec);
}

/* Decompose coefficients into hint-related high and low parts. */
void poly_decompose(poly *a1, poly *a0, const poly *a) {
  decompose_avx(a1->vec, a0->vec, a->vec);
}

/* Generate the hint polynomial and return its Hamming weight. */
unsigned int poly_make_hint(poly *h, const poly *a0, const poly *a1) {
  return make_hint_avx(h->vec, a0->vec, a1->vec);
}

/* Apply hint bits to reconstruct the high bits of a polynomial. */
void poly_use_hint(poly *b, const poly *a, const poly *h) {
  use_hint_avx(b->vec, a->vec, h->vec);
}

/* Check whether any centered coefficient exceeds the requested bound. */
int poly_chknorm(const poly *a, int32_t B) {
  unsigned int i;
  int r;
  __m256i f, t;
  const __m256i bound = _mm256_set1_epi32(B - 1);

  if(B > (Q - 1) / 8)
    return 1;

  t = _mm256_setzero_si256();
  for(i = 0; i < N/8; i++) {
    f = _mm256_load_si256(&a->vec[i]);
    f = _mm256_abs_epi32(f);
    f = _mm256_cmpgt_epi32(f, bound);
    t = _mm256_or_si256(t, f);
  }

  r = 1 - _mm256_testz_si256(t, t);
  return r;
}

/* Rejection-sample coefficients uniformly modulo Q from an XOF buffer. */
static unsigned int rej_uniform_scalar(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen) {
  unsigned int ctr = 0;
  unsigned int pos = 0;
  while(ctr < len && pos + 4 <= buflen) {
    uint32_t t = (uint32_t)buf[pos++];
    t |= (uint32_t)buf[pos++] << 8;
    t |= (uint32_t)buf[pos++] << 16;
    t |= (uint32_t)buf[pos++] << 24;
    t &= ((1u << 26) - 1u);
    if(t < Q)
      a[ctr++] = (int32_t)t;
  }
  return ctr;
}

/* Expand a seed and nonce into a uniformly random polynomial. */
void poly_uniform(poly *a, const uint8_t seed[SEEDBYTES], uint16_t nonce) {
  uint8_t in[SEEDBYTES + 2 + 4];
  uint8_t buf[REJ_UNIFORM_BUFLEN + 8];
  unsigned int ctr = 0;
  uint32_t block = 0;

  memcpy(in, seed, SEEDBYTES);
  in[SEEDBYTES + 0] = (uint8_t)nonce;
  in[SEEDBYTES + 1] = (uint8_t)(nonce >> 8);
  in[SEEDBYTES + 2] = 0;
  in[SEEDBYTES + 3] = 0;
  in[SEEDBYTES + 4] = 0;
  in[SEEDBYTES + 5] = 0;

  memset(buf + REJ_UNIFORM_BUFLEN, 0, 8);
  pseudoXOF((unsigned long long)REJ_UNIFORM_BUFLEN * 8ULL,
            in,
            (unsigned long long)sizeof(in) * 8ULL,
            buf);
  ctr = rej_uniform_avx(a->coeffs, buf);

  while(ctr < N) {
    block++;
    in[SEEDBYTES + 2] = (uint8_t)block;
    in[SEEDBYTES + 3] = (uint8_t)(block >> 8);
    in[SEEDBYTES + 4] = (uint8_t)(block >> 16);
    in[SEEDBYTES + 5] = (uint8_t)(block >> 24);
    memset(buf + REJ_UNIFORM_BUFLEN, 0, 8);
    pseudoXOF((unsigned long long)REJ_UNIFORM_BUFLEN * 8ULL,
              in,
              (unsigned long long)sizeof(in) * 8ULL,
              buf);
    ctr += rej_uniform_scalar(a->coeffs + ctr, N - ctr, buf, REJ_UNIFORM_BUFLEN);
  }
}

/* Rejection-sample coefficients in the small ETA range. */
static unsigned int rej_eta_scalar(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen) {
  unsigned int ctr = 0;
  unsigned int pos = 0;
  while(ctr < len && pos < buflen) {
    uint32_t t0 = buf[pos] & 0x0F;
    uint32_t t1 = buf[pos++] >> 4;

#if ETA == 2
    if(t0 < 15) {
      t0 = t0 - (205 * t0 >> 10) * 5;
      a[ctr++] = 2 - (int32_t)t0;
    }
    if(t1 < 15 && ctr < len) {
      t1 = t1 - (205 * t1 >> 10) * 5;
      a[ctr++] = 2 - (int32_t)t1;
    }
#elif ETA == 3
    if(t0 < 14) {
      t0 = t0 - (147 * t0 >> 10) * 7;
      a[ctr++] = 3 - (int32_t)t0;
    }
    if(t1 < 14 && ctr < len) {
      t1 = t1 - (147 * t1 >> 10) * 7;
      a[ctr++] = 3 - (int32_t)t1;
    }
#endif
  }
  return ctr;
}

/* Expand a seed and nonce into a small-noise polynomial. */
void poly_uniform_eta(poly *a, const uint8_t seed[CRHBYTES], uint16_t nonce) {
  uint8_t in[CRHBYTES + 2 + 4];
  uint8_t buf[POLY_UNIFORM_ETA_BUFLEN];
  unsigned int ctr = 0;
  uint32_t block = 0;

  memcpy(in, seed, CRHBYTES);
  in[CRHBYTES + 0] = (uint8_t)nonce;
  in[CRHBYTES + 1] = (uint8_t)(nonce >> 8);
  in[CRHBYTES + 2] = 0;
  in[CRHBYTES + 3] = 0;
  in[CRHBYTES + 4] = 0;
  in[CRHBYTES + 5] = 0;

  pseudoXOF((unsigned long long)sizeof(buf) * 8ULL,
            in,
            (unsigned long long)sizeof(in) * 8ULL,
            buf);
  ctr = rej_eta_avx(a->coeffs, buf);

  while(ctr < N) {
    block++;
    in[CRHBYTES + 2] = (uint8_t)block;
    in[CRHBYTES + 3] = (uint8_t)(block >> 8);
    in[CRHBYTES + 4] = (uint8_t)(block >> 16);
    in[CRHBYTES + 5] = (uint8_t)(block >> 24);
    pseudoXOF((unsigned long long)sizeof(buf) * 8ULL,
              in,
              (unsigned long long)sizeof(in) * 8ULL,
              buf);
    ctr += rej_eta_scalar(a->coeffs + ctr, N - ctr, buf, sizeof(buf));
  }
}

/* Expand a seed and nonce into the signing mask polynomial y. */
void poly_uniform_gamma1(poly *a, const uint8_t seed[CRHBYTES], uint16_t nonce) {
  uint8_t in[CRHBYTES + 2];
  uint8_t buf[POLYZ_PACKEDBYTES];
 
  memcpy(in, seed, CRHBYTES);
  in[CRHBYTES + 0] = (uint8_t)nonce;
  in[CRHBYTES + 1] = (uint8_t)(nonce >> 8);
  pseudoXOF((unsigned long long)sizeof(buf) * 8ULL,
            in,
            (unsigned long long)sizeof(in) * 8ULL,
            buf);
  polyz_unpack(a, buf);
}

void poly_uniform_4x(poly *a0,
                     poly *a1,
                     poly *a2,
                     poly *a3,
                     const uint8_t seed[SEEDBYTES],
                     uint16_t nonce0,
                     uint16_t nonce1,
                     uint16_t nonce2,
                     uint16_t nonce3) {
  uint8_t in0[SEEDBYTES + 2 + 4];
  uint8_t in1[SEEDBYTES + 2 + 4];
  uint8_t in2[SEEDBYTES + 2 + 4];
  uint8_t in3[SEEDBYTES + 2 + 4];
  uint8_t buf0[REJ_UNIFORM_BUFLEN + 8];
  uint8_t buf1[REJ_UNIFORM_BUFLEN + 8];
  uint8_t buf2[REJ_UNIFORM_BUFLEN + 8];
  uint8_t buf3[REJ_UNIFORM_BUFLEN + 8];
  unsigned int ctr0, ctr1, ctr2, ctr3;
  uint32_t block0 = 0;
  uint32_t block1 = 0;
  uint32_t block2 = 0;
  uint32_t block3 = 0;

  memcpy(in0, seed, SEEDBYTES);
  memcpy(in1, seed, SEEDBYTES);
  memcpy(in2, seed, SEEDBYTES);
  memcpy(in3, seed, SEEDBYTES);

  in0[SEEDBYTES + 0] = (uint8_t)nonce0;
  in0[SEEDBYTES + 1] = (uint8_t)(nonce0 >> 8);
  in1[SEEDBYTES + 0] = (uint8_t)nonce1;
  in1[SEEDBYTES + 1] = (uint8_t)(nonce1 >> 8);
  in2[SEEDBYTES + 0] = (uint8_t)nonce2;
  in2[SEEDBYTES + 1] = (uint8_t)(nonce2 >> 8);
  in3[SEEDBYTES + 0] = (uint8_t)nonce3;
  in3[SEEDBYTES + 1] = (uint8_t)(nonce3 >> 8);
  memset(in0 + SEEDBYTES + 2, 0, 4);
  memset(in1 + SEEDBYTES + 2, 0, 4);
  memset(in2 + SEEDBYTES + 2, 0, 4);
  memset(in3 + SEEDBYTES + 2, 0, 4);

  memset(buf0 + REJ_UNIFORM_BUFLEN, 0, 8);
  memset(buf1 + REJ_UNIFORM_BUFLEN, 0, 8);
  memset(buf2 + REJ_UNIFORM_BUFLEN, 0, 8);
  memset(buf3 + REJ_UNIFORM_BUFLEN, 0, 8);
  pseudoXOF_4x((unsigned long long)REJ_UNIFORM_BUFLEN * 8ULL,
               in0, in1, in2, in3,
               (unsigned long long)sizeof(in0) * 8ULL,
               buf0, buf1, buf2, buf3);
  ctr0 = rej_uniform_avx(a0->coeffs, buf0);
  ctr1 = rej_uniform_avx(a1->coeffs, buf1);
  ctr2 = rej_uniform_avx(a2->coeffs, buf2);
  ctr3 = rej_uniform_avx(a3->coeffs, buf3);

  while(ctr0 < N || ctr1 < N || ctr2 < N || ctr3 < N) {
    if(ctr0 < N) {
      block0++;
      in0[SEEDBYTES + 2] = (uint8_t)block0;
      in0[SEEDBYTES + 3] = (uint8_t)(block0 >> 8);
      in0[SEEDBYTES + 4] = (uint8_t)(block0 >> 16);
      in0[SEEDBYTES + 5] = (uint8_t)(block0 >> 24);
    }
    if(ctr1 < N) {
      block1++;
      in1[SEEDBYTES + 2] = (uint8_t)block1;
      in1[SEEDBYTES + 3] = (uint8_t)(block1 >> 8);
      in1[SEEDBYTES + 4] = (uint8_t)(block1 >> 16);
      in1[SEEDBYTES + 5] = (uint8_t)(block1 >> 24);
    }
    if(ctr2 < N) {
      block2++;
      in2[SEEDBYTES + 2] = (uint8_t)block2;
      in2[SEEDBYTES + 3] = (uint8_t)(block2 >> 8);
      in2[SEEDBYTES + 4] = (uint8_t)(block2 >> 16);
      in2[SEEDBYTES + 5] = (uint8_t)(block2 >> 24);
    }
    if(ctr3 < N) {
      block3++;
      in3[SEEDBYTES + 2] = (uint8_t)block3;
      in3[SEEDBYTES + 3] = (uint8_t)(block3 >> 8);
      in3[SEEDBYTES + 4] = (uint8_t)(block3 >> 16);
      in3[SEEDBYTES + 5] = (uint8_t)(block3 >> 24);
    }

    memset(buf0 + REJ_UNIFORM_BUFLEN, 0, 8);
    memset(buf1 + REJ_UNIFORM_BUFLEN, 0, 8);
    memset(buf2 + REJ_UNIFORM_BUFLEN, 0, 8);
    memset(buf3 + REJ_UNIFORM_BUFLEN, 0, 8);
    pseudoXOF_4x((unsigned long long)REJ_UNIFORM_BUFLEN * 8ULL,
                 in0, in1, in2, in3,
                 (unsigned long long)sizeof(in0) * 8ULL,
                 buf0, buf1, buf2, buf3);
    if(ctr0 < N)
      ctr0 += rej_uniform_scalar(a0->coeffs + ctr0, N - ctr0, buf0, REJ_UNIFORM_BUFLEN);
    if(ctr1 < N)
      ctr1 += rej_uniform_scalar(a1->coeffs + ctr1, N - ctr1, buf1, REJ_UNIFORM_BUFLEN);
    if(ctr2 < N)
      ctr2 += rej_uniform_scalar(a2->coeffs + ctr2, N - ctr2, buf2, REJ_UNIFORM_BUFLEN);
    if(ctr3 < N)
      ctr3 += rej_uniform_scalar(a3->coeffs + ctr3, N - ctr3, buf3, REJ_UNIFORM_BUFLEN);
  }
}

void poly_uniform_eta_4x(poly *a0,
                         poly *a1,
                         poly *a2,
                         poly *a3,
                         const uint8_t seed[CRHBYTES],
                         uint16_t nonce0,
                         uint16_t nonce1,
                         uint16_t nonce2,
                         uint16_t nonce3) {
  uint8_t in0[CRHBYTES + 2 + 4];
  uint8_t in1[CRHBYTES + 2 + 4];
  uint8_t in2[CRHBYTES + 2 + 4];
  uint8_t in3[CRHBYTES + 2 + 4];
  uint8_t buf0[POLY_UNIFORM_ETA_BUFLEN];
  uint8_t buf1[POLY_UNIFORM_ETA_BUFLEN];
  uint8_t buf2[POLY_UNIFORM_ETA_BUFLEN];
  uint8_t buf3[POLY_UNIFORM_ETA_BUFLEN];
  unsigned int ctr0, ctr1, ctr2, ctr3;
  uint32_t block0 = 0;
  uint32_t block1 = 0;
  uint32_t block2 = 0;
  uint32_t block3 = 0;

  memcpy(in0, seed, CRHBYTES);
  memcpy(in1, seed, CRHBYTES);
  memcpy(in2, seed, CRHBYTES);
  memcpy(in3, seed, CRHBYTES);

  in0[CRHBYTES + 0] = (uint8_t)nonce0;
  in0[CRHBYTES + 1] = (uint8_t)(nonce0 >> 8);
  in1[CRHBYTES + 0] = (uint8_t)nonce1;
  in1[CRHBYTES + 1] = (uint8_t)(nonce1 >> 8);
  in2[CRHBYTES + 0] = (uint8_t)nonce2;
  in2[CRHBYTES + 1] = (uint8_t)(nonce2 >> 8);
  in3[CRHBYTES + 0] = (uint8_t)nonce3;
  in3[CRHBYTES + 1] = (uint8_t)(nonce3 >> 8);
  memset(in0 + CRHBYTES + 2, 0, 4);
  memset(in1 + CRHBYTES + 2, 0, 4);
  memset(in2 + CRHBYTES + 2, 0, 4);
  memset(in3 + CRHBYTES + 2, 0, 4);

  pseudoXOF_4x((unsigned long long)sizeof(buf0) * 8ULL,
               in0, in1, in2, in3,
               (unsigned long long)sizeof(in0) * 8ULL,
               buf0, buf1, buf2, buf3);
  ctr0 = rej_eta_avx(a0->coeffs, buf0);
  ctr1 = rej_eta_avx(a1->coeffs, buf1);
  ctr2 = rej_eta_avx(a2->coeffs, buf2);
  ctr3 = rej_eta_avx(a3->coeffs, buf3);

  while(ctr0 < N || ctr1 < N || ctr2 < N || ctr3 < N) {
    if(ctr0 < N) {
      block0++;
      in0[CRHBYTES + 2] = (uint8_t)block0;
      in0[CRHBYTES + 3] = (uint8_t)(block0 >> 8);
      in0[CRHBYTES + 4] = (uint8_t)(block0 >> 16);
      in0[CRHBYTES + 5] = (uint8_t)(block0 >> 24);
    }
    if(ctr1 < N) {
      block1++;
      in1[CRHBYTES + 2] = (uint8_t)block1;
      in1[CRHBYTES + 3] = (uint8_t)(block1 >> 8);
      in1[CRHBYTES + 4] = (uint8_t)(block1 >> 16);
      in1[CRHBYTES + 5] = (uint8_t)(block1 >> 24);
    }
    if(ctr2 < N) {
      block2++;
      in2[CRHBYTES + 2] = (uint8_t)block2;
      in2[CRHBYTES + 3] = (uint8_t)(block2 >> 8);
      in2[CRHBYTES + 4] = (uint8_t)(block2 >> 16);
      in2[CRHBYTES + 5] = (uint8_t)(block2 >> 24);
    }
    if(ctr3 < N) {
      block3++;
      in3[CRHBYTES + 2] = (uint8_t)block3;
      in3[CRHBYTES + 3] = (uint8_t)(block3 >> 8);
      in3[CRHBYTES + 4] = (uint8_t)(block3 >> 16);
      in3[CRHBYTES + 5] = (uint8_t)(block3 >> 24);
    }

    pseudoXOF_4x((unsigned long long)sizeof(buf0) * 8ULL,
                 in0, in1, in2, in3,
                 (unsigned long long)sizeof(in0) * 8ULL,
                 buf0, buf1, buf2, buf3);
    if(ctr0 < N)
      ctr0 += rej_eta_scalar(a0->coeffs + ctr0, N - ctr0, buf0, sizeof(buf0));
    if(ctr1 < N)
      ctr1 += rej_eta_scalar(a1->coeffs + ctr1, N - ctr1, buf1, sizeof(buf1));
    if(ctr2 < N)
      ctr2 += rej_eta_scalar(a2->coeffs + ctr2, N - ctr2, buf2, sizeof(buf2));
    if(ctr3 < N)
      ctr3 += rej_eta_scalar(a3->coeffs + ctr3, N - ctr3, buf3, sizeof(buf3));
  }
}

void poly_uniform_gamma1_4x(poly *a0,
                            poly *a1,
                            poly *a2,
                            poly *a3,
                            const uint8_t seed[CRHBYTES],
                            uint16_t nonce0,
                            uint16_t nonce1,
                            uint16_t nonce2,
                            uint16_t nonce3) {
  uint8_t in0[CRHBYTES + 2];
  uint8_t in1[CRHBYTES + 2];
  uint8_t in2[CRHBYTES + 2];
  uint8_t in3[CRHBYTES + 2];
  uint8_t buf0[POLYZ_PACKEDBYTES];
  uint8_t buf1[POLYZ_PACKEDBYTES];
  uint8_t buf2[POLYZ_PACKEDBYTES];
  uint8_t buf3[POLYZ_PACKEDBYTES];

  memcpy(in0, seed, CRHBYTES);
  memcpy(in1, seed, CRHBYTES);
  memcpy(in2, seed, CRHBYTES);
  memcpy(in3, seed, CRHBYTES);
  in0[CRHBYTES + 0] = (uint8_t)nonce0;
  in0[CRHBYTES + 1] = (uint8_t)(nonce0 >> 8);
  in1[CRHBYTES + 0] = (uint8_t)nonce1;
  in1[CRHBYTES + 1] = (uint8_t)(nonce1 >> 8);
  in2[CRHBYTES + 0] = (uint8_t)nonce2;
  in2[CRHBYTES + 1] = (uint8_t)(nonce2 >> 8);
  in3[CRHBYTES + 0] = (uint8_t)nonce3;
  in3[CRHBYTES + 1] = (uint8_t)(nonce3 >> 8);

  pseudoXOF_4x((unsigned long long)sizeof(buf0) * 8ULL,
               in0, in1, in2, in3,
               (unsigned long long)sizeof(in0) * 8ULL,
               buf0, buf1, buf2, buf3);
  polyz_unpack(a0, buf0);
  polyz_unpack(a1, buf1);
  polyz_unpack(a2, buf2);
  polyz_unpack(a3, buf3);
}

static inline void unpack_gamma1_x8(__m256i *dst,
                                    const uint32_t t[8],
                                    uint32_t coeff_mask) {
  const __m256i gamma1 = _mm256_set1_epi32(GAMMA1);
  const __m256i mask = _mm256_set1_epi32((int32_t)coeff_mask);
  __m256i f = _mm256_loadu_si256((const __m256i *)t);

  f = _mm256_and_si256(f, mask);
  f = _mm256_sub_epi32(gamma1, f);
  _mm256_store_si256(dst, f);
}

/* Derive the sparse challenge polynomial from the challenge seed. */
void poly_challenge(poly *c, const uint8_t seed[CTILDEBYTES]) {
  uint8_t buf[CTILDEBYTES];
  uint64_t signs_0, signs_1;
  unsigned int pos = 0;

  pseudoXOF((unsigned long long)CTILDEBYTES * 8ULL,
            seed,
            (unsigned long long)CTILDEBYTES * 8ULL,
            buf);
  signs_0 = signs_1 = 0;
  for(unsigned i = 0; i < 8; i++) {
    signs_0 |= (uint64_t)buf[i] << (8*i);
#if N == 1024
    signs_1 |= (uint64_t)buf[i + 8] << (8*i);
#endif
  }
  pos = (N == 1024) ? 16 : 8;

  for(unsigned i = 0; i < N; i++)
    c->coeffs[i] = 0;

#if N == 1024
  for(unsigned i = N - TAU; i < N - TAU + 64; ++i) {
    uint16_t b;
    do {
      if(pos + 2 > sizeof(buf)) {
        pseudoXOF((unsigned long long)CTILDEBYTES * 8ULL,
                  buf,
                  (unsigned long long)CTILDEBYTES * 8ULL,
                  buf);
        pos = 0;
      }
      b = (uint16_t)buf[pos] | ((uint16_t)buf[pos + 1] << 8);
      b &= 0x03FF;
      pos += 2;
    } while(b > i);

    c->coeffs[i] = c->coeffs[b];
    c->coeffs[b] = (int32_t)(1 - 2*(signs_0 & 1));
    signs_0 >>= 1;
  }

  for(unsigned i = N - TAU + 64; i < N; ++i) {
    uint16_t b;
    do {
      if(pos + 2 > sizeof(buf)) {
        pseudoXOF((unsigned long long)CTILDEBYTES * 8ULL,
                  buf,
                  (unsigned long long)CTILDEBYTES * 8ULL,
                  buf);
        pos = 0;
      }
      b = (uint16_t)buf[pos] | ((uint16_t)buf[pos + 1] << 8);
      b &= 0x03FF;
      pos += 2;
    } while(b > i);

    c->coeffs[i] = c->coeffs[b];
    c->coeffs[b] = (int32_t)(1 - 2*(signs_1 & 1));
    signs_1 >>= 1;
  }
#else
  for(unsigned i = N - TAU; i < N; ++i) {
    uint16_t b;
    do {
      if(pos + 2 > sizeof(buf)) {
        pseudoXOF((unsigned long long)CTILDEBYTES * 8ULL,
                  buf,
                  (unsigned long long)CTILDEBYTES * 8ULL,
                  buf);
        pos = 0;
      }
      b = (uint16_t)buf[pos] | ((uint16_t)buf[pos + 1] << 8);
      b &= 0x01FF;
      pos += 2;
    } while(b > i);

    c->coeffs[i] = c->coeffs[b];
    c->coeffs[b] = (int32_t)(1 - 2*(signs_0 & 1));
    signs_0 >>= 1;
  }
#endif
}

/* Pack a polynomial with coefficients in [-ETA, ETA]. */
void polyeta_pack(uint8_t *r, const poly *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    uint8_t t[8];
    t[0] = (uint8_t)(ETA - a->coeffs[8*i+0]);
    t[1] = (uint8_t)(ETA - a->coeffs[8*i+1]);
    t[2] = (uint8_t)(ETA - a->coeffs[8*i+2]);
    t[3] = (uint8_t)(ETA - a->coeffs[8*i+3]);
    t[4] = (uint8_t)(ETA - a->coeffs[8*i+4]);
    t[5] = (uint8_t)(ETA - a->coeffs[8*i+5]);
    t[6] = (uint8_t)(ETA - a->coeffs[8*i+6]);
    t[7] = (uint8_t)(ETA - a->coeffs[8*i+7]);

    r[3*i+0]  = (uint8_t)((t[0] >> 0) | (t[1] << 3) | (t[2] << 6));
    r[3*i+1]  = (uint8_t)((t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7));
    r[3*i+2]  = (uint8_t)((t[5] >> 1) | (t[6] << 2) | (t[7] << 5));
  }
}

/* Unpack a polynomial with coefficients in [-ETA, ETA]. */
void polyeta_unpack(poly *r, const uint8_t *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0] =  (a[3*i+0] >> 0) & 7;
    r->coeffs[8*i+1] =  (a[3*i+0] >> 3) & 7;
    r->coeffs[8*i+2] = ((a[3*i+0] >> 6) | ((uint32_t)a[3*i+1] << 2)) & 7;
    r->coeffs[8*i+3] =  (a[3*i+1] >> 1) & 7;
    r->coeffs[8*i+4] =  (a[3*i+1] >> 4) & 7;
    r->coeffs[8*i+5] = ((a[3*i+1] >> 7) | ((uint32_t)a[3*i+2] << 1)) & 7;
    r->coeffs[8*i+6] =  (a[3*i+2] >> 2) & 7;
    r->coeffs[8*i+7] =  (a[3*i+2] >> 5) & 7;

    r->coeffs[8*i+0] = ETA - r->coeffs[8*i+0];
    r->coeffs[8*i+1] = ETA - r->coeffs[8*i+1];
    r->coeffs[8*i+2] = ETA - r->coeffs[8*i+2];
    r->coeffs[8*i+3] = ETA - r->coeffs[8*i+3];
    r->coeffs[8*i+4] = ETA - r->coeffs[8*i+4];
    r->coeffs[8*i+5] = ETA - r->coeffs[8*i+5];
    r->coeffs[8*i+6] = ETA - r->coeffs[8*i+6];
    r->coeffs[8*i+7] = ETA - r->coeffs[8*i+7];
  }
}

/* Pack the high bits t1 of the public key polynomial. */
void polyt1_pack(uint8_t *r, const poly *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    uint16_t t[8];
    uint32_t w0, w1, w2, w3;
    __m256i f = _mm256_load_si256(&a->vec[i]);
    __m128i g0 = _mm256_castsi256_si128(f);
    __m128i g1 = _mm256_extracti128_si256(f, 1);
    __m128i g = _mm_packus_epi32(g0, g1);

    _mm_storeu_si128((__m128i *)t, g);

    w0 = (uint32_t)t[0] | ((uint32_t)t[1] << 13) | ((uint32_t)t[2] << 26);
    w1 = ((uint32_t)t[2] >> 6) | ((uint32_t)t[3] << 7) | ((uint32_t)t[4] << 20);
    w2 = ((uint32_t)t[4] >> 12) | ((uint32_t)t[5] << 1) | ((uint32_t)t[6] << 14) | ((uint32_t)t[7] << 27);
    w3 = (uint32_t)t[7] >> 5;

    r[13*i + 0] = (uint8_t)w0;
    r[13*i + 1] = (uint8_t)(w0 >> 8);
    r[13*i + 2] = (uint8_t)(w0 >> 16);
    r[13*i + 3] = (uint8_t)(w0 >> 24);
    r[13*i + 4] = (uint8_t)w1;
    r[13*i + 5] = (uint8_t)(w1 >> 8);
    r[13*i + 6] = (uint8_t)(w1 >> 16);
    r[13*i + 7] = (uint8_t)(w1 >> 24);
    r[13*i + 8] = (uint8_t)w2;
    r[13*i + 9] = (uint8_t)(w2 >> 8);
    r[13*i + 10] = (uint8_t)(w2 >> 16);
    r[13*i + 11] = (uint8_t)(w2 >> 24);
    r[13*i + 12] = (uint8_t)w3;
  }
}

/* Unpack the high bits t1 of the public key polynomial. */
void polyt1_unpack(poly *r, const uint8_t *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    uint8_t buf[16] = {0};
    __m128i g;
    __m256i f;
    const __m256i shufbidx = _mm256_set_epi8(14,13,12,11,12,11,10,9,11,10,9,8,9,8,7,6,
                                             7,6,5,4,6,5,4,3,4,3,2,1,3,2,1,0);
    const __m256i srlvdidx = _mm256_set_epi32(3,6,1,4,7,2,5,0);
    const __m256i mask = _mm256_set1_epi32(0x1FFF);

    memcpy(buf, &a[13*i], 13);
    g = _mm_loadu_si128((const __m128i *)buf);
    f = _mm256_broadcastsi128_si256(g);
    f = _mm256_shuffle_epi8(f, shufbidx);
    f = _mm256_srlv_epi32(f, srlvdidx);
    f = _mm256_and_si256(f, mask);
    _mm256_store_si256(&r->vec[i], f);
  }
}

/* Pack the low bits t0 of the secret key polynomial. */
void polyt0_pack(uint8_t *r, const poly *a) {
  const __m256i half = _mm256_set1_epi32(1 << (D - 1));

  for(unsigned i = 0; i < N/8; ++i) {
    uint16_t t[8];
    uint32_t w0, w1, w2, w3;
    __m256i f = _mm256_load_si256(&a->vec[i]);
    __m128i g0, g1, g;

    f = _mm256_sub_epi32(half, f);
    g0 = _mm256_castsi256_si128(f);
    g1 = _mm256_extracti128_si256(f, 1);
    g = _mm_packus_epi32(g0, g1);
    _mm_storeu_si128((__m128i *)t, g);

    w0 = (uint32_t)t[0] | ((uint32_t)t[1] << 13) | ((uint32_t)t[2] << 26);
    w1 = ((uint32_t)t[2] >> 6) | ((uint32_t)t[3] << 7) | ((uint32_t)t[4] << 20);
    w2 = ((uint32_t)t[4] >> 12) | ((uint32_t)t[5] << 1) | ((uint32_t)t[6] << 14) | ((uint32_t)t[7] << 27);
    w3 = (uint32_t)t[7] >> 5;

    r[13*i + 0] = (uint8_t)w0;
    r[13*i + 1] = (uint8_t)(w0 >> 8);
    r[13*i + 2] = (uint8_t)(w0 >> 16);
    r[13*i + 3] = (uint8_t)(w0 >> 24);
    r[13*i + 4] = (uint8_t)w1;
    r[13*i + 5] = (uint8_t)(w1 >> 8);
    r[13*i + 6] = (uint8_t)(w1 >> 16);
    r[13*i + 7] = (uint8_t)(w1 >> 24);
    r[13*i + 8] = (uint8_t)w2;
    r[13*i + 9] = (uint8_t)(w2 >> 8);
    r[13*i + 10] = (uint8_t)(w2 >> 16);
    r[13*i + 11] = (uint8_t)(w2 >> 24);
    r[13*i + 12] = (uint8_t)w3;
  }
}

/* Unpack the low bits t0 of the secret key polynomial. */
void polyt0_unpack(poly *r, const uint8_t *a) {
  const __m256i shufbidx = _mm256_set_epi8(14,13,12,11,12,11,10,9,11,10,9,8,9,8,7,6,
                                           7,6,5,4,6,5,4,3,4,3,2,1,3,2,1,0);
  const __m256i srlvdidx = _mm256_set_epi32(3,6,1,4,7,2,5,0);
  const __m256i mask = _mm256_set1_epi32(0x1FFF);
  const __m256i half = _mm256_set1_epi32(1 << (D - 1));

  for(unsigned i = 0; i < N/8; ++i) {
    uint8_t buf[16] = {0};
    __m128i g;
    __m256i f;

    memcpy(buf, &a[13*i], 13);
    g = _mm_loadu_si128((const __m128i *)buf);
    f = _mm256_broadcastsi128_si256(g);
    f = _mm256_shuffle_epi8(f, shufbidx);
    f = _mm256_srlv_epi32(f, srlvdidx);
    f = _mm256_and_si256(f, mask);
    f = _mm256_sub_epi32(half, f);
    _mm256_store_si256(&r->vec[i], f);
  }
}

/* Pack the response polynomial z in the signature. */
void polyz_pack(uint8_t *r, const poly *a) {
#if GAMMA1 == (1 << 20)
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    __m256i f = _mm256_load_si256(&a->vec[i]);
    f = _mm256_sub_epi32(_mm256_set1_epi32(GAMMA1), f);
    f = _mm256_and_si256(f, _mm256_set1_epi32(0x1FFFFF));
    _mm256_storeu_si256((__m256i *)t, f);

    r[21*i+ 0]  =  t[0];
    r[21*i+ 1]  =  t[0] >>  8;
    r[21*i+ 2]  =  t[0] >> 16;
    r[21*i+ 2] |=  t[1] <<  5;
    r[21*i+ 3]  =  t[1] >>  3;
    r[21*i+ 4]  =  t[1] >> 11;
    r[21*i+ 5]  =  t[1] >> 19;
    r[21*i+ 5] |=  t[2] <<  2;
    r[21*i+ 6]  =  t[2] >>  6;
    r[21*i+ 7]  =  t[2] >> 14;
    r[21*i+ 7] |=  t[3] <<  7;
    r[21*i+ 8]  =  t[3] >>  1;
    r[21*i+ 9]  =  t[3] >>  9;
    r[21*i+10]  =  t[3] >> 17;
    r[21*i+10] |=  t[4] <<  4;
    r[21*i+11]  =  t[4] >>  4;
    r[21*i+12]  =  t[4] >> 12;
    r[21*i+13]  =  t[4] >> 20;
    r[21*i+13] |=  t[5] <<  1;
    r[21*i+14]  =  t[5] >>  7;
    r[21*i+15]  =  t[5] >> 15;
    r[21*i+15] |=  t[6] <<  6;
    r[21*i+16]  =  t[6] >>  2;
    r[21*i+17]  =  t[6] >> 10;
    r[21*i+18]  =  t[6] >> 18;
    r[21*i+18] |=  t[7] <<  3;
    r[21*i+19]  =  t[7] >>  5;
    r[21*i+20]  =  t[7] >> 13;
  }
#elif GAMMA1 == (1 << 21)
  {
    const __m128i gamma1 = _mm_set1_epi32(GAMMA1);
    const __m128i mask = _mm_set1_epi32(0x3FFFFF);
    const __m128i byte_mask = _mm_set1_epi32(0xFF);
    const __m128i srlv0 = _mm_set_epi32(6, 4, 2, 0);
    const __m128i srlv1 = _mm_set_epi32(14, 12, 10, 8);
    const __m128i srlv2 = _mm_set_epi32(22, 20, 18, 16);
    const __m128i sllv = _mm_set_epi32(2, 4, 6, 0);
    const __m128i shufbidx = _mm_set_epi8(-1, -1, -1, -1, -1, 7, 3, 10,
                                          6, 2, 9, 5, 1, 8, 4, 0);
    const __m128i zero = _mm_setzero_si128();

    for(unsigned i = 0; i < N/4; ++i) {
      uint8_t out[16];
      __m128i f = _mm_loadu_si128((const __m128i *)&a->coeffs[4*i]);
      __m128i g0, g1, g2, h, t;

      f = _mm_sub_epi32(gamma1, f);
      f = _mm_and_si128(f, mask);

      g0 = _mm_and_si128(_mm_srlv_epi32(f, srlv0), byte_mask);
      g1 = _mm_and_si128(_mm_srlv_epi32(f, srlv1), byte_mask);
      g2 = _mm_and_si128(_mm_srlv_epi32(f, srlv2), byte_mask);
      h = _mm_srli_si128(_mm_sllv_epi32(f, sllv), 4);
      h = _mm_and_si128(h, byte_mask);
      g2 = _mm_and_si128(_mm_or_si128(g2, h), byte_mask);

      g0 = _mm_packus_epi16(_mm_packus_epi32(g0, zero), zero);
      g1 = _mm_packus_epi16(_mm_packus_epi32(g1, zero), zero);
      g2 = _mm_packus_epi16(_mm_packus_epi32(g2, zero), zero);

      t = _mm_unpacklo_epi32(g0, g1);
      t = _mm_unpacklo_epi64(t, g2);
      t = _mm_shuffle_epi8(t, shufbidx);
      _mm_storeu_si128((__m128i *)out, t);
      memcpy(&r[11*i], out, 11);
    }
  }
#elif GAMMA1 == (1 << 22)
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    __m256i f = _mm256_load_si256(&a->vec[i]);
    f = _mm256_sub_epi32(_mm256_set1_epi32(GAMMA1), f);
    f = _mm256_and_si256(f, _mm256_set1_epi32(0x7FFFFF));
    _mm256_storeu_si256((__m256i *)t, f);

    r[23*i+ 0]  =  t[0];
    r[23*i+ 1]  =  t[0] >>  8;
    r[23*i+ 2]  =  t[0] >> 16;
    r[23*i+ 2] |=  t[1] <<  7;
    r[23*i+ 3]  =  t[1] >>  1;
    r[23*i+ 4]  =  t[1] >>  9;
    r[23*i+ 5]  =  t[1] >> 17;
    r[23*i+ 5] |=  t[2] <<  6;
    r[23*i+ 6]  =  t[2] >>  2;
    r[23*i+ 7]  =  t[2] >> 10;
    r[23*i+ 8]  =  t[2] >> 18;
    r[23*i+ 8] |=  t[3] <<  5;
    r[23*i+ 9]  =  t[3] >>  3;
    r[23*i+10]  =  t[3] >> 11;
    r[23*i+11]  =  t[3] >> 19;
    r[23*i+11] |=  t[4] <<  4;
    r[23*i+12]  =  t[4] >>  4;
    r[23*i+13]  =  t[4] >> 12;
    r[23*i+14]  =  t[4] >> 20;
    r[23*i+14] |=  t[5] <<  3;
    r[23*i+15]  =  t[5] >>  5;
    r[23*i+16]  =  t[5] >> 13;
    r[23*i+17]  =  t[5] >> 21;
    r[23*i+17] |=  t[6] <<  2;
    r[23*i+18]  =  t[6] >>  6;
    r[23*i+19]  =  t[6] >> 14;
    r[23*i+20]  =  t[6] >> 22;
    r[23*i+20] |=  t[7] <<  1;
    r[23*i+21]  =  t[7] >>  7;
    r[23*i+22]  =  t[7] >> 15;
  }
#endif
}

/* Unpack the response polynomial z in the signature. */
void polyz_unpack(poly *r, const uint8_t *a) {
#if GAMMA1 == (1 << 20)
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    t[0]  = (uint32_t)a[21*i+0];
    t[0] |= (uint32_t)a[21*i+1] << 8;
    t[0] |= (uint32_t)a[21*i+2] << 16;

    t[1]  = (uint32_t)a[21*i+2] >> 5;
    t[1] |= (uint32_t)a[21*i+3] << 3;
    t[1] |= (uint32_t)a[21*i+4] << 11;
    t[1] |= (uint32_t)a[21*i+5] << 19;

    t[2]  = (uint32_t)a[21*i+5] >> 2;
    t[2] |= (uint32_t)a[21*i+6] << 6;
    t[2] |= (uint32_t)a[21*i+7] << 14;

    t[3]  = (uint32_t)a[21*i+7] >> 7;
    t[3] |= (uint32_t)a[21*i+8] << 1;
    t[3] |= (uint32_t)a[21*i+9] << 9;
    t[3] |= (uint32_t)a[21*i+10] << 17;

    t[4]  = (uint32_t)a[21*i+10] >> 4;
    t[4] |= (uint32_t)a[21*i+11] << 4;
    t[4] |= (uint32_t)a[21*i+12] << 12;
    t[4] |= (uint32_t)a[21*i+13] << 20;

    t[5]  = (uint32_t)a[21*i+13] >> 1;
    t[5] |= (uint32_t)a[21*i+14] << 7;
    t[5] |= (uint32_t)a[21*i+15] << 15;

    t[6]  = (uint32_t)a[21*i+15] >> 6;
    t[6] |= (uint32_t)a[21*i+16] << 2;
    t[6] |= (uint32_t)a[21*i+17] << 10;
    t[6] |= (uint32_t)a[21*i+18] << 18;

    t[7]  = (uint32_t)a[21*i+18] >> 3;
    t[7] |= (uint32_t)a[21*i+19] << 5;
    t[7] |= (uint32_t)a[21*i+20] << 13;

    unpack_gamma1_x8(&r->vec[i], t, 0x1FFFFF);
  }
#elif GAMMA1 == (1 << 21)
  {
    const __m128i shufbidx = _mm_set_epi8(11,10,9,8,8,7,6,5,5,4,3,2,3,2,1,0);
    const __m128i srlvdidx = _mm_set_epi32(2,4,6,0);
    const __m128i mask = _mm_set1_epi32(0x3FFFFF);
    const __m128i gamma1 = _mm_set1_epi32(GAMMA1);

    for(unsigned i = 0; i < N/4; ++i) {
      uint8_t buf[16] = {0};
      __m128i f;

      memcpy(buf, &a[11*i], 11);
      f = _mm_loadu_si128((const __m128i *)buf);
      f = _mm_shuffle_epi8(f, shufbidx);
      f = _mm_srlv_epi32(f, srlvdidx);
      f = _mm_and_si128(f, mask);
      f = _mm_sub_epi32(gamma1, f);
      _mm_storeu_si128((__m128i *)&r->coeffs[4*i], f);
    }
  }
#elif GAMMA1 == (1 << 22)
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    t[0]  = (uint32_t)a[23*i+0];
    t[0] |= (uint32_t)a[23*i+1] << 8;
    t[0] |= (uint32_t)a[23*i+2] << 16;

    t[1]  = (uint32_t)a[23*i+2] >> 7;
    t[1] |= (uint32_t)a[23*i+3] << 1;
    t[1] |= (uint32_t)a[23*i+4] << 9;
    t[1] |= (uint32_t)a[23*i+5] << 17;

    t[2]  = (uint32_t)a[23*i+5] >> 6;
    t[2] |= (uint32_t)a[23*i+6] << 2;
    t[2] |= (uint32_t)a[23*i+7] << 10;
    t[2] |= (uint32_t)a[23*i+8] << 18;

    t[3]  = (uint32_t)a[23*i+8] >> 5;
    t[3] |= (uint32_t)a[23*i+9] << 3;
    t[3] |= (uint32_t)a[23*i+10] << 11;
    t[3] |= (uint32_t)a[23*i+11] << 19;

    t[4]  = (uint32_t)a[23*i+11] >> 4;
    t[4] |= (uint32_t)a[23*i+12] << 4;
    t[4] |= (uint32_t)a[23*i+13] << 12;
    t[4] |= (uint32_t)a[23*i+14] << 20;

    t[5]  = (uint32_t)a[23*i+14] >> 3;
    t[5] |= (uint32_t)a[23*i+15] << 5;
    t[5] |= (uint32_t)a[23*i+16] << 13;
    t[5] |= (uint32_t)a[23*i+17] << 21;

    t[6]  = (uint32_t)a[23*i+17] >> 2;
    t[6] |= (uint32_t)a[23*i+18] << 6;
    t[6] |= (uint32_t)a[23*i+19] << 14;
    t[6] |= (uint32_t)a[23*i+20] << 22;

    t[7]  = (uint32_t)a[23*i+20] >> 1;
    t[7] |= (uint32_t)a[23*i+21] << 7;
    t[7] |= (uint32_t)a[23*i+22] << 15;

    unpack_gamma1_x8(&r->vec[i], t, 0x7FFFFF);
  }
#endif
}

/* Pack the decomposed high bits w1 used in the challenge hash. */
void polyw1_pack(uint8_t *r, const poly *a) {
#if GAMMA2 == (Q-1)/32
  unsigned int i;
  __m256i f0, f1, f2, f3, f4, f5, f6, f7;
  const __m256i shift = _mm256_set1_epi16((16 << 8) + 1);
  const __m256i shufbidx = _mm256_set_epi8(15,14, 7, 6,13,12, 5, 4,11,10, 3, 2, 9, 8, 1, 0,
                                           15,14, 7, 6,13,12, 5, 4,11,10, 3, 2, 9, 8, 1, 0);

  for(i = 0; i < N/64; ++i) {
    f0 = _mm256_load_si256(&a->vec[8*i + 0]);
    f1 = _mm256_load_si256(&a->vec[8*i + 1]);
    f2 = _mm256_load_si256(&a->vec[8*i + 2]);
    f3 = _mm256_load_si256(&a->vec[8*i + 3]);
    f4 = _mm256_load_si256(&a->vec[8*i + 4]);
    f5 = _mm256_load_si256(&a->vec[8*i + 5]);
    f6 = _mm256_load_si256(&a->vec[8*i + 6]);
    f7 = _mm256_load_si256(&a->vec[8*i + 7]);
    f0 = _mm256_packus_epi32(f0, f1);
    f1 = _mm256_packus_epi32(f2, f3);
    f2 = _mm256_packus_epi32(f4, f5);
    f3 = _mm256_packus_epi32(f6, f7);
    f0 = _mm256_packus_epi16(f0, f1);
    f1 = _mm256_packus_epi16(f2, f3);
    f0 = _mm256_maddubs_epi16(f0, shift);
    f1 = _mm256_maddubs_epi16(f1, shift);
    f0 = _mm256_packus_epi16(f0, f1);
    f0 = _mm256_permute4x64_epi64(f0, 0xD8);
    f0 = _mm256_shuffle_epi8(f0, shufbidx);
    _mm256_storeu_si256((__m256i *)&r[32*i], f0);
  }
#elif GAMMA2 == (Q-1)/48
  for(unsigned i = 0; i < N/8; ++i) {
    uint16_t t[8];
    __m256i f = _mm256_load_si256(&a->vec[i]);
    __m128i g0 = _mm256_castsi256_si128(f);
    __m128i g1 = _mm256_extracti128_si256(f, 1);
    __m128i g = _mm_packus_epi32(g0, g1);

    _mm_storeu_si128((__m128i *)t, g);
    r[5*i+0] = (uint8_t)(t[0] | (t[1] << 5));
    r[5*i+1] = (uint8_t)((t[1] >> 3) | (t[2] << 2) | (t[3] << 7));
    r[5*i+2] = (uint8_t)((t[3] >> 1) | (t[4] << 4));
    r[5*i+3] = (uint8_t)((t[4] >> 4) | (t[5] << 1) | (t[6] << 6));
    r[5*i+4] = (uint8_t)((t[6] >> 2) | (t[7] << 3));
  }
#elif GAMMA2 == (Q-1)/96
  for(unsigned i = 0; i < N/4; ++i) {
    uint32_t t[4];
    __m128i f = _mm_loadu_si128((const __m128i *)&a->coeffs[4*i]);

    _mm_storeu_si128((__m128i *)t, f);
    r[3*i+0] = (uint8_t)(t[0] | (t[1] << 6));
    r[3*i+1] = (uint8_t)((t[1] >> 2) | (t[2] << 4));
    r[3*i+2] = (uint8_t)((t[2] >> 4) | (t[3] << 2));
  }
#endif
}
