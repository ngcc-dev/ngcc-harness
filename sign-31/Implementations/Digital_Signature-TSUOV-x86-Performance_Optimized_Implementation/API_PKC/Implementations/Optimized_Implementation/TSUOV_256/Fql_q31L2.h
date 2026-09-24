#pragma once

#include <immintrin.h>
#include <stdint.h>

static inline void phi_t2_q31_planar_avx2(uint8_t *data, uint8_t *dst, size_t n_pairs) {
    uint8_t *a0_ptr = data;
    uint8_t *a1_ptr = data + n_pairs;

    uint8_t *a0_dst = dst;
    uint8_t *a1_dst = dst + n_pairs;

    __m256i v_q = _mm256_set1_epi8(31);
    __m256i v_q_minus_1 = _mm256_set1_epi8(30);

    for (size_t i = 0; i < n_pairs; i += 32) {
        __m256i a0 = _mm256_loadu_si256((__m256i*)&a0_ptr[i]);
        __m256i a1 = _mm256_loadu_si256((__m256i*)&a1_ptr[i]);

        __m256i new_a0 = _mm256_add_epi8(a0, a1);
        __m256i mask0 = _mm256_cmpgt_epi8(new_a0, v_q_minus_1);
        new_a0 = _mm256_sub_epi8(new_a0, _mm256_and_si256(mask0, v_q));

        __m256i x = _mm256_slli_epi16(a1, 3);

        __m256i low = _mm256_and_si256(x, v_q);
        __m256i high = _mm256_and_si256(_mm256_srli_epi16(x, 5), _mm256_set1_epi8(0x07));
        __m256i new_a1 = _mm256_add_epi8(low, high);

        __m256i mask1 = _mm256_cmpgt_epi8(new_a1, v_q_minus_1);
        new_a1 = _mm256_sub_epi8(new_a1, _mm256_and_si256(mask1, v_q));

        _mm256_storeu_si256((__m256i*)&a0_dst[i], new_a0);
        _mm256_storeu_si256((__m256i*)&a1_dst[i], new_a1);
    }
}
