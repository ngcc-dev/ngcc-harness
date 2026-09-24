#include "polyvec.h"
#include "params.h"
#include "poly.h"
#include <stdint.h>

#ifdef PROFILE_FUNCTIONS
#include "hal.h"
extern unsigned long long basemul_cycles;
extern unsigned long long compress_cycles;
extern unsigned long long decompress_cycles;
extern unsigned long long tobytes_cycles;
extern unsigned long long frombytes_cycles;
#endif

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
  uint16_t t[4];
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
#if (KYBER_POLYVECCOMPRESSEDBYTES != (KYBER_K * 640))
#error "BW_KEM_C512 requires KYBER_POLYVECCOMPRESSEDBYTES = 640*KYBER_K"
#endif
  for(i=0;i<KYBER_K;i++) {
    for(j=0;j<KYBER_N/4;j++) {
      for(k=0;k<4;k++) {
        t[k]  = a->vec[i].coeffs[4*j+k];
        t[k] += ((int16_t)t[k] >> 15) & KYBER_Q;
        t[k]  = ((((uint32_t)t[k] << 10) + KYBER_Q/2)/KYBER_Q) & 0x3ff;
      }

      r[0] = (t[0] >> 0);
      r[1] = (t[0] >> 8) | (t[1] << 2);
      r[2] = (t[1] >> 6) | (t[2] << 4);
      r[3] = (t[2] >> 4) | (t[3] << 6);
      r[4] = (t[3] >> 2);
      r += 5;
    }
  }
#ifdef PROFILE_FUNCTIONS
  compress_cycles += (hal_get_time() - _t0);
#endif
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
  uint16_t t[4];
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
#if (KYBER_POLYVECCOMPRESSEDBYTES != (KYBER_K * 640))
#error "BW_KEM_C512 requires KYBER_POLYVECCOMPRESSEDBYTES = 640*KYBER_K"
#endif
  for(i=0;i<KYBER_K;i++) {
    for(j=0;j<KYBER_N/4;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
      t[1] = (a[1] >> 2) | ((uint16_t)a[2] << 6);
      t[2] = (a[2] >> 4) | ((uint16_t)a[3] << 4);
      t[3] = (a[3] >> 6) | ((uint16_t)a[4] << 2);
      a += 5;

      for(k=0;k<4;k++)
        r->vec[i].coeffs[4*j+k] = ((uint32_t)(t[k] & 0x3ff)*KYBER_Q + 512) >> 10;
    }
  }
#ifdef PROFILE_FUNCTIONS
  decompress_cycles += (hal_get_time() - _t0);
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
 * Name:        polyvec_invntt_tomont
 *
 * Description: Apply inverse NTT to all elements of a vector of polynomials
 *              and multiply by Montgomery factor 2^16
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
 * Name:        polyvec_basemul_acc_montgomery
 *
 * Description: Multiply elements of a and b in NTT domain, accumulate into r,
 *              and multiply by 2^-16.
 *
 * Arguments: - poly *r: pointer to output polynomial
 *            - const polyvec *a: pointer to first input vector of polynomials
 *            - const polyvec *b: pointer to second input vector of polynomials
 **************************************************/
void polyvec_basemul_acc(poly *r, const polyvec *a, const polyvec *b)
{
  unsigned int i;
  poly t;
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif

  poly_basemul(r, &a->vec[0], &b->vec[0]);
  for(i=1;i<KYBER_K;i++) {
    poly_basemul(&t, &a->vec[i], &b->vec[i]);
    poly_add(r, r, &t);
  }

  poly_reduce(r);
#ifdef PROFILE_FUNCTIONS
  basemul_cycles += (hal_get_time() - _t0);
#endif
}

/* Cache the za1,za2,za3 partials for every poly of a polyvec. */
void polyvec_cache_prime(polyvec *b_prime, const polyvec *b)
{
#ifdef PROFILE_FUNCTIONS
  uint64_t _t0 = hal_get_time();
#endif
  for (unsigned i = 0; i < KYBER_K; i++)
    poly_cache_prime(&b_prime->vec[i], &b->vec[i]);
#ifdef PROFILE_FUNCTIONS
  basemul_cycles += (hal_get_time() - _t0);
#endif
}

/* Cached inner product: opt_16_32 (first) + acc_32_32 (middle) + acc_32_16
 * (final + plant_red). Accumulates in a 32-bit r_tmp buffer to defer reduction
 * across the K polys.  Output is already in tight Plant range, no extra
 * poly_reduce needed by the caller before invntt. */
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