#ifndef NTT_AVX_H
#define NTT_AVX_H

#include <immintrin.h>

#define ntt_avx COMPASS_SIG_NAMESPACE(ntt_avx)
void ntt_avx(__m256i *a, const __m256i *qdata);
#define invntt_avx COMPASS_SIG_NAMESPACE(invntt_avx)
void invntt_avx(__m256i *a, const __m256i *qdata);

#define nttunpack_avx COMPASS_SIG_NAMESPACE(nttunpack_avx)
void nttunpack_avx(__m256i *a);

#define pointwise_avx COMPASS_SIG_NAMESPACE(pointwise_avx)
void pointwise_avx(__m256i *c, const __m256i *a, const __m256i *b, const __m256i *qdata);
#define pointwise_acc_avx COMPASS_SIG_NAMESPACE(pointwise_acc_avx)
void pointwise_acc_avx(__m256i *c, const __m256i *a, const __m256i *b, const __m256i *qdata);

#endif
