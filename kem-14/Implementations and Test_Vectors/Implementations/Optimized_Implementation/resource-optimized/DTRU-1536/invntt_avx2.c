#include <stdio.h>
#include <immintrin.h>
#include "reduce.h"
#include "ntt_avx2.h"

const int16_t zetas_inv_avx2[386] = {
    1975, 933, 1791, 3013, 2363, 489, 1672, 2524, 3374, 3222, 1157, 512, 158, 277, 5, 235, 182, 927, 3419, 437, 195, 1364, 947, 2530, 2982, 1734, 707, 1209, 3195, 2943, 2486, 1723, 2581, 1341, 3298, 1874, 543, 3215, 2052, 2116, 3198, 696, 320, 1514, 1451, 2210, 3306, 2761, 2109, 432, 1839, 2251, 1025, 2683, 3205, 3025, 517, 3426, 82, 1671, 307, 1640, 3051, 31, 2793, 2245, 2342, 1655, 1264, 443, 40, 1212, 1619, 183, 3005, 2514, 500, 2697, 1985, 3274, 3114, 352, 2199, 1322, 1361, 1674, 2603, 3105, 1560, 871, 662, 2898, 3153, 312, 1697, 2586, 1385, 3060, 2560, 100, 1237, 3160, 2249, 397, 887, 1797, 2588, 601, 2185, 2398, 2279, 1660, 679, 1473, 656, 3234, 2456, 1250, 209, 1984, 1286, 2319, 1441, 8, 884, 2327, 1297, 1138, 2350, 1188, 687, 1869, 2024, 3057, 983, 2269, 3402, 779, 1683, 274, 188, 1053, 2544, 2678, 3411, 1914, 2099, 2435, 2420, 892, 2002, 1543, 310, 1063, 885, 2362, 826, 3425, 1689, 2394, 1627, 218, 686, 1408, 2484, 1626, 735, 3239, 1661, 2209, 337, 27, 545, 2236, 608, 1248, 2772, 3040, 219, 732, 2970, 315, 2457, 417, 1009, 294, 929, 1772, 1328, 2066, 2230, 3163, 3017, 1458, 3093, 251, 1504, 1709, 3067, 1999, 2364, 2056, 950, 22, 2039, 2078, 524, 1401, 2480, 2349, 166, 2517, 3151, 1409, 3141, 1108, 2075, 1776, 2188, 1956, 2964, 275, 3332, 1681, 2917, 3096, 2696, 1728, 903, 1367, 1407, 361, 2587, 354, 2423, 3333, 2031, 230, 2843, 3103, 1158, 1531, 518, 2784, 253, 858, 555, 1926, 3018, 2875, 2371, 2489, 1752, 1907, 318, 582, 1575, 1946, 126, 2783, 3297, 801, 1370, 2000, 19, 3081, 2214, 1491, 2621, 2716, 2837, 79, 2286, 612, 736, 2538, 3126, 728, 2186, 2409, 3129, 2852, 1080, 643, 604, 2421, 878, 2821, 833, 2074, 1726, 1687, 1375, 2083, 110, 1826, 809, 2367, 203, 1987, 2431, 3019, 1439, 2454, 2177, 2951, 589, 2172, 1008, 1522, 1740, 1228, 226, 986, 1954, 632, 427, 1557, 449, 3305, 410, 3349, 2107, 1789, 2702, 1295, 631, 795, 1278, 2547, 2453, 1310, 2537, 2013, 2692, 1129, 3346, 923, 2895, 3116, 1427, 872, 529, 1176, 25, 415, 2, 1416, 2357, 2482, 3369, 3379, 291, 2065, 3342, 1548, 1024, 2479, 1603, 1028, 1045, 62, 775, 2494, 2418, 729, 470, 888, 200, 3320, 16, 957, 1202, 594, 511, 2260, 2644, 1715, 2753, 2833, 1571, 1520, 1665, 2571, 2568, 1679
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
    uint16_t j;
    __m256i barrett_v_vec = _mm256_set1_epi16(BARRETT_V);
    __m256i q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i qinv_vec = _mm256_set1_epi16(QINV);
    __m256i zeta1_vec, zeta2_vec, rho_vec;

    // level 7-3
    for (j = 0; j < 8; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11;
        __m256i z[3];

        // load as they were stored
        f0  = _mm256_lddqu_si256((__m256i const *)(a +   0 + j * 192));
        f4  = _mm256_lddqu_si256((__m256i const *)(a +  16 + j * 192));
        f3  = _mm256_lddqu_si256((__m256i const *)(a +  32 + j * 192));
        f2  = _mm256_lddqu_si256((__m256i const *)(a +  48 + j * 192));
        f1  = _mm256_lddqu_si256((__m256i const *)(a +  64 + j * 192));
        f5  = _mm256_lddqu_si256((__m256i const *)(a +  80 + j * 192));
        f6  = _mm256_lddqu_si256((__m256i const *)(a +  96 + j * 192));
        f10 = _mm256_lddqu_si256((__m256i const *)(a + 112 + j * 192));
        f9  = _mm256_lddqu_si256((__m256i const *)(a + 128 + j * 192));
        f8  = _mm256_lddqu_si256((__m256i const *)(a + 144 + j * 192));
        f7  = _mm256_lddqu_si256((__m256i const *)(a + 160 + j * 192));
        f11 = _mm256_lddqu_si256((__m256i const *)(a + 176 + j * 192));

        // level 7 interval = 4 last level Radix-3 NTT
        rho_vec = _mm256_set1_epi16(zetas_inv_avx2[383]);

        // 寄存器前4位 0 - 11
        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 +  0], zetas_inv_avx2[j * 32 +  0], zetas_inv_avx2[j * 32 +  0], zetas_inv_avx2[j * 32 +  0], \
            zetas_inv_avx2[j * 32 +  4], zetas_inv_avx2[j * 32 +  4], zetas_inv_avx2[j * 32 +  4], zetas_inv_avx2[j * 32 +  4], \
            zetas_inv_avx2[j * 32 +  8], zetas_inv_avx2[j * 32 +  8], zetas_inv_avx2[j * 32 +  8], zetas_inv_avx2[j * 32 +  8], \
            zetas_inv_avx2[j * 32 + 12], zetas_inv_avx2[j * 32 + 12], zetas_inv_avx2[j * 32 + 12], zetas_inv_avx2[j * 32 + 12]);
        zeta2_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 +  1], zetas_inv_avx2[j * 32 +  1], zetas_inv_avx2[j * 32 +  1], zetas_inv_avx2[j * 32 +  1], \
            zetas_inv_avx2[j * 32 +  5], zetas_inv_avx2[j * 32 +  5], zetas_inv_avx2[j * 32 +  5], zetas_inv_avx2[j * 32 +  5], \
            zetas_inv_avx2[j * 32 +  9], zetas_inv_avx2[j * 32 +  9], zetas_inv_avx2[j * 32 +  9], zetas_inv_avx2[j * 32 +  9], \
            zetas_inv_avx2[j * 32 + 13], zetas_inv_avx2[j * 32 + 13], zetas_inv_avx2[j * 32 + 13], zetas_inv_avx2[j * 32 + 13]);

        z[0] = f4;
        z[1] = f3;
        z[2] = _mm256_sub_epi16(f4, f3); // (f4 - f3)*rho
        z[2] = _mm256_fqmul_epi16(z[2], rho_vec, q_vec, qinv_vec);

        f4 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_add_epi16(f0, z[2]), z[1]), zeta1_vec, q_vec, qinv_vec);
        f3 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_sub_epi16(f0, z[2]), z[0]), zeta2_vec, q_vec, qinv_vec);
        f0 = _mm256_add_epi16(f0, z[0]);
        f0 = _mm256_add_epi16(f0, z[1]);
        f0 = _mm256_barrett_epi16(f0, barrett_v_vec, q_vec);

        // 寄存器前4位 12 - 23
        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 +  2], zetas_inv_avx2[j * 32 +  2], zetas_inv_avx2[j * 32 +  2], zetas_inv_avx2[j * 32 +  2], \
            zetas_inv_avx2[j * 32 +  6], zetas_inv_avx2[j * 32 +  6], zetas_inv_avx2[j * 32 +  6], zetas_inv_avx2[j * 32 +  6], \
            zetas_inv_avx2[j * 32 + 10], zetas_inv_avx2[j * 32 + 10], zetas_inv_avx2[j * 32 + 10], zetas_inv_avx2[j * 32 + 10], \
            zetas_inv_avx2[j * 32 + 14], zetas_inv_avx2[j * 32 + 14], zetas_inv_avx2[j * 32 + 14], zetas_inv_avx2[j * 32 + 14]);
        zeta2_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 +  3], zetas_inv_avx2[j * 32 +  3], zetas_inv_avx2[j * 32 +  3], zetas_inv_avx2[j * 32 +  3], \
            zetas_inv_avx2[j * 32 +  7], zetas_inv_avx2[j * 32 +  7], zetas_inv_avx2[j * 32 +  7], zetas_inv_avx2[j * 32 +  7], \
            zetas_inv_avx2[j * 32 + 11], zetas_inv_avx2[j * 32 + 11], zetas_inv_avx2[j * 32 + 11], zetas_inv_avx2[j * 32 + 11], \
            zetas_inv_avx2[j * 32 + 15], zetas_inv_avx2[j * 32 + 15], zetas_inv_avx2[j * 32 + 15], zetas_inv_avx2[j * 32 + 15]);

        z[0] = f1;
        z[1] = f5;
        z[2] = _mm256_sub_epi16(f1, f5); // (f1 - f5)*rho
        z[2] = _mm256_fqmul_epi16(z[2], rho_vec, q_vec, qinv_vec);

        f1 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_add_epi16(f2, z[2]), z[1]), zeta1_vec, q_vec, qinv_vec);
        f5 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_sub_epi16(f2, z[2]), z[0]), zeta2_vec, q_vec, qinv_vec);
        f2 = _mm256_add_epi16(f2, z[0]);
        f2 = _mm256_add_epi16(f2, z[1]);
        f2 = _mm256_barrett_epi16(f2, barrett_v_vec, q_vec);

        // 寄存器前4位 96 - 107
        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 + 16], zetas_inv_avx2[j * 32 + 16], zetas_inv_avx2[j * 32 + 16], zetas_inv_avx2[j * 32 + 16], \
            zetas_inv_avx2[j * 32 + 20], zetas_inv_avx2[j * 32 + 20], zetas_inv_avx2[j * 32 + 20], zetas_inv_avx2[j * 32 + 20], \
            zetas_inv_avx2[j * 32 + 24], zetas_inv_avx2[j * 32 + 24], zetas_inv_avx2[j * 32 + 24], zetas_inv_avx2[j * 32 + 24], \
            zetas_inv_avx2[j * 32 + 28], zetas_inv_avx2[j * 32 + 28], zetas_inv_avx2[j * 32 + 28], zetas_inv_avx2[j * 32 + 28]);
        zeta2_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 + 17], zetas_inv_avx2[j * 32 + 17], zetas_inv_avx2[j * 32 + 17], zetas_inv_avx2[j * 32 + 17], \
            zetas_inv_avx2[j * 32 + 21], zetas_inv_avx2[j * 32 + 21], zetas_inv_avx2[j * 32 + 21], zetas_inv_avx2[j * 32 + 21], \
            zetas_inv_avx2[j * 32 + 25], zetas_inv_avx2[j * 32 + 25], zetas_inv_avx2[j * 32 + 25], zetas_inv_avx2[j * 32 + 25], \
            zetas_inv_avx2[j * 32 + 29], zetas_inv_avx2[j * 32 + 29], zetas_inv_avx2[j * 32 + 29], zetas_inv_avx2[j * 32 + 29]);

        z[0] = f10;
        z[1] = f9;
        z[2] = _mm256_sub_epi16(f10, f9); // (f10 - f9)*rho
        z[2] = _mm256_fqmul_epi16(z[2], rho_vec, q_vec, qinv_vec);

        f10 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_add_epi16(f6, z[2]), z[1]), zeta1_vec, q_vec, qinv_vec);
        f9 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_sub_epi16(f6, z[2]), z[0]), zeta2_vec, q_vec, qinv_vec);
        f6 = _mm256_add_epi16(f6, z[0]);
        f6 = _mm256_add_epi16(f6, z[1]);
        f6 = _mm256_barrett_epi16(f6, barrett_v_vec, q_vec);

        // 寄存器前4位 108 - 119
        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 + 18], zetas_inv_avx2[j * 32 + 18], zetas_inv_avx2[j * 32 + 18], zetas_inv_avx2[j * 32 + 18], \
            zetas_inv_avx2[j * 32 + 22], zetas_inv_avx2[j * 32 + 22], zetas_inv_avx2[j * 32 + 22], zetas_inv_avx2[j * 32 + 22], \
            zetas_inv_avx2[j * 32 + 26], zetas_inv_avx2[j * 32 + 26], zetas_inv_avx2[j * 32 + 26], zetas_inv_avx2[j * 32 + 26], \
            zetas_inv_avx2[j * 32 + 30], zetas_inv_avx2[j * 32 + 30], zetas_inv_avx2[j * 32 + 30], zetas_inv_avx2[j * 32 + 30]);
        zeta2_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[j * 32 + 19], zetas_inv_avx2[j * 32 + 19], zetas_inv_avx2[j * 32 + 19], zetas_inv_avx2[j * 32 + 19], \
            zetas_inv_avx2[j * 32 + 23], zetas_inv_avx2[j * 32 + 23], zetas_inv_avx2[j * 32 + 23], zetas_inv_avx2[j * 32 + 23], \
            zetas_inv_avx2[j * 32 + 27], zetas_inv_avx2[j * 32 + 27], zetas_inv_avx2[j * 32 + 27], zetas_inv_avx2[j * 32 + 27], \
            zetas_inv_avx2[j * 32 + 31], zetas_inv_avx2[j * 32 + 31], zetas_inv_avx2[j * 32 + 31], zetas_inv_avx2[j * 32 + 31]);

        z[0] = f7;
        z[1] = f11;
        z[2] = _mm256_sub_epi16(f7, f11); // (f7 - f11)*rho
        z[2] = _mm256_fqmul_epi16(z[2], rho_vec, q_vec, qinv_vec);

        f7 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_add_epi16(f8, z[2]), z[1]), zeta1_vec, q_vec, qinv_vec);
        f11 = _mm256_fqmul_epi16(_mm256_sub_epi16(_mm256_sub_epi16(f8, z[2]), z[0]), zeta2_vec, q_vec, qinv_vec);
        f8 = _mm256_add_epi16(f8, z[0]);
        f8 = _mm256_add_epi16(f8, z[1]);
        f8 = _mm256_barrett_epi16(f8, barrett_v_vec, q_vec);

        // level 6 interval = 12
        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[256 + j * 8 + 0], zetas_inv_avx2[256 + j * 8 + 0], zetas_inv_avx2[256 + j * 8 + 0], zetas_inv_avx2[256 + j * 8 + 0], \
            zetas_inv_avx2[256 + j * 8 + 1], zetas_inv_avx2[256 + j * 8 + 1], zetas_inv_avx2[256 + j * 8 + 1], zetas_inv_avx2[256 + j * 8 + 1], \
            zetas_inv_avx2[256 + j * 8 + 2], zetas_inv_avx2[256 + j * 8 + 2], zetas_inv_avx2[256 + j * 8 + 2], zetas_inv_avx2[256 + j * 8 + 2], \
            zetas_inv_avx2[256 + j * 8 + 3], zetas_inv_avx2[256 + j * 8 + 3], zetas_inv_avx2[256 + j * 8 + 3], zetas_inv_avx2[256 + j * 8 + 3]);
        _mm256_GSbutterfly_epi16(& f0, & f2, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f4, & f1, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f3, & f5, zeta1_vec, q_vec, qinv_vec);
        
        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[256 + j * 8 + 4], zetas_inv_avx2[256 + j * 8 + 4], zetas_inv_avx2[256 + j * 8 + 4], zetas_inv_avx2[256 + j * 8 + 4], \
            zetas_inv_avx2[256 + j * 8 + 5], zetas_inv_avx2[256 + j * 8 + 5], zetas_inv_avx2[256 + j * 8 + 5], zetas_inv_avx2[256 + j * 8 + 5], \
            zetas_inv_avx2[256 + j * 8 + 6], zetas_inv_avx2[256 + j * 8 + 6], zetas_inv_avx2[256 + j * 8 + 6], zetas_inv_avx2[256 + j * 8 + 6], \
            zetas_inv_avx2[256 + j * 8 + 7], zetas_inv_avx2[256 + j * 8 + 7], zetas_inv_avx2[256 + j * 8 + 7], zetas_inv_avx2[256 + j * 8 + 7]);
        _mm256_GSbutterfly_epi16(& f6, & f8, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(&f10, & f7, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f9, &f11, zeta1_vec, q_vec, qinv_vec);

        // shuffle 4
        shuffle4(& f0, & f4);
        shuffle4(& f3, & f2);
        shuffle4(& f1, & f5);
        shuffle4(& f6, &f10);
        shuffle4(& f9, & f8);
        shuffle4(& f7, &f11);

        // level 5 interval = 24
        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[320 + j * 4 + 0], zetas_inv_avx2[320 + j * 4 + 0], zetas_inv_avx2[320 + j * 4 + 0], zetas_inv_avx2[320 + j * 4 + 0], \
            zetas_inv_avx2[320 + j * 4 + 0], zetas_inv_avx2[320 + j * 4 + 0], zetas_inv_avx2[320 + j * 4 + 0], zetas_inv_avx2[320 + j * 4 + 0], \
            zetas_inv_avx2[320 + j * 4 + 1], zetas_inv_avx2[320 + j * 4 + 1], zetas_inv_avx2[320 + j * 4 + 1], zetas_inv_avx2[320 + j * 4 + 1], \
            zetas_inv_avx2[320 + j * 4 + 1], zetas_inv_avx2[320 + j * 4 + 1], zetas_inv_avx2[320 + j * 4 + 1], zetas_inv_avx2[320 + j * 4 + 1]);
        _mm256_GSbutterfly_epi16(& f0, & f4, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f3, & f2, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f1, & f5, zeta1_vec, q_vec, qinv_vec);

        zeta1_vec = _mm256_setr_epi16(\
            zetas_inv_avx2[320 + j * 4 + 2], zetas_inv_avx2[320 + j * 4 + 2], zetas_inv_avx2[320 + j * 4 + 2], zetas_inv_avx2[320 + j * 4 + 2], \
            zetas_inv_avx2[320 + j * 4 + 2], zetas_inv_avx2[320 + j * 4 + 2], zetas_inv_avx2[320 + j * 4 + 2], zetas_inv_avx2[320 + j * 4 + 2], \
            zetas_inv_avx2[320 + j * 4 + 3], zetas_inv_avx2[320 + j * 4 + 3], zetas_inv_avx2[320 + j * 4 + 3], zetas_inv_avx2[320 + j * 4 + 3], \
            zetas_inv_avx2[320 + j * 4 + 3], zetas_inv_avx2[320 + j * 4 + 3], zetas_inv_avx2[320 + j * 4 + 3], zetas_inv_avx2[320 + j * 4 + 3]);
        _mm256_GSbutterfly_epi16(& f6, &f10, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f9, & f8, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f7, &f11, zeta1_vec, q_vec, qinv_vec);

        // shuffle 8
        shuffle8(& f0, & f3);
        shuffle8(& f1, & f4);
        shuffle8(& f2, & f5);
        shuffle8(& f6, & f9);
        shuffle8(& f7, &f10);
        shuffle8(& f8, &f11);

        // level 4
        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[352 + j * 2]);
        _mm256_GSbutterfly_epi16(& f0, & f3, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f1, & f4, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f2, & f5, zeta1_vec, q_vec, qinv_vec);

        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[352 + j * 2 +1]);
        _mm256_GSbutterfly_epi16(& f6, & f9, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f7, &f10, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f8, &f11, zeta1_vec, q_vec, qinv_vec);

        // reduce
        f0  = _mm256_barrett_epi16( f0, barrett_v_vec, q_vec);
        f1  = _mm256_barrett_epi16( f1, barrett_v_vec, q_vec);
        f2  = _mm256_barrett_epi16( f2, barrett_v_vec, q_vec);
        f3  = _mm256_barrett_epi16( f3, barrett_v_vec, q_vec);
        f4  = _mm256_barrett_epi16( f4, barrett_v_vec, q_vec);
        f5  = _mm256_barrett_epi16( f5, barrett_v_vec, q_vec);
        
        f6  = _mm256_barrett_epi16( f6, barrett_v_vec, q_vec);
        f7  = _mm256_barrett_epi16( f7, barrett_v_vec, q_vec);
        f8  = _mm256_barrett_epi16( f8, barrett_v_vec, q_vec);
        f9  = _mm256_barrett_epi16( f9, barrett_v_vec, q_vec);
        f10 = _mm256_barrett_epi16(f10, barrett_v_vec, q_vec);
        f11 = _mm256_barrett_epi16(f11, barrett_v_vec, q_vec);
        // level 3
        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[368 + j]);

        _mm256_GSbutterfly_epi16(& f0, & f6, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f1, & f7, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f2, & f8, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f3, & f9, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f4, &f10, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(& f5, &f11, zeta1_vec, q_vec, qinv_vec);

        // store
        _mm256_storeu_si256((__m256i *)(b +   0 + j * 192),  f0);
        _mm256_storeu_si256((__m256i *)(b +  16 + j * 192),  f1);
        _mm256_storeu_si256((__m256i *)(b +  32 + j * 192),  f2);
        _mm256_storeu_si256((__m256i *)(b +  48 + j * 192),  f3);
        _mm256_storeu_si256((__m256i *)(b +  64 + j * 192),  f4);
        _mm256_storeu_si256((__m256i *)(b +  80 + j * 192),  f5);

        _mm256_storeu_si256((__m256i *)(b +  96 + j * 192),  f6);
        _mm256_storeu_si256((__m256i *)(b + 112 + j * 192),  f7);
        _mm256_storeu_si256((__m256i *)(b + 128 + j * 192),  f8);
        _mm256_storeu_si256((__m256i *)(b + 144 + j * 192),  f9);
        _mm256_storeu_si256((__m256i *)(b + 160 + j * 192), f10);
        _mm256_storeu_si256((__m256i *)(b + 176 + j * 192), f11);
    }

    // level 2-0
    for (j = 0; j < 12; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7, f8;

        // load
        f0 = _mm256_lddqu_si256((__m256i const *)(b +    0 + j * 16));
        f1 = _mm256_lddqu_si256((__m256i const *)(b +  192 + j * 16));
        f2 = _mm256_lddqu_si256((__m256i const *)(b +  384 + j * 16));
        f3 = _mm256_lddqu_si256((__m256i const *)(b +  576 + j * 16));

        f4 = _mm256_lddqu_si256((__m256i const *)(b +  768 + j * 16));
        f5 = _mm256_lddqu_si256((__m256i const *)(b +  960 + j * 16));
        f6 = _mm256_lddqu_si256((__m256i const *)(b + 1152 + j * 16));
        f7 = _mm256_lddqu_si256((__m256i const *)(b + 1344 + j * 16));

        // level 2
        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[376]);
        _mm256_GSbutterfly_epi16(&f0, &f1, zeta1_vec, q_vec, qinv_vec);

        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[377]);
        _mm256_GSbutterfly_epi16(&f2, &f3, zeta1_vec, q_vec, qinv_vec);

        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[378]);
        _mm256_GSbutterfly_epi16(&f4, &f5, zeta1_vec, q_vec, qinv_vec);

        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[379]);
        _mm256_GSbutterfly_epi16(&f6, &f7, zeta1_vec, q_vec, qinv_vec);

        // level 1
        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[380]);
        _mm256_GSbutterfly_epi16(&f0, &f2, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(&f1, &f3, zeta1_vec, q_vec, qinv_vec);
        
        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[381]);
        _mm256_GSbutterfly_epi16(&f4, &f6, zeta1_vec, q_vec, qinv_vec);
        _mm256_GSbutterfly_epi16(&f5, &f7, zeta1_vec, q_vec, qinv_vec);

        // reduce
        f0 = _mm256_barrett_epi16(f0, barrett_v_vec, q_vec);
        f1 = _mm256_barrett_epi16(f1, barrett_v_vec, q_vec);
        f4 = _mm256_barrett_epi16(f4, barrett_v_vec, q_vec);
        f5 = _mm256_barrett_epi16(f5, barrett_v_vec, q_vec);


        // level 0
        __m256i n1 = _mm256_set1_epi16(zetas_inv_avx2[384]);
        __m256i n2 = _mm256_set1_epi16(zetas_inv_avx2[385]);

        zeta1_vec = _mm256_set1_epi16(zetas_inv_avx2[382]);
        
        f8 = _mm256_sub_epi16(f0, f4);
        f8 = _mm256_fqmul_epi16(f8, zeta1_vec, q_vec, qinv_vec);
        f0 = _mm256_add_epi16(f0, f4);
        f0 = _mm256_sub_epi16(f0, f8);
        f0 = _mm256_fqmul_epi16(f0, n1, q_vec, qinv_vec);
        f4 = _mm256_fqmul_epi16(f8, n2, q_vec, qinv_vec);

        f8 = _mm256_sub_epi16(f1, f5);
        f8 = _mm256_fqmul_epi16(f8, zeta1_vec, q_vec, qinv_vec);
        f1 = _mm256_add_epi16(f1, f5);
        f1 = _mm256_sub_epi16(f1, f8);
        f1 = _mm256_fqmul_epi16(f1, n1, q_vec, qinv_vec);
        f5 = _mm256_fqmul_epi16(f8, n2, q_vec, qinv_vec);

        f8 = _mm256_sub_epi16(f2, f6);
        f8 = _mm256_fqmul_epi16(f8, zeta1_vec, q_vec, qinv_vec);
        f2 = _mm256_add_epi16(f2, f6);
        f2 = _mm256_sub_epi16(f2, f8);
        f2 = _mm256_fqmul_epi16(f2, n1, q_vec, qinv_vec);
        f6 = _mm256_fqmul_epi16(f8, n2, q_vec, qinv_vec);

        f8 = _mm256_sub_epi16(f3, f7);
        f8 = _mm256_fqmul_epi16(f8, zeta1_vec, q_vec, qinv_vec);
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
        _mm256_storeu_si256((__m256i *)(b +    0 + j * 16), f0);
        _mm256_storeu_si256((__m256i *)(b +  192 + j * 16), f1);
        _mm256_storeu_si256((__m256i *)(b +  384 + j * 16), f2);
        _mm256_storeu_si256((__m256i *)(b +  576 + j * 16), f3);

        _mm256_storeu_si256((__m256i *)(b +  768 + j * 16), f4);
        _mm256_storeu_si256((__m256i *)(b +  960 + j * 16), f5);
        _mm256_storeu_si256((__m256i *)(b + 1152 + j * 16), f6);
        _mm256_storeu_si256((__m256i *)(b + 1344 + j * 16), f7);
    }

}



