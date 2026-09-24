#ifndef SHUFFLE_H
#define SHUFFLE_H

#include <immintrin.h>
#include <stdint.h>
#include "consts.h"
#define shuffle8(a,b) \
    t = (a); \
    (a) = _mm256_permute2f128_si256((a), (b), 0x20); \
    (b) = _mm256_permute2f128_si256(t, (b), 0x31); \
\
 
#define shuffle4(a,b) \
    t = (a);\
    (a) =  _mm256_unpacklo_epi64((a), (b));\
    (b) =  _mm256_unpackhi_epi64(t, (b));\
\

#define shuffle2(a,b) \
    t1 = _mm256_slli_epi64((b), 32);\
    t2 = _mm256_srli_epi64((a), 32);\
    (a) = _mm256_blend_epi32((a), t1, 0xAA);\
    (b) = _mm256_blend_epi32(t2, (b), 0xAA);\
\

#define shuffle1(a,b) \
    t1 = _mm256_slli_epi32((b), 16);\
    t2 = _mm256_srli_epi32((a), 16);\
    (a) = _mm256_blend_epi16((a), t1, 0xAA);\
    (b) = _mm256_blend_epi16(t2, (b), 0xAA);\
\

#endif // SHUFFLE_H
