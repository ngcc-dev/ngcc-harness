#ifndef RING_H
#define RING_H

#include "poly.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct{
    poly x[RRLWR_K];
  } ring_element;

  void ring_ntt32(ring_element *r, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]);
  void ring_mul_invntt32(poly *r, ring_element *a, ring_element *b, int ncoeffs, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N]);
  void ring_mul32(poly *r, ring_element *a, ring_element *b, int ncoeffs, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N]);
  void ring_round_xtoy(ring_element *r, const ring_element *f, int32_t x, int32_t y);

#ifdef __cplusplus
}
#endif

#endif