#ifndef ROUNDING_H
#define ROUNDING_H

#include <stdint.h>
#include <immintrin.h>

__m256i decompose_keygen_avx2(__m256i a, __m256i *a0);
__m256i decompose_sign_avx2(__m256i a, __m256i *a0);
__m256i make_hint_avx2(const __m256i a, const __m256i b);
__m256i use_hint_avx2(__m256i a, __m256i hint);

#endif
