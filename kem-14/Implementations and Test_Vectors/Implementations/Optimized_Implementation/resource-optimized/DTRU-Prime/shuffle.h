#ifndef SHUFFLE_H
#define SHUFFLE_H

#include <immintrin.h>
#include <stdint.h>

#define shuffle8(a, b) { \
    t = (a); \
    (a) = _mm256_permute2x128_si256(t, (b), 0x20); \
    (b) = _mm256_permute2x128_si256(t, (b), 0x31); \
}

#define shuffle4(a, b) { \
    t = (a); \
    (a) = _mm256_unpacklo_epi64(t, (b)); \
    (b) = _mm256_unpackhi_epi64(t, (b)); \
}

#define shuffle2(a, b) { \
    t = (a); \
    __m256i b_dup = _mm256_castps_si256(_mm256_moveldup_ps(_mm256_castsi256_ps((b)))); \
    (a) = _mm256_blend_epi32(t, b_dup, 0xAA); \
    __m256i t_srl = _mm256_srli_epi64(t, 32); \
    (b) = _mm256_blend_epi32(t_srl, (b), 0xAA); \
}

#define shuffle1(a, b) { \
    t = (a); \
    __m256i b_sll = _mm256_slli_epi32((b), 16); \
    (a) = _mm256_blend_epi16(t, b_sll, 0xAA); \
    __m256i t_srl = _mm256_srli_epi32(t, 16); \
    (b) = _mm256_blend_epi16(t_srl, (b), 0xAA); \
}
#endif // SHUFFLE_H
