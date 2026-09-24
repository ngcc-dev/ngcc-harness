#ifndef NTT7681_AVX_H
#define NTT7681_AVX_H

#include <stdint.h>
#include "params.h"

#if defined(WEAVER_USE_AVX_NTT7681) && (WEAVER_Q == 7681) && \
    (WEAVER_N == 256 || WEAVER_N == 512)

#define ntt7681_avx WEAVER_NAMESPACE(_ntt7681_avx)
void ntt7681_avx(int16_t r[WEAVER_N]);

#define invntt7681_avx WEAVER_NAMESPACE(_invntt7681_avx)
void invntt7681_avx(int16_t r[WEAVER_N]);

#define basemul7681_avx WEAVER_NAMESPACE(_basemul7681_avx)
void basemul7681_avx(int16_t r[WEAVER_N],
                     const int16_t a[WEAVER_N],
                     const int16_t b[WEAVER_N]);

#define poly7681_reduce_avx WEAVER_NAMESPACE(_poly7681_reduce_avx)
void poly7681_reduce_avx(int16_t r[WEAVER_N]);

#endif

#endif
