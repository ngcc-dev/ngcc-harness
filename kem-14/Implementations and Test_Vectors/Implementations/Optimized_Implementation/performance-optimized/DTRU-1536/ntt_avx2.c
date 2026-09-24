#include <stdio.h>
#include <immintrin.h>
#include "reduce.h"
#include "ntt_avx2.h"

const int16_t zetas_avx2[384] = {
    1033, 886, 1937, 1886, 624, 704, 1742, 813, 1197, 2946, 2863, 2255, 2500, 3441, 137, 3257, 2569, 2987, 2728, 1039, 963, 2682, 3395, 2412, 2429, 1854, 978, 2433, 1909, 115, 1392, 3166, 78, 88, 975, 1100, 2041, 3455, 3042, 3432, 2281, 2928, 2585, 2030, 341, 562, 2534, 111, 2328, 765, 1444, 920, 2147, 1004, 910, 2179, 2662, 2826, 2162, 755, 1668, 1350, 108, 3047, 152, 3008, 1900, 3030, 2825, 1503, 2471, 3231, 2229, 1717, 1935, 2449, 1285, 2868, 506, 1280, 1003, 2018, 438, 1026, 1470, 3254, 1090, 2648, 1631, 3347, 1374, 2082, 1770, 1731, 1383, 2624, 636, 2579, 1036, 2853, 2814, 2377, 605, 328, 1048, 1271, 2729, 331, 919, 2721, 2845, 1171, 3378, 620, 741, 836, 1966, 1243, 376, 3438, 1457, 2087, 2656, 160, 674, 3331, 1511, 1882, 1434, 968, 3139, 582, 647, 1550, 1086, 2489, 3155, 673, 2902, 1926, 640, 2599, 2939, 2784, 2645, 124, 614, 3103, 164, 3227, 1034, 3333, 2953, 1729, 2050, 361, 221, 2090, 761, 1728, 3089, 1501, 125, 1681, 3344, 3182, 1269, 1956, 10, 940, 316, 1108, 2314, 2048, 3291, 2517, 1515, 3435, 2933, 1401, 1414, 1379, 2507, 22, 1894, 3206, 390, 1999, 3381, 1748, 364, 251, 2555, 1685, 1227, 3163, 80, 1391, 2528, 1772, 513, 2725, 1000, 417, 2553, 3142, 3238, 732, 3394, 3430, 2849, 1248, 1324, 1221, 3120, 27, 1749, 2049, 2722, 3239, 941, 1831, 2771, 1408, 2594, 1095, 1768, 2394, 2882, 32, 2572, 2362, 418, 1022, 1455, 1543, 1312, 2565, 1358, 2435, 1101, 3183, 913, 2678, 1719, 2404, 1774, 274, 1041, 1588, 2474, 2269, 1663, 400, 2770, 1869, 3044, 3449, 2160, 1138, 3302, 1130, 2016, 8, 2247, 223, 3248, 1984, 23, 2207, 2801, 3234, 3363, 2856, 1178, 1660, 1756, 1059, 869, 601, 2445, 3357, 1208, 397, 2282, 297, 897, 100, 1456, 559, 1760, 2586, 898, 3145, 2795, 2898, 2215, 2135, 854, 3105, 915, 1783, 1258, 1322, 1972, 943, 1472, 3274, 2071, 760, 452, 2514, 1224, 1802, 3417, 1212, 451, 3014, 1115, 1655, 713, 1786, 406, 31, 435, 1817, 3375, 1671, 1277, 1206, 252, 3025, 270, 774, 1618, 2251, 1602, 1943, 151, 2761, 2878, 1247, 3137, 1514, 1948, 1583, 1405, 2116, 2740, 242, 159, 1874, 709, 2248, 971, 1723, 2275, 514, 2750, 1209, 2705, 3020, 2510, 2530, 220, 2093, 38, 437, 153, 2945, 3452, 235, 2217, 3180, 2300, 512, 691, 444, 1785, 2524, 184, 2968, 1666, 3013
};

const int16_t zetas_base_avx2[128] = {
    1705, 2023, 439, 2810, 3204, 302, 2299, 2817, 1426, 812, 870, 3293, 2554, 504, 540, 3236, 493, 368, 1382, 113, 306, 3447, 977, 1143, 1418, 1942, 1093, 2043, 1953, 1563, 440, 76, 2129, 902, 2448, 3377, 487, 2944, 685, 904, 2912, 63, 1796, 2133, 973, 1708, 1830, 2516, 2631, 863, 3147, 575, 1037, 3039, 46, 2145, 3269, 2356, 55, 1738, 1433, 2416, 1107, 1794, 2573, 413, 2171, 155, 1001, 1210, 2778, 3434, 1272, 94, 2570, 1701, 2220, 1012, 2072, 1175, 304, 2001, 1897, 2559, 2096, 1242, 343, 2542, 2957, 1485, 1838, 1386, 2193, 2233, 664, 3006, 3150, 2744, 2940, 3022, 2432, 2180, 1348, 3187, 2006, 1855, 259, 579, 2914, 1509, 876, 717, 262, 2748, 475, 1182, 3262, 752, 3275, 3237, 3299, 3304, 83, 1240, 1094, 2766, 1482, 3273
};

/**
 * @brief stored as basemul/inv_avx2 order
 * 
 * zetas_mul_inv_avx2[i] = zetas_mul[index], where
 * index =
 * 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15
 * as this loop...
 */
const int16_t zetas_mul_inv_avx2[384] = {
     1434,  1705,   318,  3139,   647,   439,  2371,  1086,  3155,  3204,   555,  2902,   640,  2299,   518,  2939,
     2023,  1752,  2645,  1426,  2810,  3018,   164,   870,   302,   253,  2953,  2554,  2817,  1158,   221,   540,
     2843,   614,   812,  2031,  2423,  1034,  3293,  2587,  1407,  2050,   504,   903,  2696,   761,  3236,  2917,
     3089,   493,  3332,   125,  3344,  1382,  2188,  1269,    10,   306,  3141,   316,  2314,   977,   166,  3291,
      368,  2964,  1515,  1418,   113,  2075,  1414,  1093,  3447,  3151,  1894,  1953,  1143,  2480,  3381,   440,
      524,  2933,  1942,  2039,   950,  2507,  2043,  2364,  3067,   390,  1563,  1504,  3093,   364,    76,  3017,
     2555,  2129,  2230,  1227,    80,  2448,   929,  2528,   513,   487,  2457,  1000,  2553,   685,   219,  3238,
      902,  1328,  3394,  2912,  3377,  1009,  1324,  1796,  2944,  2970,  1749,   973,   904,  2772,   941,  1830,
      608,  2849,    63,   545,   337,  3120,  2133,  1661,   735,  2722,  1708,  2484,   686,  2771,  2516,  1627,
     2594,  2631,  1689,  1768,  2882,  3147,   885,  2572,   418,  1037,  2002,  1455,  1312,    46,  2099,  1358,
      863,   826,  1101,  3269,   575,   310,  1719,    55,  3039,  2420,  1041,  1433,  2145,  3411,  1663,  1107,
     2544,   913,  2356,   188,  1683,  1774,  1738,  3402,   983,  2474,  2416,  2024,   687,  2770,  1794,  2350,
     3044,  2573,  1297,  2160,  3302,  2171,  1441,  2016,  2247,  1001,   209,  3248,    23,  2778,   656,  2801,
      413,   884,  3363,  1272,   155,  1286,  1756,  2570,  1210,  2456,  2445,  2220,  3434,   679,  2282,  2072,
     2279,  1178,    94,  2185,  2588,   869,  1701,   887,  2249,  1208,  1012,  1237,  2560,   897,  1175,  1385,
     1456,   304,  1697,  1760,   898,  1897,   662,  2795,  2215,  2096,  2603,   854,   915,   343,  2199,  1258,
     2001,  3153,  1972,  2957,  2559,  1560,  2071,  1838,  1242,  1361,  1224,  2193,  2542,  3114,   451,   664,
     1985,  1472,  1485,   500,  3005,   452,  1386,  1619,    40,  3417,  2233,  1264,  2342,  1115,  3006,  2793,
      713,  3150,  3051,   406,   435,  2940,    82,  3375,  1277,  2432,  3205,   252,   270,  1348,  1839,  1618,
     2744,   307,  1602,  2006,  3022,   517,  2878,   259,  2180,  1025,  1948,  2914,  3187,  2109,  2740,   876,
     3306,   151,  1855,  1451,   320,  3137,   579,  3198,  2052,  1405,  1509,   543,  3298,   159,   717,  2581,
      709,   262,  2486,   971,  2275,   475,   707,  2750,  2705,  3262,   947,  2510,   220,  3275,  3419,    38,
     2748,  3195,   153,  3299,  1182,  2982,  2217,    83,   752,   195,   691,  1094,  3237,   182,   184,  1482,
        5,  3452,  3304,   158,  1157,  2300,  1240,  3374,  1672,  1785,  2766,  2363,  1791,  1666,  3273,  1975,
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
    __m256i barrett_v_vec = _mm256_set1_epi16(BARRETT_V);
    __m256i q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i qinv_vec = _mm256_set1_epi16(QINV);
    __m256i zeta_vec;

    // level 0-2
    for (j = 0; j < 12; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7, f8;

        // load
        f0 = _mm256_lddqu_si256((__m256i const *)(a +    0 + j * 16));
        f1 = _mm256_lddqu_si256((__m256i const *)(a +  192 + j * 16));
        f2 = _mm256_lddqu_si256((__m256i const *)(a +  384 + j * 16));
        f3 = _mm256_lddqu_si256((__m256i const *)(a +  576 + j * 16));

        f4 = _mm256_lddqu_si256((__m256i const *)(a +  768 + j * 16));
        f5 = _mm256_lddqu_si256((__m256i const *)(a +  960 + j * 16));
        f6 = _mm256_lddqu_si256((__m256i const *)(a + 1152 + j * 16));
        f7 = _mm256_lddqu_si256((__m256i const *)(a + 1344 + j * 16));

        // level 0
        zeta_vec = _mm256_set1_epi16(zetas_avx2[1]);

        f8 = _mm256_fqmul_epi16(f4, zeta_vec, q_vec, qinv_vec);   // barrett(b[j + 768] * zeta)
        f4 = _mm256_add_epi16(f0, f4); // b[j] + b[j + 768] - t
        f4 = _mm256_sub_epi16(f4, f8);
        f0 = _mm256_add_epi16(f0, f8); // b[j] + t
        
        f8 = _mm256_fqmul_epi16(f5, zeta_vec, q_vec, qinv_vec);   // barrett(b[j + 768] * zeta)
        f5 = _mm256_add_epi16(f1, f5); // b[j] + b[j + 768] - t
        f5 = _mm256_sub_epi16(f5, f8);
        f1 = _mm256_add_epi16(f1, f8); // b[j] + t

        f8 = _mm256_fqmul_epi16(f6, zeta_vec, q_vec, qinv_vec);   // barrett(b[j + 768] * zeta)
        f6 = _mm256_add_epi16(f2, f6); // b[j] + b[j + 768] - t
        f6 = _mm256_sub_epi16(f6, f8);
        f2 = _mm256_add_epi16(f2, f8); // b[j] + t

        f8 = _mm256_fqmul_epi16(f7, zeta_vec, q_vec, qinv_vec);   // barrett(b[j + 768] * zeta)
        f7 = _mm256_add_epi16(f3, f7); // b[j] + b[j + 768] - t
        f7 = _mm256_sub_epi16(f7, f8);
        f3 = _mm256_add_epi16(f3, f8); // b[j] + t

        // level 1
        zeta_vec = _mm256_set1_epi16(zetas_avx2[2]);
        _mm256_CTbutterfly_epi16(&f0, &f2, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f1, &f3, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[3]);
        _mm256_CTbutterfly_epi16(&f4, &f6, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f5, &f7, zeta_vec, q_vec, qinv_vec);

        // level 2
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
        _mm256_storeu_si256((__m256i *)(b +    0 + j * 16), f0);
        _mm256_storeu_si256((__m256i *)(b +  192 + j * 16), f1);
        _mm256_storeu_si256((__m256i *)(b +  384 + j * 16), f2);
        _mm256_storeu_si256((__m256i *)(b +  576 + j * 16), f3);

        _mm256_storeu_si256((__m256i *)(b +  768 + j * 16), f4);
        _mm256_storeu_si256((__m256i *)(b +  960 + j * 16), f5);
        _mm256_storeu_si256((__m256i *)(b + 1152 + j * 16), f6);
        _mm256_storeu_si256((__m256i *)(b + 1344 + j * 16), f7);
    }

    // level 3-7
    for (j = 0; j < 8; j++)
    {
        __m256i f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, ftmp1, ftmp2, ftmp3;

        // load
        f0  = _mm256_lddqu_si256((__m256i const *)(b +   0 + j * 192));
        f1  = _mm256_lddqu_si256((__m256i const *)(b +  16 + j * 192));
        f2  = _mm256_lddqu_si256((__m256i const *)(b +  32 + j * 192));
        f3  = _mm256_lddqu_si256((__m256i const *)(b +  48 + j * 192));
        f4  = _mm256_lddqu_si256((__m256i const *)(b +  64 + j * 192));
        f5  = _mm256_lddqu_si256((__m256i const *)(b +  80 + j * 192));
        f6  = _mm256_lddqu_si256((__m256i const *)(b +  96 + j * 192));
        f7  = _mm256_lddqu_si256((__m256i const *)(b + 112 + j * 192));
        f8  = _mm256_lddqu_si256((__m256i const *)(b + 128 + j * 192));
        f9  = _mm256_lddqu_si256((__m256i const *)(b + 144 + j * 192));
        f10 = _mm256_lddqu_si256((__m256i const *)(b + 160 + j * 192));
        f11 = _mm256_lddqu_si256((__m256i const *)(b + 176 + j * 192));

        // level 3
        zeta_vec = _mm256_set1_epi16(zetas_avx2[8 + j]);

        _mm256_CTbutterfly_epi16(& f0, & f6, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f1, & f7, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f2, & f8, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f3, & f9, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f4, &f10, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f5, &f11, zeta_vec, q_vec, qinv_vec);

        // level 4
        zeta_vec = _mm256_set1_epi16(zetas_avx2[16 + j * 2]);
        _mm256_CTbutterfly_epi16(& f0, & f3, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f1, & f4, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f2, & f5, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[16 + j * 2 +1]);
        _mm256_CTbutterfly_epi16(& f6, & f9, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f7, &f10, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f8, &f11, zeta_vec, q_vec, qinv_vec);

        // level 5 interval = 24
        // shuffle 8
        shuffle8(& f0, & f3);
        shuffle8(& f1, & f4);
        shuffle8(& f2, & f5);
        shuffle8(& f6, & f9);
        shuffle8(& f7, &f10);
        shuffle8(& f8, &f11);

        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[32 + j * 4 + 0], zetas_avx2[32 + j * 4 + 0], zetas_avx2[32 + j * 4 + 0], zetas_avx2[32 + j * 4 + 0], \
            zetas_avx2[32 + j * 4 + 0], zetas_avx2[32 + j * 4 + 0], zetas_avx2[32 + j * 4 + 0], zetas_avx2[32 + j * 4 + 0], \
            zetas_avx2[32 + j * 4 + 1], zetas_avx2[32 + j * 4 + 1], zetas_avx2[32 + j * 4 + 1], zetas_avx2[32 + j * 4 + 1], \
            zetas_avx2[32 + j * 4 + 1], zetas_avx2[32 + j * 4 + 1], zetas_avx2[32 + j * 4 + 1], zetas_avx2[32 + j * 4 + 1]);
        _mm256_CTbutterfly_epi16(& f0, & f4, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f3, & f2, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f1, & f5, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[32 + j * 4 + 2], zetas_avx2[32 + j * 4 + 2], zetas_avx2[32 + j * 4 + 2], zetas_avx2[32 + j * 4 + 2], \
            zetas_avx2[32 + j * 4 + 2], zetas_avx2[32 + j * 4 + 2], zetas_avx2[32 + j * 4 + 2], zetas_avx2[32 + j * 4 + 2], \
            zetas_avx2[32 + j * 4 + 3], zetas_avx2[32 + j * 4 + 3], zetas_avx2[32 + j * 4 + 3], zetas_avx2[32 + j * 4 + 3], \
            zetas_avx2[32 + j * 4 + 3], zetas_avx2[32 + j * 4 + 3], zetas_avx2[32 + j * 4 + 3], zetas_avx2[32 + j * 4 + 3]);
        _mm256_CTbutterfly_epi16(& f6, &f10, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f9, & f8, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f7, &f11, zeta_vec, q_vec, qinv_vec);

        // level 6 interval = 12
        // shuffle 4
        shuffle4(& f0, & f4);
        shuffle4(& f3, & f2);
        shuffle4(& f1, & f5);
        shuffle4(& f6, &f10);
        shuffle4(& f9, & f8);
        shuffle4(& f7, &f11);

        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[64 + j * 8 + 0], zetas_avx2[64 + j * 8 + 0], zetas_avx2[64 + j * 8 + 0], zetas_avx2[64 + j * 8 + 0], \
            zetas_avx2[64 + j * 8 + 1], zetas_avx2[64 + j * 8 + 1], zetas_avx2[64 + j * 8 + 1], zetas_avx2[64 + j * 8 + 1], \
            zetas_avx2[64 + j * 8 + 2], zetas_avx2[64 + j * 8 + 2], zetas_avx2[64 + j * 8 + 2], zetas_avx2[64 + j * 8 + 2], \
            zetas_avx2[64 + j * 8 + 3], zetas_avx2[64 + j * 8 + 3], zetas_avx2[64 + j * 8 + 3], zetas_avx2[64 + j * 8 + 3]);
        _mm256_CTbutterfly_epi16(& f0, & f2, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f4, & f1, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f3, & f5, zeta_vec, q_vec, qinv_vec);
        
        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[64 + j * 8 + 4], zetas_avx2[64 + j * 8 + 4], zetas_avx2[64 + j * 8 + 4], zetas_avx2[64 + j * 8 + 4], \
            zetas_avx2[64 + j * 8 + 5], zetas_avx2[64 + j * 8 + 5], zetas_avx2[64 + j * 8 + 5], zetas_avx2[64 + j * 8 + 5], \
            zetas_avx2[64 + j * 8 + 6], zetas_avx2[64 + j * 8 + 6], zetas_avx2[64 + j * 8 + 6], zetas_avx2[64 + j * 8 + 6], \
            zetas_avx2[64 + j * 8 + 7], zetas_avx2[64 + j * 8 + 7], zetas_avx2[64 + j * 8 + 7], zetas_avx2[64 + j * 8 + 7]);
        _mm256_CTbutterfly_epi16(& f6, & f8, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(&f10, & f7, zeta_vec, q_vec, qinv_vec);
        _mm256_CTbutterfly_epi16(& f9, &f11, zeta_vec, q_vec, qinv_vec);

        // level 7 interval = 4 last level Radix-3 NTT
        // 0 - 11
        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 +  0], zetas_avx2[128 + j * 32 +  0], zetas_avx2[128 + j * 32 +  0], zetas_avx2[128 + j * 32 +  0], \
            zetas_avx2[128 + j * 32 +  4], zetas_avx2[128 + j * 32 +  4], zetas_avx2[128 + j * 32 +  4], zetas_avx2[128 + j * 32 +  4], \
            zetas_avx2[128 + j * 32 +  8], zetas_avx2[128 + j * 32 +  8], zetas_avx2[128 + j * 32 +  8], zetas_avx2[128 + j * 32 +  8], \
            zetas_avx2[128 + j * 32 + 12], zetas_avx2[128 + j * 32 + 12], zetas_avx2[128 + j * 32 + 12], zetas_avx2[128 + j * 32 + 12]);
        ftmp1 = _mm256_fqmul_epi16(f4, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 +  1], zetas_avx2[128 + j * 32 +  1], zetas_avx2[128 + j * 32 +  1], zetas_avx2[128 + j * 32 +  1], \
            zetas_avx2[128 + j * 32 +  5], zetas_avx2[128 + j * 32 +  5], zetas_avx2[128 + j * 32 +  5], zetas_avx2[128 + j * 32 +  5], \
            zetas_avx2[128 + j * 32 +  9], zetas_avx2[128 + j * 32 +  9], zetas_avx2[128 + j * 32 +  9], zetas_avx2[128 + j * 32 +  9], \
            zetas_avx2[128 + j * 32 + 13], zetas_avx2[128 + j * 32 + 13], zetas_avx2[128 + j * 32 + 13], zetas_avx2[128 + j * 32 + 13]);
        ftmp2 = _mm256_fqmul_epi16(f3, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[0]);
        ftmp3 = _mm256_sub_epi16(ftmp1, ftmp2);
        ftmp3 = _mm256_fqmul_epi16(ftmp3, zeta_vec, q_vec, qinv_vec);

        f3 = _mm256_sub_epi16(f0, ftmp1);
        f3 = _mm256_sub_epi16(f3, ftmp3);
        f4 = _mm256_sub_epi16(f0, ftmp2);
        f4 = _mm256_add_epi16(f4, ftmp3);
        f0 = _mm256_add_epi16(f0, ftmp1);
        f0 = _mm256_add_epi16(f0, ftmp2);

        // 12 - 23
        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 +  2], zetas_avx2[128 + j * 32 +  2], zetas_avx2[128 + j * 32 +  2], zetas_avx2[128 + j * 32 +  2], \
            zetas_avx2[128 + j * 32 +  6], zetas_avx2[128 + j * 32 +  6], zetas_avx2[128 + j * 32 +  6], zetas_avx2[128 + j * 32 +  6], \
            zetas_avx2[128 + j * 32 + 10], zetas_avx2[128 + j * 32 + 10], zetas_avx2[128 + j * 32 + 10], zetas_avx2[128 + j * 32 + 10], \
            zetas_avx2[128 + j * 32 + 14], zetas_avx2[128 + j * 32 + 14], zetas_avx2[128 + j * 32 + 14], zetas_avx2[128 + j * 32 + 14]);
        ftmp1 = _mm256_fqmul_epi16(f1, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 +  3], zetas_avx2[128 + j * 32 +  3], zetas_avx2[128 + j * 32 +  3], zetas_avx2[128 + j * 32 +  3], \
            zetas_avx2[128 + j * 32 +  7], zetas_avx2[128 + j * 32 +  7], zetas_avx2[128 + j * 32 +  7], zetas_avx2[128 + j * 32 +  7], \
            zetas_avx2[128 + j * 32 + 11], zetas_avx2[128 + j * 32 + 11], zetas_avx2[128 + j * 32 + 11], zetas_avx2[128 + j * 32 + 11], \
            zetas_avx2[128 + j * 32 + 15], zetas_avx2[128 + j * 32 + 15], zetas_avx2[128 + j * 32 + 15], zetas_avx2[128 + j * 32 + 15]);
        ftmp2 = _mm256_fqmul_epi16(f5, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[0]);
        ftmp3 = _mm256_sub_epi16(ftmp1, ftmp2);
        ftmp3 = _mm256_fqmul_epi16(ftmp3, zeta_vec, q_vec, qinv_vec);

        f5 = _mm256_sub_epi16(f2, ftmp1);
        f5 = _mm256_sub_epi16(f5, ftmp3);
        f1 = _mm256_sub_epi16(f2, ftmp2);
        f1 = _mm256_add_epi16(f1, ftmp3);
        f2 = _mm256_add_epi16(f2, ftmp1);
        f2 = _mm256_add_epi16(f2, ftmp2);

        // 96 - 107
        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 + 16], zetas_avx2[128 + j * 32 + 16], zetas_avx2[128 + j * 32 + 16], zetas_avx2[128 + j * 32 + 16], \
            zetas_avx2[128 + j * 32 + 20], zetas_avx2[128 + j * 32 + 20], zetas_avx2[128 + j * 32 + 20], zetas_avx2[128 + j * 32 + 20], \
            zetas_avx2[128 + j * 32 + 24], zetas_avx2[128 + j * 32 + 24], zetas_avx2[128 + j * 32 + 24], zetas_avx2[128 + j * 32 + 24], \
            zetas_avx2[128 + j * 32 + 28], zetas_avx2[128 + j * 32 + 28], zetas_avx2[128 + j * 32 + 28], zetas_avx2[128 + j * 32 + 28]);
        ftmp1 = _mm256_fqmul_epi16(f10, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 + 17], zetas_avx2[128 + j * 32 + 17], zetas_avx2[128 + j * 32 + 17], zetas_avx2[128 + j * 32 + 17], \
            zetas_avx2[128 + j * 32 + 21], zetas_avx2[128 + j * 32 + 21], zetas_avx2[128 + j * 32 + 21], zetas_avx2[128 + j * 32 + 21], \
            zetas_avx2[128 + j * 32 + 25], zetas_avx2[128 + j * 32 + 25], zetas_avx2[128 + j * 32 + 25], zetas_avx2[128 + j * 32 + 25], \
            zetas_avx2[128 + j * 32 + 29], zetas_avx2[128 + j * 32 + 29], zetas_avx2[128 + j * 32 + 29], zetas_avx2[128 + j * 32 + 29]);
        ftmp2 = _mm256_fqmul_epi16(f9, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[0]);
        ftmp3 = _mm256_sub_epi16(ftmp1, ftmp2);
        ftmp3 = _mm256_fqmul_epi16(ftmp3, zeta_vec, q_vec, qinv_vec);

        f9  = _mm256_sub_epi16( f6, ftmp1);
        f9  = _mm256_sub_epi16( f9, ftmp3);
        f10 = _mm256_sub_epi16( f6, ftmp2);
        f10 = _mm256_add_epi16(f10, ftmp3);
        f6  = _mm256_add_epi16( f6, ftmp1);
        f6  = _mm256_add_epi16( f6, ftmp2);

        // 108 - 119
        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 + 18], zetas_avx2[128 + j * 32 + 18], zetas_avx2[128 + j * 32 + 18], zetas_avx2[128 + j * 32 + 18], \
            zetas_avx2[128 + j * 32 + 22], zetas_avx2[128 + j * 32 + 22], zetas_avx2[128 + j * 32 + 22], zetas_avx2[128 + j * 32 + 22], \
            zetas_avx2[128 + j * 32 + 26], zetas_avx2[128 + j * 32 + 26], zetas_avx2[128 + j * 32 + 26], zetas_avx2[128 + j * 32 + 26], \
            zetas_avx2[128 + j * 32 + 30], zetas_avx2[128 + j * 32 + 30], zetas_avx2[128 + j * 32 + 30], zetas_avx2[128 + j * 32 + 30]);
        ftmp1 = _mm256_fqmul_epi16(f7, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_setr_epi16(\
            zetas_avx2[128 + j * 32 + 19], zetas_avx2[128 + j * 32 + 19], zetas_avx2[128 + j * 32 + 19], zetas_avx2[128 + j * 32 + 19], \
            zetas_avx2[128 + j * 32 + 23], zetas_avx2[128 + j * 32 + 23], zetas_avx2[128 + j * 32 + 23], zetas_avx2[128 + j * 32 + 23], \
            zetas_avx2[128 + j * 32 + 27], zetas_avx2[128 + j * 32 + 27], zetas_avx2[128 + j * 32 + 27], zetas_avx2[128 + j * 32 + 27], \
            zetas_avx2[128 + j * 32 + 31], zetas_avx2[128 + j * 32 + 31], zetas_avx2[128 + j * 32 + 31], zetas_avx2[128 + j * 32 + 31]);
        ftmp2 = _mm256_fqmul_epi16(f11, zeta_vec, q_vec, qinv_vec);

        zeta_vec = _mm256_set1_epi16(zetas_avx2[0]);
        ftmp3 = _mm256_sub_epi16(ftmp1, ftmp2);
        ftmp3 = _mm256_fqmul_epi16(ftmp3, zeta_vec, q_vec, qinv_vec);

        f11 = _mm256_sub_epi16( f8, ftmp1);
        f11 = _mm256_sub_epi16(f11, ftmp3);
        f7  = _mm256_sub_epi16( f8, ftmp2);
        f7  = _mm256_add_epi16( f7, ftmp3);
        f8  = _mm256_add_epi16( f8, ftmp1);
        f8  = _mm256_add_epi16( f8, ftmp2);

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

        // // shuffle back
        // shuffle4(& f0, & f4);
        // shuffle4(& f3, & f2);
        // shuffle4(& f1, & f5);
        // shuffle4(& f6, &f10);
        // shuffle4(& f9, & f8);
        // shuffle4(& f7, &f11);
        // shuffle8(& f0, & f3);
        // shuffle8(& f1, & f4);
        // shuffle8(& f2, & f5);
        // shuffle8(& f6, & f9);
        // shuffle8(& f7, &f10);
        // shuffle8(& f8, &f11);

        // _mm256_storeu_si256((__m256i *)(b +   0 + j * 192),  f0);
        // _mm256_storeu_si256((__m256i *)(b +  16 + j * 192),  f1);
        // _mm256_storeu_si256((__m256i *)(b +  32 + j * 192),  f2);
        // _mm256_storeu_si256((__m256i *)(b +  48 + j * 192),  f3);
        // _mm256_storeu_si256((__m256i *)(b +  64 + j * 192),  f4);
        // _mm256_storeu_si256((__m256i *)(b +  80 + j * 192),  f5);

        // _mm256_storeu_si256((__m256i *)(b +  96 + j * 192),  f6);
        // _mm256_storeu_si256((__m256i *)(b + 112 + j * 192),  f7);
        // _mm256_storeu_si256((__m256i *)(b + 128 + j * 192),  f8);
        // _mm256_storeu_si256((__m256i *)(b + 144 + j * 192),  f9);
        // _mm256_storeu_si256((__m256i *)(b + 160 + j * 192), f10);
        // _mm256_storeu_si256((__m256i *)(b + 176 + j * 192), f11);

        // store
        _mm256_storeu_si256((__m256i *)(b +   0 + j * 192),  f0);
        _mm256_storeu_si256((__m256i *)(b +  16 + j * 192),  f4);
        _mm256_storeu_si256((__m256i *)(b +  32 + j * 192),  f3);
        _mm256_storeu_si256((__m256i *)(b +  48 + j * 192),  f2);
        _mm256_storeu_si256((__m256i *)(b +  64 + j * 192),  f1);
        _mm256_storeu_si256((__m256i *)(b +  80 + j * 192),  f5);

        _mm256_storeu_si256((__m256i *)(b +  96 + j * 192),  f6);
        _mm256_storeu_si256((__m256i *)(b + 112 + j * 192), f10);
        _mm256_storeu_si256((__m256i *)(b + 128 + j * 192),  f9);
        _mm256_storeu_si256((__m256i *)(b + 144 + j * 192),  f8);
        _mm256_storeu_si256((__m256i *)(b + 160 + j * 192),  f7);
        _mm256_storeu_si256((__m256i *)(b + 176 + j * 192), f11);
    }
}


#define CALC_D_VEC(va, vb, vd, x, y, q_vec, qinv_vec) __extension__({               \
    __m256i t1 = _mm256_add_epi16(va##x, va##y);                     \
    __m256i t2 = _mm256_add_epi16(vb##x, vb##y);                     \
    __m256i prod = _mm256_fqmul_epi16(t1, t2, q_vec, qinv_vec);        \
    __m256i sumd = _mm256_add_epi16(vd##x, vd##y);                   \
    _mm256_sub_epi16(prod, sumd);                                    \
})


/**
 * @brief batch 4-D mul, 16 times in a cycle
 * 
 * @details for parallel calculating, a[0-3], b[0-3], d[0-3],zeta in 13 registers
 * c[0-2] in 3 registers, c[3] in a used register, as it's the last to calculate
 * 数据每6组按照竖列存放，而basemul对每4组进行操作。lcm(4, 6) = 12，因此每次运行basemul对12组数据进行操作
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

    // shuffle
    shuffle2(&va0, &va2);
    shuffle2(&va1, &va3);
    shuffle2(&vb0, &vb2);
    shuffle2(&vb1, &vb3);

    shuffle1(&va0, &va1);
    shuffle1(&va2, &va3);
    shuffle1(&vb0, &vb1);
    shuffle1(&vb2, &vb3);

    // calculate d
    vd0 = _mm256_fqmul_epi16(va0, vb0, q_vec, qinv_vec);
    vd1 = _mm256_fqmul_epi16(va1, vb1, q_vec, qinv_vec);
    vd2 = _mm256_fqmul_epi16(va2, vb2, q_vec, qinv_vec);
    vd3 = _mm256_fqmul_epi16(va3, vb3, q_vec, qinv_vec);

    // calculate c
    // vc0 needs no extra register, vc1 needs 1, vc2 needs 1, vc3 needs 1
    vc1 = CALC_D_VEC(va, vb, vd, 0, 1, q_vec, qinv_vec);
    vc2 = CALC_D_VEC(va, vb, vd, 2, 3, q_vec, qinv_vec);
    vc2 = _mm256_fqmul_epi16(vc2, zeta_vec, q_vec, qinv_vec);
    vc1 = _mm256_add_epi16(vc1, vc2);

    vc2 = CALC_D_VEC(va, vb, vd, 0, 2, q_vec, qinv_vec);
    vc3 = _mm256_fqmul_epi16(vd3, zeta_vec, q_vec, qinv_vec);
    vc2 = _mm256_add_epi16(vc2, vc3);
    vc2 = _mm256_add_epi16(vc2, vd1);

    vc3 = CALC_D_VEC(va, vb, vd, 1, 2, q_vec, qinv_vec);
    vc0 = CALC_D_VEC(va, vb, vd, 0, 3, q_vec, qinv_vec);
    vc3 = _mm256_add_epi16(vc3, vc0);

    vc0 = CALC_D_VEC(va, vb, vd, 1, 3, q_vec, qinv_vec);
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
    shuffle1(&vc0, &vc1);
    shuffle1(&vc2, &vc3);

    shuffle2(&vc0, &vc2);
    shuffle2(&vc1, &vc3);
    
    // store
    _mm256_storeu_si256((__m256i *)(c +  0), vc0);
    _mm256_storeu_si256((__m256i *)(c + 16), vc1);
    _mm256_storeu_si256((__m256i *)(c + 32), vc2);
    _mm256_storeu_si256((__m256i *)(c + 48), vc3);
}
