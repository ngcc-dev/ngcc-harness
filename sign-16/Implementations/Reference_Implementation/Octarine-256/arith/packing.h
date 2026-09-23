#ifndef PACKING_H
#define PACKING_H

#include "parameters.h"
#include "ring.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void poly_pack(unsigned char *b, poly *r, int32_t bitlen);
  void ring_pack(unsigned char *b, ring_element *r, int32_t bitlen);
  void subpoly_unpack(int32_t *r, unsigned int ncoeffs, const unsigned char *b, unsigned int bitlen);
  void poly_unpack(poly *r, const unsigned char *b, int32_t bitlen);
  void ring_unpack(ring_element *r, const unsigned char *b, int32_t bitlen);

#ifdef __cplusplus
}
#endif

#endif