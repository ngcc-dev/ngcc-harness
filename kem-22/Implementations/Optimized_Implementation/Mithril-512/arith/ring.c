#include "ring.h"
#include "packing.h"
#include "uniform.h"

#ifndef RRLWR_DISABLE_NTT_AVX
void ring_mul_Awin_row_avx(poly *r,
                           const poly *row,
                           const ring_element *b,
                           int k,
                           int32_t prime,
                           int32_t primeinv);
void ring_mul_Awin_row_k5_avx(poly *r,
                              const poly *row,
                              const ring_element *b,
                              int k,
                              int32_t prime,
                              int32_t primeinv);
void ring_mul_Awin_row_k9_avx(poly *r,
                              const poly *row,
                              const ring_element *b,
                              int k,
                              int32_t prime,
                              int32_t primeinv);
void ring_mul_Awin_row_k17_avx(poly *r,
                               const poly *row,
                               const ring_element *b,
                               int k,
                               int32_t prime,
                               int32_t primeinv);
void ring_mul_Awin_2rows_rev_k5_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_2rows_rev_k9_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_2rows_rev_k17_avx(poly *r,
                                     const poly *row,
                                     const ring_element *b,
                                     int32_t prime,
                                     int32_t primeinv);
void ring_mul_Awin_4rows_rev_k5_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_4rows_rev_k9_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_4rows_rev_k17_avx(poly *r,
                                     const poly *row,
                                     const ring_element *b,
                                     int32_t prime,
                                     int32_t primeinv);
void ring_mul_Awin_5rows_rev_k5_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_5rows_rev_k9_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_5rows_rev_k17_avx(poly *r,
                                     const poly *row,
                                     const ring_element *b,
                                     int32_t prime,
                                     int32_t primeinv);
#endif

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
#ifdef RRLWR_DISABLE_NTT_AVX
  poly yp2;
#else
  poly yp2 __attribute__((aligned(32)));
#endif

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
#ifndef RRLWR_DISABLE_NTT_AVX
    poly_basemul32_avx(&aw->x[RRLWR_K + i],
                       &aw->x[i],
                       &yp2,
                       prime,
                       primeinv);
#else
    poly_basemul32(&aw->x[RRLWR_K + i],
                   &aw->x[i],
                   &yp2,
                   prime,
                   primeinv);
#endif
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

void ring_Awin_ntt_prepare(ring_element_Awin *aw,
                           int32_t prime,
                           int32_t primeinv,
                           int32_t oneR,
                           int32_t twoR,
                           int32_t fp_zetas[RRLWR_N])
{
  ring_Awin_ntt_prepare_ncoeffs(aw, RRLWR_K, prime, primeinv, oneR, twoR, fp_zetas);
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
  ring_Awin_ntt_prepare(aw, prime, primeinv, oneR, twoR, fp_zetas);
}

void ring_to_Awin_ncoeffs(ring_element_Awin *aw,
                          ring_element *a,
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

void ring_unpack_Awin_ncoeffs(ring_element_Awin *aw,
                              const unsigned char *b,
                              int32_t bitlen,
                              int ncoeffs,
                              int32_t prime,
                              int32_t primeinv,
                              int32_t oneR,
                              int32_t twoR,
                              int32_t fp_zetas[RRLWR_N])
{
  unsigned int offset = bitlen * (RRLWR_N >> 3);

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_unpack(&aw->x[RRLWR_K - 1 - i], b + i * offset, bitlen);
  }

  ring_Awin_ntt_prepare_ncoeffs(aw, ncoeffs, prime, primeinv, oneR, twoR, fp_zetas);
}

/// @brief Full ring multiplication assuming its inputs are already in NTT domain.
///        The output element r is not in NTT domain and is fully reduced with all coefficients in [-q/2, q/2+1]
void ring_mul_invntt32(poly *r, ring_element *a, ring_element *b, int ncoeffs, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N]) {
#ifdef RRLWR_DISABLE_NTT_AVX
  poly yp2, t;
#else
  poly yp2;
#endif

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
#ifndef RRLWR_DISABLE_NTT_AVX
      poly_basemul_add32(&r[rindex], &a->x[i-j], &b->x[j], prime, primeinv); // Introduces a Montgomery factor R^-1
#else
      poly_basemul32(&t, &a->x[i-j], &b->x[j], prime, primeinv); // Introduces a Montgomery factor R^-1
      poly_add32(&r[rindex], &r[rindex], &t, prime);
#endif
    }

    // Multiply matrix element by (y+2)
    if (i+1 < RRLWR_K) {
#ifndef RRLWR_DISABLE_NTT_AVX
      poly_basemul32_avx(&a->x[i+1], &a->x[i+1], &yp2, prime, primeinv); // Montgomery factor R cancelled from yp2
#else
      poly_basemul32(&a->x[i+1], &a->x[i+1], &yp2, prime, primeinv); // Montgomery factor R cancelled from yp2
#endif
    }

    // Process row elements multiplied by (y+2)
    for(int j = i+1; j < RRLWR_K; j++) {
#ifndef RRLWR_DISABLE_NTT_AVX
      poly_basemul_add32(&r[rindex], &a->x[i-j+RRLWR_K], &b->x[j], prime, primeinv); // Introduces a Montgomery factor R^-1
#else
      poly_basemul32(&t, &a->x[i-j+RRLWR_K], &b->x[j], prime, primeinv); // Introduces a Montgomery factor R^-1
      poly_add32(&r[rindex], &r[rindex], &t, prime);
#endif
    }

    rindex--;
  }

  // Transfrom to polynomial domain
  for(int i = 0; i < ncoeffs; i++) {
    poly_invntt32(&r[i], prime, primeinv, finalconst, fp_zetas); // Removes the factor R^-1 in the final multiplication
    poly_conditional_final_reduce32(&r[i], prime); // Reduce to unique representation in [-(p-1)/2+1, (p-1)/2]
  }
}

static void ring_mul_Awin_ntt_dot32(poly *r,
                                    const ring_element_Awin *a,
                                    ring_element *b,
                                    int ncoeffs,
                                    int32_t prime,
                                    int32_t primeinv)
{
#ifdef RRLWR_DISABLE_NTT_AVX
  poly t;
#endif

#ifndef RRLWR_DISABLE_NTT_AVX
#if RRLWR_K == 5
  int out = 0;
  for(; out + 4 < ncoeffs; out += 5) {
    ring_mul_Awin_5rows_rev_k5_avx(&r[out], &a->x[ncoeffs - 5 - out],
                                   b, prime, primeinv);
  }
  for(; out + 3 < ncoeffs; out += 4) {
    ring_mul_Awin_4rows_rev_k5_avx(&r[out], &a->x[ncoeffs - 4 - out],
                                   b, prime, primeinv);
  }
  for(; out + 1 < ncoeffs; out += 2) {
    ring_mul_Awin_2rows_rev_k5_avx(&r[out], &a->x[ncoeffs - 2 - out],
                                   b, prime, primeinv);
  }
  if(out < ncoeffs) {
    ring_mul_Awin_row_k5_avx(&r[out], &a->x[0], b, RRLWR_K, prime, primeinv);
  }
#elif RRLWR_K == 9
  int out = 0;
  for(; out + 4 < ncoeffs; out += 5) {
    ring_mul_Awin_5rows_rev_k9_avx(&r[out], &a->x[ncoeffs - 5 - out],
                                   b, prime, primeinv);
  }
  for(; out + 3 < ncoeffs; out += 4) {
    ring_mul_Awin_4rows_rev_k9_avx(&r[out], &a->x[ncoeffs - 4 - out],
                                   b, prime, primeinv);
  }
  for(; out + 1 < ncoeffs; out += 2) {
    ring_mul_Awin_2rows_rev_k9_avx(&r[out], &a->x[ncoeffs - 2 - out],
                                   b, prime, primeinv);
  }
  if(out < ncoeffs) {
    ring_mul_Awin_row_k9_avx(&r[out], &a->x[0], b, RRLWR_K, prime, primeinv);
  }
#elif RRLWR_K == 17
  int out = 0;
  for(; out + 4 < ncoeffs; out += 5) {
    ring_mul_Awin_5rows_rev_k17_avx(&r[out], &a->x[ncoeffs - 5 - out],
                                    b, prime, primeinv);
  }
  for(; out + 3 < ncoeffs; out += 4) {
    ring_mul_Awin_4rows_rev_k17_avx(&r[out], &a->x[ncoeffs - 4 - out],
                                    b, prime, primeinv);
  }
  for(; out + 1 < ncoeffs; out += 2) {
    ring_mul_Awin_2rows_rev_k17_avx(&r[out], &a->x[ncoeffs - 2 - out],
                                    b, prime, primeinv);
  }
  if(out < ncoeffs) {
    ring_mul_Awin_row_k17_avx(&r[out], &a->x[0], b, RRLWR_K, prime, primeinv);
  }
#else
  for(int out = 0; out < ncoeffs; out++) {
    const poly *row = &a->x[ncoeffs - 1 - out];

    ring_mul_Awin_row_avx(&r[out], row, b, RRLWR_K, prime, primeinv);
  }
#endif
#else
  int row_min = RRLWR_K - ncoeffs;

  for(int i = RRLWR_K - 1; i >= row_min; i--) {
    int out = i - row_min;
    const poly *row = &a->x[RRLWR_K - 1 - i];

    for(int j = 0; j < RRLWR_N; j++) {
      r[out].coeffs[j] = 0;
    }

    for(int j = 0; j < RRLWR_K; j++) {
      poly_basemul32(&t, (poly *)&row[j], &b->x[j], prime, primeinv);
      poly_add32(&r[out], &r[out], &t, prime);
    }
  }
#endif
}

void ring_mul_Awin_invntt_reduce_pow2_32(poly *r,
                                         const ring_element_Awin *a,
                                         ring_element *b,
                                         int ncoeffs,
                                         int32_t prime,
                                         int32_t primeinv,
                                         int32_t finalconst,
                                         int32_t d,
                                         int32_t fp_zetas[RRLWR_N])
{
  ring_mul_Awin_ntt_dot32(r, a, b, ncoeffs, prime, primeinv);

  for(int i = 0; i < ncoeffs; i++) {
    poly_invntt32(&r[i], prime, primeinv, finalconst, fp_zetas);
    poly_reduce_pow2(&r[i], &r[i], d);
  }
}

void ring_mul_Awin_reduce_pow2_32(poly *r,
                                  const ring_element_Awin *a,
                                  ring_element *b,
                                  int ncoeffs,
                                  int32_t prime,
                                  int32_t primeinv,
                                  int32_t finalconst,
                                  int32_t d,
                                  int32_t fp_zetas[RRLWR_N])
{
  ring_ntt32(b, prime, primeinv, fp_zetas);
  ring_mul_Awin_invntt_reduce_pow2_32(r, a, b, ncoeffs, prime, primeinv,
                                      finalconst, d, fp_zetas);
}

void ring_mul_Awin_invntt_round_xtoy_32(poly *r,
                                        const ring_element_Awin *a,
                                        ring_element *b,
                                        int ncoeffs,
                                        int32_t prime,
                                        int32_t primeinv,
                                        int32_t finalconst,
                                        int32_t x,
                                        int32_t y,
                                        int32_t fp_zetas[RRLWR_N])
{
  ring_mul_Awin_ntt_dot32(r, a, b, ncoeffs, prime, primeinv);

  for(int i = 0; i < ncoeffs; i++) {
    poly_invntt32(&r[i], prime, primeinv, finalconst, fp_zetas);
    poly_reduce_round_xtoy(&r[i], &r[i], x, y);
  }
}

void ring_mul_Awin_round_xtoy_32(poly *r,
                                 const ring_element_Awin *a,
                                 ring_element *b,
                                 int ncoeffs,
                                 int32_t prime,
                                 int32_t primeinv,
                                 int32_t finalconst,
                                 int32_t x,
                                 int32_t y,
                                 int32_t fp_zetas[RRLWR_N])
{
  ring_ntt32(b, prime, primeinv, fp_zetas);
  ring_mul_Awin_invntt_round_xtoy_32(r, a, b, ncoeffs, prime, primeinv,
                                     finalconst, x, y, fp_zetas);
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
