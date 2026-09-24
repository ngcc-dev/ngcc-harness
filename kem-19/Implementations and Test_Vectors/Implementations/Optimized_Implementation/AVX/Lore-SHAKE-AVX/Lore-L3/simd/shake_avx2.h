#ifndef LORE_SHAKE_AVX2_H
#define LORE_SHAKE_AVX2_H

#include <stddef.h>
#include <stdint.h>

#ifdef LORE_USE_AVX2_SHAKE
void lore_shake128x4_absorb_once_squeezeblocks(uint8_t *out0,
                                               uint8_t *out1,
                                               uint8_t *out2,
                                               uint8_t *out3,
                                               size_t outblocks,
                                               const uint8_t *in0,
                                               const uint8_t *in1,
                                               const uint8_t *in2,
                                               const uint8_t *in3,
                                               size_t inlen);
#endif

#endif
