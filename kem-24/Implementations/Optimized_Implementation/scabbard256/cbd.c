#include <stdint.h>
#include <stddef.h>
#include "cbd.h"
#include <immintrin.h>

/// @brief Loads 4 bytes into a 32-bit unsigned integer in little-endian order.
/// @param[in] x byte array
/// @return 32-bit unsigned integer, loaded from x (most significant byte is set to 0)

void cbd(poly *r, const uint8_t buf[SCABBARD_CBD_POLYBYTES])
{
    unsigned int coeff_idx = 0;
    unsigned int buf_idx = 0;

    __m256i mask0F = _mm256_set1_epi8(0x0F);
    __m256i mask55 = _mm256_set1_epi8(0x55);
    __m256i mask03 = _mm256_set1_epi8(0x03);

    // Process 64 coefficients (32 bytes) per iteration
    for (; coeff_idx + 64 <= SCABBARD_N; coeff_idx += 64, buf_idx += 32) {
        __m256i t = _mm256_loadu_si256((const __m256i*)&buf[buf_idx]);
        __m256i t_lo = _mm256_and_si256(t, mask0F);
        __m256i t_hi = _mm256_and_si256(_mm256_srli_epi16(t, 4), mask0F);
        __m256i d_lo = _mm256_add_epi8(_mm256_and_si256(t_lo, mask55), _mm256_and_si256(_mm256_srli_epi16(t_lo, 1), mask55));
        __m256i d_hi = _mm256_add_epi8(_mm256_and_si256(t_hi, mask55),_mm256_and_si256(_mm256_srli_epi16(t_hi, 1), mask55));

        __m256i a_lo = _mm256_and_si256(d_lo, mask03);
        __m256i b_lo = _mm256_and_si256(_mm256_srli_epi16(d_lo, 2), mask03);
        __m256i coeff_lo = _mm256_sub_epi8(a_lo, b_lo); // 32 even coefficients

        __m256i a_hi = _mm256_and_si256(d_hi, mask03);
        __m256i b_hi = _mm256_and_si256(_mm256_srli_epi16(d_hi, 2), mask03);
        __m256i coeff_hi = _mm256_sub_epi8(a_hi, b_hi); // 32 odd coefficients

        __m256i R0 = _mm256_unpacklo_epi8(coeff_lo, coeff_hi);
        __m256i R1 = _mm256_unpackhi_epi8(coeff_lo, coeff_hi);

        __m128i R0_lo = _mm256_castsi256_si128(R0);      // Coeffs 0..15
        __m128i R0_hi = _mm256_extracti128_si256(R0, 1); // Coeffs 32..47
        __m128i R1_lo = _mm256_castsi256_si128(R1);      // Coeffs 16..31
        __m128i R1_hi = _mm256_extracti128_si256(R1, 1); // Coeffs 48..63

        _mm256_storeu_si256((__m256i*)&r->coeffs[coeff_idx + 0],  _mm256_cvtepi8_epi16(R0_lo));
        _mm256_storeu_si256((__m256i*)&r->coeffs[coeff_idx + 16], _mm256_cvtepi8_epi16(R1_lo));
        _mm256_storeu_si256((__m256i*)&r->coeffs[coeff_idx + 32], _mm256_cvtepi8_epi16(R0_hi));
        _mm256_storeu_si256((__m256i*)&r->coeffs[coeff_idx + 48], _mm256_cvtepi8_epi16(R1_hi));
    }
}

