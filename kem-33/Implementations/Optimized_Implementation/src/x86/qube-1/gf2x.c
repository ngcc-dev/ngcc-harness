/**
 * \file gf2x.c
 * \brief AVX2 implementation of multiplication of two polynomials
 */

#include "gf2x.h"
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include "parameters.h"
#include <stdio.h>

#define VEC_N_ARRAY_SIZE_VEC CEIL_DIVIDE(PARAM_N_MULT, 256) /*!< The number of needed vectors to store PARAM_N bits*/
#define WORD                 64
#define LAST64               (PARAM_N >> 6)

#define T_TM3R_3W_256 ((PARAM_N_MULT/3) / (4 * WORD))
#define T_TM3R_3W_64  (T_TM3R_3W_256 << 2)

__m256i a1_times_a2[VEC_N_256_SIZE_64 >> 1];
__m256i o256[VEC_N_ARRAY_SIZE_VEC];
uint64_t *tmp_reduce = (uint64_t *)o256;

static inline void reduce(__m256i *o, const __m256i *a);
static inline void karat_mult_1(__m128i *C, __m128i *A, __m128i *B);
static inline void karat_mult_2(__m256i *C, __m256i *A, __m256i *B);
static inline void karat_mult_4(__m256i *C, __m256i *A, __m256i *B);
static inline void karat_mult_8(__m256i *C, __m256i *A, __m256i *B);
static inline void karat_mult_16(__m256i *C, __m256i *A, __m256i *B);
static inline void karat_mult_32(__m256i *C, __m256i *A, __m256i *B);
static inline void karat_mult_64(__m256i *C, __m256i *A, __m256i *B);
static inline void divide_by_x_plus_one_256(__m256i *out, __m256i *in, int32_t size);

/**
 * @brief Compute o(x) = a(x) mod \f$ X^n - 1\f$
 *
 * This function computes the modular reduction of the polynomial a(x)
 *
 * @param[out] o Pointer to the result
 * @param[in] a256 Pointer to the polynomial a(x)
 */
static inline void reduce(__m256i *o, const __m256i *a256) {
    __m256i r256, carry256;
    uint64_t *a = (uint64_t *)a256;
    static const int32_t dec64 = PARAM_N & 0x3f;
    static int32_t d0;
    int32_t i, i2;

    d0 = WORD - dec64;
    for (i = LAST64; i < (PARAM_N >> 5) - 4; i += 4) {
        r256 = _mm256_lddqu_si256((__m256i const *)(&a[i]));
        r256 = _mm256_srli_epi64(r256, dec64);
        carry256 = _mm256_lddqu_si256((__m256i const *)(&a[i + 1]));
        carry256 = _mm256_slli_epi64(carry256, d0);
        r256 ^= carry256;
        i2 = (i - LAST64) >> 2;
        o256[i2] = a256[i2] ^ r256;
    }

    i = i - LAST64;

    for (; i < LAST64 + 1; i++) {
        uint64_t r = a[i + LAST64] >> dec64;
        uint64_t carry = a[i + LAST64 + 1] << d0;
        r ^= carry;
        tmp_reduce[i] = a[i] ^ r;
    }

    tmp_reduce[LAST64] &= BITMASK(PARAM_N, 64);
    memcpy(o, tmp_reduce, VEC_N_SIZE_BYTES);
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 * A(x) and B(x) are stored in 128-bit registers
 * This function computes A(x)*B(x) using Karatsuba
 *
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_1(__m128i *C, __m128i *A, __m128i *B) {
    __m128i D1[2];
    __m128i D0[2], D2[2];
    __m128i Al = _mm_loadu_si128(A);
    __m128i Ah = _mm_loadu_si128(A + 1);
    __m128i Bl = _mm_loadu_si128(B);
    __m128i Bh = _mm_loadu_si128(B + 1);

    //	Compute Al.Bl=D0
    __m128i DD0 = _mm_clmulepi64_si128(Al, Bl, 0);
    __m128i DD2 = _mm_clmulepi64_si128(Al, Bl, 0x11);
    __m128i AAlpAAh = _mm_xor_si128(Al, _mm_shuffle_epi32(Al, 0x4e));
    __m128i BBlpBBh = _mm_xor_si128(Bl, _mm_shuffle_epi32(Bl, 0x4e));
    __m128i DD1 = _mm_xor_si128(_mm_xor_si128(DD0, DD2), _mm_clmulepi64_si128(AAlpAAh, BBlpBBh, 0));
    D0[0] = _mm_xor_si128(DD0, _mm_unpacklo_epi64(_mm_setzero_si128(), DD1));
    D0[1] = _mm_xor_si128(DD2, _mm_unpackhi_epi64(DD1, _mm_setzero_si128()));

    //	Compute Ah.Bh=D2
    DD0 = _mm_clmulepi64_si128(Ah, Bh, 0);
    DD2 = _mm_clmulepi64_si128(Ah, Bh, 0x11);
    AAlpAAh = _mm_xor_si128(Ah, _mm_shuffle_epi32(Ah, 0x4e));
    BBlpBBh = _mm_xor_si128(Bh, _mm_shuffle_epi32(Bh, 0x4e));
    DD1 = _mm_xor_si128(_mm_xor_si128(DD0, DD2), _mm_clmulepi64_si128(AAlpAAh, BBlpBBh, 0));
    D2[0] = _mm_xor_si128(DD0, _mm_unpacklo_epi64(_mm_setzero_si128(), DD1));
    D2[1] = _mm_xor_si128(DD2, _mm_unpackhi_epi64(DD1, _mm_setzero_si128()));

    // Compute AlpAh.BlpBh=D1
    // Initialisation of AlpAh and BlpBh
    __m128i AlpAh = _mm_xor_si128(Al, Ah);
    __m128i BlpBh = _mm_xor_si128(Bl, Bh);
    DD0 = _mm_clmulepi64_si128(AlpAh, BlpBh, 0);
    DD2 = _mm_clmulepi64_si128(AlpAh, BlpBh, 0x11);
    AAlpAAh = _mm_xor_si128(AlpAh, _mm_shuffle_epi32(AlpAh, 0x4e));
    BBlpBBh = _mm_xor_si128(BlpBh, _mm_shuffle_epi32(BlpBh, 0x4e));
    DD1 = _mm_xor_si128(_mm_xor_si128(DD0, DD2), _mm_clmulepi64_si128(AAlpAAh, BBlpBBh, 0));
    D1[0] = _mm_xor_si128(DD0, _mm_unpacklo_epi64(_mm_setzero_si128(), DD1));
    D1[1] = _mm_xor_si128(DD2, _mm_unpackhi_epi64(DD1, _mm_setzero_si128()));

    // Final comutation of C
    __m128i middle = _mm_xor_si128(D0[1], D2[0]);
    C[0] = D0[0];
    C[1] = middle ^ D0[0] ^ D1[0];
    C[2] = middle ^ D1[1] ^ D2[1];
    C[3] = D2[1];
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_2(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[2], D1[2], D2[2], SAA, SBB;
    __m128i *A128 = (__m128i *)A, *B128 = (__m128i *)B;

    karat_mult_1((__m128i *)D0, A128, B128);
    karat_mult_1((__m128i *)D2, A128 + 2, B128 + 2);

    SAA = _mm256_xor_si256(A[0], A[1]);
    SBB = _mm256_xor_si256(B[0], B[1]);

    karat_mult_1((__m128i *)D1, (__m128i *)&SAA, (__m128i *)&SBB);
    __m256i middle = _mm256_xor_si256(D0[1], D2[0]);

    C[0] = D0[0];
    C[1] = middle ^ D0[0] ^ D1[0];
    C[2] = middle ^ D1[1] ^ D2[1];
    C[3] = D2[1];
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_4(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[4], D1[4], D2[4], SAA[2], SBB[2];

    karat_mult_2(D0, A, B);
    karat_mult_2(D2, A + 2, B + 2);

    SAA[0] = A[0] ^ A[2];
    SBB[0] = B[0] ^ B[2];
    SAA[1] = A[1] ^ A[3];
    SBB[1] = B[1] ^ B[3];

    karat_mult_2(D1, SAA, SBB);

    __m256i middle0 = _mm256_xor_si256(D0[2], D2[0]);
    __m256i middle1 = _mm256_xor_si256(D0[3], D2[1]);

    C[0] = D0[0];
    C[1] = D0[1];
    C[2] = middle0 ^ D0[0] ^ D1[0];
    C[3] = middle1 ^ D0[1] ^ D1[1];
    C[4] = middle0 ^ D1[2] ^ D2[2];
    C[5] = middle1 ^ D1[3] ^ D2[3];
    C[6] = D2[2];
    C[7] = D2[3];
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_8(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[8], D1[8], D2[8], SAA[4], SBB[4];

    karat_mult_4(D0, A, B);
    karat_mult_4(D2, A + 4, B + 4);

    for (int32_t i = 0; i < 4; i++) {
        int32_t is = i + 4;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_4(D1, SAA, SBB);

    for (int32_t i = 0; i < 4; i++) {
        int32_t is = i + 4;
        int32_t is2 = is + 4;
        int32_t is3 = is2 + 4;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_16(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[16], D1[16], D2[16], SAA[8], SBB[8];

    karat_mult_8(D0, A, B);
    karat_mult_8(D2, A + 8, B + 8);

    for (int32_t i = 0; i < 8; i++) {
        int32_t is = i + 8;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_8(D1, SAA, SBB);

    for (int32_t i = 0; i < 8; i++) {
        int32_t is = i + 8;
        int32_t is2 = is + 8;
        int32_t is3 = is2 + 8;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_32(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[32], D1[32], D2[32], SAA[16], SBB[16];

    karat_mult_16(D0, A, B);
    karat_mult_16(D2, A + 16, B + 16);

    for (int32_t i = 0; i < 16; i++) {
        int32_t is = i + 16;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_16(D1, SAA, SBB);

    for (int32_t i = 0; i < 16; i++) {
        int32_t is = i + 16;
        int32_t is2 = is + 16;
        int32_t is3 = is2 + 16;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}


/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_64(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[64], D1[64], D2[64], SAA[32], SBB[32];

    karat_mult_32(D0, A, B);
    karat_mult_32(D2, A + 32, B + 32);

    for (int32_t i = 0; i < 32; i++) {
        int32_t is = i + 32;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_32(D1, SAA, SBB);

    for (int32_t i = 0; i < 32; i++) {
        int32_t is = i + 32;
        int32_t is2 = is + 32;
        int32_t is3 = is2 + 32;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}



/**
 * @brief Compute B(x) = A(x)/(x+1)
 *
 * This function computes A(x)/(x+1) using a Quercia like algorithm
 * @param[out] out Pointer to the result
 * @param[in] in Pointer to the polynomial A(x)
 * @param[in] size used to define the number of coeeficients of A
 */
static inline void divide_by_x_plus_one_256(__m256i *out, __m256i *in, int32_t size) {
    uint64_t *A = (uint64_t *)in;
    uint64_t *B = (uint64_t *)out;

    B[0] = A[0];
    for (int32_t i = 1; i < 2 * (size << 2); i++) {
        B[i] = B[i - 1] ^ A[i];
    }
}

/**
 * @brief Multiply two polynomials modulo \f$ X^n - 1\f$.
 *
 * This functions multiplies a dense polynomial <b>a1</b> (of Hamming weight equal to <b>weight</b>)
 * and a dense polynomial <b>a2</b>. The multiplication is done modulo \f$ X^n - 1\f$.
 *
 * @param[out] o Pointer to the result
 * @param[in] a1 Pointer to a polynomial
 * @param[in] a2 Pointer to a polynomial
 */
void vect_mul(__m256i *o, const __m256i *a1, const __m256i *a2) {
    karat_mult_64(a1_times_a2, a1, a2);
    reduce(o, a1_times_a2);
}


void ring_mul( uint8_t *c, const uint8_t *a, const uint8_t *b )
{
    ;
}
