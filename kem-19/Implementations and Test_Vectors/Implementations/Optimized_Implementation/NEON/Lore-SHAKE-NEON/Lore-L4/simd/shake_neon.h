#ifndef LORE_SHAKE_NEON_H
#define LORE_SHAKE_NEON_H

#include <stddef.h>
#include <stdint.h>

#ifdef LORE_USE_NEON_SHAKE
void lore_shake128x2_absorb_once_squeezeblocks(uint8_t *out0,
                                               uint8_t *out1,
                                               size_t outblocks,
                                               const uint8_t *in0,
                                               const uint8_t *in1,
                                               size_t inlen);
#endif

#endif
