#ifndef REJSAMPLE_H
#define REJSAMPLE_H

#include <stdint.h>
#include "params.h"
#include "symmetric.h"

#define REJ_UNIFORM_AVX_NBLOCKS ((12*COMPASS_KEM_N/8*(1 << 12)/COMPASS_KEM_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)
#define REJ_UNIFORM_AVX_BUFLEN (REJ_UNIFORM_AVX_NBLOCKS*XOF_BLOCKBYTES)

#define rej_uniform_avx COMPASS_KEM_NAMESPACE(rej_uniform_avx)
unsigned int rej_uniform_avx(int16_t * restrict r, unsigned int len, const uint8_t *buf, unsigned int buflen);

#endif
