#include <stdio.h>
#include "params.h"
#include "ntt_avx2.h"

const int16_t zetas_inv_avx2[128] = {
    354, 353, 278, 451, 642, 184, 439, 466, 568, 158, 168, 350, 222, 78, 44, 348, 331, 241, 412, 602, 105, 411, 291, 414, 366, 378, 509, 740, 755, 99, 115, 560, 70, 274, 194, 276, 531, 145, 417, 292, 247, 66, 333, 117, 83, 237, 252, 525, 148, 52, 542, 232, 112, 746, 618, 134, 698, 557, 748, 533, 549, 567, 379, 341, 490, 380, 589, 394, 641, 246, 612, 506, 191, 462, 24, 50, 581, 121, 226, 599, 267, 364, 718, 86, 15, 608, 481, 169, 272, 54, 622, 655, 767, 124, 346, 80, 10, 149, 577, 369, 735, 570, 499, 591, 255, 339, 487, 566, 671, 693, 36, 75, 131, 337, 407, 143, 16, 546, 308, 129, 649, 519, 766, 186, 408, 81, 164, 655
};

/**
 * @brief Gentleman–Sande butterfly for inverse NTT
 */
static inline void  _mm256_GSbutterfly_epi16(__m256i *a, __m256i *b, __m256i zeta_inv_avx, __m256i q_vec, __m256i qinv_vec)
{
    __m256i c = _mm256_sub_epi16(*a, *b);
    // *a = _mm256_barrett_epi16(_mm256_add_epi16(*a, *b), _mm256_set1_epi16(BARRETT_V), q_vec);
    *a = _mm256_add_epi16(*a, *b);
    *b = _mm256_fqmul_epi16(c, zeta_inv_avx, q_vec, qinv_vec);
}

static inline void  _mm256_GSbutterfly_rdc_epi16(__m256i *a, __m256i *b, __m256i zeta_inv_avx, __m256i q_vec, __m256i qinv_vec)
{
    __m256i c = _mm256_sub_epi16(*a, *b);
    *a = _mm256_barrett_epi16(_mm256_add_epi16(*a, *b), _mm256_set1_epi16(BARRETT_V), q_vec);
    // *a = _mm256_add_epi16(*a, *b);
    *b = _mm256_fqmul_epi16(c, zeta_inv_avx, q_vec, qinv_vec);
}


void invntt_avx2(int16_t b[DTRU_N], const int16_t a[DTRU_N])
{
    uint16_t i, j;
    __m256i zeta_avx, q_vec, qinv_vec;

    q_vec = _mm256_set1_epi16(DTRU_Q);
    qinv_vec = _mm256_set1_epi16(QINV);

    __m256i ymm[32], t;

    // load
    for (j = 0; j < 32; j++) {
        ymm[j] = _mm256_lddqu_si256((__m256i const *)(a + j * 16));
    }

    // shuffle
    for (j = 0; j < 16; j++) {
        shuffle8(ymm[j * 2], ymm[j * 2 + 1]);
        shuffle4(ymm[j * 2], ymm[j * 2 + 1]);
    }

    // level 6 interval = 4
    for (i = 0; i < 16; i++) {
        zeta_avx = _mm256_setr_epi16(\
            zetas_inv_avx2[i * 4 + 0], zetas_inv_avx2[i * 4 + 0], zetas_inv_avx2[i * 4 + 0], zetas_inv_avx2[i * 4 + 0], \
            zetas_inv_avx2[i * 4 + 1], zetas_inv_avx2[i * 4 + 1], zetas_inv_avx2[i * 4 + 1], zetas_inv_avx2[i * 4 + 1], \
            zetas_inv_avx2[i * 4 + 2], zetas_inv_avx2[i * 4 + 2], zetas_inv_avx2[i * 4 + 2], zetas_inv_avx2[i * 4 + 2], \
            zetas_inv_avx2[i * 4 + 3], zetas_inv_avx2[i * 4 + 3], zetas_inv_avx2[i * 4 + 3], zetas_inv_avx2[i * 4 + 3]);
        _mm256_GSbutterfly_rdc_epi16(&ymm[i * 2], &ymm[i * 2 + 1], zeta_avx, q_vec, qinv_vec);
        shuffle4(ymm[i * 2], ymm[i * 2 + 1]);
    }

    // level 5 interval = 8
    for (i = 0; i < 16; i++){
        zeta_avx = _mm256_setr_epi16(\
            zetas_inv_avx2[64 + i * 2 + 0], zetas_inv_avx2[64 + i * 2 + 0], zetas_inv_avx2[64 + i * 2 + 0], zetas_inv_avx2[64 + i * 2 + 0], \
            zetas_inv_avx2[64 + i * 2 + 0], zetas_inv_avx2[64 + i * 2 + 0], zetas_inv_avx2[64 + i * 2 + 0], zetas_inv_avx2[64 + i * 2 + 0], \
            zetas_inv_avx2[64 + i * 2 + 1], zetas_inv_avx2[64 + i * 2 + 1], zetas_inv_avx2[64 + i * 2 + 1], zetas_inv_avx2[64 + i * 2 + 1], \
            zetas_inv_avx2[64 + i * 2 + 1], zetas_inv_avx2[64 + i * 2 + 1], zetas_inv_avx2[64 + i * 2 + 1], zetas_inv_avx2[64 + i * 2 + 1]);
        _mm256_GSbutterfly_rdc_epi16(&ymm[i * 2], &ymm[i * 2 + 1], zeta_avx, q_vec, qinv_vec);
        shuffle8(ymm[i * 2], ymm[i * 2 + 1]);
    }

    // level 4 interval = 16
    for (i = 0; i < 16; i++) {
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[i + 96]);
        _mm256_GSbutterfly_rdc_epi16(&ymm[i * 2], &ymm[i * 2 + 1], zeta_avx, q_vec, qinv_vec);
    }

    // level 3 interval = 32
    for (i = 0; i < 8; i++) {
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[i + 112]);
        for (j = 0; j < 2; j++) {
            _mm256_GSbutterfly_rdc_epi16(&ymm[j + i * 4], &ymm[j + i * 4 + 2], zeta_avx, q_vec, qinv_vec);
        }
    }

    // level 2 interval = 64
    for (i = 0; i < 4; i++) {
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[i + 120]);
        for (j = 0; j < 4; j++) {
        _mm256_GSbutterfly_rdc_epi16(&ymm[j + i * 8], &ymm[j + i * 8 + 4], zeta_avx, q_vec, qinv_vec);
        }
    }

    // level 1 interval = 128
    zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[124]);
    for (j = 0; j < 8; j++) {
        _mm256_GSbutterfly_rdc_epi16(&ymm[j], &ymm[j + 8], zeta_avx, q_vec, qinv_vec);
    }
    zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[125]);
    for (j = 16; j < 24; j++) {
        _mm256_GSbutterfly_rdc_epi16(&ymm[j], &ymm[j + 8], zeta_avx, q_vec, qinv_vec);
    }

    // level 0 interval = 256
    zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[126]);
    for (j = 0; j < 16; j++) {
        _mm256_GSbutterfly_rdc_epi16(&ymm[j], &ymm[j + 16], zeta_avx, q_vec, qinv_vec);
    }

    // reduce
    zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[127]);
    for (j = 0; j < 32; j++) {
        ymm[j] = _mm256_fqmul_epi16(ymm[j], zeta_avx, q_vec, qinv_vec);
        // ymm[j] = _mm256_barrett_epi16(ymm[j], barrett_v_vec, q_vec);
    }

    // store
    for (j = 0; j < 32; j++) {
        _mm256_storeu_si256((__m256i *)(b + j * 16), ymm[j]);
    }

}



