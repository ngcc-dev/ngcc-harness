#include "shuffle.h"

/**
 * @brief shuffle 8 out of every 16 params
 * 
 * @param[in] a  0-15
 * @param[in] b 16-31
 * @param[out] a  0- 7 48-55
 * @param[out] b  8-15 56-63
 */
static inline void _mm256_shuffle8_epi16(__m256i *a, __m256i *b) {
    __m256i c = _mm256_permute2x128_si256(*a, *b, 0x20);

    *b = _mm256_permute2x128_si256(*a, *b, 0x31); // hi hi  8-15 56-63
    *a = c;                                       // lo lo  0- 7 48-55
}

/**
 * @brief shuffle 4 out of every 8 params
 * 
 * @param[in] a  0- 7 48-55
 * @param[in] b 24-31 72-79
 * @param[out] a  0  1  2  3 24 25 26 27 48 49 50 51 72 73 74 75
 * @param[out] b  4  5  6  7 28 29 30 31 52 53 54 55 76 77 78 79
 */
static inline void _mm256_shuffle4_epi16(__m256i *a, __m256i *b) {
    __m256i c = _mm256_unpacklo_epi64(*a, *b);
    *b = _mm256_unpackhi_epi64(*a, *b);
    *a = c;
}

/**
 * @brief shuffle 2 out of every 4 params
 * 
 * @param[in] a  0  1  2  3 24 25 26 27 48 49 50 51 72 73 74 75
 * @param[in] b  8  9 10 11 32 33 34 35 56 57 58 59 80 81 82 83
 * @param[out] a  0  1  8  9 24 25 32 33 48 49 56 57 72 73 80 81
 * @param[out] b  2  3 10 11 26 27 34 35 50 51 58 59 74 75 82 83
 */
static inline void _mm256_shuffle2_epi16(__m256i *a, __m256i *b)
{
    __m256i b_shift = _mm256_slli_epi64(*b, 32);         // vpsllq $32,rh1 -> rh2
    __m256i a_shift = _mm256_srli_epi64(*a, 32); // vpsrlq $32,rh0 -> rh0
    *a = _mm256_blend_epi32(*a, b_shift, 0xAA);          // vpblendd $0xAA,rh2,rh0 -> rh2
    *b = _mm256_blend_epi32(a_shift, *b, 0xAA);  // vpblendd $0xAA,rh1,rh0 -> rh3
}

/**
 * @brief shuffle 1 out of every 2 params
 * 
 * @param[in] a  0  1  8  9 24 25 32 33 48 49 56 57 72 73 80 81
 * @param[in] b  4  5 12 13 28 29 36 37 52 53 60 61 76 77 84 85
 * @param[out] a  0  4  8 12 24 28 32 36 48 52 56 60 72 76 80 84
 * @param[out] b  1  5  9 13 25 29 33 37 49 53 57 61 73 77 81 85
 */
static inline void _mm256_shuffle1_epi16(__m256i *a, __m256i *b)
{
    __m256i b_shift = _mm256_slli_epi32(*b, 16);         // vpslld $16,rh1 -> rh2
    __m256i a_shift = _mm256_srli_epi32(*a, 16); // vpsrld $16,rh0 -> rh0
    *a = _mm256_blend_epi16(*a, b_shift, 0xAA);          // vpblendw $0xAA,rh2,rh0 -> rh2
    *b = _mm256_blend_epi16(a_shift, *b, 0xAA);  // vpblendw $0xAA,rh1,rh0 -> rh3
}

void shuffle_to_basemul16x16(__m256i input[16])
{
    _mm256_shuffle8_epi16(&input[ 0], &input[ 8]);
    _mm256_shuffle8_epi16(&input[ 1], &input[ 9]);
    _mm256_shuffle8_epi16(&input[ 2], &input[10]);
    _mm256_shuffle8_epi16(&input[ 3], &input[11]);
    _mm256_shuffle8_epi16(&input[ 4], &input[12]);
    _mm256_shuffle8_epi16(&input[ 5], &input[13]);
    _mm256_shuffle8_epi16(&input[ 6], &input[14]);
    _mm256_shuffle8_epi16(&input[ 7], &input[15]);

    _mm256_shuffle4_epi16(&input[ 0], &input[ 4]);
    _mm256_shuffle4_epi16(&input[ 1], &input[ 5]);
    _mm256_shuffle4_epi16(&input[ 2], &input[ 6]);
    _mm256_shuffle4_epi16(&input[ 3], &input[ 7]);
    _mm256_shuffle4_epi16(&input[ 8], &input[12]);
    _mm256_shuffle4_epi16(&input[ 9], &input[13]);
    _mm256_shuffle4_epi16(&input[10], &input[14]);
    _mm256_shuffle4_epi16(&input[11], &input[15]);

    _mm256_shuffle2_epi16(&input[ 0], &input[ 2]);
    _mm256_shuffle2_epi16(&input[ 1], &input[ 3]);
    _mm256_shuffle2_epi16(&input[ 4], &input[ 6]);
    _mm256_shuffle2_epi16(&input[ 5], &input[ 7]);
    _mm256_shuffle2_epi16(&input[ 8], &input[10]);
    _mm256_shuffle2_epi16(&input[ 9], &input[11]);
    _mm256_shuffle2_epi16(&input[12], &input[14]);
    _mm256_shuffle2_epi16(&input[13], &input[15]);

    _mm256_shuffle1_epi16(&input[ 0], &input[ 1]);
    _mm256_shuffle1_epi16(&input[ 2], &input[ 3]);
    _mm256_shuffle1_epi16(&input[ 4], &input[ 5]);
    _mm256_shuffle1_epi16(&input[ 6], &input[ 7]);
    _mm256_shuffle1_epi16(&input[ 8], &input[ 9]);
    _mm256_shuffle1_epi16(&input[10], &input[11]);
    _mm256_shuffle1_epi16(&input[12], &input[13]);
    _mm256_shuffle1_epi16(&input[14], &input[15]);
}
/**
与shuffle_to_basemul16x16相反
*/
void shuffle_from_basemul16x16(__m256i input[16])
{
    _mm256_shuffle1_epi16(&input[ 0], &input[ 1]);
    _mm256_shuffle1_epi16(&input[ 2], &input[ 3]);
    _mm256_shuffle1_epi16(&input[ 4], &input[ 5]);
    _mm256_shuffle1_epi16(&input[ 6], &input[ 7]);
    _mm256_shuffle1_epi16(&input[ 8], &input[ 9]);
    _mm256_shuffle1_epi16(&input[10], &input[11]);
    _mm256_shuffle1_epi16(&input[12], &input[13]);
    _mm256_shuffle1_epi16(&input[14], &input[15]);

    _mm256_shuffle2_epi16(&input[ 0], &input[ 2]);
    _mm256_shuffle2_epi16(&input[ 1], &input[ 3]);
    _mm256_shuffle2_epi16(&input[ 4], &input[ 6]);
    _mm256_shuffle2_epi16(&input[ 5], &input[ 7]);
    _mm256_shuffle2_epi16(&input[ 8], &input[10]);
    _mm256_shuffle2_epi16(&input[ 9], &input[11]);
    _mm256_shuffle2_epi16(&input[12], &input[14]);
    _mm256_shuffle2_epi16(&input[13], &input[15]);

    _mm256_shuffle4_epi16(&input[ 0], &input[ 4]);
    _mm256_shuffle4_epi16(&input[ 1], &input[ 5]);
    _mm256_shuffle4_epi16(&input[ 2], &input[ 6]);
    _mm256_shuffle4_epi16(&input[ 3], &input[ 7]);
    _mm256_shuffle4_epi16(&input[ 8], &input[12]);
    _mm256_shuffle4_epi16(&input[ 9], &input[13]);
    _mm256_shuffle4_epi16(&input[10], &input[14]);
    _mm256_shuffle4_epi16(&input[11], &input[15]);

    _mm256_shuffle8_epi16(&input[ 0], &input[ 8]);
    _mm256_shuffle8_epi16(&input[ 1], &input[ 9]);
    _mm256_shuffle8_epi16(&input[ 2], &input[10]);
    _mm256_shuffle8_epi16(&input[ 3], &input[11]);
    _mm256_shuffle8_epi16(&input[ 4], &input[12]);
    _mm256_shuffle8_epi16(&input[ 5], &input[13]);
    _mm256_shuffle8_epi16(&input[ 6], &input[14]);
    _mm256_shuffle8_epi16(&input[ 7], &input[15]);
}
