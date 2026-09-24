#ifndef POLY_H
#define POLY_H

#include "fprime.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct{
    int32_t coeffs[RRLWR_N];
  } poly;

  void poly_ntt32(poly *f, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]);
  void poly_invntt32(poly *f, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t fp_zetas[RRLWR_N]);
  void poly_basemul32(poly *r, poly *f, poly *g, int32_t prime, int32_t primeinv);
#ifndef RRLWR_DISABLE_NTT_AVX
  void poly_basemul_add32(poly *r, const poly *f, const poly *g, int32_t prime,
                          int32_t primeinv);
  void poly_basemul32_avx(poly *r, poly *f, poly *g, int32_t prime, int32_t primeinv);
#endif
  void poly_add32(poly *r, poly *f, poly *g, int32_t prime);
  void poly_add(poly *r, poly *f, poly *g);
  void poly_sub32(poly *r, poly *f, poly *g, int32_t prime);
  void poly_sub(poly *r, poly *f, poly *g);
  void poly_reduce_pow2(poly *r, poly *f, int32_t d);
  void poly_conditional_final_reduce32(poly *r, int32_t prime);
  void poly_final_reduce_pow2(poly *r, int32_t prime, int32_t d);
  void poly_reduce_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y);
  void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y);
  void poly_compress(poly *r, int32_t x);
  void poly_decompress(poly *r, int32_t x);

#ifdef __cplusplus
}
#endif

#endif
