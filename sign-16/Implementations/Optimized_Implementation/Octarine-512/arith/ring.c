#include "ring.h"
#include "uniform.h"

void ring_mul_Awin_row_avx(poly *r,
                           const poly *row,
                           const ring_element *b,
                           int k,
                           int32_t prime,
                           int32_t primeinv);

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

static void ring_Awin_prepare_twists_ncoeffs(ring_element_Awin *aw,
                                             int ncoeffs,
                                             int32_t prime,
                                             int32_t primeinv,
                                             int32_t oneR,
                                             int32_t twoR,
                                             int32_t fp_zetas[RRLWR_N])
{
  poly yp2 __attribute__((aligned(32)));

  if(ncoeffs <= 1) {
    (void)prime;
    (void)primeinv;
    (void)oneR;
    (void)twoR;
    (void)fp_zetas;
    return;
  }

  compute_yp2(&yp2, prime, primeinv, fp_zetas, oneR, twoR);

  for(int i = 0; i < ncoeffs - 1; i++) {
    poly_basemul32(&aw->x[RRLWR_K + i],
                   &aw->x[i],
                   &yp2,
                   prime,
                   primeinv);
  }
}

void ring_Awin_ntt_prepare_ncoeffs(ring_element_Awin *aw,
                                   int ncoeffs,
                                   int32_t prime,
                                   int32_t primeinv,
                                   int32_t oneR,
                                   int32_t twoR,
                                   int32_t fp_zetas[RRLWR_N])
{
  for(int i = 0; i < RRLWR_K; i++) {
    poly_ntt32(&aw->x[i], prime, primeinv, fp_zetas);
  }

  ring_Awin_prepare_twists_ncoeffs(aw, ncoeffs, prime, primeinv, oneR, twoR, fp_zetas);
}

void ring_uniform_Awin(ring_element_Awin *aw,
                       int32_t bitlen,
                       const unsigned char *seed,
                       int32_t seed_len,
                       int32_t prime,
                       int32_t primeinv,
                       int32_t oneR,
                       int32_t twoR,
                       int32_t fp_zetas[RRLWR_N])
{
  ring_uniform_Awin_base(aw, bitlen, seed, seed_len);
  ring_Awin_ntt_prepare_ncoeffs(aw, RRLWR_K, prime, primeinv, oneR, twoR, fp_zetas);
}

void ring_to_Awin_ncoeffs(ring_element_Awin *aw,
                          const ring_element *a,
                          int ncoeffs,
                          int32_t prime,
                          int32_t primeinv,
                          int32_t oneR,
                          int32_t twoR,
                          int32_t fp_zetas[RRLWR_N])
{
  for(int i = 0; i < RRLWR_K; i++) {
    aw->x[RRLWR_K - 1 - i] = a->x[i];
  }

  ring_Awin_ntt_prepare_ncoeffs(aw, ncoeffs, prime, primeinv, oneR, twoR, fp_zetas);
}

static void ring_mul_Awin_ntt_dot32(poly *r,
                                    const ring_element_Awin *a,
                                    ring_element *b,
                                    int ncoeffs,
                                    int32_t prime,
                                    int32_t primeinv)
{
  int row_min = RRLWR_K - ncoeffs;

  for(int i = RRLWR_K - 1; i >= row_min; i--) {
    int out = i - row_min;
    const poly *row = &a->x[RRLWR_K - 1 - i];

    ring_mul_Awin_row_avx(&r[out], row, b, RRLWR_K, prime, primeinv);
  }
}

void ring_mul_Awin_invntt32(poly *r,
                            const ring_element_Awin *a,
                            ring_element *b,
                            int ncoeffs,
                            int32_t prime,
                            int32_t primeinv,
                            int32_t finalconst,
                            int32_t fp_zetas[RRLWR_N])
{
  ring_mul_Awin_ntt_dot32(r, a, b, ncoeffs, prime, primeinv);

  for(int i = 0; i < ncoeffs; i++) {
    poly_invntt32(&r[i], prime, primeinv, finalconst, fp_zetas);
    poly_conditional_final_reduce32(&r[i], prime);
  }
}

void ring_round_xtoy(ring_element *r, const ring_element *f, int32_t x, int32_t y) {
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_round_xtoy(&r->x[i], &f->x[i], x, y);
  }
}
