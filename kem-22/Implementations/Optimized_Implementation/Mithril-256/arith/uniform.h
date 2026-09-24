#ifndef UNIFORM_H
#define UNIFORM_H

#include "parameters.h"
#include "ring.h"
#include "packing.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void ring_uniform(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len);
  void ring_uniform_Awin_base(ring_element_Awin *aw, int32_t bitlen, const unsigned char *seed, int32_t seed_len);

#ifdef __cplusplus
}
#endif

#endif
