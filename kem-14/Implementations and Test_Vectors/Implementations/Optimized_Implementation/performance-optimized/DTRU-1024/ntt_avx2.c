#include <stdio.h>
#include "params.h"
#include "reduce.h"
#include "ntt_avx2.h"

const int16_t zetas_avx2[128] = {
    3310,  886, 1520, 1571,  704,  624,  813, 1742,  594, 2255, 2260, 2946,  137,  200, 2500,   16, 
    2682,  963, 3395, 1045, 2987, 2569, 2728, 2418, 1548,  115, 3166, 1392, 1028, 1854, 2433,  978, 
    2585, 1427, 2281,  529,  341, 2895, 3346,  923,  975, 2357,   78, 3369, 2041,    2,   25,  415, 
     631,  795, 1295,  755, 3349, 3047, 1789, 1350, 2692, 1129, 2013,  920, 2547, 2179, 1310, 1004, 
    1987, 3254, 2648, 1090, 2454, 2018, 1026,  438, 1826, 3347, 2082, 1374, 2624, 1383, 1731, 1770, 
    2825, 1954,  226,  986,  152,  449,  427, 1557, 2229, 1740, 1008, 1522, 2177, 2951,  589, 2172, 
    2621, 2716, 2837,   79, 2214, 1491, 3081, 3438,  126, 2783, 1946, 1882, 1370, 2000,  801,  160, 
    2853, 1036, 2579,  636, 2377, 2814,  605, 3129, 2721,  919, 2845, 2286, 1271, 1048, 2729, 3126
};

/**
 * @brief Cooley–Tukey butterfly for forward NTT
 */
static inline void _mm256_CTbutterfly_epi16(__m256i *a, __m256i *b, __m256i zeta_vec, __m256i q_vec, __m256i qinv_vec)
{
    __m256i c = _mm256_fqmul_epi16(*b, zeta_vec, q_vec, qinv_vec);

    *b = _mm256_sub_epi16(*a, c);
    *a = _mm256_add_epi16(*a, c);
}

void ntt_avx2(int16_t b[DTRU_N], const int16_t a[DTRU_N]) // load阶段的a改成b，接口就可以变成原本的形式
{
    uint16_t j;
    __m256i zeta_vec, barrett_v_vec, q_vec, qinv_vec;

    barrett_v_vec = _mm256_set1_epi16(BARRETT_V);
    q_vec = _mm256_set1_epi16(DTRU_Q);
    qinv_vec = _mm256_set1_epi16(QINV);

    // level 0-2
    for (j = 0; j < 8; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7, f8;

        // load
        f0 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 0 / 8 + j * 16));
        f1 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 1 / 8 + j * 16));
        f2 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 2 / 8 + j * 16));
        f3 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 3 / 8 + j * 16));

        f4 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 4 / 8 + j * 16));
        f5 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 5 / 8 + j * 16));
        f6 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 6 / 8 + j * 16));
        f7 = _mm256_lddqu_si256((__m256i const *)(a + DTRU_N * 7 / 8 + j * 16));

        // level 0
        zeta_vec = _mm256_set1_epi16(zetas_avx2[1]);

        f8 = _mm256_fqmul_epi16(f4, zeta_vec, q_vec, qinv_vec); // barrett(b[j] * zetaqinv)
        f4 = _mm256_add_epi16(f0, f4); // b[j] + b[j + 512] - t
        f4 = _mm256_sub_epi16(f4, f8);
        f0 = _mm256_add_epi16(f0, f8); // b[j] + t
        
        f8 = _mm256_fqmul_epi16(f5, zeta_vec, q_vec, qinv_vec);   // barrett(b[j + 512] * zeta)
        f5 = _mm256_add_epi16(f1, f5); // b[j] + b[j + 512] - t
        f5 = _mm256_sub_epi16(f5, f8);
        f1 = _mm256_add_epi16(f1, f8); // b[j] + t

        f8 = _mm256_fqmul_epi16(f6, zeta_vec, q_vec, qinv_vec);   // barrett(b[j + 512] * zeta)
        f6 = _mm256_add_epi16(f2, f6); // b[j] + b[j + 512] - t
        f6 = _mm256_sub_epi16(f6, f8);
        f2 = _mm256_add_epi16(f2, f8); // b[j] + t

        f8 = _mm256_fqmul_epi16(f7, zeta_vec, q_vec, qinv_vec);   // barrett(b[j + 512] * zeta)
        f7 = _mm256_add_epi16(f3, f7); // b[j] + b[j + 512] - t
        f7 = _mm256_sub_epi16(f7, f8);
        f3 = _mm256_add_epi16(f3, f8); // b[j] + t

        // level 1 interval = 256
        zeta_vec = _mm256_set1_epi16(zetas_avx2[2]);
        _mm256_CTbutterfly_epi16(&f0, &f2, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f1, &f3, zeta_vec, q_vec, qinv_vec);
        
        zeta_vec = _mm256_set1_epi16(zetas_avx2[3]);
        _mm256_CTbutterfly_epi16(&f4, &f6, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f5, &f7, zeta_vec, q_vec, qinv_vec);

        // level 2 interval = 128
        zeta_vec = _mm256_set1_epi16(zetas_avx2[4]);
        _mm256_CTbutterfly_epi16(&f0, &f1, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[5]);
        _mm256_CTbutterfly_epi16(&f2, &f3, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[6]);
        _mm256_CTbutterfly_epi16(&f4, &f5, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[7]);
        _mm256_CTbutterfly_epi16(&f6, &f7, zeta_vec, q_vec, qinv_vec);

        // reduce
        f0 = _mm256_barrett_epi16(f0, barrett_v_vec, q_vec);
        f1 = _mm256_barrett_epi16(f1, barrett_v_vec, q_vec);
        f2 = _mm256_barrett_epi16(f2, barrett_v_vec, q_vec);
        f3 = _mm256_barrett_epi16(f3, barrett_v_vec, q_vec);
        
        f4 = _mm256_barrett_epi16(f4, barrett_v_vec, q_vec);
        f5 = _mm256_barrett_epi16(f5, barrett_v_vec, q_vec);
        f6 = _mm256_barrett_epi16(f6, barrett_v_vec, q_vec);
        f7 = _mm256_barrett_epi16(f7, barrett_v_vec, q_vec);

        // store
        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 0 / 8 + j * 16), f0);
        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 1 / 8 + j * 16), f1);
        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 2 / 8 + j * 16), f2);
        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 3 / 8 + j * 16), f3);

        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 4 / 8 + j * 16), f4);
        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 5 / 8 + j * 16), f5);
        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 6 / 8 + j * 16), f6);
        _mm256_storeu_si256((__m256i *)(b + DTRU_N * 7 / 8 + j * 16), f7);
    }

    // level 3-6
    for (j = 0; j < 8; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7;

        // load
        f0  = _mm256_lddqu_si256((__m256i const *)(b +   0 + j * 128));
        f1  = _mm256_lddqu_si256((__m256i const *)(b +  16 + j * 128));
        f2  = _mm256_lddqu_si256((__m256i const *)(b +  32 + j * 128));
        f3  = _mm256_lddqu_si256((__m256i const *)(b +  48 + j * 128));

        f4  = _mm256_lddqu_si256((__m256i const *)(b +  64 + j * 128));
        f5  = _mm256_lddqu_si256((__m256i const *)(b +  80 + j * 128));
        f6  = _mm256_lddqu_si256((__m256i const *)(b +  96 + j * 128));
        f7  = _mm256_lddqu_si256((__m256i const *)(b + 112 + j * 128));

        // level 3 interval = 64
        zeta_vec = _mm256_set1_epi16(zetas_avx2[8 + j]);
        _mm256_CTbutterfly_epi16(&f0, &f4, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f1, &f5, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f2, &f6, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f3, &f7, zeta_vec, q_vec, qinv_vec);

        // level 4 interval = 32
        zeta_vec = _mm256_set1_epi16(zetas_avx2[16 + j * 2 + 0]);
        _mm256_CTbutterfly_epi16(&f0, &f2, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f1, &f3, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[16 + j * 2 + 1]);
        _mm256_CTbutterfly_epi16(&f4, &f6, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f5, &f7, zeta_vec, q_vec, qinv_vec);

        // reduce
        f0 = _mm256_barrett_epi16(f0, barrett_v_vec, q_vec);
        f1 = _mm256_barrett_epi16(f1, barrett_v_vec, q_vec);
        f2 = _mm256_barrett_epi16(f2, barrett_v_vec, q_vec);
        f3 = _mm256_barrett_epi16(f3, barrett_v_vec, q_vec);

        f4 = _mm256_barrett_epi16(f4, barrett_v_vec, q_vec);
        f5 = _mm256_barrett_epi16(f5, barrett_v_vec, q_vec);
        f6 = _mm256_barrett_epi16(f6, barrett_v_vec, q_vec);
        f7 = _mm256_barrett_epi16(f7, barrett_v_vec, q_vec);

        // level 5 interval = 16
        zeta_vec = _mm256_set1_epi16(zetas_avx2[32 + j * 4 + 0]);
        _mm256_CTbutterfly_epi16(&f0, &f1, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[32 + j * 4 + 1]);
        _mm256_CTbutterfly_epi16(&f2, &f3, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[32 + j * 4 + 2]);
        _mm256_CTbutterfly_epi16(&f4, &f5, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[32 + j * 4 + 3]);
        _mm256_CTbutterfly_epi16(&f6, &f7, zeta_vec, q_vec, qinv_vec);

        // shuffle
        _mm256_shuffle8_epi16(&f0, &f1); // 0- 7 | 16-23
        _mm256_shuffle8_epi16(&f2, &f3); // 8-15 | 24-31
        _mm256_shuffle8_epi16(&f4, &f5);
        _mm256_shuffle8_epi16(&f6, &f7);

        // level 6 interval = 8
        __m128i lo = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 0]);
        __m128i hi = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 1]);
        zeta_vec = _mm256_setr_m128i(lo, hi);
        _mm256_CTbutterfly_epi16(&f0, &f1, zeta_vec, q_vec, qinv_vec);

        lo = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 2]);
        hi = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 3]);
        zeta_vec = _mm256_setr_m128i(lo, hi);
        _mm256_CTbutterfly_epi16(&f2, &f3, zeta_vec, q_vec, qinv_vec);

        lo = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 4]);
        hi = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 5]);
        zeta_vec = _mm256_setr_m128i(lo, hi);
        _mm256_CTbutterfly_epi16(&f4, &f5, zeta_vec, q_vec, qinv_vec);

        lo = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 6]);
        hi = _mm_set1_epi16(zetas_avx2[64 + j * 8 + 7]);
        zeta_vec = _mm256_setr_m128i(lo, hi);
        _mm256_CTbutterfly_epi16(&f6, &f7, zeta_vec, q_vec, qinv_vec);

        // shuffle
        _mm256_shuffle8_epi16(&f0, &f1);
        _mm256_shuffle8_epi16(&f2, &f3);
        _mm256_shuffle8_epi16(&f4, &f5);
        _mm256_shuffle8_epi16(&f6, &f7);

        // reduce
        f0 = _mm256_barrett_epi16(f0, barrett_v_vec, q_vec);
        f1 = _mm256_barrett_epi16(f1, barrett_v_vec, q_vec);
        f2 = _mm256_barrett_epi16(f2, barrett_v_vec, q_vec);
        f3 = _mm256_barrett_epi16(f3, barrett_v_vec, q_vec);

        f4 = _mm256_barrett_epi16(f4, barrett_v_vec, q_vec);
        f5 = _mm256_barrett_epi16(f5, barrett_v_vec, q_vec);
        f6 = _mm256_barrett_epi16(f6, barrett_v_vec, q_vec);
        f7 = _mm256_barrett_epi16(f7, barrett_v_vec, q_vec);

        // store
        _mm256_storeu_si256((__m256i *)(b +   0 + j * 128), f0);
        _mm256_storeu_si256((__m256i *)(b +  16 + j * 128), f1);
        _mm256_storeu_si256((__m256i *)(b +  32 + j * 128), f2);
        _mm256_storeu_si256((__m256i *)(b +  48 + j * 128), f3);

        _mm256_storeu_si256((__m256i *)(b +  64 + j * 128), f4);
        _mm256_storeu_si256((__m256i *)(b +  80 + j * 128), f5);
        _mm256_storeu_si256((__m256i *)(b +  96 + j * 128), f6);
        _mm256_storeu_si256((__m256i *)(b + 112 + j * 128), f7);
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
static inline __m256i calc_d_vec(const __m256i *va, const __m256i *vb,
                                  const __m256i *vd, int x, int y,
                                  __m256i q_vec, __m256i qinv_vec)
{
    __m256i t1 = _mm256_add_epi16(va[x], va[y]);
    __m256i t2 = _mm256_add_epi16(vb[x], vb[y]);
    __m256i prod = _mm256_fqmul_epi16(t1, t2, q_vec, qinv_vec);
    __m256i sumd = _mm256_add_epi16(vd[x], vd[y]);
    return _mm256_sub_epi16(prod, sumd);
}

void shuffle_to_basemul8x8(__m256i input[8])
{
        _mm256_shuffle8_epi16(&input[0], &input[4]);
        _mm256_shuffle8_epi16(&input[1], &input[5]);
        _mm256_shuffle8_epi16(&input[2], &input[6]);
        _mm256_shuffle8_epi16(&input[3], &input[7]);

        _mm256_shuffle4_epi16(&input[0], &input[2]);
        _mm256_shuffle4_epi16(&input[1], &input[3]);
        _mm256_shuffle4_epi16(&input[4], &input[6]);
        _mm256_shuffle4_epi16(&input[5], &input[7]);

        _mm256_shuffle2_epi16(&input[0], &input[4]);
        _mm256_shuffle2_epi16(&input[1], &input[5]);
        _mm256_shuffle2_epi16(&input[2], &input[6]);
        _mm256_shuffle2_epi16(&input[3], &input[7]);

        _mm256_shuffle1_epi16(&input[0], &input[1]);
        _mm256_shuffle1_epi16(&input[2], &input[3]);
        _mm256_shuffle1_epi16(&input[4], &input[5]);
        _mm256_shuffle1_epi16(&input[6], &input[7]);
        __m256i tmp = input[2];
        input[2] = input[4];
        input[4] = tmp;
        tmp = input[3];
        input[3] = input[5];
        input[5] = tmp;
}

void shuffle_from_basemul8x8(__m256i input[8])
{
        __m256i tmp = input[2];
        input[2] = input[4];
        input[4] = tmp;
        tmp = input[3];
        input[3] = input[5];
        input[5] = tmp;
        
        _mm256_shuffle1_epi16(&input[0], &input[1]);
        _mm256_shuffle1_epi16(&input[2], &input[3]);
        _mm256_shuffle1_epi16(&input[4], &input[5]);
        _mm256_shuffle1_epi16(&input[6], &input[7]);

        _mm256_shuffle2_epi16(&input[0], &input[4]);
        _mm256_shuffle2_epi16(&input[1], &input[5]);
        _mm256_shuffle2_epi16(&input[2], &input[6]);
        _mm256_shuffle2_epi16(&input[3], &input[7]);

        _mm256_shuffle4_epi16(&input[0], &input[2]);
        _mm256_shuffle4_epi16(&input[1], &input[3]);
        _mm256_shuffle4_epi16(&input[4], &input[6]);
        _mm256_shuffle4_epi16(&input[5], &input[7]);

        _mm256_shuffle8_epi16(&input[0], &input[4]);
        _mm256_shuffle8_epi16(&input[1], &input[5]);
        _mm256_shuffle8_epi16(&input[2], &input[6]);
        _mm256_shuffle8_epi16(&input[3], &input[7]);
}

void basemul_avx2 (int16_t *c, const int16_t *a, const int16_t *b, const int16_t *zeta) 
{
    __m256i va[8], vb[8], vc[8], vd[8];
    __m256i zeta_vec;
    
    __m256i q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i qinv_vec = _mm256_set1_epi16(QINV);
    __m256i barrett_v_vec = _mm256_set1_epi16(BARRETT_V);

    int i;

    // load
    for(i = 0; i < 8; i++) {
        va[i] = _mm256_lddqu_si256((__m256i const *)(a + i * 16));
        vb[i] = _mm256_lddqu_si256((__m256i const *)(b + i * 16));
    }

    // shuffle to basemul8x8
    shuffle_to_basemul8x8(va);
    shuffle_to_basemul8x8(vb);

    // zeta_vec = _mm256_setr_epi16(zeta[0],  zeta[1],  zeta[2],  zeta[3],
    //                              zeta[4],  zeta[5],  zeta[6],  zeta[7],
    //                             -zeta[0], -zeta[1], -zeta[2], -zeta[3],
    //                             -zeta[4], -zeta[5], -zeta[6], -zeta[7]);
    zeta_vec = _mm256_setr_epi16(zeta[0], zeta[1], -zeta[0], -zeta[1],
                                 zeta[2], zeta[3], -zeta[2], -zeta[3],
                                 zeta[4], zeta[5], -zeta[4], -zeta[5],
                                 zeta[6], zeta[7], -zeta[6], -zeta[7]);

    // calculate vd
    for(i = 0; i < 8; i++) {
        vd[i] = _mm256_fqmul_epi16(va[i], vb[i], q_vec, qinv_vec);
    }

    __m256i tmp0, tmp1;
    // c_0 = d_0 + zeta' * ( (a_1*b_7 + a_7*b_1) + (a_2*b_6 + a_6*b_2) + (a_3*b_5 + a_5*b_3) + a_4*b_4 )
    tmp0 = calc_d_vec(va, vb, vd, 1, 7, q_vec, qinv_vec);
    tmp1 = calc_d_vec(va, vb, vd, 2, 6, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, tmp1);
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp1 = calc_d_vec(va, vb, vd, 3, 5, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, tmp1);
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_add_epi16(tmp0, vd[4]);
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_fqmul_epi16(tmp0, zeta_vec, q_vec, qinv_vec);
    vc[0] = _mm256_add_epi16(vd[0], tmp0);
    vc[0] = _mm256_barrett_epi16(vc[0], barrett_v_vec, q_vec);

    // c_1 = (a_0*b_1 + a_1*b_0) + zeta' * ( (a_2*b_7 + a_7*b_2) + (a_3*b_6 + a_6*b_3) + (a_4*b_5 + a_5*b_4) )
    tmp0 = calc_d_vec(va, vb, vd, 0, 1, q_vec, qinv_vec);
    tmp1 = calc_d_vec(va, vb, vd, 2, 7, q_vec, qinv_vec);
    tmp1 = _mm256_add_epi16(tmp1, calc_d_vec(va, vb, vd, 3, 6, q_vec, qinv_vec));
    tmp1 = _mm256_barrett_epi16(tmp1, barrett_v_vec, q_vec);
    tmp1 = _mm256_add_epi16(tmp1, calc_d_vec(va, vb, vd, 4, 5, q_vec, qinv_vec));
    tmp1 = _mm256_barrett_epi16(tmp1, barrett_v_vec, q_vec);
    tmp1 = _mm256_fqmul_epi16(tmp1, zeta_vec, q_vec, qinv_vec);
    vc[1] = _mm256_add_epi16(tmp0, tmp1);
    vc[1] = _mm256_barrett_epi16(vc[1], barrett_v_vec, q_vec);

    // c_2 = (a_0*b_2 + a_2*b_0) + a_1*b_1 + zeta' * ( (a_3*b_7 + a_7*b_3) + (a_4*b_6 + a_6*b_4) + a_5*b_5 )
    tmp0 = calc_d_vec(va, vb, vd, 0, 2, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, vd[1]);
    tmp1 = calc_d_vec(va, vb, vd, 3, 7, q_vec, qinv_vec);
    tmp1 = _mm256_add_epi16(tmp1, calc_d_vec(va, vb, vd, 4, 6, q_vec, qinv_vec));
    tmp1 = _mm256_barrett_epi16(tmp1, barrett_v_vec, q_vec);
    tmp1 = _mm256_add_epi16(tmp1, vd[5]);
    tmp1 = _mm256_barrett_epi16(tmp1, barrett_v_vec, q_vec);
    tmp1 = _mm256_fqmul_epi16(tmp1, zeta_vec, q_vec, qinv_vec);
    vc[2] = _mm256_add_epi16(tmp0, tmp1);
    vc[2] = _mm256_barrett_epi16(vc[2], barrett_v_vec, q_vec);

    // c_3 = (a_0*b_3 + a_3*b_0) + (a_1*b_2 + a_2*b_1) + zeta' * ( (a_4*b_7 + a_7*b_4) + (a_5*b_6 + a_6*b_5) )
    tmp0 = calc_d_vec(va, vb, vd, 0, 3, q_vec, qinv_vec);
    tmp1 = calc_d_vec(va, vb, vd, 1, 2, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, tmp1);
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp1 = calc_d_vec(va, vb, vd, 4, 7, q_vec, qinv_vec);
    tmp1 = _mm256_add_epi16(tmp1, calc_d_vec(va, vb, vd, 5, 6, q_vec, qinv_vec));
    tmp1 = _mm256_barrett_epi16(tmp1, barrett_v_vec, q_vec);
    tmp1 = _mm256_fqmul_epi16(tmp1, zeta_vec, q_vec, qinv_vec);
    vc[3] = _mm256_add_epi16(tmp0, tmp1);
    vc[3] = _mm256_barrett_epi16(vc[3], barrett_v_vec, q_vec);

    // c_4 = (a_0*b_4 + a_4*b_0) + (a_1*b_3 + a_3*b_1) + a_2*b_2 + zeta' * ( (a_5*b_7 + a_7*b_5) + a_6*b_6 )
    tmp0 = calc_d_vec(va, vb, vd, 0, 4, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 1, 3, q_vec, qinv_vec));
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_add_epi16(tmp0, vd[2]);
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp1 = calc_d_vec(va, vb, vd, 5, 7, q_vec, qinv_vec);
    tmp1 = _mm256_add_epi16(tmp1, vd[6]);
    tmp1 = _mm256_barrett_epi16(tmp1, barrett_v_vec, q_vec);
    tmp1 = _mm256_fqmul_epi16(tmp1, zeta_vec, q_vec, qinv_vec);
    vc[4] = _mm256_add_epi16(tmp0, tmp1);
    vc[4] = _mm256_barrett_epi16(vc[4], barrett_v_vec, q_vec);

    // c_5 = (a_0*b_5 + a_5*b_0) + (a_1*b_4 + a_4*b_1) + (a_2*b_3 + a_3*b_2) + zeta' * ( a_6*b_7 + a_7*b_6 )
    tmp0 = calc_d_vec(va, vb, vd, 0, 5, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 1, 4, q_vec, qinv_vec));
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 2, 3, q_vec, qinv_vec));
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp1 = calc_d_vec(va, vb, vd, 6, 7, q_vec, qinv_vec);
    tmp1 = _mm256_fqmul_epi16(tmp1, zeta_vec, q_vec, qinv_vec);
    vc[5] = _mm256_add_epi16(tmp0, tmp1);
    vc[5] = _mm256_barrett_epi16(vc[5], barrett_v_vec, q_vec);

    // c_6 = (a_0*b_6 + a_6*b_0) + (a_1*b_5 + a_5*b_1) + (a_2*b_4 + a_4*b_2) + a_3*b_3 + zeta' * ( a_7*b_7 )
    tmp0 = calc_d_vec(va, vb, vd, 0, 6, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 1, 5, q_vec, qinv_vec));
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 2, 4, q_vec, qinv_vec));
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_add_epi16(tmp0, vd[3]);
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp1 = _mm256_fqmul_epi16(vd[7], zeta_vec, q_vec, qinv_vec);
    vc[6] = _mm256_add_epi16(tmp0, tmp1);
    vc[6] = _mm256_barrett_epi16(vc[6], barrett_v_vec, q_vec);

    // c_7 = (a_0*b_7 + a_7*b_0) + (a_1*b_6 + a_6*b_1) + (a_2*b_5 + a_5*b_2) + (a_3*b_4 + a_4*b_3)
    tmp0 = calc_d_vec(va, vb, vd, 0, 7, q_vec, qinv_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 1, 6, q_vec, qinv_vec));
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 2, 5, q_vec, qinv_vec));
    tmp0 = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);
    tmp0 = _mm256_add_epi16(tmp0, calc_d_vec(va, vb, vd, 3, 4, q_vec, qinv_vec));
    vc[7] = _mm256_barrett_epi16(tmp0, barrett_v_vec, q_vec);

    shuffle_from_basemul8x8(vc);
    // store
    for(i = 0; i < 8; i++) {
        _mm256_storeu_si256((__m256i *)(c + i * 16), vc[i]);
    }
}
