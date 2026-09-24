#include "poly.h"
#include "ntt.h"

void poly_basemul32_avx(poly *r, const poly *f, const poly *g, int32_t prime, int32_t primeinv);
void poly_add32_avx(poly *r, const poly *f, const poly *g, int32_t prime);
void poly_add_avx(poly *r, const poly *f, const poly *g);
void poly_sub32_avx(poly *r, const poly *f, const poly *g, int32_t prime);
void poly_sub_avx(poly *r, const poly *f, const poly *g);
void poly_reduce_pow2_avx(poly *r, const poly *f, int32_t d);
void poly_conditional_final_reduce32_avx(poly *r, int32_t prime);
void poly_round_xtoy_avx(poly *r, const poly *f, int32_t x, int32_t y);
void poly_compress_avx(poly *r, int32_t x);
void poly_decompress_avx(poly *r, int32_t x);

void poly_ntt32(poly *f, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]) {
  (void)primeinv;
  (void)fp_zetas;
  if (prime == RRLWR_SIGN_PRIME1) {
    ntt_avx(f->coeffs, prime, RRLWR_SIGN_NTT_AVX_PARAMS1);
  } else {
    ntt_avx(f->coeffs, prime, RRLWR_SIGN_NTT_AVX_PARAMS2);
  }
}

void poly_invntt32(poly *f, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t fp_zetas[RRLWR_N]) {
  (void)primeinv;
  (void)finalconst;
  (void)fp_zetas;
  if (prime == RRLWR_SIGN_PRIME1) {
    intt_avx(f->coeffs, prime, RRLWR_SIGN_INTT_FUSED_AVX_PARAMS1);
  } else {
    intt_avx(f->coeffs, prime, RRLWR_SIGN_INTT_FUSED_AVX_PARAMS2);
  }
}

void poly_basemul32(poly *r, const poly *f, const poly *g, int32_t prime, int32_t primeinv) {
  poly_basemul32_avx(r, f, g, prime, primeinv);
}

void poly_add32(poly *r, poly *f, poly *g, int32_t prime) {
  poly_add32_avx(r, f, g, prime);
}

void poly_add(poly *r, poly *f, poly *g) {
  poly_add_avx(r, f, g);
}

void poly_sub32(poly *r, poly *f, poly *g, int32_t prime) {
  poly_sub32_avx(r, f, g, prime);
}

void poly_sub(poly *r, poly *f, poly *g) {
  poly_sub_avx(r, f, g);
}

/// @brief Reduce the input modulo 2**d into signed interval [-2**d, 2**d-1]
void poly_reduce_pow2(poly *r, poly *f, int32_t d) {
  poly_reduce_pow2_avx(r, f, d);
}

/// @brief Reduce the input modulo prime into signed interval [-(prime-1)/2, (prime-1)/2-1]
void poly_conditional_final_reduce32(poly *r, int32_t prime) {
  poly_conditional_final_reduce32_avx(r, prime);
}

void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y) {
  poly_round_xtoy_avx(r, f, x, y);
}

void poly_compress(poly *r, int32_t x) {
  poly_compress_avx(r, x);
}

void poly_decompress(poly *r, int32_t x) {
  poly_decompress_avx(r, x);
}
