#include "ring.h"

/// @brief Compute NTT(y+2) in Montgomery domain, on-the-fly only if PRECOMPUTE_TWIST is not defined
static void compute_yp2(poly *r, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N], int32_t oneR, int32_t twoR)
{
  #ifdef PRECOMPUTE_TWIST
    (void)prime;
    (void)primeinv;
    (void)fp_zetas;
    (void)oneR;
    (void)twoR;
    #ifdef CRT
    if(prime == RRLWR_SIGN_PRIME1) {
      for(unsigned int i = 0; i < RRLWR_N; i++) {
        r->coeffs[i] = precomputed_twist1[i];
      }
    } else if (prime == RRLWR_SIGN_PRIME2) {
      for(unsigned int i = 0; i < RRLWR_N; i++) {
        r->coeffs[i] = precomputed_twist2[i];
      }
    }
    #else
    for(unsigned int i = 0; i < RRLWR_N; i++) {
      r->coeffs[i] = precomputed_twist[i];
    }
    #endif
  #else
    // Initialize the polynomial y+2 in Montgomery domain
    r->coeffs[0] = twoR; // 2
    r->coeffs[1] = oneR; // 1
    for(unsigned int i = 2; i < RRLWR_N; i++) {
      r->coeffs[i] = 0;
    }
    poly_ntt32(r, prime, primeinv, fp_zetas);
  #endif
}

void ring_ntt32(ring_element *r, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]) {
  for(int i = 0; i < RRLWR_K; i++) {
    poly_ntt32(&r->x[i], prime, primeinv, fp_zetas);
  }
}

/// @brief Full ring multiplication assuming its inputs are already in NTT domain.
///        The output element r is not in NTT domain and is fully reduced with all coefficients in [-q/2, q/2+1]
void ring_mul_invntt32(poly *r, ring_element *a, ring_element *b, int ncoeffs, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N]) {
  poly yp2, t;

  if(RRLWR_K > 1) { // Not required if K = 1
    compute_yp2(&yp2, prime, primeinv, fp_zetas, oneR, twoR); // NTT(y+2) in Montgomery domain
  }

  int rindex = ncoeffs-1;

  // Remaining rows of matrix multiplication that are needed according to ncoeffs
  for(int i = RRLWR_K-1; i > RRLWR_K-1-ncoeffs; i--) {

    for(int j = 0; j < RRLWR_N; j++) {
      (r+rindex)->coeffs[j] = 0;
    }

    // Process row elements not multiplied by (y+2)
    for(int j = 0; j < i+1; j++) {
      poly_basemul32(&t, &a->x[i-j], &b->x[j], prime, primeinv); // Introduces a Montgomery factor R^-1
      poly_add32(&r[rindex], &r[rindex], &t, prime);
    }

    // Multiply matrix element by (y+2)
    if (i+1 < RRLWR_K) {
      poly_basemul32(&a->x[i+1], &a->x[i+1], &yp2, prime, primeinv); // Montgomery factor R cancelled from yp2
    }

    // Process row elements multiplied by (y+2)
    for(int j = i+1; j < RRLWR_K; j++) {
      poly_basemul32(&t, &a->x[i-j+RRLWR_K], &b->x[j], prime, primeinv); // Introduces a Montgomery factor R^-1
      poly_add32(&r[rindex], &r[rindex], &t, prime);
    }

    rindex--;
  }

  // Transfrom to polynomial domain
  for(int i = 0; i < ncoeffs; i++) {
    poly_invntt32(&r[i], prime, primeinv, finalconst, fp_zetas); // Removes the factor R^-1 in the final multiplication
    poly_conditional_final_reduce32(&r[i], prime); // Reduce to unique representation in [-(p-1)/2+1, (p-1)/2]
  }
}

/// @brief Full ring multiplication assuming its inputs are not yet in NTT domain.
///        The output element r is not in NTT domain and is fully reduced with all coefficients in [-q/2, q/2+1]
void ring_mul32(poly *r, ring_element *a, ring_element *b, int ncoeffs, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N])
{
  ring_ntt32(a, prime, primeinv, fp_zetas);
  ring_ntt32(b, prime, primeinv, fp_zetas);
  ring_mul_invntt32(r, a, b, ncoeffs, prime, primeinv, finalconst, oneR, twoR, fp_zetas);
}

void ring_round_xtoy(ring_element *r, const ring_element *f, int32_t x, int32_t y) {
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_round_xtoy(&r->x[i], &f->x[i], x, y);
  }
}
