#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

/*************************************************
* Name:        polyvec_compress
*
* Description: Compress and serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for COMPASS_KEM_POLYVECCOMPRESSEDBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
// void polyvec_compress(uint8_t *r, const polyvec *a, int8_t d) {
//   unsigned int i;
//   unsigned int bytes_per_poly = 0;
  
//   // Dynamically compute bytes per compressed polynomial
// #if (COMPASS_KEM_Q == 3329)
//   // q=3329, n=256
//   if (d == 2) {
//     bytes_per_poly = 320; // 256 * 10 bits / 8
//   } else if (d == 6) {
//     bytes_per_poly = 192; // 256 * 6 bits / 8
//   }
// #elif (COMPASS_KEM_Q == 7681)
//   // q=7681, n=512
//   if (d == 2) {
//     bytes_per_poly = 704; // 512 * 11 bits / 8
//   } else if (d == 6) {
//     bytes_per_poly = 448; // 512 * 7 bits / 8
//   }
// #else
//   #error "Unsupported COMPASS_KEM_Q in polyvec_compress"
// #endif

//   // Call per-polynomial compression in a loop
//   for(i = 0; i < COMPASS_KEM_K; i++) {
//     poly_compress(r + i * bytes_per_poly, &a->vec[i], d);
//   }
// }

void polyvec_compress(uint8_t *r, const polyvec *a, int8_t d) {
  unsigned int i;
  unsigned int bytes_per_poly = 0;
  
  // Dynamically compute bytes per compressed polynomial
#if (COMPASS_KEM_Q == 3329)
  // q=3329, n=256
  if (d == 2) {
    bytes_per_poly = 320; // 256 * 10 bits / 8
  } else if (d == 6) {
    bytes_per_poly = 192; // 256 * 6 bits / 8
  } else if (d == 8) {
    bytes_per_poly = 128; // 256 * 4 bits / 8 (drop 8 bits, keep 4 bits)
  }
#elif (COMPASS_KEM_Q == 7681)
  // q=7681, n=512
  if (d == 2) {
    bytes_per_poly = 704; // 512 * 11 bits / 8
  } else if (d == 6) {
    bytes_per_poly = 448; // 512 * 7 bits / 8
  } else if (d == 8) {
    bytes_per_poly = 320; // 512 * 5 bits / 8 (drop 8 bits, keep 5 bits)
  }
#else
  #error "Unsupported COMPASS_KEM_Q in polyvec_compress"
#endif

  // Call per-polynomial compression in a loop
  for(i = 0; i < COMPASS_KEM_K; i++) {
    poly_compress(r + i * bytes_per_poly, &a->vec[i], d);
  }
}

/*************************************************
* Name:        polyvec_decompress
*
* Description: De-serialize and decompress vector of polynomials;
* approximate inverse of polyvec_compress
*
* Arguments:   - polyvec *r:       pointer to output vector of polynomials
* - const uint8_t *a: pointer to input byte array
* (of length COMPASS_KEM_POLYVECCOMPRESSEDBYTES)
**************************************************/
void polyvec_decompress(polyvec *r, const uint8_t *a, int8_t d) {
  unsigned int i;
  unsigned int bytes_per_poly = 0;

  // Dynamically compute bytes per compressed polynomial
#if (COMPASS_KEM_Q == 3329)
  if (d == 2) {
    bytes_per_poly = 320; 
  } else if (d == 6) {
    bytes_per_poly = 192; 
  } else if (d == 8) {
    bytes_per_poly = 128; 
  }
#elif (COMPASS_KEM_Q == 7681)
  if (d == 2) {
    bytes_per_poly = 704; 
  } else if (d == 6) {
    bytes_per_poly = 448; 
  } else if (d == 8) {
    bytes_per_poly = 320; 
  }
#else
  #error "Unsupported COMPASS_KEM_Q in polyvec_decompress"
#endif

  // Call per-polynomial decompression in a loop
  for(i = 0; i < COMPASS_KEM_K; i++) {
    poly_decompress(&r->vec[i], a + i * bytes_per_poly, d);
  }
}

/*************************************************
* Name:        polyvec_tobytes
*
* Description: Serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for COMPASS_KEM_POLYVECBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_tobytes(uint8_t r[COMPASS_KEM_POLYVECBYTES], const polyvec *a)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_tobytes(r+i*COMPASS_KEM_POLYBYTES, &a->vec[i]);
}

/*************************************************
* Name:        polyvec_frombytes
*
* Description: De-serialize vector of polynomials;
*              inverse of polyvec_tobytes
*
* Arguments:   - uint8_t *r:       pointer to output byte array
*              - const polyvec *a: pointer to input vector of polynomials
*                                  (of length COMPASS_KEM_POLYVECBYTES)
**************************************************/
void polyvec_frombytes(polyvec *r, const uint8_t a[COMPASS_KEM_POLYVECBYTES])
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_frombytes(&r->vec[i], a+i*COMPASS_KEM_POLYBYTES);
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
  for(i=0;i<COMPASS_KEM_K;i++)
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
void polyvec_invntt_tomont(polyvec *r)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_invntt_tomont(&r->vec[i]);
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
void polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b)
{
  unsigned int i;
  poly t;

  poly_basemul_montgomery(r, &a->vec[0], &b->vec[0]);
  for(i=1;i<COMPASS_KEM_K;i++) {
    poly_basemul_montgomery(&t, &a->vec[i], &b->vec[i]);
    poly_add(r, r, &t);
  }

  poly_reduce(r);
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
  for(i=0;i<COMPASS_KEM_K;i++)
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
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}
