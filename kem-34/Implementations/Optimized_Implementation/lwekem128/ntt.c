#include "ntt.h"
#include "params.h"
#include "reduce.h"
#include <stdint.h>
#include <stdio.h>

/*************************************************
 * Name:        ntt
 *
 * Description: Computes negacyclic number-theoretic transform (NTT) of
 *              a polynomial (vector of 64 coefficients) in place;
 *              inputs assumed to be in normal order, output in bitreversed
 * order
 *
 * Arguments:   - uint16_t *p: pointer to in/output polynomial
 **************************************************/
void
ntt (uint16_t p[64])
{
	ntt_avx (p, qdata);
}


/*************************************************
 * Name:        invntt
 *
 * Description: Computes inverse of negacyclic number-theoretic transform (NTT)
 * of a polynomial (vector of 64 coefficients) in place; inputs assumed to be
 * in bitreversed order, output in normal order
 *
 * Arguments:   - uint16_t *a: pointer to in/output polynomial
 **************************************************/
void
invntt (uint16_t a[64])
{
	invntt_avx (a, qdata);
}

///*************************************************
//* Name:        poly_basemul_montgomery
//*
//* Description: Multiplication of two polynomials in NTT domain
//*
//* Arguments:   - uint16_t *r: pointer to output polynomial
//*              - const uint16_t *a: pointer to first input polynomial
//*              - const uint16_t *b: pointer to second input polynomial
//**************************************************/
// void poly_basemul_montgomery(uint16_t r[KEM_N], const uint16_t a[KEM_N],
// const uint16_t b[KEM_N])
//{
//  int j;
//  uint16_t t;
//  for(j=0;j<KEM_N;j++)
//  {
//	t = montgomery_reduce(1674* (uint32_t)b[j]); // 1674 = 2^{2*18} % q
//    r[j] = montgomery_reduce(a[j] * t);
//    r[j] = barrett_reduce(r[j]);
//  }
//}
