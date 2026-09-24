#include <immintrin.h>
#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "cbd.h"
#include "symmetric.h"
#include "BWcoding.h"
#include "consts.h"

static inline __m256i pack_i32_to_i16_ordered(__m256i lo, __m256i hi)
{
  return _mm256_permute4x64_epi64(_mm256_packs_epi32(lo, hi), 0xD8);
}

static void normalize_u16_block(uint16_t out[16], const int16_t *in)
{
  const __m256i q = _mm256_set1_epi16(KYBER_Q);
  __m256i v = _mm256_loadu_si256((const __m256i *)in);
  __m256i sign = _mm256_srai_epi16(v, 15);
  __m256i norm = _mm256_add_epi16(v, _mm256_and_si256(sign, q));

  _mm256_storeu_si256((__m256i *)out, norm);
}

/*
 * Pack 8 normalized 12-bit coefficients (held one-per-16-bit-lane in a 128-bit
 * register) into 12 contiguous bytes, matching the scalar layout:
 *   r0 = t0 & 0xFF; r1 = (t0>>8)|(t1<<4); r2 = t1>>4; ...
 * Builds a 24-bit little-endian value per coefficient pair, then gathers the
 * low 3 bytes of each 32-bit lane via a byte shuffle.
 */
static inline __m128i pack12_8coeffs(__m128i c)
{
  /* even lanes = t0,t2,t4,t6 ; odd lanes = t1,t3,t5,t7 (16-bit each) */
  const __m128i even = _mm_and_si128(c, _mm_set1_epi32(0x0000FFFF));
  const __m128i odd  = _mm_srli_epi32(c, 16);
  /* combined[k] = t_even | (t_odd << 12), a 24-bit value in a 32-bit lane */
  __m128i combined = _mm_or_si128(even, _mm_slli_epi32(odd, 12));
  /* gather low 3 bytes of lanes 0,1,2,3 -> 12 bytes, rest zero */
  const __m128i sh = _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14,
                                   -1, -1, -1, -1);
  return _mm_shuffle_epi8(combined, sh);
}

/*
 * Unpack 12 bytes into 8 coefficients (one-per-16-bit-lane), matching scalar:
 *   c0 = (a0 | a1<<8) & 0xFFF ; c1 = (a1>>4 | a2<<4) & 0xFFF ; ...
 */
static inline __m128i unpack12_8coeffs(__m128i bytes)
{
  /* spread the 12 input bytes so each 32-bit lane holds one 3-byte group */
  const __m128i sh = _mm_setr_epi8(0, 1, 2, -1, 3, 4, 5, -1,
                                   6, 7, 8, -1, 9, 10, 11, -1);
  __m128i g = _mm_shuffle_epi8(bytes, sh); /* lane k = a[3k] | a[3k+1]<<8 | a[3k+2]<<16 */
  __m128i lo = _mm_and_si128(g, _mm_set1_epi32(0x00000FFF));
  __m128i hi = _mm_and_si128(_mm_srli_epi32(g, 12), _mm_set1_epi32(0x00000FFF));
  /* interleave: even 16-bit lanes = lo, odd = hi */
  return _mm_or_si128(lo, _mm_slli_epi32(hi, 16));
}

static void quantize_u6_block(uint8_t out[16], const int16_t *in)
{
  uint16_t norm[16];
  uint32_t tmp[16];
  unsigned int i;
  __m256i lo;
  __m256i hi;
  const __m256i bias = _mm256_set1_epi32(KYBER_Q / 2);

  normalize_u16_block(norm, in);
  lo = _mm256_cvtepu16_epi32(_mm_loadu_si128((const __m128i *)norm));
  hi = _mm256_cvtepu16_epi32(_mm_loadu_si128((const __m128i *)(norm + 8)));
  lo = _mm256_add_epi32(_mm256_slli_epi32(lo, 6), bias);
  hi = _mm256_add_epi32(_mm256_slli_epi32(hi, 6), bias);
  _mm256_storeu_si256((__m256i *)tmp, lo);
  _mm256_storeu_si256((__m256i *)(tmp + 8), hi);

  for(i = 0; i < 16; i++)
    out[i] = (uint8_t)((tmp[i] / KYBER_Q) & 0x3fU);
}

static void dequantize_u6_block(int16_t out[16], const uint8_t in[16])
{
  uint16_t t[16];
  unsigned int i;
  __m256i lo;
  __m256i hi;
  const __m256i q = _mm256_set1_epi32(KYBER_Q);
  const __m256i bias = _mm256_set1_epi32(32);

  for(i = 0; i < 16; i++)
    t[i] = (uint16_t)(in[i] & 0x3fU);

  lo = _mm256_cvtepu16_epi32(_mm_loadu_si128((const __m128i *)t));
  hi = _mm256_cvtepu16_epi32(_mm_loadu_si128((const __m128i *)(t + 8)));
  lo = _mm256_srli_epi32(_mm256_add_epi32(_mm256_mullo_epi32(lo, q), bias), 6);
  hi = _mm256_srli_epi32(_mm256_add_epi32(_mm256_mullo_epi32(hi, q), bias), 6);
  _mm256_storeu_si256((__m256i *)out, pack_i32_to_i16_ordered(lo, hi));
}

static inline __m256i montgomery_reduce_avx2_i32(__m256i a)
{
  const __m256i q = _mm256_set1_epi32(KYBER_Q);
  const __m256i qinv = _mm256_set1_epi32(QINV);
  __m256i t = _mm256_srai_epi32(_mm256_slli_epi32(a, 16), 16);

  t = _mm256_mullo_epi32(t, qinv);
  t = _mm256_srai_epi32(_mm256_slli_epi32(t, 16), 16);
  t = _mm256_sub_epi32(a, _mm256_mullo_epi32(t, q));
  return _mm256_srai_epi32(t, 16);
}

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
  unsigned int i,k;
  uint8_t t[16];

#if (KYBER_POLYCOMPRESSEDBYTES == 128)

  uint32_t d0;
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

  uint32_t d0;
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
#elif (KYBER_POLYCOMPRESSEDBYTES == 384)

  for(i=0;i<KYBER_N/16;i++) {
    quantize_u6_block(t, &a->coeffs[16*i]);

    for(k=0;k<16;k += 4) {
      r[0] = (uint8_t)((t[k + 0] >> 0) | (t[k + 1] << 6));
      r[1] = (uint8_t)((t[k + 1] >> 2) | (t[k + 2] << 4));
      r[2] = (uint8_t)((t[k + 2] >> 4) | (t[k + 3] << 2));
      r += 3;
    }
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
#elif (KYBER_POLYCOMPRESSEDBYTES == 384)
  uint8_t t[16];
  for(i=0;i<KYBER_N/16;i++) {
    t[0] = (a[0] >> 0);
    t[1] = (a[0] >> 6) | (a[1] << 2);
    t[2] = (a[1] >> 4) | (a[2] << 4);
    t[3] = (a[2] >> 2);
    t[4] = (a[3] >> 0);
    t[5] = (a[3] >> 6) | (a[4] << 2);
    t[6] = (a[4] >> 4) | (a[5] << 4);
    t[7] = (a[5] >> 2);
    t[8] = (a[6] >> 0);
    t[9] = (a[6] >> 6) | (a[7] << 2);
    t[10] = (a[7] >> 4) | (a[8] << 4);
    t[11] = (a[8] >> 2);
    t[12] = (a[9] >> 0);
    t[13] = (a[9] >> 6) | (a[10] << 2);
    t[14] = (a[10] >> 4) | (a[11] << 4);
    t[15] = (a[11] >> 2);
    a += 12;

    dequantize_u6_block(r->coeffs + 16*i, t);
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
  const __m256i q = _mm256_set1_epi16(KYBER_Q);
  unsigned int i;

  /* Process 16 coefficients -> 24 bytes per iteration. KYBER_N/16 iterations.
   * The last iteration would store past the 24th byte, so use a staging
   * buffer and copy exactly 24 bytes. */
  for(i=0;i<KYBER_N/16;i++) {
    __m256i v = _mm256_loadu_si256((const __m256i *)&a->coeffs[16*i]);
    __m256i sign = _mm256_srai_epi16(v, 15);
    __m256i norm = _mm256_add_epi16(v, _mm256_and_si256(sign, q));
    __m128i lo = pack12_8coeffs(_mm256_castsi256_si128(norm));    /* 12 bytes */
    __m128i hi = pack12_8coeffs(_mm256_extracti128_si256(norm, 1)); /* 12 bytes */

    if(i + 1 < KYBER_N/16) {
      /* lo's valid 12 bytes go to [0,11]; hi's storeu at +12 overwrites lo's
       * zero padding at [12,15] and writes hi's 12 valid bytes to [12,23].
       * Writes up to 24*i+28 <= KYBER_POLYBYTES for all but the last block. */
      _mm_storeu_si128((__m128i *)&r[24*i], lo);
      _mm_storeu_si128((__m128i *)&r[24*i+12], hi);
    } else {
      uint8_t tail[32];
      _mm_storeu_si128((__m128i *)tail, lo);
      _mm_storeu_si128((__m128i *)(tail + 12), hi);
      __builtin_memcpy(&r[24*i], tail, 24);
    }
  }
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
  unsigned int i;

  /* Process 24 input bytes -> 16 coefficients per iteration. The unpack helper
   * loads 16 bytes, so all but the last group read 4 bytes ahead (in bounds);
   * the last group is staged into a zero-padded buffer. */
  for(i=0;i<KYBER_N/16;i++) {
    __m128i b_lo, b_hi;

    if(i + 1 < KYBER_N/16) {
      b_lo = _mm_loadu_si128((const __m128i *)&a[24*i]);
      b_hi = _mm_loadu_si128((const __m128i *)&a[24*i+12]);
    } else {
      uint8_t tail[32];
      __builtin_memcpy(tail, &a[24*i], 24);
      b_lo = _mm_loadu_si128((const __m128i *)tail);
      b_hi = _mm_loadu_si128((const __m128i *)(tail + 12));
    }

    {
      __m128i c_lo = unpack12_8coeffs(b_lo); /* 8 coeffs */
      __m128i c_hi = unpack12_8coeffs(b_hi); /* 8 coeffs */
      _mm_storeu_si128((__m128i *)&r->coeffs[16*i], c_lo);
      _mm_storeu_si128((__m128i *)&r->coeffs[16*i+8], c_hi);
    }
  }
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
  unsigned int i;
  const int16_t f = (1ULL << 32) % KYBER_Q;
  const __m256i vf = _mm256_set1_epi32(f);

  for(i=0;i<KYBER_N;i += 16) {
    __m256i v = _mm256_loadu_si256((const __m256i *)(r->coeffs + i));
    __m256i lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(v));
    __m256i hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(v, 1));

    lo = montgomery_reduce_avx2_i32(_mm256_mullo_epi32(lo, vf));
    hi = montgomery_reduce_avx2_i32(_mm256_mullo_epi32(hi, vf));
    _mm256_storeu_si256((__m256i *)(r->coeffs + i), pack_i32_to_i16_ordered(lo, hi));
  }
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
  unsigned int i;
  /* Mirror of the assembly `barret` macro in ntt_avx.S: 5 ops on 16x16-bit
   * lanes per group, equivalent to scalar barrett_reduce.
   *   t = mulhi(a, V); t = (t + 512) >> 10; t *= q; a -= t   (V = floor(2^26/q+0.5)) */
  const __m256i v = _mm256_set1_epi16(((1<<26) + KYBER_Q/2)/KYBER_Q);
  const __m256i q = _mm256_set1_epi16(KYBER_Q);
  const __m256i round = _mm256_set1_epi16(512);

  for(i=0;i<KYBER_N;i += 16) {
    __m256i a = _mm256_loadu_si256((const __m256i *)(r->coeffs + i));
    __m256i t = _mm256_mulhi_epi16(a, v);
    t = _mm256_srai_epi16(_mm256_add_epi16(t, round), 10);
    t = _mm256_mullo_epi16(t, q);
    _mm256_storeu_si256((__m256i *)(r->coeffs + i), _mm256_sub_epi16(a, t));
  }
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
