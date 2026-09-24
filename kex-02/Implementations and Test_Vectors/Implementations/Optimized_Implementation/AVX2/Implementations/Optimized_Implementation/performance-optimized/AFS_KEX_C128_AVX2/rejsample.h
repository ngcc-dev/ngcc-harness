#ifndef REJSAMPLE_H
#define REJSAMPLE_H

#include <stdint.h>
#include "params.h"

/*
 * Bit-exact AVX2 rejection sampling, generic over output length and buffer
 * length. Produces exactly the same accepted 12-bit coefficient sequence as
 * the scalar rej_uniform(), so KAT output is preserved.
 */
#define rej_uniform_avx KYBER_NAMESPACE(rej_uniform_avx)
unsigned int rej_uniform_avx(int16_t *r,
                             unsigned int len,
                             const uint8_t *buf,
                             unsigned int buflen);

#endif
