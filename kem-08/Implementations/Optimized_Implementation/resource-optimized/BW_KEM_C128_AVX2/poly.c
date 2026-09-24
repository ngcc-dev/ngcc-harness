#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "cbd.h"
#include "BWcoding.h"
#include "symmetric.h"

/*************************************************
* Name:        poly_compress
*
* Description: Compression and subsequent serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (of length KYBER_POLYCOMPRESSEDBYTES)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_compress(uint8_t r[BWKEM128_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i,j;
  int16_t u;
  uint32_t d0;
  uint8_t t[8];

#if (BWKEM128_POLYCOMPRESSEDBYTES == 128)

  for(i=0;i<BWKEM128_N/4;i++) {
    for(j=0;j<4;j++) {
      // map to positive standard representatives
      u  = a->coeffs[4*i+j];
      u += (u >> 15) & BWKEM128_Q;
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
#elif (BWKEM128_POLYCOMPRESSEDBYTES == 160)

  for(i=0;i<BWKEM128_N/8;i++) {
    for(j=0;j<8;j++) {
      // map to positive standard representatives
      u  = a->coeffs[8*i+j];
      u += (u >> 15) & BWKEM128_Q;
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
#error "BWKEM128_POLYCOMPRESSEDBYTES needs to be in {128, 160}"
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
void poly_decompress(poly *r, const uint8_t a[BWKEM128_POLYCOMPRESSEDBYTES])
{
  unsigned int i;

#if (BWKEM128_POLYCOMPRESSEDBYTES == 128)
  for(i=0;i<BWKEM128_N/2;i++) {
    r->coeffs[2*i+0] = (((uint16_t)(a[0] & 0xf)*BWKEM128_Q) + 8) >> 4;
    r->coeffs[2*i+1] = (((uint16_t)(a[0] >> 4)*BWKEM128_Q) + 8) >> 4;
    a += 1;
  }
#elif (BWKEM128_POLYCOMPRESSEDBYTES == 160)
  unsigned int j;
  uint8_t t[8];
  for(i=0;i<BWKEM128_N/8;i++) {
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
      r->coeffs[8*i+j] = ((uint32_t)(t[j] & 0x1f)*BWKEM128_Q + 16) >> 5;
  }
#else
#error "BWKEM128_POLYCOMPRESSEDBYTES needs to be in {128, 160}"
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
void poly_tobytes(uint8_t r[BWKEM128_POLYBYTES], const poly *a)
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
void poly_frombytes(poly *r, const uint8_t a[BWKEM128_POLYBYTES])
{
  nttfrombytes_avx(r->coeffs, a, qdata.coeffs);
}

/*************************************************
* Name:        poly_frommsg
*
* Description: Convert active IND-CPA message to polynomial
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
**************************************************/
void poly_frommsg(poly *r, const uint8_t msg[BWKEM128_INDCPA_MSGBYTES])
{
  codec_encode(r->coeffs, msg);
}

static inline __m256i bwkem128_barrett_centered_avx2(__m256i x)
{
  /* Mirrors scalar barrett_reduce(): centered representative mod KYBER_Q. */
  const __m256i V = _mm256_set1_epi32(20159); /* ((1<<26) + Q/2)/Q */
  const __m256i Q = _mm256_set1_epi32(BWKEM128_Q);
  const __m256i bias = _mm256_set1_epi32(1 << 25);

  __m256i t = _mm256_mullo_epi32(V, x);
  t = _mm256_add_epi32(t, bias);
  t = _mm256_srai_epi32(t, 26);
  t = _mm256_mullo_epi32(t, Q);
  return _mm256_sub_epi32(x, t);
}

static inline __m256i bwkem128_div_by_q_avx2(__m256i y)
{
  /*
   * Returns floor(y / KYBER_Q) for y >= 0 with y < 2^32.
   * Uses unsigned magic M = ceil(2^32 / Q) = 1290255; M*(Q-1) < 2^32 so the
   * approximation is exact for the full unsigned 32-bit range, in particular
   * for our input range y in [0, 2^23].
   */
  const __m256i M = _mm256_set1_epi32(1290255);

  __m256i prod_even = _mm256_mul_epu32(y, M);                  /* 4 x i64 */
  __m256i y_odd = _mm256_srli_epi64(y, 32);
  __m256i prod_odd = _mm256_mul_epu32(y_odd, M);               /* 4 x i64 */

  __m256i hi_even = _mm256_srli_epi64(prod_even, 32);          /* lanes 0,2,4,6 */
  __m256i hi_odd_shifted = _mm256_and_si256(prod_odd,
      _mm256_set1_epi64x((int64_t)0xFFFFFFFF00000000LL));      /* lanes 1,3,5,7 */

  return _mm256_or_si256(hi_even, hi_odd_shifted);
}

static inline __m256i bwkem128_modswitch_block_avx2(__m256i x)
{
  const __m256i QHAT = _mm256_set1_epi32(BWKEM128_QHAT);
  const __m256i half_Q = _mm256_set1_epi32(BWKEM128_Q / 2);
  const __m256i mask = _mm256_set1_epi32(BWKEM128_QHAT - 1);

  __m256i centered = bwkem128_barrett_centered_avx2(x);
  __m256i num = _mm256_mullo_epi32(centered, QHAT);
  __m256i abs_num = _mm256_abs_epi32(num);
  __m256i abs_num_biased = _mm256_add_epi32(abs_num, half_Q);
  __m256i div = bwkem128_div_by_q_avx2(abs_num_biased);
  __m256i scaled = _mm256_sign_epi32(div, num);
  return _mm256_and_si256(scaled, mask);
}

static void poly_modswitch_q_to_qhat(poly *r, const poly *a)
{
  unsigned int i;

  for(i = 0; i < BWKEM128_N; i += 16) {
    __m256i v16 = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
    __m128i lo = _mm256_castsi256_si128(v16);
    __m128i hi = _mm256_extracti128_si256(v16, 1);
    __m256i x_lo = _mm256_cvtepi16_epi32(lo);
    __m256i x_hi = _mm256_cvtepi16_epi32(hi);

    __m256i res_lo = bwkem128_modswitch_block_avx2(x_lo);
    __m256i res_hi = bwkem128_modswitch_block_avx2(x_hi);

    /* Each lane is in [0, QHAT-1] = [0, 4095], no saturation in packus. */
    __m256i packed = _mm256_packus_epi32(res_lo, res_hi);
    packed = _mm256_permute4x64_epi64(packed, 0xD8);

    _mm256_storeu_si256((__m256i *)(r->coeffs + i), packed);
  }
}

static void poly_tomsg_qhat(uint8_t msg[BWKEM128_INDCPA_MSGBYTES], const poly *a)
{
  codec_decode(msg, a->coeffs);
}

/*************************************************
* Name:        poly_tomsg
*
* Description: Convert polynomial to active IND-CPA message
*
* Arguments:   - uint8_t *msg: pointer to output message
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tomsg(uint8_t msg[BWKEM128_INDCPA_MSGBYTES], const poly *a)
{
  poly qhat_poly;

  poly_modswitch_q_to_qhat(&qhat_poly, a);
  poly_tomsg_qhat(msg, &qhat_poly);
}

static void poly_zero(poly *r)
{
  unsigned int i;

  for(i = 0; i < BWKEM128_N; i++) {
    r->coeffs[i] = 0;
  }
}

/*************************************************
* Name:        poly_getnoise_eta
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with an explicit eta parameter.
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length KYBER_SYMBYTES bytes)
*              - uint8_t nonce: one-byte input nonce
*              - unsigned int eta: centered binomial parameter
**************************************************/
static void poly_getnoise_eta(poly *r,
                              const uint8_t seed[BWKEM128_SYMBYTES],
                              uint8_t nonce,
                              unsigned int eta)
{
  uint8_t buf[BWKEM128_MAX_SUPPORTED_ETA * BWKEM128_N / 4];
  size_t buflen;

  if(eta == 0 || eta > BWKEM128_MAX_SUPPORTED_ETA) {
    poly_zero(r);
    return;
  }

  buflen = ((size_t)eta * BWKEM128_N) / 4;
  prf(buf, buflen, seed, nonce);
  if(eta == BWKEM128_ETA_S)
    poly_cbd_eta1(r, buf);
  else if(eta == BWKEM128_ETA_CT)
    poly_cbd_eta2(r, buf);
  else
    poly_zero(r);
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
void poly_getnoise_eta1(poly *r, const uint8_t seed[BWKEM128_SYMBYTES], uint8_t nonce)
{
  poly_getnoise_eta(r, seed, nonce, BWKEM128_ETA_S);
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
void poly_getnoise_eta2(poly *r, const uint8_t seed[BWKEM128_SYMBYTES], uint8_t nonce)
{
  poly_getnoise_eta(r, seed, nonce, BWKEM128_ETA_CT);
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
  for(i=0;i<BWKEM128_N;i += 16) {
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
  for(i=0;i<BWKEM128_N;i += 16) {
    __m256i va = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
    __m256i vb = _mm256_loadu_si256((const __m256i *)(b->coeffs + i));
    _mm256_storeu_si256((__m256i *)(r->coeffs + i), _mm256_sub_epi16(va, vb));
  }
}
