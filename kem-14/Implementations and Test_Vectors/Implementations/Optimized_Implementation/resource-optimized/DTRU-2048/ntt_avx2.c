#include <stdio.h>
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

    // level 0-2
    for (j = 0; j < 16; j++)
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
        zeta_avx = _mm256_set1_epi16(zetas_avx2[1]);

        f8 = _mm256_fqmul_epi16(f4, zeta_avx, q_vec, qinv_vec); // barrett(b[j] * zetaqinv)
        f4 = _mm256_add_epi16(f0, f4); // b[j] + b[j + 1024] - t
        f4 = _mm256_sub_epi16(f4, f8);
        f0 = _mm256_add_epi16(f0, f8); // b[j] + t
        
        f8 = _mm256_fqmul_epi16(f5, zeta_avx, q_vec, qinv_vec);   // barrett(b[j + 1024] * zeta)
        f5 = _mm256_add_epi16(f1, f5); // b[j] + b[j + 1024] - t
        f5 = _mm256_sub_epi16(f5, f8);
        f1 = _mm256_add_epi16(f1, f8); // b[j] + t

        f8 = _mm256_fqmul_epi16(f6, zeta_avx, q_vec, qinv_vec);   // barrett(b[j + 1024] * zeta)
        f6 = _mm256_add_epi16(f2, f6); // b[j] + b[j + 1024] - t
        f6 = _mm256_sub_epi16(f6, f8);
        f2 = _mm256_add_epi16(f2, f8); // b[j] + t

        f8 = _mm256_fqmul_epi16(f7, zeta_avx, q_vec, qinv_vec);   // barrett(b[j + 1024] * zeta)
        f7 = _mm256_add_epi16(f3, f7); // b[j] + b[j + 1024] - t
        f7 = _mm256_sub_epi16(f7, f8);
        f3 = _mm256_add_epi16(f3, f8); // b[j] + t

        // level 1
        zeta_avx = _mm256_set1_epi16(zetas_avx2[2]);
        _mm256_CTbutterfly_epi16(&f0, &f2, zeta_avx, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f1, &f3, zeta_avx, q_vec, qinv_vec);
        
        zeta_avx = _mm256_set1_epi16(zetas_avx2[3]);
        _mm256_CTbutterfly_epi16(&f4, &f6, zeta_avx, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f5, &f7, zeta_avx, q_vec, qinv_vec);

        // level 2
        zeta_avx = _mm256_set1_epi16(zetas_avx2[4]);
        _mm256_CTbutterfly_epi16(&f0, &f1, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_avx2[5]);
        _mm256_CTbutterfly_epi16(&f2, &f3, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_avx2[6]);
        _mm256_CTbutterfly_epi16(&f4, &f5, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_avx2[7]);
        _mm256_CTbutterfly_epi16(&f6, &f7, zeta_avx, q_vec, qinv_vec);

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

    // level 3-4
    for (j =0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            __m256i f0, f1, f2, f3, f4, f5, f6, f7;

            // load
            f0  = _mm256_lddqu_si256((__m256i const *)(b +   0 + j * DTRU_N / 4 + i * 16));
            f1  = _mm256_lddqu_si256((__m256i const *)(b +  64 + j * DTRU_N / 4 + i * 16));
            f2  = _mm256_lddqu_si256((__m256i const *)(b + 128 + j * DTRU_N / 4 + i * 16));
            f3  = _mm256_lddqu_si256((__m256i const *)(b + 192 + j * DTRU_N / 4 + i * 16));

            f4  = _mm256_lddqu_si256((__m256i const *)(b + 256 + j * DTRU_N / 4 + i * 16));
            f5  = _mm256_lddqu_si256((__m256i const *)(b + 320 + j * DTRU_N / 4 + i * 16));
            f6  = _mm256_lddqu_si256((__m256i const *)(b + 384 + j * DTRU_N / 4 + i * 16));
            f7  = _mm256_lddqu_si256((__m256i const *)(b + 448 + j * DTRU_N / 4 + i * 16));
   

            // level 3 interval = 128
            // every 128 consecutive parameters use the same zeta
            zeta_avx = _mm256_set1_epi16(zetas_avx2[8 + j *2 + 0]);
            _mm256_CTbutterfly_epi16(& f0, & f2, zeta_avx, q_vec, qinv_vec);
            _mm256_CTbutterfly_epi16(& f1, & f3, zeta_avx, q_vec, qinv_vec);
           
            zeta_avx = _mm256_set1_epi16(zetas_avx2[8 + j *2 + 1]);
            _mm256_CTbutterfly_epi16(& f4, & f6, zeta_avx, q_vec, qinv_vec);
            _mm256_CTbutterfly_epi16(& f5, & f7, zeta_avx, q_vec, qinv_vec);

            // level 4 interval = 64
            // every 64 consecutive parameters use the same zeta
            zeta_avx = _mm256_set1_epi16(zetas_avx2[16 + j *4 + 0]);
            _mm256_CTbutterfly_epi16(&f0, &f1, zeta_avx, q_vec, qinv_vec);

            zeta_avx = _mm256_set1_epi16(zetas_avx2[16 + j *4 + 1]);
            _mm256_CTbutterfly_epi16(&f2, &f3, zeta_avx, q_vec, qinv_vec);

            zeta_avx = _mm256_set1_epi16(zetas_avx2[16 + j *4 + 2]);
            _mm256_CTbutterfly_epi16(&f4, &f5, zeta_avx, q_vec, qinv_vec);

            zeta_avx = _mm256_set1_epi16(zetas_avx2[16 + j *4 + 3]);
            _mm256_CTbutterfly_epi16(&f6, &f7, zeta_avx, q_vec, qinv_vec);

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
            _mm256_storeu_si256((__m256i *)(b +   0 + j * DTRU_N / 4 + i * 16), f0);
            _mm256_storeu_si256((__m256i *)(b +  64 + j * DTRU_N / 4 + i * 16), f1);
            _mm256_storeu_si256((__m256i *)(b + 128 + j * DTRU_N / 4 + i * 16), f2);
            _mm256_storeu_si256((__m256i *)(b + 192 + j * DTRU_N / 4 + i * 16), f3);

            _mm256_storeu_si256((__m256i *)(b + 256 + j * DTRU_N / 4 + i * 16), f4);
            _mm256_storeu_si256((__m256i *)(b + 320 + j * DTRU_N / 4 + i * 16), f5);
            _mm256_storeu_si256((__m256i *)(b + 384 + j * DTRU_N / 4 + i * 16), f6);
            _mm256_storeu_si256((__m256i *)(b + 448 + j * DTRU_N / 4 + i * 16), f7);
        }
    }

    // level 5-6
    for (j = 0; j < 16; j++)
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

        // level 5 interval = 32
        zeta_avx = _mm256_set1_epi16(zetas_avx2[32 + j * 2 + 0]);
        _mm256_CTbutterfly_epi16(&f0, &f2, zeta_avx, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f1, &f3, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_avx2[32 + j * 2 + 1]);
        _mm256_CTbutterfly_epi16(&f4, &f6, zeta_avx, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f5, &f7, zeta_avx, q_vec, qinv_vec);

        // level 6 interval = 16
        zeta_avx = _mm256_set1_epi16(zetas_avx2[64 + j * 4 + 0]);
        _mm256_CTbutterfly_epi16(&f0, &f1, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_avx2[64 + j * 4 + 1]);
        _mm256_CTbutterfly_epi16(&f2, &f3, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_avx2[64 + j * 4 + 2]);
        _mm256_CTbutterfly_epi16(&f4, &f5, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_avx2[64 + j * 4 + 3]);
        _mm256_CTbutterfly_epi16(&f6, &f7, zeta_avx, q_vec, qinv_vec);

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


/**
 * @brief batch 16-D mul, 16 times in a cycle
 */

void basemul_avx2 (int16_t *c, const int16_t *a, const int16_t *b, const int16_t *zeta) 
{
    __m256i va[16], vb[16], vc[16], vd[16], r[32];
    __m256i zeta_vec, tmp;
    int i, j;
    __m256i q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i qinv_vec = _mm256_set1_epi16(QINV);
    __m256i barrett_v_vec = _mm256_set1_epi16(BARRETT_V);

    // load
    for(i = 0; i < 16; i++) {
        va[i] = _mm256_lddqu_si256((__m256i const *)(a + i * 16));
        vb[i] = _mm256_lddqu_si256((__m256i const *)(b + i * 16));
    }

    // shuffle to basemul16x16
    shuffle_to_basemul16x16(va);
    shuffle_to_basemul16x16(vb);

    zeta_vec = _mm256_setr_epi16(zeta[0], -zeta[0], zeta[1], -zeta[1],
                                zeta[2], -zeta[2], zeta[3], -zeta[3],
                                zeta[4], -zeta[4], zeta[5], -zeta[5],
                                zeta[6], -zeta[6], zeta[7], -zeta[7]);

    // calculate vd
    for(i = 0; i < 16; i++) {
        vd[i] = _mm256_fqmul_epi16(va[i], vb[i], q_vec, qinv_vec);
    }

    // calculate r[i+j] = sigma(a[i]*b[j])
    for (i = 1; i < 32; i += 2) {
        r[i] = _mm256_setzero_si256();
    }
    for (i = 0; i < 16; i++) {
        r[2*i] = vd[i];
    }
    for(i = 0; i < 16; i ++) {
        for(j = i + 1; j < 16; j++) {
            tmp = calc_d_vec(va, vb, vd, i, j, q_vec, qinv_vec);
            r[i + j] = _mm256_add_epi16(r[i + j], tmp);
            r[i + j] = _mm256_barrett_epi16(r[i + j], barrett_v_vec, q_vec);
        }
    }

    for(i = 0; i < 16; i++) {
        r[i + 16] = _mm256_fqmul_epi16(r[i + 16], zeta_vec, q_vec, qinv_vec);
        vc[i] = _mm256_add_epi16(r[i], r[i + 16]);
        vc[i] = _mm256_barrett_epi16(vc[i], barrett_v_vec, q_vec);
    }

    shuffle_from_basemul16x16(vc);

    // store
    for(i = 0; i < 16; i++) {
        _mm256_storeu_si256((__m256i *)(c + i * 16), vc[i]);
    }
}
