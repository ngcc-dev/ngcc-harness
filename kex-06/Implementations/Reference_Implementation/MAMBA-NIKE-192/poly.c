#include "poly.h"
#include "reduce.h"
#include "randombytes.h"
#include "fips202.h"
#include "crypto_stream_chacha20.h"
#include "toom.h"
#include <string.h>

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

      /* emit coefficients whenever we have >= 13 bits */
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
 * Pointwise multiply â€?aliased to convolution (no NTT domain)
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
 * NTT wrappers â€?no-ops (polynomials stay in time domain)
 * ================================================================ */

void poly_ntt(poly *r)
{
  (void)r;
}

void poly_invntt(poly *r)
{
  (void)r;
}
