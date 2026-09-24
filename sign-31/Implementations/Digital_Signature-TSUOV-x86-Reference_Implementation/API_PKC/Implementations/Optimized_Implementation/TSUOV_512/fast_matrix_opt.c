#include "tsuov_params.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static inline int hsum_avx2_epi16(__m256i x) {
    const __m256i v_one = _mm256_set1_epi16(1);
    __m256i sum32_256 = _mm256_madd_epi16(x, v_one);

    __m128i sum32_lo = _mm256_castsi256_si128(sum32_256);
    __m128i sum32_hi = _mm256_extracti128_si256(sum32_256, 1);
    __m128i sum128 = _mm_add_epi32(sum32_lo, sum32_hi);

    __m128i hsum = _mm_hadd_epi32(sum128, sum128);
    hsum = _mm_hadd_epi32(hsum, hsum);

    return _mm_cvtsi128_si32(hsum);
}

void MATRIX_mul_MATRIX(const uint8_t *A, const uint8_t *B, uint8_t *C, int A_rows, int A_cols, int B_cols, int C_rows) {
    if(A_cols >= 291) {
        printf("error: MATRIX_mul_MATRIX may overflow! A_cols=%d\n", A_cols);
        exit(1);
    }

    int A_colsPADDED = (A_cols + 31) & ~31;
    const __m256i v_const3 = _mm256_set1_epi16(3);

    for (int i = 0; i < A_rows; i++) {
        const uint8_t *A_ptr0 = &A[i * 2 * A_colsPADDED];
        const uint8_t *A_ptr1 = &A[i * 2 * A_colsPADDED + A_colsPADDED];

        for (int j = 0; j < B_cols; j++) {
            __m256i acc00 = _mm256_setzero_si256();
            __m256i acc11 = _mm256_setzero_si256();
            __m256i acc01 = _mm256_setzero_si256();
            __m256i acc10 = _mm256_setzero_si256();

            for (int k = 0; k < A_colsPADDED; k += 32) {
                __m256i va0 = _mm256_load_si256((__m256i*)&A_ptr0[k]);
                __m256i va1 = _mm256_load_si256((__m256i*)&A_ptr1[k]);

                __m256i vb0 = _mm256_load_si256((__m256i*)&B[j*2*A_colsPADDED + k]);
                __m256i vb1 = _mm256_load_si256((__m256i*)&B[j*2*A_colsPADDED + A_colsPADDED + k]);

                acc00 = _mm256_add_epi16(acc00, _mm256_maddubs_epi16(va0, vb0));
                acc11 = _mm256_add_epi16(acc11, _mm256_maddubs_epi16(va1, vb1));
                acc01 = _mm256_add_epi16(acc01, _mm256_maddubs_epi16(va0, vb1));
                acc10 = _mm256_add_epi16(acc10, _mm256_maddubs_epi16(va1, vb0));
            }

            __m256i final0 = _mm256_add_epi16(acc00, _mm256_mullo_epi16(acc11, v_const3));
            __m256i final1 = _mm256_add_epi16(acc01, acc10);

            C[j * 2 * C_rows + i] = (uint8_t)(hsum_avx2_epi16(final0) % TSUOV_q);
            C[j * 2 * C_rows + C_rows + i] = (uint8_t)(hsum_avx2_epi16(final1) % TSUOV_q);
        }
    }
}

void VECTOR_mul_MATRIX(const uint8_t *V, const uint8_t *M, uint8_t *C, int V_cols, int M_cols, int C_cols) {
    if(V_cols >= 291) {
        printf("error: VECTOR_mul_MATRIX may overflow! V_cols=%d\n", V_cols);
        exit(1);
    }

    int V_colsPADDED = (V_cols + 31) & ~31;
    const __m256i v_const3 = _mm256_set1_epi16(3);

    const uint8_t *V_ptr0 = V;
    const uint8_t *V_ptr1 = &V[V_colsPADDED];

    for (int j = 0; j < M_cols; j++) {
        __m256i acc00 = _mm256_setzero_si256();
        __m256i acc11 = _mm256_setzero_si256();
        __m256i acc01 = _mm256_setzero_si256();
        __m256i acc10 = _mm256_setzero_si256();

        const uint8_t *M_col_j0 = &M[j * 2 * V_colsPADDED];
        const uint8_t *M_col_j1 = &M[j * 2 * V_colsPADDED + V_colsPADDED];

        for (int k = 0; k < V_colsPADDED; k += 32) {
            __m256i vv0 = _mm256_load_si256((__m256i*)&V_ptr0[k]);
            __m256i vv1 = _mm256_load_si256((__m256i*)&V_ptr1[k]);

            __m256i vm0 = _mm256_load_si256((__m256i*)&M_col_j0[k]);
            __m256i vm1 = _mm256_load_si256((__m256i*)&M_col_j1[k]);

            acc00 = _mm256_add_epi16(acc00, _mm256_maddubs_epi16(vv0, vm0));
            acc11 = _mm256_add_epi16(acc11, _mm256_maddubs_epi16(vv1, vm1));
            acc01 = _mm256_add_epi16(acc01, _mm256_maddubs_epi16(vv0, vm1));
            acc10 = _mm256_add_epi16(acc10, _mm256_maddubs_epi16(vv1, vm0));
        }

        __m256i final0 = _mm256_add_epi16(acc00, _mm256_mullo_epi16(acc11, v_const3));
        __m256i final1 = _mm256_add_epi16(acc01, acc10);

        C[j] = (uint8_t)(hsum_avx2_epi16(final0) % TSUOV_q);
        C[C_cols + j] = (uint8_t)(hsum_avx2_epi16(final1) % TSUOV_q);
    }
}

void vector_add(uint8_t *C, const uint8_t *D, int total_len) {
    int i = 0;

    if (total_len >= 32) {
        int aligned_len = total_len & ~31;
        __m256i v_q = _mm256_set1_epi8((char)TSUOV_q);
        __m256i v_q_minus_1 = _mm256_set1_epi8((char)(TSUOV_q - 1));

        for (; i < aligned_len; i += 32) {
            __m256i vc = _mm256_loadu_si256((const __m256i*)&C[i]);
            __m256i vd = _mm256_loadu_si256((const __m256i*)&D[i]);

            __m256i sum = _mm256_add_epi8(vc, vd);

            __m256i mask = _mm256_cmpgt_epi8(sum, v_q_minus_1);

            __m256i res = _mm256_sub_epi8(sum, _mm256_and_si256(mask, v_q));

            _mm256_storeu_si256((__m256i*)&C[i], res);
        }
    }

    for (; i < total_len; i++) {
        uint8_t sum = C[i] + D[i];
        if (sum >= TSUOV_q) {
            sum -= TSUOV_q;
        }
        C[i] = sum;
    }
}

uint8_t vector_dot(const uint8_t *a, const uint8_t *b, size_t length) {
    if(length >= 1165) {
        printf("error: vector_dot may overflow! length=%zu\n", length);
        exit(1);
    }
    size_t i = 0;
    size_t len_32 = length & ~((size_t)31);

    __m256i acc_16 = _mm256_setzero_si256();

    for (; i < len_32; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i*)&a[i]);
        __m256i vb = _mm256_loadu_si256((const __m256i*)&b[i]);
        __m256i prod_16 = _mm256_maddubs_epi16(va, vb);

        acc_16 = _mm256_add_epi16(acc_16, prod_16);
    }

    uint32_t total_sum = hsum_avx2_epi16(acc_16);

    for (; i < length; ++i) {
        total_sum += a[i] * b[i];
    }

    return (uint8_t)(total_sum % TSUOV_q);
}
