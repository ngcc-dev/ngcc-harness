#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

#ifdef PROFILE_FUNCTIONS
#include "hal.h"
extern unsigned long long basemul_cycles;
extern unsigned long long compress_cycles;
extern unsigned long long decompress_cycles;
extern unsigned long long tobytes_cycles;
extern unsigned long long frombytes_cycles;
#endif

/* 32-bit Plant constants for basemul zetas, defined in ntt.c (length 64) */
extern const int32_t zetas[64];

extern void poly_cache_prime_asm(int16_t *dst, const int16_t *src, const int32_t *zetas);

/*************************************************
* Name:        polyvec_cache_prime
*
* Description: For each polynomial b->vec[i], produce b_prime->vec[i] in the
*              Plant cache layout consumed by basemul_asm_{opt_16_32,
*              acc_opt_32_32,acc_opt_32_16}. Each 4-coeff block uses +zeta
*              then -zeta, matching basemul's zeta-pair pattern.
**************************************************/
void polyvec_cache_prime(polyvec *b_prime, const polyvec *b)
{
#ifdef PROFILE_FUNCTIONS
  uint64_t _t1 = hal_get_time();
#endif
  for (unsigned i = 0; i < KYBER_K; i++) {
    poly_cache_prime_asm(b_prime->vec[i].coeffs, b->vec[i].coeffs, zetas);
  }
#ifdef PROFILE_FUNCTIONS
  basemul_cycles += (hal_get_time() - _t1);
#endif
}

/*************************************************
* Name:        polyvec_compress
*
* Description: Compress and serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for KYBER_POLYVECCOMPRESSEDBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_compress(uint8_t r[KYBER_POLYVECCOMPRESSEDBYTES], const polyvec *a)
{
  unsigned int i,j,k;
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif

#if (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 288))
  uint64_t d0;
  uint16_t t[8];
  for(i=0;i<KYBER_K;i++) {
    for(j=0;j<KYBER_N/8;j++) {
      for(k=0;k<8;k++) {
        t[k]  = a->vec[i].coeffs[8*j+k];
        t[k] += ((int16_t)t[k] >> 15) & KYBER_Q;
/*      t[k]  = ((((uint32_t)t[k] << 9) + KYBER_Q/2)/KYBER_Q) & 0x1ff; */
        d0 = t[k];
        d0 <<= 9;
        d0 += 1664;
        d0 *= 161271;
        d0 >>= 29;
        t[k] = d0 & 0x1ff;
      }

      r[0] = (t[0] >>  0);
      r[1] = (t[0] >>  8) | (t[1] << 1);
      r[2] = (t[1] >>  7) | (t[2] << 2);
      r[3] = (t[2] >>  6) | (t[3] << 3);
      r[4] = (t[3] >>  5) | (t[4] << 4);
      r[5] = (t[4] >>  4) | (t[5] << 5);
      r[6] = (t[5] >>  3) | (t[6] << 6);
      r[7] = (t[6] >>  2) | (t[7] << 7);
      r[8] = (t[7] >>  1);
      r += 9;
    }
  }
#elif (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 320))
  uint16_t t[4];
  for(i=0;i<KYBER_K;i++) {
    for(j=0;j<KYBER_N/4;j++) {
      for(k=0;k<4;k++) {
        int16_t u;

        u  = a->vec[i].coeffs[4*j+k];
        u += (u >> 15) & KYBER_Q;
        t[k] = (uint16_t)((((u << 10) + KYBER_Q/2) / KYBER_Q) & 0x3ff);
      }

      r[0] = (uint8_t)(t[0] >> 0);
      r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 2));
      r[2] = (uint8_t)((t[1] >> 6) | (t[2] << 4));
      r[3] = (uint8_t)((t[2] >> 4) | (t[3] << 6));
      r[4] = (uint8_t)(t[3] >> 2);
      r += 5;
    }
  }
#else
#error "KYBER_POLYVECCOMPRESSEDBYTES needs to be in {288*KYBER_K, 320*KYBER_K}"
#endif
#ifdef PROFILE_FUNCTIONS
  compress_cycles += (hal_get_time() - _t0);
#endif
}

/*************************************************
* Name:        polyvec_compress_one
*
* Description: Compress+serialize a single polynomial of the ciphertext vector
*              b into the slot it would occupy in a full polyvec_compress
*              output (index i). Enables streaming b one poly at a time
*              (on-the-fly) without materializing the whole vector on stack.
*              Both widths (du=9 -> 288B, du=10 -> 320B per poly) are byte
*              aligned, so the slot is exactly r + i*(BYTES/KYBER_K).
**************************************************/
void polyvec_compress_one(uint8_t r[KYBER_POLYVECCOMPRESSEDBYTES], const poly *a, unsigned int i)
{
  unsigned int j,k;

#if (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 288))
  uint64_t d0;
  uint16_t t[8];
  r += i*288;
  for(j=0;j<KYBER_N/8;j++) {
    for(k=0;k<8;k++) {
      t[k]  = a->coeffs[8*j+k];
      t[k] += ((int16_t)t[k] >> 15) & KYBER_Q;
      d0 = t[k];
      d0 <<= 9;
      d0 += 1664;
      d0 *= 161271;
      d0 >>= 29;
      t[k] = d0 & 0x1ff;
    }
    r[0] = (t[0] >>  0);
    r[1] = (t[0] >>  8) | (t[1] << 1);
    r[2] = (t[1] >>  7) | (t[2] << 2);
    r[3] = (t[2] >>  6) | (t[3] << 3);
    r[4] = (t[3] >>  5) | (t[4] << 4);
    r[5] = (t[4] >>  4) | (t[5] << 5);
    r[6] = (t[5] >>  3) | (t[6] << 6);
    r[7] = (t[6] >>  2) | (t[7] << 7);
    r[8] = (t[7] >>  1);
    r += 9;
  }
#elif (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 320))
  uint16_t t[4];
  r += i*320;
  for(j=0;j<KYBER_N/4;j++) {
    for(k=0;k<4;k++) {
      int16_t u;
      u  = a->coeffs[4*j+k];
      u += (u >> 15) & KYBER_Q;
      t[k] = (uint16_t)((((u << 10) + KYBER_Q/2) / KYBER_Q) & 0x3ff);
    }
    r[0] = (uint8_t)(t[0] >> 0);
    r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 2));
    r[2] = (uint8_t)((t[1] >> 6) | (t[2] << 4));
    r[3] = (uint8_t)((t[2] >> 4) | (t[3] << 6));
    r[4] = (uint8_t)(t[3] >> 2);
    r += 5;
  }
#else
#error "KYBER_POLYVECCOMPRESSEDBYTES needs to be in {288*KYBER_K, 320*KYBER_K}"
#endif
}

/*************************************************
* Name:        cmp_polyvec_compress_one
*
* Description: Compress a single ciphertext-vector polynomial (index i) and
*              constant-time compare it against the corresponding slot of an
*              already serialized compressed vector r, without materializing
*              the bytes. Counterpart of polyvec_compress_one for FO re-enc.
*
* Returns:     accumulated byte diff (0 == equal).
**************************************************/
uint8_t cmp_polyvec_compress_one(const uint8_t r[KYBER_POLYVECCOMPRESSEDBYTES], const poly *a, unsigned int i)
{
  unsigned int j,k;
  uint8_t rc = 0;

#if (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 288))
  uint64_t d0;
  uint16_t t[8];
  r += i*288;
  for(j=0;j<KYBER_N/8;j++) {
    for(k=0;k<8;k++) {
      t[k]  = a->coeffs[8*j+k];
      t[k] += ((int16_t)t[k] >> 15) & KYBER_Q;
      d0 = t[k];
      d0 <<= 9;
      d0 += 1664;
      d0 *= 161271;
      d0 >>= 29;
      t[k] = d0 & 0x1ff;
    }
    rc |= r[0] ^ (uint8_t)(t[0] >>  0);
    rc |= r[1] ^ (uint8_t)((t[0] >>  8) | (t[1] << 1));
    rc |= r[2] ^ (uint8_t)((t[1] >>  7) | (t[2] << 2));
    rc |= r[3] ^ (uint8_t)((t[2] >>  6) | (t[3] << 3));
    rc |= r[4] ^ (uint8_t)((t[3] >>  5) | (t[4] << 4));
    rc |= r[5] ^ (uint8_t)((t[4] >>  4) | (t[5] << 5));
    rc |= r[6] ^ (uint8_t)((t[5] >>  3) | (t[6] << 6));
    rc |= r[7] ^ (uint8_t)((t[6] >>  2) | (t[7] << 7));
    rc |= r[8] ^ (uint8_t)(t[7] >>  1);
    r += 9;
  }
#elif (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 320))
  uint16_t t[4];
  r += i*320;
  for(j=0;j<KYBER_N/4;j++) {
    for(k=0;k<4;k++) {
      int16_t u;
      u  = a->coeffs[4*j+k];
      u += (u >> 15) & KYBER_Q;
      t[k] = (uint16_t)((((u << 10) + KYBER_Q/2) / KYBER_Q) & 0x3ff);
    }
    rc |= r[0] ^ (uint8_t)(t[0] >> 0);
    rc |= r[1] ^ (uint8_t)((t[0] >> 8) | (t[1] << 2));
    rc |= r[2] ^ (uint8_t)((t[1] >> 6) | (t[2] << 4));
    rc |= r[3] ^ (uint8_t)((t[2] >> 4) | (t[3] << 6));
    rc |= r[4] ^ (uint8_t)(t[3] >> 2);
    r += 5;
  }
#else
#error "KYBER_POLYVECCOMPRESSEDBYTES needs to be in {288*KYBER_K, 320*KYBER_K}"
#endif
  return rc;
}

/*************************************************
* Name:        polyvec_decompress
*
* Description: De-serialize and decompress vector of polynomials;
*              approximate inverse of polyvec_compress
*
* Arguments:   - polyvec *r:       pointer to output vector of polynomials
*              - const uint8_t *a: pointer to input byte array
*                                  (of length KYBER_POLYVECCOMPRESSEDBYTES)
**************************************************/
void polyvec_decompress(polyvec *r, const uint8_t a[KYBER_POLYVECCOMPRESSEDBYTES])
{
  unsigned int i,j,k;
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif

#if (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 288))
  uint16_t t[8];
  for(i=0;i<KYBER_K;i++) {
    for(j=0;j<KYBER_N/8;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[ 1] << 8);
      t[1] = (a[1] >> 1) | ((uint16_t)a[ 2] << 7);
      t[2] = (a[2] >> 2) | ((uint16_t)a[ 3] << 6);
      t[3] = (a[3] >> 3) | ((uint16_t)a[ 4] << 5);
      t[4] = (a[4] >> 4) | ((uint16_t)a[ 5] << 4);
      t[5] = (a[5] >> 5) | ((uint16_t)a[ 6] << 3);
      t[6] = (a[6] >> 6) | ((uint16_t)a[ 7] << 2);
      t[7] = (a[7] >> 7) | ((uint16_t)a[ 8] << 1);
      a += 9;

      for(k=0;k<8;k++)
        r->vec[i].coeffs[8*j+k] = ((uint32_t)(t[k] & 0x1ff)*KYBER_Q + 256) >> 9;
    }
  }
#elif (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 320))
  uint16_t t[4];
  for(i=0;i<KYBER_K;i++) {
    for(j=0;j<KYBER_N/4;j++) {
      t[0] = ((uint16_t)a[0] >> 0) | (((uint16_t)a[1] & 0x03u) << 8);
      t[1] = (((uint16_t)a[1] >> 2) | ((uint16_t)a[2] << 6)) & 0x3ff;
      t[2] = (((uint16_t)a[2] >> 4) | ((uint16_t)a[3] << 4)) & 0x3ff;
      t[3] = (((uint16_t)a[3] >> 6) | ((uint16_t)a[4] << 2)) & 0x3ff;
      a += 5;

      for(k=0;k<4;k++) {
        r->vec[i].coeffs[4*j+k] = ((uint32_t)(t[k] & 0x3ff) * KYBER_Q + 512) >> 10;
      }
    }
  }
#else
#error "KYBER_POLYVECCOMPRESSEDBYTES needs to be in {288*KYBER_K, 320*KYBER_K}"
#endif
#ifdef PROFILE_FUNCTIONS
  decompress_cycles += (hal_get_time() - _t0);
#endif
}

/*************************************************
* Name:        polyvec_decompress_one
*
* Description: De-serialize+decompress a single polynomial (index i) of the
*              ciphertext vector b. Counterpart of polyvec_compress_one, used
*              by indcpa_dec to consume b one poly at a time on the fly.
**************************************************/
void polyvec_decompress_one(poly *r, const uint8_t a[KYBER_POLYVECCOMPRESSEDBYTES], unsigned int i)
{
  unsigned int j,k;

#if (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 288))
  uint16_t t[8];
  a += i*288;
  for(j=0;j<KYBER_N/8;j++) {
    t[0] = (a[0] >> 0) | ((uint16_t)a[ 1] << 8);
    t[1] = (a[1] >> 1) | ((uint16_t)a[ 2] << 7);
    t[2] = (a[2] >> 2) | ((uint16_t)a[ 3] << 6);
    t[3] = (a[3] >> 3) | ((uint16_t)a[ 4] << 5);
    t[4] = (a[4] >> 4) | ((uint16_t)a[ 5] << 4);
    t[5] = (a[5] >> 5) | ((uint16_t)a[ 6] << 3);
    t[6] = (a[6] >> 6) | ((uint16_t)a[ 7] << 2);
    t[7] = (a[7] >> 7) | ((uint16_t)a[ 8] << 1);
    a += 9;
    for(k=0;k<8;k++)
      r->coeffs[8*j+k] = ((uint32_t)(t[k] & 0x1ff)*KYBER_Q + 256) >> 9;
  }
#elif (KYBER_POLYVECCOMPRESSEDBYTES == (KYBER_K * 320))
  uint16_t t[4];
  a += i*320;
  for(j=0;j<KYBER_N/4;j++) {
    t[0] = ((uint16_t)a[0] >> 0) | (((uint16_t)a[1] & 0x03u) << 8);
    t[1] = (((uint16_t)a[1] >> 2) | ((uint16_t)a[2] << 6)) & 0x3ff;
    t[2] = (((uint16_t)a[2] >> 4) | ((uint16_t)a[3] << 4)) & 0x3ff;
    t[3] = (((uint16_t)a[3] >> 6) | ((uint16_t)a[4] << 2)) & 0x3ff;
    a += 5;
    for(k=0;k<4;k++)
      r->coeffs[4*j+k] = ((uint32_t)(t[k] & 0x3ff) * KYBER_Q + 512) >> 10;
  }
#else
#error "KYBER_POLYVECCOMPRESSEDBYTES needs to be in {288*KYBER_K, 320*KYBER_K}"
#endif
}

/*************************************************
* Name:        polyvec_tobytes
*
* Description: Serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for KYBER_POLYVECBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_tobytes(uint8_t r[KYBER_POLYVECBYTES], const polyvec *a)
{
  unsigned int i;
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  for(i=0;i<KYBER_K;i++)
    poly_tobytes(r+i*KYBER_POLYBYTES, &a->vec[i]);
#ifdef PROFILE_FUNCTIONS
  tobytes_cycles += (hal_get_time() - _t0);
#endif
}

/*************************************************
* Name:        polyvec_frombytes
*
* Description: De-serialize vector of polynomials;
*              inverse of polyvec_tobytes
*
* Arguments:   - uint8_t *r:       pointer to output byte array
*              - const polyvec *a: pointer to input vector of polynomials
*                                  (of length KYBER_POLYVECBYTES)
**************************************************/
void polyvec_frombytes(polyvec *r, const uint8_t a[KYBER_POLYVECBYTES])
{
  unsigned int i;
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  for(i=0;i<KYBER_K;i++)
    poly_frombytes(&r->vec[i], a+i*KYBER_POLYBYTES);
#ifdef PROFILE_FUNCTIONS
  frombytes_cycles += (hal_get_time() - _t0);
#endif
}

/*************************************************
* Name:        polyvec_ntt
*
* Description: Apply forward NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_ntt(polyvec *r)
{
  unsigned int i;
  for(i=0;i<KYBER_K;i++)
    poly_ntt(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_invntt
*
* Description: Apply inverse NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_invntt(polyvec *r)
{
  unsigned int i;
  for(i=0;i<KYBER_K;i++)
    poly_invntt(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_basemul_acc_cached
*
* Description: NTT-domain inner product of two polyvecs using a precomputed
*              a_prime (see polyvec_cache_prime). Three-stage pipeline:
*                - poly_basemul_opt_16_32:    first term, produces 32-bit acc
*                - poly_basemul_acc_opt_32_32: middle terms, 32-bit accumulate
*                - poly_basemul_acc_opt_32_16: final term + plant_red to 16-bit
*              For KYBER_K == 2 the middle loop is empty (just stage 1 + 3).
*              The 16-bit output already lives in a tight Plant range, so the
*              caller does NOT need a follow-up poly_reduce before invntt.
**************************************************/
void polyvec_basemul_acc_cached(poly *r, const polyvec *a, const polyvec *b,
                                const polyvec *a_prime)
{
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  int32_t r_tmp[KYBER_N];
  poly_basemul_opt_16_32(r_tmp, &a->vec[0], &b->vec[0], &a_prime->vec[0]);
  for (unsigned i = 1; i < KYBER_K - 1; i++)
    poly_basemul_acc_opt_32_32(r_tmp, &a->vec[i], &b->vec[i], &a_prime->vec[i]);
  poly_basemul_acc_opt_32_16(r, &a->vec[KYBER_K-1], &b->vec[KYBER_K-1],
                                 &a_prime->vec[KYBER_K-1], r_tmp);
#ifdef PROFILE_FUNCTIONS
  basemul_cycles += (hal_get_time() - _t0);
#endif
}

/*************************************************
* Name:        polyvec_reduce
*
* Description: Applies Barrett reduction to each coefficient
*              of each element of a vector of polynomials;
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - polyvec *r: pointer to input/output polynomial
**************************************************/
void polyvec_reduce(polyvec *r)
{
  unsigned int i;
  for(i=0;i<KYBER_K;i++)
    poly_reduce(&r->vec[i]);
}


/*************************************************
* Name:        polyvec_add
*
* Description: Add vectors of polynomials
*
* Arguments: - polyvec *r: pointer to output vector of polynomials
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b)
{
  unsigned int i;
  for(i=0;i<KYBER_K;i++)
    poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}
