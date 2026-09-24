#ifndef NTT3329_AVX128_H
#define NTT3329_AVX128_H

#include <stdint.h>
#include "params.h"

#if defined(WEAVER_USE_AVX_NTT128) && (WEAVER_Q == 3329) && (WEAVER_N == 128)

#define ntt3329_avx128 WEAVER_NAMESPACE(_ntt3329_avx128)
void ntt3329_avx128(int16_t r[WEAVER_N]);

#define invntt3329_avx128 WEAVER_NAMESPACE(_invntt3329_avx128)
void invntt3329_avx128(int16_t r[WEAVER_N]);

#define basemul3329_avx128 WEAVER_NAMESPACE(_basemul3329_avx128)
void basemul3329_avx128(int16_t r[WEAVER_N],
                        const int16_t a[WEAVER_N],
                        const int16_t b[WEAVER_N]);

#define poly3329_reduce_avx128 WEAVER_NAMESPACE(_poly3329_reduce_avx128)
void poly3329_reduce_avx128(int16_t r[WEAVER_N]);

#endif

#endif
