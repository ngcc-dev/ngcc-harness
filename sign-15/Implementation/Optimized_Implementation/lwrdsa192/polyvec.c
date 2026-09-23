#include <stdint.h>
#include "params.h"
#include "rounding.h"
#include "poly.h"
#include "polyvec.h"


void polyvec_l_mul(polyvecl mat[K], polyvecl *s1, polyveck *t){

  poly t_temp;

  for (unsigned int i1 = 0; i1 < K; i1++)
  {
    for (unsigned int i2 = 0; i2 < N; i2++)
    {
      t->vec[i1].coeffs[i2] = 0;
    }

  }

  for (unsigned int i1 = 0; i1 < K; i1++)
  {
    for (unsigned int j1 = 0; j1 < L; j1++)
    {
      pol_mul(&t_temp, &mat[i1].vec[j1], &s1->vec[j1]);
      for (unsigned int i2 = 0; i2 < N; i2++)
      {
        t->vec[i1].coeffs[i2] += t_temp.coeffs[i2];

        t->vec[i1].coeffs[i2] = t->vec[i1].coeffs[i2] & (Q-1);

      }
    }
  }
}

/**************************************************************/
/************ Vectors of polynomials of length L **************/
/**************************************************************/

/*************************************************
 * Name:        polyvecl_freeze
 *
 * Description: Reduce coefficients of polynomials in vector of
 *              length L to standard representatives
 *
 * Arguments:   - polyvecl *v: pointer to input/output vector
 **************************************************/
void polyvecl_freeze(polyvecl *v, int mod) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_freeze_avx2(v->vec+i, mod);
}

/*************************************************
 * Name:        polyvecl_add
 *
 * Description: Add vectors of polynomials of length L.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyvecl *w: pointer to output vector
 *              - polyvecl *u: pointer to first summand
 *              - polyvecl *v: pointer to second summand
 **************************************************/
void polyvecl_add(polyvecl *w, const polyvecl *u, const polyvecl *v) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_add(w->vec+i, u->vec+i, v->vec+i);
}


/*************************************************
 * Name:        polyvecl_chknorm
 *
 * Description: Check infinity norm of polynomials in vector of length L.
 *              Assumes input coefficients to be standard representatives.
 *
 * Arguments:   - const polyvecl *v: pointer to vector of length L
 *              - uint32_t B: norm bound
 *
 * Returns 0 if norm of all polynomials is strictly smaller than B and 1
 * otherwise.
 **************************************************/
int polyvecl_chknorm(const polyvecl *v, uint32_t bound, int mod)  {
  unsigned int i;

  for(i = 0; i < L; ++i)
    if (poly_chknorm(v->vec+i, bound, mod))
      return 1;

  return 0;
}

/**************************************************************/
/************ Vectors of polynomials of length K **************/
/**************************************************************/

/*************************************************
 * Name:        polyveck_freeze
 *
 * Description: Reduce coefficients of polynomials in vector of length K
 *              to standard representatives
 *
 * Arguments:   - polyveck *v: pointer to input/output vector
 **************************************************/
void polyveck_freeze(polyveck *v, int mod)  {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_freeze_avx2(v->vec+i, mod);
}

/*************************************************
 * Name:        polyveck_add
 *
 * Description: Add vectors of polynomials of length K.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyveck *w: pointer to output vector
 *              - polyveck *u: pointer to first summand
 *              - polyveck *v: pointer to second summand
 **************************************************/
void polyveck_add(polyveck *w, const polyveck *u, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_add(w->vec+i, u->vec+i, v->vec+i);
}

/*************************************************
 * Name:        polyveck_sub
 *
 * Description: Subtract vectors of polynomials of length K.
 *              Assumes coefficients of polynomials in input vectors to be less
 *              than 2*Q. No modular reduction is performed.
 *
 * Arguments:   - polyveck *w: pointer to output vector
 *              - polyveck *u: pointer to first input vector
 *              - polyveck *v: pointer to second input vector to be subtracted
 *                             from first input vector
 **************************************************/
void polyveck_sub(polyveck *w, const polyveck *u, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_sub(w->vec+i, u->vec+i, v->vec+i);
}

/*************************************************
 * Name:        polyveck_neg
 *
 * Description: Negate vector of polynomials of length K.
 *              Assumes input coefficients to be less than 2*Q.
 *
 * Arguments:   - polyveck *v: pointer to input/output vector
 **************************************************/
void polyveck_neg(polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_neg(v->vec+i);
}

/*************************************************
 * Name:        polyveck_shiftl
 *
 * Description: Multiply vector of polynomials of Length K by 2^k
 *
 * Arguments:   - polyveck *v: pointer to input/output vector
 *              - unsigned int k: exponent
 **************************************************/
void polyveck_shiftl(polyveck *v, unsigned int k) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_shiftl(v->vec+i, k);
}

/*************************************************
 * Name:        polyveck_chknorm
 *
 * Description: Check infinity norm of polynomials in vector of length K.
 *              Assumes input coefficients to be standard representatives.
 *
 * Arguments:   - const polyveck *v: pointer to vector of length K
 *              - uint32_t B: norm bound
 *
 * Returns 0 if norm of all polynomials is strictly smaller than B and 1
 * otherwise.
 **************************************************/
int polyveck_chknorm(const polyveck *v, uint32_t bound, int mod) {
  unsigned int i;
  int ret = 0;

  for(i = 0; i < K; ++i)
    ret |= poly_chknorm(v->vec+i, bound, mod);

  return ret;
}

/*************************************************
 * Name:        polyveck_power2round
 *
 * Description: For all coefficients a of polynomials in vector of length K,
 *              compute a0, a1 such that a = a1*2^D + a0
 *              with -2^{D/2} < a0 <= 2^{D/2}
 *
 * Arguments:   - polyveck *v1: pointer to output vector of polynomials with
 *                              coefficients a1
 *              - polyveck *v0: pointer to output vector of polynomials with
 *                              coefficients a0
 *              - polyveck *v: pointer to input vector
 **************************************************/
void polyveck_power2round_avx2(polyveck *v1, polyveck *v0, const polyveck *v) {
  unsigned int i, j;

  for(i = 0; i < K; ++i)
  {
    for(j = 0; j < N/8; ++j){
      v1->vec[i].vec[j] = decompose_keygen_avx2(v->vec[i].vec[j], &v0->vec[i].vec[j]);

    }

  }

}

/*************************************************
 * Name:        polyveck_decompose
 *
 * Description: For all coefficients a of polynomials in vector of length K,
 *              compute high and low bits a0, a1 such a = a1*ALPHA + a0
 *              with -ALPHA/2 < a0 <= ALPHA/2. Assumes a to be standard
 *              representative.
 *
 * Arguments:   - polyveck *v1: pointer to output vector of polynomials with
 *                              coefficients a1
 *              - polyveck *v0: pointer to output vector of polynomials with
 *                              coefficients a0
 *              - polyveck *v: pointer to input vector
 **************************************************/
void polyveck_decompose_avx2(polyveck *v1, polyveck *v0, const polyveck *v) {
  unsigned int i, j;

  for(i = 0; i < K; ++i)
    for(j = 0; j < N/8; ++j)
      v1->vec[i].vec[j] = decompose_sign_avx2(v->vec[i].vec[j],
          &v0->vec[i].vec[j]);
}

/*************************************************
 * Name:        polyveck_make_hint
 *
 * Description: Compute hint vector. The coefficients of the polynomials indicate
 *              whether or not the high bits of the corresponding input
 *              polynomials differ.
 *
 * Arguments:   - polyveck *h: pointer to output vector
 *              - const polyveck *u: pointer to first input vector
 *              - const polyveck *u: pointer to second input vector
 *
 * Returns number of 1 bits.
 **************************************************/
unsigned int polyveck_make_hint_avx2(polyveck *h,
    const polyveck *u,
    const polyveck *v)
{
  unsigned int i, j, s = 0;

  for (i = 0; i < K; i++)
    for (j = 0; j < N/8; j++) {
      __m256i u_avx = _mm256_loadu_si256(u->vec[i].vec+j);
      __m256i v_avx = _mm256_loadu_si256(v->vec[i].vec+j);
      __m256i h_avx = make_hint_avx2(u_avx, v_avx);
      s += _mm256_extract_epi32(h_avx, 0);
      s += _mm256_extract_epi32(h_avx, 1);
      s += _mm256_extract_epi32(h_avx, 2);
      s += _mm256_extract_epi32(h_avx, 3);
      s += _mm256_extract_epi32(h_avx, 4);
      s += _mm256_extract_epi32(h_avx, 5);
      s += _mm256_extract_epi32(h_avx, 6);
      s += _mm256_extract_epi32(h_avx, 7);
      _mm256_storeu_si256(h->vec[i].vec+j, h_avx);
    }

  return s;
}

/*************************************************
 * Name:        polyveck_use_hint
 *
 * Description: Use hint vector to correct the high bits of input vector
 *
 * Arguments:   - polyveck *w: pointer to output vector of polynomials with
 *                             corrected high bits
 *              - polyveck *u: pointer to input vector
 *              - polyveck *h: pointer to input hint vector
 **************************************************/
void polyveck_use_hint_avx2(polyveck *w, const polyveck *u, const polyveck *h) {
  unsigned int i, j;

  for(i = 0; i < K; i++)
    for (j = 0; j < N/8; j++) {
      __m256i u_avx = _mm256_loadu_si256(u->vec[i].vec+j);
      __m256i h_avx = _mm256_loadu_si256(h->vec[i].vec+j);
      __m256i w_avx = use_hint_avx2(u_avx, h_avx);
      _mm256_storeu_si256(w->vec[i].vec+j, w_avx);
    }
}
