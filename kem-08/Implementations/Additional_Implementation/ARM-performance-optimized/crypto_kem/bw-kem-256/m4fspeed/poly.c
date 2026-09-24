#include <stddef.h>
#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"

#include "cbd.h"
#include "BWcoding.h"
#include "symmetric.h"
#include "verify.h"

#ifdef PROFILE_FUNCTIONS
#include "hal.h"
extern unsigned long long ntt_cycles;
extern unsigned long long invntt_cycles;
extern unsigned long long reduce_cycles;
extern unsigned long long bw32enc_cycles;
extern unsigned long long bw32dec_cycles;
extern unsigned long long compress_cycles;
extern unsigned long long decompress_cycles;
extern unsigned long long frommsg_cycles;
extern unsigned long long tomsg_cycles;
extern unsigned long long getnoise_eta1_cycles;
extern unsigned long long getnoise_eta2_cycles;
#endif

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
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif

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
#ifdef PROFILE_FUNCTIONS
  compress_cycles += (hal_get_time() - _t0);
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
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif

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
#ifdef PROFILE_FUNCTIONS
  decompress_cycles += (hal_get_time() - _t0);
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
  unsigned int i;
  uint16_t t0, t1;

  poly_reduce((poly *)a);

  for(i=0;i<KYBER_N/2;i++) {
    t0  = a->coeffs[2*i]     - KYBER_Q;
    t0 += ((int16_t)t0 >> 15) & KYBER_Q;
    t1  = a->coeffs[2*i+1]   - KYBER_Q;
    t1 += ((int16_t)t1 >> 15) & KYBER_Q;
    r[3*i+0] = (t0 >> 0);
    r[3*i+1] = (t0 >> 8) | (t1 << 4);
    r[3*i+2] = (t1 >> 4);
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
  for(i=0;i<KYBER_N/2;i++) {
    r->coeffs[2*i]   = ((a[3*i+0] >> 0) | ((uint16_t)a[3*i+1] << 8)) & 0xFFF;
    r->coeffs[2*i+1] = ((a[3*i+1] >> 4) | ((uint16_t)a[3*i+2] << 4)) & 0xFFF;
  }
}

extern void frombytes_mul_asm_16_32(int32_t *r_tmp, const int16_t *b,
                                    const uint8_t *c, const int32_t zetas[64]);
void poly_frombytes_mul_16_32(int32_t *r_tmp, const poly *b,
                              const uint8_t a[KYBER_POLYBYTES])
{
  frombytes_mul_asm_16_32(r_tmp, b->coeffs, a, zetas);
}

extern void frombytes_mul_asm_acc_32_32(int32_t *r_tmp, const int16_t *b,
                                        const uint8_t *c, const int32_t zetas[64]);
void poly_frombytes_mul_32_32(int32_t *r_tmp, const poly *b,
                              const uint8_t a[KYBER_POLYBYTES])
{
  frombytes_mul_asm_acc_32_32(r_tmp, b->coeffs, a, zetas);
}

extern void frombytes_mul_asm_acc_32_16(int16_t *r, const int16_t *b,
                                        const uint8_t *c, const int32_t zetas[64],
                                        const int32_t *r_tmp);
void poly_frombytes_mul_32_16(poly *r, const poly *b,
                              const uint8_t a[KYBER_POLYBYTES],
                              const int32_t *r_tmp)
{
  frombytes_mul_asm_acc_32_16(r->coeffs, b->coeffs, a, zetas, r_tmp);
}

void poly_frommsg(poly *r, const uint8_t msg[KYBER_INDCPA_MSGBYTES])
{
  int i, j;
  uint32_t m;
  uint64_t mh;
  int16_t mask1, mask2;
#ifdef PROFILE_FUNCTIONS
  uint64_t _tfn = hal_get_time();
#endif

#if (KYBER_INDCPA_MSGBYTES != KYBER_N/8)
#error "KYBER_INDCPA_MSGBYTES must be equal to KYBER_N/8 bytes!"
#endif

  for(i = 0; i < KYBER_N/32; i++) {
    m = (msg[4*i]) | (msg[4*i + 1] << 8) | (msg[4*i + 2] << 16) | (msg[4*i + 3] << 24);
#ifdef PROFILE_FUNCTIONS
    { uint64_t _t0 = hal_get_time();
#endif
    mh = encode_bw32(m);
#ifdef PROFILE_FUNCTIONS
      bw32enc_cycles += (hal_get_time() - _t0); }
#endif
    for(j = 0; j < 32; j++) {
      mask1 = -((mh >> (2*j)) & 1);
      mask2 = -((mh >> (2*j + 1)) & 1);
      r->coeffs[32*i + j] = (mask1 & 832) + (mask2 & 1664);
    }
  }
#ifdef PROFILE_FUNCTIONS
  frommsg_cycles += (hal_get_time() - _tfn);
#endif
}

void poly_tomsg(uint8_t msg[KYBER_INDCPA_MSGBYTES], const poly *a)
{
  unsigned int i, j;
  uint32_t t;
  int16_t vec[32];
  int16_t u;
  uint64_t d0;
#ifdef PROFILE_FUNCTIONS
  uint64_t _tfn = hal_get_time();
#endif

  for(i = 0; i < KYBER_N/32; i++) {
    for(j = 0; j < 32; j++) {
      u = a->coeffs[32*i + j];
      u += (u >> 15) & KYBER_Q;
      d0 = u << 12;
      d0 += 1664;
      d0 *= 2580335;
      d0 >>= 33;
      vec[31 - j] = d0 & 0xfff;
    }
#ifdef PROFILE_FUNCTIONS
    { uint64_t _t0 = hal_get_time();
#endif
    t = decode_bw32(vec);
#ifdef PROFILE_FUNCTIONS
      bw32dec_cycles += (hal_get_time() - _t0); }
#endif
    msg[4*i]     = (t >>  0) & 0xff;
    msg[4*i + 1] = (t >>  8) & 0xff;
    msg[4*i + 2] = (t >> 16) & 0xff;
    msg[4*i + 3] = (t >> 24) & 0xff;
  }
#ifdef PROFILE_FUNCTIONS
  tomsg_cycles += (hal_get_time() - _tfn);
#endif
}

void poly_getnoise_eta1(poly *r, const uint8_t seed[KYBER_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[KYBER_ETA1 * KYBER_N / 4];
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta1(r, buf);
#ifdef PROFILE_FUNCTIONS
  getnoise_eta1_cycles += (hal_get_time() - _t0);
#endif
}

void poly_getnoise_eta2(poly *r, const uint8_t seed[KYBER_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[KYBER_ETA2 * KYBER_N / 4];
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta2(r, buf);
#ifdef PROFILE_FUNCTIONS
  getnoise_eta2_cycles += (hal_get_time() - _t0);
#endif
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
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  ntt(r->coeffs);
#ifdef PROFILE_FUNCTIONS
  ntt_cycles += (hal_get_time() - _t0);
#endif
}

/*************************************************
* Name:        poly_invntt
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *a: pointer to in/output polynomial
**************************************************/
void poly_invntt(poly *r)
{
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  invntt(r->coeffs);
#ifdef PROFILE_FUNCTIONS
  invntt_cycles += (hal_get_time() - _t0);
#endif
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
// void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
// {
//   unsigned int i;
//   for(i=0;i<KYBER_N/4;i++) {
//     basemul(&r->coeffs[4*i], &a->coeffs[4*i], &b->coeffs[4*i], zetas[64+i]);
//     basemul(&r->coeffs[4*i+2], &a->coeffs[4*i+2], &b->coeffs[4*i+2], -zetas[64+i]);
//   }
// }

/*************************************************
* Name:        poly_basemul_opt_16_32
*
* Description: Multiplication of two polynomials using asymmetric multiplication.
*              Cached values are generated during matrix-vector product.
*              Using strategy of better accumulation (initial step).
* Arguments:   - const poly *a:       pointer to input polynomial
*              - const poly *b:       pointer to input polynomial
*              - const poly *a_prime: pointer to a pre-multiplied by zetas 
*              - int32_t *r_tmp:      array for accumulating unreduced results
**************************************************/
extern void basemul_asm_opt_16_32(int32_t *, const int16_t *, const int16_t *, const int16_t *);
void poly_basemul_opt_16_32(int32_t *r_tmp, const poly *a, const poly *b, const poly *a_prime) {
    basemul_asm_opt_16_32(r_tmp, a->coeffs, b->coeffs, a_prime->coeffs);
}

/*************************************************
* Name:        poly_basemul_acc_opt_32_32
*
* Description: Multiplication of two polynomials using asymmetric multiplication.
*              Cached values are generated during matrix-vector product.
*              Using strategy of better accumulation.
* Arguments:   - const poly *a:       pointer to input polynomial
*              - const poly *b:       pointer to input polynomial
*              - const poly *a_prime: pointer to a pre-multiplied by zetas 
*              - int32_t *r_tmp:      array for accumulating unreduced results
**************************************************/
extern void basemul_asm_acc_opt_32_32(int32_t *, const int16_t *, const int16_t *, const int16_t *);
void poly_basemul_acc_opt_32_32(int32_t *r, const poly *a, const poly *b, const poly *a_prime) {
    basemul_asm_acc_opt_32_32(r, a->coeffs, b->coeffs, a_prime->coeffs);
}

/*************************************************
* Name:        poly_basemul_acc_opt_32_16
*
* Description: Multiplication of two polynomials using asymmetric multiplication.
*              Cached values are generated during matrix-vector product.
*              Using strategy of better accumulation (final step).
* Arguments:   - const poly *a:        pointer to input polynomial
*              - const poly *b:        pointer to input polynomial
*              - const poly *a_prime:  pointer to a pre-multiplied by zetas 
*              - poly *r:              pointer to output polynomial
*              - const int32_t *r_tmp: array containing unreduced results
**************************************************/
extern void basemul_asm_acc_opt_32_16(int16_t *, const int16_t *, const int16_t *, const int16_t *, const int32_t *);
void poly_basemul_acc_opt_32_16(poly *r, const poly *a, const poly *b, const poly *a_prime, const int32_t * r_tmp) {
    basemul_asm_acc_opt_32_16(r->coeffs, a->coeffs, b->coeffs, a_prime->coeffs, r_tmp);
}

/*************************************************
* Name:        poly_reduce
*
* Description: Applies Barrett reduction to all coefficients of a polynomial
*              for details of the Barrett reduction see comments in reduce.S
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
// void poly_reduce(poly *r)
// {
//   unsigned int i;
//   for(i=0;i<KYBER_N;i++)
//     r->coeffs[i] = barrett_reduce(r->coeffs[i]);
// }

extern void asm_fromplant(int16_t *r);
/*************************************************
* Name:        poly_fromplantt
*
* Description: Inplace conversion of all coefficients of a polynomial
*              from Montgomery domain to normal domain
*
* Arguments:   - poly *r:       pointer to input/output polynomial
**************************************************/
void poly_fromplant(poly *r) {
  asm_fromplant(r->coeffs);
}

extern void asm_barrett_reduce(int16_t *r);
/*************************************************
* Name:        poly_reduce
*
* Description: Applies Barrett reduction to all coefficients of a polynomial
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - poly *r:       pointer to input/output polynomial
**************************************************/
void poly_reduce(poly *r) {
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  asm_barrett_reduce(r->coeffs);
#ifdef PROFILE_FUNCTIONS
  reduce_cycles += (hal_get_time() - _t0);
#endif
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
// void poly_add(poly *r, const poly *a, const poly *b)
// {
//   unsigned int i;
//   for(i=0;i<KYBER_N;i++)
//     r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
// }

extern void pointwise_add(int16_t *, const int16_t *, const int16_t *);
/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b) {
    pointwise_add(r->coeffs,a->coeffs,b->coeffs);
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
// void poly_sub(poly *r, const poly *a, const poly *b)
// {
//   unsigned int i;
//   for(i=0;i<KYBER_N;i++)
//     r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
// }

extern void pointwise_sub(int16_t *, const int16_t *, const int16_t *);
/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b) {
    pointwise_sub(r->coeffs,a->coeffs,b->coeffs);
}
