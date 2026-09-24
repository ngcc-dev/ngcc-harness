#include <stdio.h>
#include <immintrin.h>
#include "ntt_avx2.h"
#include "params.h"
#include "reduce.h"
#include "shuffle.h"

const int16_t zetas_avx2[128] = {
    171, 605, 688, 361, 583, 3, 250, 120, 640, 461, 223, 753, 626, 362, 432, 638, 694, 733, 76, 98, 203, 282, 430, 514, 178, 270, 199, 34, 400, 192, 620, 759, 689, 423, 645, 2, 114, 147, 715, 497, 600, 288, 161, 754, 683, 51, 405, 502, 170, 543, 648, 188, 719, 745, 307, 578, 263, 157, 523, 128, 375, 180, 389, 279, 428, 390, 202, 220, 236, 21, 212, 71, 635, 151, 23, 657, 537, 227, 717, 621, 244, 517, 532, 686, 652, 436, 703, 522, 477, 352, 624, 238, 493, 575, 495, 699, 209, 654, 670, 14, 29, 260, 391, 403, 355, 478, 358, 664, 167, 357, 528, 438, 421, 725, 691, 547, 419, 601, 611, 201, 303, 330, 585, 127, 318, 491, 416, 415
};

/**
 * @brief Cooley–Tukey butterfly for forward NTT
 */
static inline void _mm256_CTbutterfly_epi16(__m256i *a, __m256i *b, __m256i zeta_avx, __m256i q_vec, __m256i qinv_vec)
{
    __m256i c = _mm256_fqmul_epi16(*b, zeta_avx, q_vec, qinv_vec);

    *b = _mm256_sub_epi16(*a, c);
    *a = _mm256_add_epi16(*a, c);
}

void ntt_avx2(int16_t b[DTRU_N], const int16_t a[DTRU_N]) // load阶段的a改成b，接口就可以变成原本的形式
{
    uint16_t i, j;
    __m256i zeta_avx, barrett_v_vec, q_vec, qinv_vec;

    barrett_v_vec = _mm256_set1_epi16(BARRETT_V);
    q_vec = _mm256_set1_epi16(DTRU_Q);
    qinv_vec = _mm256_set1_epi16(QINV);

    __m256i ymm[32], t;

    // load
    for (j = 0; j < 32; j++) {
        ymm[j] = _mm256_lddqu_si256((__m256i const *)(a + j * 16));
    }
    
    // level 0 interval = 256
    zeta_avx = _mm256_set1_epi16(zetas_avx2[1]);
    for (j = 0; j < 16; j++) {
        _mm256_CTbutterfly_epi16(&ymm[j], &ymm[j + 16], zeta_avx, q_vec, qinv_vec);
    }

    // level 1 interval = 128
    zeta_avx = _mm256_set1_epi16(zetas_avx2[2]);
    for (j = 0; j < 8; j++) {
        _mm256_CTbutterfly_epi16(&ymm[j], &ymm[j + 8], zeta_avx, q_vec, qinv_vec);
    }
    zeta_avx = _mm256_set1_epi16(zetas_avx2[3]);
    for (j = 16; j < 24; j++) {
        _mm256_CTbutterfly_epi16(&ymm[j], &ymm[j + 8], zeta_avx, q_vec, qinv_vec);
    }

    // level 2 interval = 64
    for (i = 0; i < 4; i++) {
        zeta_avx = _mm256_set1_epi16(zetas_avx2[i + 4]);
        for (j = 0; j < 4; j++) {
        _mm256_CTbutterfly_epi16(&ymm[j + i * 8], &ymm[j + i * 8 + 4], zeta_avx, q_vec, qinv_vec);
        }
    }

    // level 3 interval = 32
    for (i = 0; i < 8; i++) {
        zeta_avx = _mm256_set1_epi16(zetas_avx2[i + 8]);
        for (j = 0; j < 2; j++) {
            _mm256_CTbutterfly_epi16(&ymm[j + i * 4], &ymm[j + i * 4 + 2], zeta_avx, q_vec, qinv_vec);
        }
    }

    // reduce
    for (j = 0; j < 32; j++) {
        ymm[j] = _mm256_barrett_epi16(ymm[j], barrett_v_vec, q_vec);
    }

    // level 4 interval = 16
    for (i = 0; i < 16; i++) {
        zeta_avx = _mm256_set1_epi16(zetas_avx2[i + 16]);
        _mm256_CTbutterfly_epi16(&ymm[i * 2], &ymm[i * 2 + 1], zeta_avx, q_vec, qinv_vec);
    }

    // // reduce
    // for (j = 0; j < 32; j++) {
    //     ymm[j] = _mm256_barrett_epi16(ymm[j], barrett_v_vec, q_vec);
    // }

    // level 5 interval = 8
    for (i = 0; i < 16; i++){
        shuffle8(ymm[i * 2], ymm[i * 2 + 1]);
        zeta_avx = _mm256_setr_epi16(\
            zetas_avx2[32 + i * 2 + 0], zetas_avx2[32 + i * 2 + 0], zetas_avx2[32 + i * 2 + 0], zetas_avx2[32 + i * 2 + 0], \
            zetas_avx2[32 + i * 2 + 0], zetas_avx2[32 + i * 2 + 0], zetas_avx2[32 + i * 2 + 0], zetas_avx2[32 + i * 2 + 0], \
            zetas_avx2[32 + i * 2 + 1], zetas_avx2[32 + i * 2 + 1], zetas_avx2[32 + i * 2 + 1], zetas_avx2[32 + i * 2 + 1], \
            zetas_avx2[32 + i * 2 + 1], zetas_avx2[32 + i * 2 + 1], zetas_avx2[32 + i * 2 + 1], zetas_avx2[32 + i * 2 + 1]);
        _mm256_CTbutterfly_epi16(&ymm[i * 2], &ymm[i * 2 + 1], zeta_avx, q_vec, qinv_vec);
    }

    // level 6 interval = 4
    for (i = 0; i < 16; i++) {
        shuffle4(ymm[i * 2], ymm[i * 2 + 1]);
        zeta_avx = _mm256_setr_epi16(\
            zetas_avx2[64 + i * 4 + 0], zetas_avx2[64 + i * 4 + 0], zetas_avx2[64 + i * 4 + 0], zetas_avx2[64 + i * 4 + 0], \
            zetas_avx2[64 + i * 4 + 1], zetas_avx2[64 + i * 4 + 1], zetas_avx2[64 + i * 4 + 1], zetas_avx2[64 + i * 4 + 1], \
            zetas_avx2[64 + i * 4 + 2], zetas_avx2[64 + i * 4 + 2], zetas_avx2[64 + i * 4 + 2], zetas_avx2[64 + i * 4 + 2], \
            zetas_avx2[64 + i * 4 + 3], zetas_avx2[64 + i * 4 + 3], zetas_avx2[64 + i * 4 + 3], zetas_avx2[64 + i * 4 + 3]);
        _mm256_CTbutterfly_epi16(&ymm[i * 2], &ymm[i * 2 + 1], zeta_avx, q_vec, qinv_vec);
    }
    
    // shuffle back && reduce
    for (j = 0; j < 16; j++) {
        shuffle4(ymm[j * 2], ymm[j * 2 + 1]);
        shuffle8(ymm[j * 2], ymm[j * 2 + 1]);
        ymm[j * 2] = _mm256_barrett_epi16(ymm[j * 2], barrett_v_vec, q_vec);
        ymm[j * 2 + 1] = _mm256_barrett_epi16(ymm[j * 2 + 1], barrett_v_vec, q_vec);
    }

    // store
    for (j = 0; j < 32; j++) {
        _mm256_storeu_si256((__m256i *)(b + j * 16), ymm[j]);
    }
}

/**
 * @brief Calculate a[x]b[y]+ a[y]b[x] 
 * 
 * @param va Array of __m256i containing a coefficients
 * @param vb Array of __m256i containing b coefficients
 * @param vd Array of __m256i containing d coefficients
 * @param x Index
 * @param y Index
 * @param q_vec Vector containing DTRU_Q
 * @param qinv_vec Vector containing QINV
 */
static inline __m256i calc_d_vec(__m256i vax, __m256i vay, __m256i vbx, __m256i vby,
                                 __m256i vdx, __m256i vdy, __m256i q_vec, __m256i qinv_vec)
{
    __m256i t1 = _mm256_add_epi16(vax, vay);
    __m256i t2 = _mm256_add_epi16(vbx, vby);
    __m256i prod = _mm256_fqmul_epi16(t1, t2, q_vec, qinv_vec);
    __m256i sumd = _mm256_add_epi16(vdx, vdy);
    return _mm256_sub_epi16(prod, sumd);
}


/**
 * @brief batch 4-D mul, 16 times in a cycle
 */

void basemul_avx2(int16_t *c, const int16_t *a, const int16_t *b, const int16_t *zeta) 
{
    __m256i q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i qinv_vec = _mm256_set1_epi16(QINV);

    // load
    __m256i va0 = _mm256_lddqu_si256((__m256i const *)(a +  0));
    __m256i va1 = _mm256_lddqu_si256((__m256i const *)(a + 16));
    __m256i va2 = _mm256_lddqu_si256((__m256i const *)(a + 32));
    __m256i va3 = _mm256_lddqu_si256((__m256i const *)(a + 48));

    __m256i vb0 = _mm256_lddqu_si256((__m256i const *)(b +  0));
    __m256i vb1 = _mm256_lddqu_si256((__m256i const *)(b + 16));
    __m256i vb2 = _mm256_lddqu_si256((__m256i const *)(b + 32));
    __m256i vb3 = _mm256_lddqu_si256((__m256i const *)(b + 48));

    __m256i zeta_vec = _mm256_lddqu_si256((__m256i const *)(zeta + 0));
    __m256i vc0, vc1, vc2, vc3;
    __m256i vd0, vd1, vd2, vd3;
    __m256i t, t1, t2;

    // shuffle
    shuffle8(va0, va2);
    shuffle8(va1, va3);
    shuffle8(vb0, vb2);
    shuffle8(vb1, vb3);

    shuffle4(va0, va1);
    shuffle4(va2, va3);
    shuffle4(vb0, vb1);
    shuffle4(vb2, vb3);

    shuffle2(va0, va2);
    shuffle2(va1, va3);
    shuffle2(vb0, vb2);
    shuffle2(vb1, vb3);

    shuffle1(va0, va1);
    shuffle1(va2, va3);
    shuffle1(vb0, vb1);
    shuffle1(vb2, vb3);

    // calculate d
    vd0 = _mm256_fqmul_epi16(va0, vb0, q_vec, qinv_vec);
    vd1 = _mm256_fqmul_epi16(va1, vb1, q_vec, qinv_vec);
    vd2 = _mm256_fqmul_epi16(va2, vb2, q_vec, qinv_vec);
    vd3 = _mm256_fqmul_epi16(va3, vb3, q_vec, qinv_vec);

    // calculate c
    // vc0 needs no extra register, vc1 needs 1, vc2 needs 1, vc3 needs 1
    vc1 = calc_d_vec(va0, va1, vb0, vb1, vd0, vd1, q_vec, qinv_vec);
    vc2 = calc_d_vec(va2, va3, vb2, vb3, vd2, vd3, q_vec, qinv_vec);
    vc2 = _mm256_fqmul_epi16(vc2, zeta_vec, q_vec, qinv_vec);
    vc1 = _mm256_add_epi16(vc1, vc2);

    vc2 = calc_d_vec(va0, va2, vb0, vb2, vd0, vd2, q_vec, qinv_vec);
    vc3 = _mm256_fqmul_epi16(vd3, zeta_vec, q_vec, qinv_vec);
    vc2 = _mm256_add_epi16(vc2, vc3);
    vc2 = _mm256_add_epi16(vc2, vd1);

    vc3 = calc_d_vec(va1, va2, vb1, vb2, vd1, vd2, q_vec, qinv_vec);
    vc0 = calc_d_vec(va0, va3, vb0, vb3, vd0, vd3, q_vec, qinv_vec);
    vc3 = _mm256_add_epi16(vc3, vc0);

    vc0 = calc_d_vec(va1, va3, vb1, vb3, vd1, vd3, q_vec, qinv_vec);
    vc0 = _mm256_add_epi16(vc0, vd2);
    vc0 = _mm256_fqmul_epi16(vc0, zeta_vec, q_vec, qinv_vec);
    vc0 = _mm256_add_epi16(vc0, vd0);

    // reduce
    __m256i barrett_v_vec = _mm256_set1_epi16(BARRETT_V);
    
    vc0  = _mm256_barrett_epi16( vc0, barrett_v_vec, q_vec);
    vc1  = _mm256_barrett_epi16( vc1, barrett_v_vec, q_vec);
    vc2  = _mm256_barrett_epi16( vc2, barrett_v_vec, q_vec);
    vc3  = _mm256_barrett_epi16( vc3, barrett_v_vec, q_vec);

    // shuffle back
    shuffle1(vc0, vc1);
    shuffle1(vc2, vc3);

    shuffle2(vc0, vc2);
    shuffle2(vc1, vc3);

    shuffle4(vc0, vc1);
    shuffle4(vc2, vc3);

    shuffle8(vc0, vc2);
    shuffle8(vc1, vc3);

    // store
    _mm256_storeu_si256((__m256i *)(c +  0), vc0);
    _mm256_storeu_si256((__m256i *)(c + 16), vc1);
    _mm256_storeu_si256((__m256i *)(c + 32), vc2);
    _mm256_storeu_si256((__m256i *)(c + 48), vc3);
}
