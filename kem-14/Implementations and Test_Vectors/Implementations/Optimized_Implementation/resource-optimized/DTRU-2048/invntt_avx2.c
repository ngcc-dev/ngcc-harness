#include <stdio.h>
#include "params.h"
#include "ntt_avx2.h"

const int16_t zetas_inv_avx2[193] = {
     331,  728, 2409, 2186, 1171,  612, 2538,  736,  328, 2852,  643, 1080, 2821,  878, 2421,  604,
    3297, 2656, 1457, 2087, 1575, 1511,  674, 3331,   19,  376, 1966, 1243, 3378,  620,  741,  836,
    1285, 2868,  506, 1280, 1935, 2449, 1717, 1228, 1900, 3030, 3008, 3305, 2471, 3231, 1503,  632,
    1687, 1726, 2074,  833, 2083, 1375,  110, 1631, 3019, 2431, 1439, 1003, 2367,  809,  203, 1470,
    2453, 2147, 1278,  910, 2537, 1444, 2328,  765, 2107, 1668,  410,  108, 2702, 2162, 2662, 2826,
    3042, 3432, 3455, 1416,   88, 3379, 1100, 2482, 2534,  111,  562, 3116, 2928, 1176, 2030,  872,
    2479, 1024, 1603, 2429, 2065,  291, 3342, 1909, 1039,  729,  888,  470, 2412,   62, 2494,  775,
    3441,  957, 3257, 3320,  511, 1197, 1202, 2863, 1715, 2644, 2833, 2753, 1886, 1937, 1665,  790,
    1580
};

/**
 * @brief Gentleman–Sande butterfly for inverse NTT
 */
static inline void  _mm256_GSbutterfly_epi16(__m256i *a, __m256i *b, __m256i zeta_inv_avx, __m256i q_vec, __m256i qinv_vec)
{
    __m256i c = _mm256_sub_epi16(*a, *b);
    *a = _mm256_add_epi16(*a, *b);
    *b = _mm256_fqmul_epi16(c, zeta_inv_avx, q_vec, qinv_vec);
}


void invntt_avx2(int16_t b[DTRU_N], const int16_t a[DTRU_N])
{
    uint16_t i, j;
    __m256i zeta_avx, barrett_v_vec, q_vec, qinv_vec;

    barrett_v_vec = _mm256_set1_epi16(BARRETT_V);
    q_vec = _mm256_set1_epi16(DTRU_Q);
    qinv_vec = _mm256_set1_epi16(QINV);

    // level 6-5
    for (j = 0; j < 16; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7;

        // load
        f0  = _mm256_lddqu_si256((__m256i const *)(a +   0 + j * 128));
        f1  = _mm256_lddqu_si256((__m256i const *)(a +  16 + j * 128));
        f2  = _mm256_lddqu_si256((__m256i const *)(a +  32 + j * 128));
        f3  = _mm256_lddqu_si256((__m256i const *)(a +  48 + j * 128));

        f4  = _mm256_lddqu_si256((__m256i const *)(a +  64 + j * 128));
        f5  = _mm256_lddqu_si256((__m256i const *)(a +  80 + j * 128));
        f6  = _mm256_lddqu_si256((__m256i const *)(a +  96 + j * 128));
        f7  = _mm256_lddqu_si256((__m256i const *)(a + 112 + j * 128));

        // level 6 interval = 16
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[j * 4 + 0]);
        _mm256_GSbutterfly_epi16(&f0, &f1, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[j * 4 + 1]);
        _mm256_GSbutterfly_epi16(&f2, &f3, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[j * 4 + 2]);
        _mm256_GSbutterfly_epi16(&f4, &f5, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[j * 4 + 3]);
        _mm256_GSbutterfly_epi16(&f6, &f7, zeta_avx, q_vec, qinv_vec);

        // level 5 interval = 32
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[64 + j * 2 + 0]);
        _mm256_GSbutterfly_epi16(&f0, &f2, zeta_avx, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(&f1, &f3, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[64 + j * 2 + 1]);
        _mm256_GSbutterfly_epi16(&f4, &f6, zeta_avx, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(&f5, &f7, zeta_avx, q_vec, qinv_vec);

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

    // level 4-3
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

            // level 4 interval = 64
            // every 64 consecutive parameters use the same zeta
            zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[96 + j *4 + 0]);
            _mm256_GSbutterfly_epi16(&f0, &f1, zeta_avx, q_vec, qinv_vec);

            zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[96 + j *4 + 1]);
            _mm256_GSbutterfly_epi16(&f2, &f3, zeta_avx, q_vec, qinv_vec);

            zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[96 + j *4 + 2]);
            _mm256_GSbutterfly_epi16(&f4, &f5, zeta_avx, q_vec, qinv_vec);

            zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[96 + j *4 + 3]);
            _mm256_GSbutterfly_epi16(&f6, &f7, zeta_avx, q_vec, qinv_vec);

            // level 3 interval = 128
            // every 128 consecutive parameters use the same zeta
            zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[112 + j *2 + 0]);
            _mm256_GSbutterfly_epi16(& f0, & f2, zeta_avx, q_vec, qinv_vec);
            _mm256_GSbutterfly_epi16(& f1, & f3, zeta_avx, q_vec, qinv_vec);
           
            zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[112 + j *2 + 1]);
            _mm256_GSbutterfly_epi16(& f4, & f6, zeta_avx, q_vec, qinv_vec);
            _mm256_GSbutterfly_epi16(& f5, & f7, zeta_avx, q_vec, qinv_vec);

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

    // level 2-0
    for (j = 0; j < 16; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7, f8;

        // load
        f0 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 0 / 8 + j * 16));
        f1 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 1 / 8 + j * 16));
        f2 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 2 / 8 + j * 16));
        f3 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 3 / 8 + j * 16));

        f4 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 4 / 8 + j * 16));
        f5 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 5 / 8 + j * 16));
        f6 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 6 / 8 + j * 16));
        f7 = _mm256_lddqu_si256((__m256i const *)(b + DTRU_N * 7 / 8 + j * 16));

        // level 2
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[120]);
        _mm256_GSbutterfly_epi16(&f0, &f1, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[121]);
        _mm256_GSbutterfly_epi16(&f2, &f3, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[122]);
        _mm256_GSbutterfly_epi16(&f4, &f5, zeta_avx, q_vec, qinv_vec);

        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[123]);
        _mm256_GSbutterfly_epi16(&f6, &f7, zeta_avx, q_vec, qinv_vec);

        // level 1
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[124]);
        _mm256_GSbutterfly_epi16(&f0, &f2, zeta_avx, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(&f1, &f3, zeta_avx, q_vec, qinv_vec);
        
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[125]);
        _mm256_GSbutterfly_epi16(&f4, &f6, zeta_avx, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(&f5, &f7, zeta_avx, q_vec, qinv_vec);

        // level 0
        zeta_avx = _mm256_set1_epi16(zetas_inv_avx2[126]);
        __m256i n1 = _mm256_set1_epi16(zetas_inv_avx2[127]);
        __m256i n2 = _mm256_set1_epi16(zetas_inv_avx2[128]);

        f8 = _mm256_sub_epi16(f0, f4);
        f8 = _mm256_fqmul_epi16(f8, zeta_avx, q_vec, qinv_vec);
        f0 = _mm256_add_epi16(f0, f4);
        f0 = _mm256_sub_epi16(f0, f8);
        f0 = _mm256_fqmul_epi16(f0, n1, q_vec, qinv_vec);
        f4 = _mm256_fqmul_epi16(f8, n2, q_vec, qinv_vec);

        f8 = _mm256_sub_epi16(f1, f5);
        f8 = _mm256_fqmul_epi16(f8, zeta_avx, q_vec, qinv_vec);
        f1 = _mm256_add_epi16(f1, f5);
        f1 = _mm256_sub_epi16(f1, f8);
        f1 = _mm256_fqmul_epi16(f1, n1, q_vec, qinv_vec);
        f5 = _mm256_fqmul_epi16(f8, n2, q_vec, qinv_vec);

        f8 = _mm256_sub_epi16(f2, f6);
        f8 = _mm256_fqmul_epi16(f8, zeta_avx, q_vec, qinv_vec);
        f2 = _mm256_add_epi16(f2, f6);
        f2 = _mm256_sub_epi16(f2, f8);
        f2 = _mm256_fqmul_epi16(f2, n1, q_vec, qinv_vec);
        f6 = _mm256_fqmul_epi16(f8, n2, q_vec, qinv_vec);

        f8 = _mm256_sub_epi16(f3, f7);
        f8 = _mm256_fqmul_epi16(f8, zeta_avx, q_vec, qinv_vec);
        f3 = _mm256_add_epi16(f3, f7);
        f3 = _mm256_sub_epi16(f3, f8);
        f3 = _mm256_fqmul_epi16(f3, n1, q_vec, qinv_vec);
        f7 = _mm256_fqmul_epi16(f8, n2, q_vec, qinv_vec);

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
}



