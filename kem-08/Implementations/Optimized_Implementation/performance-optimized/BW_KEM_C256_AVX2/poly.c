#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "cbd.h"
#include "symmetric.h"
#include "verify.h"
#include "BWcoding.h"
#include "iccs_xof_avx2.h"

/*************************************************
* Name:        poly_compress
*
* Description: Compression and subsequent serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (of length KYBER_POLYCOMPRESSEDBYTES)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_compress(uint8_t r[KYBER_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i,j;
  int16_t u;
  uint32_t d0;
  uint8_t t[8];

#if (KYBER_POLYCOMPRESSEDBYTES == 128)

  for(i=0;i<KYBER_N/4;i++) {
    for(j=0;j<4;j++) {
      // map to positive standard representatives
      u  = a->coeffs[4*i+j];
      u += (u >> 15) & KYBER_Q;
/*    t[j] = ((((uint16_t)u << 4) + KYBER_Q/2)/KYBER_Q) & 15; */
      d0 = u << 4;
      d0 += 1664;
      d0 *= 315;
      d0 >>= 20;
      t[j] = d0 & 0xf;
    }

    r[0] = t[0] | (t[1] << 4);
    r[1] = t[2] | (t[3] << 4);
    r += 2;
  }
#elif (KYBER_POLYCOMPRESSEDBYTES == 160)

  for(i=0;i<KYBER_N/8;i++) {
    for(j=0;j<8;j++) {
      // map to positive standard representatives
      u  = a->coeffs[8*i+j];
      u += (u >> 15) & KYBER_Q;
/*    t[j] = ((((uint16_t)u << 5) + KYBER_Q/2)/KYBER_Q) & 31; */
      d0 = u << 5;
      d0 += 1664;
      d0 *= 315;
      d0 >>= 20;
      t[j] = d0 & 0x1f;
    }

    r[0] = (t[0] >> 0) | (t[1] << 5);
    r[1] = (t[1] >> 3) | (t[2] << 2) | (t[3] << 7);
    r[2] = (t[3] >> 1) | (t[4] << 4);
    r[3] = (t[4] >> 4) | (t[5] << 1) | (t[6] << 6);
    r[4] = (t[6] >> 2) | (t[7] << 3);
    r += 5;
  }
#else
#error "KYBER_POLYCOMPRESSEDBYTES needs to be in {128, 160}"
#endif
}

/*************************************************
* Name:        poly_decompress
*
* Description: De-serialization and subsequent decompression of a polynomial;
*              approximate inverse of poly_compress
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of length KYBER_POLYCOMPRESSEDBYTES bytes)
**************************************************/
void poly_decompress(poly *r, const uint8_t a[KYBER_POLYCOMPRESSEDBYTES])
{
  unsigned int i;

#if (KYBER_POLYCOMPRESSEDBYTES == 128)
  for(i=0;i<KYBER_N/2;i++) {
    r->coeffs[2*i+0] = (((uint16_t)(a[0] & 0xf)*KYBER_Q) + 8) >> 4;
    r->coeffs[2*i+1] = (((uint16_t)(a[0] >> 4)*KYBER_Q) + 8) >> 4;
    a += 1;
  }
#elif (KYBER_POLYCOMPRESSEDBYTES == 160)
  unsigned int j;
  uint8_t t[8];
  for(i=0;i<KYBER_N/8;i++) {
    t[0] = (a[0] >> 0);
    t[1] = (a[0] >> 5) | (a[1] << 3);
    t[2] = (a[1] >> 2);
    t[3] = (a[1] >> 7) | (a[2] << 1);
    t[4] = (a[2] >> 4) | (a[3] << 4);
    t[5] = (a[3] >> 1);
    t[6] = (a[3] >> 6) | (a[4] << 2);
    t[7] = (a[4] >> 3);
    a += 5;

    for(j=0;j<8;j++)
      r->coeffs[8*i+j] = ((uint32_t)(t[j] & 0x1f)*KYBER_Q + 16) >> 5;
  }
#else
#error "KYBER_POLYCOMPRESSEDBYTES needs to be in {128, 160}"
#endif
}

/*************************************************
* Name:        poly_tobytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for KYBER_POLYBYTES bytes)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tobytes(uint8_t r[KYBER_POLYBYTES], const poly *a)
{
  ntttobytes_avx(r, a->coeffs, qdata.coeffs);
}

/*************************************************
* Name:        poly_frombytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_tobytes
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of KYBER_POLYBYTES bytes)
**************************************************/
void poly_frombytes(poly *r, const uint8_t a[KYBER_POLYBYTES])
{
  nttfrombytes_avx(r->coeffs, a, qdata.coeffs);
}

/*************************************************
* Name:        poly_frommsg
*
* Description: Convert 32-byte message to polynomial
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
**************************************************/
void poly_frommsg(poly *r, const uint8_t msg[KYBER_INDCPA_MSGBYTES])
{
  int i,j;
  uint32_t m;
  uint64_t mh;
  int16_t mask1,mask2;

#if (KYBER_INDCPA_MSGBYTES != KYBER_N/8)
#error "KYBER_INDCPA_MSGBYTES must be equal to KYBER_N/8 bytes!"
#endif

  for(i=0;i<KYBER_N/32;i++) {
    m = (msg[4 * i]) | (msg[4 * i + 1] << 8) | (msg[4 * i + 2] << 16) | (msg[4 * i + 3] << 24);
    mh = encode_bw32(m);
    for(j=0;j<32;j++) {
      mask1 = -((mh >> (2*j)) & 1);
      mask2 = -((mh >> (2*j + 1)) & 1);
      // r->coeffs[32*i+j] = (mask1 & (KYBER_Q>>2)) + (mask2 & ((KYBER_Q>>1) + 1));
      r->coeffs[32*i+j] = (mask1 & 832) + (mask2 & 1664);
    }
  }
}

/*************************************************
* Name:        poly_tomsg
*
* Description: Convert polynomial to 32-byte message
*
* Arguments:   - uint8_t *msg: pointer to output message
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tomsg(uint8_t msg[KYBER_INDCPA_MSGBYTES], const poly *a)
{
  unsigned int i,j;
  uint32_t t;
  int16_t vec[32];
  int16_t u;
  uint64_t d0;

  for(i=0;i<KYBER_N/32;i++) {
    for(j=0;j<32;j++) {
      // map to positive standard representatives
      u  = a->coeffs[32*i+j];
      u += (u >> 15) & KYBER_Q;
/*    t[j] = ((((uint16_t)u << 12) + KYBER_Q/2)/KYBER_Q) & 0xfff; */
      d0 = u << 12;
      d0 += 1664;
      d0 *= 2580335;
      d0 >>= 33;
      vec[31 - j] = d0 & 0xfff;
    }
    t = decode_bw32(vec);
    msg[4*i] = (t >> 0) & 0xff;
    msg[4*i+1] = (t >> 8) & 0xff;
    msg[4*i+2] = (t >> 16) & 0xff;
    msg[4*i+3] = (t >> 24) & 0xff;
  }
}

/*************************************************
* Name:        poly_getnoise_eta1
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter KYBER_ETA1
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length KYBER_SYMBYTES bytes)
*              - uint8_t nonce: one-byte input nonce
**************************************************/
void poly_getnoise_eta1(poly *r, const uint8_t seed[KYBER_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[KYBER_ETA1*KYBER_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta1(r, buf);
}

void poly_getnoise_eta1_4x(poly *r0,
                           poly *r1,
                           poly *r2,
                           poly *r3,
                           const uint8_t seed[KYBER_SYMBYTES],
                           uint8_t nonce0)
{
  uint8_t buf0[KYBER_ETA1 * KYBER_N / 4];
  uint8_t buf1[KYBER_ETA1 * KYBER_N / 4];
  uint8_t buf2[KYBER_ETA1 * KYBER_N / 4];
  uint8_t buf3[KYBER_ETA1 * KYBER_N / 4];
  uint8_t *outs[4] = {buf0, buf1, buf2, buf3};

  kyber_iccs_prf4(outs, sizeof(buf0), seed, nonce0);
  poly_cbd_eta1(r0, buf0);
  poly_cbd_eta1(r1, buf1);
  poly_cbd_eta1(r2, buf2);
  poly_cbd_eta1(r3, buf3);
}

/*************************************************
* Name:        poly_getnoise_eta2
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter KYBER_ETA2
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length KYBER_SYMBYTES bytes)
*              - uint8_t nonce: one-byte input nonce
**************************************************/
void poly_getnoise_eta2(poly *r, const uint8_t seed[KYBER_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[KYBER_ETA2*KYBER_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta2(r, buf);
}

void poly_getnoise_eta2_4x(poly *r0,
                           poly *r1,
                           poly *r2,
                           poly *r3,
                           const uint8_t seed[KYBER_SYMBYTES],
                           uint8_t nonce0)
{
  uint8_t buf0[KYBER_ETA2 * KYBER_N / 4];
  uint8_t buf1[KYBER_ETA2 * KYBER_N / 4];
  uint8_t buf2[KYBER_ETA2 * KYBER_N / 4];
  uint8_t buf3[KYBER_ETA2 * KYBER_N / 4];
  uint8_t *outs[4] = {buf0, buf1, buf2, buf3};

  kyber_iccs_prf4(outs, sizeof(buf0), seed, nonce0);
  poly_cbd_eta2(r0, buf0);
  poly_cbd_eta2(r1, buf1);
  poly_cbd_eta2(r2, buf2);
  poly_cbd_eta2(r3, buf3);
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
  ntt_avx(r->coeffs, qdata.coeffs);
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
  invntt_avx(r->coeffs, qdata.coeffs);
}

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
{
  basemul_avx(r->coeffs, a->coeffs, b->coeffs, qdata.coeffs);
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
  tomont_avx(r->coeffs, qdata.coeffs);
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
  reduce_avx(r->coeffs, qdata.coeffs);
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials; no modular reduction is performed
*
* Arguments: - poly *r: pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b)
{
  unsigned int i;
  for(i=0;i<KYBER_N;i += 16) {
    __m256i va = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
    __m256i vb = _mm256_loadu_si256((const __m256i *)(b->coeffs + i));
    _mm256_storeu_si256((__m256i *)(r->coeffs + i), _mm256_add_epi16(va, vb));
  }
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials; no modular reduction is performed
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b)
{
  unsigned int i;
  for(i=0;i<KYBER_N;i += 16) {
    __m256i va = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
    __m256i vb = _mm256_loadu_si256((const __m256i *)(b->coeffs + i));
    _mm256_storeu_si256((__m256i *)(r->coeffs + i), _mm256_sub_epi16(va, vb));
  }
}
