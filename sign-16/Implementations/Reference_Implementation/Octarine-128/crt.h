#ifndef CRT_H 
#define CRT_H

#include "parameters.h"
#include "ring.h"

typedef struct {
  poly polyp1;
  poly polyp2;
} polyx2;

typedef struct {
  ring_element xp1;
  ring_element xp2;
} ring_elementx2;

extern int32_t rrlwr_sign_zetas1[RRLWR_N];
extern int32_t rrlwr_sign_zetas2[RRLWR_N];

#ifdef __cplusplus
extern "C"
{
#endif

  int64_t crt(int32_t a, int32_t b);
  void poly_crt(poly *r, const poly *f, const poly *g);
  void ring_crt(ring_element *r, const ring_element *f, const ring_element *g);
  void ring_ntt32x2(ring_elementx2 *r, const ring_element *f);
  void ring_mul_invntt32x2(ring_element *r, ring_elementx2 *a, ring_elementx2 *b);
  void ring_mul32x2(ring_element *r, const ring_element *f, const ring_element *g);

#ifdef __cplusplus
}
#endif

#endif