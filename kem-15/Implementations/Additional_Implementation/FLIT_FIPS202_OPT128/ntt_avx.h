#ifndef NTT_AVX_H
#define NTT_AVX_H

#include <stdint.h>
#include "params.h"

#define ntt_avx KEM_NAMESPACE(_ntt_avx)
void ntt_avx(int16_t r[N], const int16_t *qdata);

#define invntt_avx KEM_NAMESPACE(_invntt_avx)
void invntt_avx(int16_t r[N], const int16_t *qdata);

#endif
