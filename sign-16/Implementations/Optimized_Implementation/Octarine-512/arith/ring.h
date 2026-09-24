#ifndef RING_H
#define RING_H

#include "poly.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct{
    poly x[RRLWR_K] __attribute__((aligned(32)));
  } ring_element;

  typedef struct{
    poly x[2 * RRLWR_K - 1] __attribute__((aligned(32)));
  } ring_element_Awin;

  void ring_ntt32(ring_element *r, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]);
  void ring_Awin_ntt_prepare_ncoeffs(ring_element_Awin *aw, int ncoeffs, int32_t prime, int32_t primeinv, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N]);
  void ring_uniform_Awin(ring_element_Awin *aw, int32_t bitlen, const unsigned char *seed, int32_t seed_len, int32_t prime, int32_t primeinv, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N]);
  void ring_to_Awin_ncoeffs(ring_element_Awin *aw, const ring_element *a, int ncoeffs, int32_t prime, int32_t primeinv, int32_t oneR, int32_t twoR, int32_t fp_zetas[RRLWR_N]);
  void ring_mul_Awin_invntt32(poly *r, const ring_element_Awin *a, ring_element *b, int ncoeffs, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t fp_zetas[RRLWR_N]);
  void ring_round_xtoy(ring_element *r, const ring_element *f, int32_t x, int32_t y);

#ifdef __cplusplus
}
#endif

#endif
