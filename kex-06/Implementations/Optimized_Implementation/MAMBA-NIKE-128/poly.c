#include "poly.h"
#include "reduce.h"
#include "randombytes.h"
#include "fips202.h"
#include "crypto_stream_chacha20.h"
#include "toom.h"
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif

/* ================================================================
 * Internal serialization: each q-domain coefficient is stored in
 * exactly 16 bits (2 bytes). Public/message labels use separate
 * p-packed serialization in nike.c.
 * ================================================================ */

void poly_frombytes(poly *r, const unsigned char *a)
{
  int i;
  for(i = 0; i < PARAM_N; i++)
    r->coeffs[i] = ((uint16_t)a[2*i]) | ((uint16_t)a[2*i+1] << 8);
}

void poly_tobytes(unsigned char *r, const poly *p)
{
  int i;
  uint16_t t;
  for(i = 0; i < PARAM_N; i++)
  {
    t = p->coeffs[i] & (PARAM_Q - 1);
    r[2*i]   = t & 0xFF;
    r[2*i+1] = (t >> 8) & 0xFF;
  }
}

/* ================================================================
 * Uniform sampling over Z_q.
 *
 * Two paths:
 *   - q == 8192 = 2^13: compact 13-bit unpacking (100% acceptance,
 *     since every 13-bit chunk is already uniform mod 2^13).
 *   - otherwise: legacy 16-bit rejection sampling.
 * ================================================================ */

void poly_uniform(poly *a, const unsigned char *seed)
{
  uint64_t state[25];
  shake128_absorb(state, seed, NIKE_SEEDBYTES);

#if PARAM_Q == 8192
  /* -------- compact 13-bit unpacking (acceptance = 100%) -------- */
  {
    unsigned int ctr = 0;
    unsigned int buf_pos = SHAKE128_RATE;  /* force initial squeeze */
    unsigned int bits_in_acc = 0;
    uint32_t acc = 0;
    uint8_t buf[SHAKE128_RATE];

    while (ctr < PARAM_N) {
      /* refill buffer one rate-block at a time */
      if (buf_pos >= SHAKE128_RATE) {
        shake128_squeezeblocks(buf, 1, state);
        buf_pos = 0;
      }

      /* ingest one byte (8 bits) */
      acc |= (uint32_t)buf[buf_pos++] << bits_in_acc;
      bits_in_acc += 8;

      /* emit coefficients whenever we have ≥ 13 bits */
      while (bits_in_acc >= 13 && ctr < PARAM_N) {
        a->coeffs[ctr++] = (uint16_t)(acc & 0x1FFF);
        acc >>= 13;
        bits_in_acc -= 13;
      }
    }
  }
#else
  /* -------- legacy 16-bit rejection sampling -------- */
  {
    unsigned int pos = 0, ctr = 0;
    uint16_t val;
    unsigned int nblocks = 16;
    uint8_t buf[SHAKE128_RATE * nblocks];

    shake128_squeezeblocks((unsigned char *)buf, nblocks, state);

    while (ctr < PARAM_N) {
      val = buf[pos] | ((uint16_t)buf[pos+1] << 8);
#if PARAM_Q < 65536
      if (val < PARAM_Q)
        a->coeffs[ctr++] = val;
#else
      a->coeffs[ctr++] = val;
#endif
      pos += 2;
      if (pos > SHAKE128_RATE * nblocks - 2) {
        nblocks = 1;
        shake128_squeezeblocks((unsigned char *)buf, nblocks, state);
        pos = 0;
      }
    }
  }
#endif
}

/* ================================================================
 * Noise sampling: centered binomial Binomial(2*PARAM_K) - PARAM_K
 * For PARAM_K = 2: sum of 4 random bits - 2, range [-2, 2]
 * ================================================================ */

static unsigned int noise_bit(const unsigned char *buf, unsigned int pos)
{
  return (unsigned int)((buf[pos >> 3] >> (pos & 7)) & 1);
}

void poly_getnoise(poly *r, const unsigned char *seed, unsigned char nonce)
{
  enum { NOISE_BYTES = (PARAM_N * 2 * PARAM_K + 7) / 8 };
  unsigned char buf[NOISE_BYTES];
  unsigned char n[8];
  unsigned int bitpos = 0;
  int i, j;

  for(i = 1; i < 8; i++)
    n[i] = 0;
  n[0] = nonce;

  crypto_stream_chacha20(buf, NOISE_BYTES, n, seed);

  for(i = 0; i < PARAM_N; i++)
  {
    unsigned int a = 0;
    unsigned int b = 0;
    int centered;

    for(j = 0; j < PARAM_K; j++)
      a += noise_bit(buf, bitpos++);
    for(j = 0; j < PARAM_K; j++)
      b += noise_bit(buf, bitpos++);

    centered = (int)a - (int)b;
    r->coeffs[i] = (uint16_t)((centered + PARAM_Q) & (PARAM_Q - 1));
  }

  memset(buf, 0, sizeof(buf));
}

/* ================================================================
 * Negacyclic convolution in Z_q[x]/(x^n+1)
 * Uses Toom-Cook-4 for the full product, then folds modulo x^n+1.
 * ================================================================ */

void poly_convolution(poly *r, const poly *a, const poly *b)
{
  int i;
  int n = PARAM_N;
  int rlen = 2 * n - 1;
  int64_t aa[PARAM_N];
  int64_t bb[PARAM_N];
  int64_t prod[2 * PARAM_N - 1];

  /* Convert uint16_t -> int64_t */
  for(i = 0; i < n; i++) { aa[i] = a->coeffs[i]; bb[i] = b->coeffs[i]; }

  /* Full product over Z via Toom-Cook-4 */
  toom4_mul(prod, aa, bb, n);

  /* Fold into Z_q[x]/(x^n+1): r[i] = prod[i] - prod[i+n] for i=0..n-1 */
  for(i = 0; i < n; i++) {
    int64_t val = prod[i];
    if (i + n < rlen)
      val -= prod[i + n];
    r->coeffs[i] = (uint16_t)(val & (PARAM_Q - 1));
  }

  memset(aa, 0, sizeof(aa));
  memset(bb, 0, sizeof(bb));
  memset(prod, 0, sizeof(prod));
}

/* ================================================================
 * Constant-time AVX2 negacyclic multiplication for protocol products.
 *
 * The protocol always multiplies a q-domain polynomial by a centered-
 * binomial polynomial.  For q = 2^13, 16-bit lane arithmetic may wrap
 * modulo 2^16 throughout the accumulation; the final q-1 mask then gives
 * exactly the same result modulo q because q divides 2^16.
 *
 * The AVX2 path uses fixed public loop bounds and fixed memory accesses.
 * It processes 64 output coefficients with four 16-lane accumulators.
 * Non-AVX2 builds retain the audited Toom-Cook-4 convolution fallback.
 * ================================================================ */

#if defined(__AVX2__)
static int16_t poly_center_q(uint16_t x)
{
  uint32_t ux = (uint32_t)(x & (PARAM_Q - 1));
  uint32_t upper = (ux + (PARAM_Q >> 1)) >> LOG2Q;
  return (int16_t)((int32_t)ux - (int32_t)(upper * PARAM_Q));
}
#endif

void poly_mul_small(poly *r, const poly *a, const poly *small)
{
#if defined(__AVX2__)
  uint16_t ext[2 * PARAM_N] __attribute__((aligned(32)));
  int16_t scoeff[PARAM_N] __attribute__((aligned(32)));
  const __m256i qmask = _mm256_set1_epi16((short)(PARAM_Q - 1));
  int i;
  int j;

  /*
   * ext = (-a_0,...,-a_{n-1},a_0,...,a_{n-1}).  For coefficient j of
   * small, ext[n-j .. n-j+n-1] is the corresponding negacyclic shift.
   */
  for(i = 0; i < PARAM_N; i++) {
    uint16_t x = (uint16_t)(a->coeffs[i] & (PARAM_Q - 1));
    ext[i] = (uint16_t)(0u - (uint32_t)x);
    ext[PARAM_N + i] = x;
    scoeff[i] = poly_center_q(small->coeffs[i]);
  }

  i = 0;
  for(; i + 64 <= PARAM_N; i += 64) {
    __m256i acc0 = _mm256_setzero_si256();
    __m256i acc1 = _mm256_setzero_si256();
    __m256i acc2 = _mm256_setzero_si256();
    __m256i acc3 = _mm256_setzero_si256();

    for(j = 0; j < PARAM_N; j++) {
      const uint16_t *w = ext + PARAM_N - j + i;
      __m256i sj = _mm256_set1_epi16(scoeff[j]);
      __m256i a0 = _mm256_loadu_si256((const __m256i *)(const void *)(w +  0));
      __m256i a1 = _mm256_loadu_si256((const __m256i *)(const void *)(w + 16));
      __m256i a2 = _mm256_loadu_si256((const __m256i *)(const void *)(w + 32));
      __m256i a3 = _mm256_loadu_si256((const __m256i *)(const void *)(w + 48));

      acc0 = _mm256_add_epi16(acc0, _mm256_mullo_epi16(a0, sj));
      acc1 = _mm256_add_epi16(acc1, _mm256_mullo_epi16(a1, sj));
      acc2 = _mm256_add_epi16(acc2, _mm256_mullo_epi16(a2, sj));
      acc3 = _mm256_add_epi16(acc3, _mm256_mullo_epi16(a3, sj));
    }

    acc0 = _mm256_and_si256(acc0, qmask);
    acc1 = _mm256_and_si256(acc1, qmask);
    acc2 = _mm256_and_si256(acc2, qmask);
    acc3 = _mm256_and_si256(acc3, qmask);
    _mm256_storeu_si256((__m256i *)(void *)(r->coeffs + i +  0), acc0);
    _mm256_storeu_si256((__m256i *)(void *)(r->coeffs + i + 16), acc1);
    _mm256_storeu_si256((__m256i *)(void *)(r->coeffs + i + 32), acc2);
    _mm256_storeu_si256((__m256i *)(void *)(r->coeffs + i + 48), acc3);
  }

  for(; i < PARAM_N; i += 16) {
    __m256i acc = _mm256_setzero_si256();
    for(j = 0; j < PARAM_N; j++) {
      const uint16_t *w = ext + PARAM_N - j + i;
      __m256i sj = _mm256_set1_epi16(scoeff[j]);
      __m256i av = _mm256_loadu_si256((const __m256i *)(const void *)w);
      acc = _mm256_add_epi16(acc, _mm256_mullo_epi16(av, sj));
    }
    acc = _mm256_and_si256(acc, qmask);
    _mm256_storeu_si256((__m256i *)(void *)(r->coeffs + i), acc);
  }

  memset(ext, 0, sizeof(ext));
  memset(scoeff, 0, sizeof(scoeff));
#else
  poly_convolution(r, a, small);
#endif
}

/* ================================================================
 * Generic pointwise multiply — retained as the Toom-Cook fallback.
 * ================================================================ */

void poly_pointwise(poly *r, const poly *a, const poly *b)
{
  poly_convolution(r, a, b);
}

/* ================================================================
 * Addition: r = a + b mod q
 * ================================================================ */

void poly_add(poly *r, const poly *a, const poly *b)
{
  int i;
  for(i = 0; i < PARAM_N; i++)
    r->coeffs[i] = (uint16_t)(((uint32_t)a->coeffs[i] + (uint32_t)b->coeffs[i]) & (PARAM_Q - 1));
}

/* ================================================================
 * NTT wrappers — no-ops (polynomials stay in time domain)
 * ================================================================ */

void poly_ntt(poly *r)
{
  (void)r;
}

void poly_invntt(poly *r)
{
  (void)r;
}
