#ifndef SIGN_RING_H
#define SIGN_RING_H

#include "parameters.h"
#include "ring.h"
#include "packing.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void poly_shift(poly *r, int32_t shift);
  void ring_shift(ring_element *r, int32_t shift);
  void ring_shift_and_sub(ring_element *r, const ring_element *f, const ring_element *g, int32_t shift);
  void poly_power2round(poly *r0, poly *r1, const poly *f, int32_t d);
  void ring_power2round_and_pack(unsigned char *r0, unsigned char *r1, const ring_element *f);
  void ring_w1(unsigned char w1[RRLWR_SIGN_PACKED_W1_LEN], const ring_element *w);
  int poly_sampleInBall(poly *c, unsigned char ctilde[RRLWR_SIGN_CTILDE_LEN]);
  int poly_check_norm(const poly *f, int32_t bound);
  int poly_make_hint(poly *h, const poly *f, const poly *g);
  void poly_apply_hint(poly *r, const poly *f, const poly *h);
  void ring_pack_hint(unsigned char h[RRLWR_SIGN_H_LEN8], const ring_element *hint);
  int ring_unpack_hint(ring_element *hint, const unsigned char h[RRLWR_SIGN_H_LEN8]);

#ifdef __cplusplus
}
#endif

#endif