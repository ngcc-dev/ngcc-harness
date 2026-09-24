#include <stdint.h>
#include <stdlib.h>
#include "params.h"
#include "poly.h"
#include <string.h>
#include "ntt.h"
#include "reduce.h"
#include "cbd.h"
#include "symmetric.h"

#if defined(WEAVER_USE_AVX_NTT7681_ON) || defined(WEAVER_USE_AVX_NTT128_ON)
#include "ntt7681_avx.h"
#include "ntt3329_avx128.h"
#endif

/*************************************************
* Name:        poly_tobytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for WEAVER_POLYBYTES bytes)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tobytes(uint8_t r[WEAVER_POLYBYTES], const poly *a)
{
    unsigned int i;

#if WEAVER_QBITS == 12
    uint16_t t0, t1;

    for (i = 0; i < WEAVER_N / 2; i++) {

        t0 = a->coeffs[2 * i];
        t0 += ((int16_t)t0 >> 15) & WEAVER_Q;
        t1 = a->coeffs[2 * i + 1];
        t1 += ((int16_t)t1 >> 15) & WEAVER_Q;
        r[3 * i + 0] = (t0 >> 0);
        r[3 * i + 1] = (t0 >> 8) | (t1 << 4);
        r[3 * i + 2] = (t1 >> 4);
    }
#elif WEAVER_QBITS == 13
    uint16_t t[8];

    for (i = 0; i < WEAVER_N / 8; i++) {
        unsigned int j;
        for (j = 0; j < 8; j++) {
            t[j] = a->coeffs[8 * i + j];
            t[j] += ((int16_t)t[j] >> 15) & WEAVER_Q;
            t[j] &= 0x1FFF;
        }

        r[0] = (uint8_t)(t[0] >> 0);
        r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 5));
        r[2] = (uint8_t)(t[1] >> 3);
        r[3] = (uint8_t)((t[1] >> 11) | (t[2] << 2));
        r[4] = (uint8_t)((t[2] >> 6) | (t[3] << 7));
        r[5] = (uint8_t)(t[3] >> 1);
        r[6] = (uint8_t)((t[3] >> 9) | (t[4] << 4));
        r[7] = (uint8_t)(t[4] >> 4);
        r[8] = (uint8_t)((t[4] >> 12) | (t[5] << 1));
        r[9] = (uint8_t)((t[5] >> 7) | (t[6] << 6));
        r[10] = (uint8_t)(t[6] >> 2);
        r[11] = (uint8_t)((t[6] >> 10) | (t[7] << 3));
        r[12] = (uint8_t)(t[7] >> 5);
        r += 13;
    }
#else
#error "Unsupported WEAVER_QBITS"
#endif
}

/*************************************************
* Name:        poly_frombytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_tobytes
*
* Arguments:   - poly *r:          pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of WEAVER_POLYBYTES bytes)
**************************************************/
void poly_frombytes(poly *r, const uint8_t a[WEAVER_POLYBYTES])
{
    unsigned int i;

#if WEAVER_QBITS == 12
    for (i = 0; i < WEAVER_N / 2; i++) {
        r->coeffs[2 * i] = ((a[3 * i + 0] >> 0) | ((uint16_t)a[3 * i + 1] << 8)) & 0xFFF;
        r->coeffs[2 * i + 1] = ((a[3 * i + 1] >> 4) | ((uint16_t)a[3 * i + 2] << 4)) & 0xFFF;
    }
#elif WEAVER_QBITS == 13
    for (i = 0; i < WEAVER_N / 8; i++) {
        r->coeffs[8 * i + 0] = (int16_t)((((uint16_t)a[0] >> 0) | ((uint16_t)a[1] << 8)) & 0x1FFF);
        r->coeffs[8 * i + 1] = (int16_t)((((uint16_t)a[1] >> 5) | ((uint16_t)a[2] << 3) | ((uint16_t)a[3] << 11)) & 0x1FFF);
        r->coeffs[8 * i + 2] = (int16_t)((((uint16_t)a[3] >> 2) | ((uint16_t)a[4] << 6)) & 0x1FFF);
        r->coeffs[8 * i + 3] = (int16_t)((((uint16_t)a[4] >> 7) | ((uint16_t)a[5] << 1) | ((uint16_t)a[6] << 9)) & 0x1FFF);
        r->coeffs[8 * i + 4] = (int16_t)((((uint16_t)a[6] >> 4) | ((uint16_t)a[7] << 4) | ((uint16_t)a[8] << 12)) & 0x1FFF);
        r->coeffs[8 * i + 5] = (int16_t)((((uint16_t)a[8] >> 1) | ((uint16_t)a[9] << 7)) & 0x1FFF);
        r->coeffs[8 * i + 6] = (int16_t)((((uint16_t)a[9] >> 6) | ((uint16_t)a[10] << 2) | ((uint16_t)a[11] << 10)) & 0x1FFF);
        r->coeffs[8 * i + 7] = (int16_t)((((uint16_t)a[11] >> 3) | ((uint16_t)a[12] << 5)) & 0x1FFF);
        a += 13;
    }
#else
#error "Unsupported WEAVER_QBITS"
#endif
}

/*************************************************
* Name:        poly_getnoise_eta1
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter WEAVER_ETA1
*
* Arguments:   - poly *r:             pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length WEAVER_SYMBYTES bytes)
*              - uint8_t nonce:       one-byte input nonce
**************************************************/
void poly_getnoise_eta1(poly *r, const uint8_t seed[WEAVER_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[WEAVER_ETA1*WEAVER_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  cbd_eta1(r, buf);
}

void poly_getnoise_eta2(poly *r, const uint8_t seed[WEAVER_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[WEAVER_ETA2*WEAVER_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  cbd_eta2(r, buf);
}


/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
void poly_ntt(poly *r)
{
#if defined(WEAVER_USE_AVX_NTT7681_ON)
  ntt7681_avx(r->coeffs);
#elif defined(WEAVER_USE_AVX_NTT128_ON)
  ntt3329_avx128(r->coeffs);
#else
  ntt(r->coeffs);
#endif
  poly_reduce(r);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *a: pointer to in/output polynomial
**************************************************/
void poly_invntt_tomont(poly *r)
{
#if defined(WEAVER_USE_AVX_NTT7681_ON)
  invntt7681_avx(r->coeffs);
#elif defined(WEAVER_USE_AVX_NTT128_ON)
  invntt3329_avx128(r->coeffs);
#else
  invntt(r->coeffs);
#endif
}

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
// void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
// {
//   unsigned int i;
//   for(i=0;i<WEAVER_N/4;i++) {
//     basemul(&r->coeffs[4*i], &a->coeffs[4*i], &b->coeffs[4*i], zetas[64+i]);
//     basemul(&r->coeffs[4*i+2], &a->coeffs[4*i+2], &b->coeffs[4*i+2],
//             -zetas[64+i]);
//   }
// }

void poly_basemul_montgomery(poly *r, const poly *a, const poly *b) {
  unsigned int i;

#if WEAVER_N == 128
#if defined(WEAVER_USE_AVX_NTT128_ON)
  basemul3329_avx128(r->coeffs, a->coeffs, b->coeffs);
#else
  for(i = 0; i < WEAVER_N; ++i) {
    r->coeffs[i] = montgomery_reduce((int32_t)a->coeffs[i] * b->coeffs[i]);
  }
#endif

#elif WEAVER_N == 256
#if defined(WEAVER_USE_AVX_NTT7681_ON)
  basemul7681_avx(r->coeffs, a->coeffs, b->coeffs);
#else
  for(i = 0; i < WEAVER_N; ++i) {
    r->coeffs[i] = montgomery_reduce((int32_t)a->coeffs[i] * b->coeffs[i]);
  }
#endif

#elif WEAVER_N == 512
#if defined(WEAVER_USE_AVX_NTT7681_ON)
  basemul7681_avx(r->coeffs, a->coeffs, b->coeffs);
#else
  for(i = 0; i < WEAVER_N / 4; ++i) {
    basemul(&r->coeffs[4*i],   &a->coeffs[4*i],   &b->coeffs[4*i],    zetas[128 + i]);
    basemul(&r->coeffs[4*i+2], &a->coeffs[4*i+2], &b->coeffs[4*i+2], -zetas[128 + i]);
  }
#endif
#endif
}

/*************************************************
* Name:        poly_tomont
*
* Description: Inplace conversion of all coefficients of a polynomial
*              from normal domain to Montgomery domain
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_tomont(poly *r)
{
  unsigned int i;
  const int16_t f = (1ULL << 32) % WEAVER_Q;
  for(i=0;i<WEAVER_N;i++)
    r->coeffs[i] = montgomery_reduce((int32_t)r->coeffs[i]*f);
}

/*************************************************
* Name:        poly_reduce
*
* Description: Applies Barrett reduction to all coefficients of a polynomial
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_reduce(poly *r)
{
#if defined(WEAVER_USE_AVX_NTT7681_ON)
  poly7681_reduce_avx(r->coeffs);
#elif defined(WEAVER_USE_AVX_NTT128_ON)
  poly3329_reduce_avx128(r->coeffs);
#else
  unsigned int i;
  for(i=0;i<WEAVER_N;i++)
    r->coeffs[i] = barrett_reduce(r->coeffs[i]);
#endif
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b)
{
  unsigned int i;
  for(i=0;i<WEAVER_N;i++)
    r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b)
{
  unsigned int i;
  for(i=0;i< WEAVER_N;i++)
    r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
}
