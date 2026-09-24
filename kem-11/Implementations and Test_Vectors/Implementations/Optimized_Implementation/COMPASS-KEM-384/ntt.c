#include <stdint.h>
#include "params.h"
#include "ntt.h"
#include "reduce.h"
#include <immintrin.h>

// #include "zetas_7681_avx.h"


 /* COMPASS_KEM_Q == 7681 */
/* Code to generate zetas and zetas_inv used in the number-theoretic transform:

#define COMPASS_KEM_ROOT_OF_UNITY 17

static const uint8_t tree[128] = {
  0, 64, 32, 96, 16, 80, 48, 112, 8, 72, 40, 104, 24, 88, 56, 120,
  4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
  2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122,
  6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
  1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121,
  5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
  3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
  7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127
};

void init_ntt() {
  unsigned int i;
  int16_t tmp[128];

  tmp[0] = MONT;
  for(i=1;i<128;i++)
    tmp[i] = fqmul(tmp[i-1],MONT*COMPASS_KEM_ROOT_OF_UNITY % COMPASS_KEM_Q);

  for(i=0;i<128;i++) {
    zetas[i] = tmp[tree[i]];
    if(zetas[i] > COMPASS_KEM_Q/2)
      zetas[i] -= COMPASS_KEM_Q;
    if(zetas[i] < -COMPASS_KEM_Q/2)
      zetas[i] += COMPASS_KEM_Q;
  }
}
*/
#if (COMPASS_KEM_Q == 3329) && (COMPASS_KEM_N == 256)
const int16_t zetas[128] = {
  -1044,  -758,  -359, -1517,  1493,  1422,   287,   202,
   -171,   622,  1577,   182,   962, -1202, -1474,  1468,
    573, -1325,   264,   383,  -829,  1458, -1602,  -130,
   -681,  1017,   732,   608, -1542,   411,  -205, -1571,
   1223,   652,  -552,  1015, -1293,  1491,  -282, -1544,
    516,    -8,  -320,  -666, -1618, -1162,   126,  1469,
   -853,   -90,  -271,   830,   107, -1421,  -247,  -951,
   -398,   961, -1508,  -725,   448, -1065,   677, -1275,
  -1103,   430,   555,   843, -1251,   871,  1550,   105,
    422,   587,   177,  -235,  -291,  -460,  1574,  1653,
   -246,   778,  1159,  -147,  -777,  1483,  -602,  1119,
  -1590,   644,  -872,   349,   418,   329,  -156,   -75,
    817,  1097,   603,   610,  1322, -1285, -1465,   384,
  -1215,  -136,  1218, -1335,  -874,   220, -1187, -1659,
  -1185, -1530, -1278,   794, -1510,  -854,  -870,   478,
   -108,  -308,   996,   991,   958, -1460,  1522,  1628
};
#elif (COMPASS_KEM_Q == 7681) && (COMPASS_KEM_N == 512)
const int16_t zetas[256] = {
    -3593, 3777, -3182, 3625, -3696, -1100, 2456, 2194,
     121, -2250,  834, -2495, -2319, 2876, -1701, 1414,
    2816, -2088, -2237, 1986, -1599, 1993, 3706, -2006,
    -1525, -2557, 1296, 1483, -2830, 3364,  617, 1921,
    -3689, -1738, 3266, -3600,  810, 1887, -638,   -7,
    -438, -679, -1305, -1760,  396, -3174, -3555, -1881,
    3772, -2535, -2440, -2555, 1535, -549, 3153, 2310,
    -1399, 1321,  514, -2956, -103, 2804, -2043, -1431,
    -1054, 1698, -3456, 1166, 2426, 3831,  915,   -2,
    -3417, -194, 2919, 2789, 3405, 2385, -2113, -2732,
    2175,  373, 3692, -730, -1756, 3135, -2391,  660,
    -1497, 2572, -3145, 1350, -2224, -3588, -1681, 2883,
    -1390, 1598, 3750, 2762, 2835, 2764, -2233, 3816,
    -1533, 1464, -727, 1521, 1386, -3428, -921, -2743,
    -2160, 2649, -859, 2579, 1532, 1919, -486,  404,
    -1056,  783, 1799, -2665, 3480, 2133, -3310, -1168,
     -17, 3744, 2422, 2001, 1278,  929, -1348, -2230,
    -179, -1242, -2059, -1070, 2161, 1649, 2072, 3177,
    -2071, 1121, -436,  236,  715,  670, -658, -1476,
    -2378, 2767, 3542, -226, 1203, 1181, -151, -3794,
    1712, -222, 2786, -451, -3547, 1779, -1151, -434,
    3568, -3693, 3581, -1586, 1509, 2918, 2339, -1407,
    3434, -3550, 2340, 2891, 2998, -3314, 3461, -2719,
    -2247, -2589, 1144, 1072, 1295, -2815, -3770, 3450,
    3781, -2258,  796, 3163, -3208, -589, 2963, -124,
    3214, 3334, -3366, -3745, 3723, 1931, -429, -402,
    -3408,   83, -1526,  826, -1338, 2345, -2303, 2515,
    -642, -1837, -2965, -791,  370,  293, 3312, 2083,
    -1689, -777, 2070, 2262, -893, 2386, -188, -1519,
    -2874, -1404, 1012, 2130, 1441, 2532, -3335, -1084,
    -3343, 2937,  509, -1403, 2812, 3763,  592, 2005,
    3657, 2460, -3677, 3752,  692, 1669, 2167, -3287
};

#else
#error "Unsupported parameters for NTT zetas table!"
#endif
#if (COMPASS_KEM_Q == 7681)
#include "zetas_7681_avx.h"
// static inline __m256i fqmul_avx(__m256i a, __m256i b, __m256i q_vec, __m256i qinv_vec) {
//     // c = a * b
//     __m256i c_lo = _mm256_mullo_epi16(a, b);
//     __m256i c_hi = _mm256_mulhi_epi16(a, b);
    
//     // t = (c_lo * QINV) & 0xFFFF
//     __m256i t = _mm256_mullo_epi16(c_lo, qinv_vec);
    
//     // t_hi = (t * Q) >> 16
//     __m256i t_hi = _mm256_mulhi_epi16(t, q_vec);
    
//     // Return c_hi - t_hi
//     return _mm256_sub_epi16(c_hi, t_hi);
// }

// // 2. Vectorized Barrett reduction (bring coefficients back near [-q/2, q/2])
// static inline __m256i barrett_reduce_avx(__m256i a, __m256i v_vec, __m256i q_vec) {
//     // t = (a * V) >> 16
//     __m256i t = _mm256_mulhi_epi16(a, v_vec);
//     // Continue right shift by 10 bits, completing the total >> 26 operation
//     t = _mm256_srai_epi16(t, 10);
    
//     // t_q = t * Q
//     __m256i t_q = _mm256_mullo_epi16(t, q_vec);
    
//     // Return a - t_q
//     return _mm256_sub_epi16(a, t_q);
// }

void ntt_avx_7681(int16_t *r) {
    __m256i q_vec = _mm256_set1_epi16(Q_7681);
    __m256i qinv_vec = _mm256_set1_epi16(QINV_7681);
    __m256i v_vec = _mm256_set1_epi16(V_7681);

    int k = 1;

    // --- Stage 1: Wide-span upper network (len = 256, 128, 64, 32, 16) ---
    for (int len = 256; len >= 16; len >>= 1) {
        for (int start = 0; start < 512; start += 2 * len) {
            __m256i zeta_vec = _mm256_set1_epi16(zetas[k++]);

            for (int j = start; j < start + len; j += 16) {
                // Use _mm256_load_si256 for 32-byte aligned load
                __m256i a = _mm256_load_si256((__m256i*)&r[j]);
                __m256i b = _mm256_load_si256((__m256i*)&r[j + len]);

                __m256i t = fqmul_avx(b, zeta_vec, q_vec, qinv_vec);

                __m256i out_a = _mm256_add_epi16(a, t);
                __m256i out_b = _mm256_sub_epi16(a, t);

                out_a = barrett_reduce_avx(out_a, v_vec, q_vec);
                out_b = barrett_reduce_avx(out_b, v_vec, q_vec);

                _mm256_store_si256((__m256i*)&r[j], out_a);
                _mm256_store_si256((__m256i*)&r[j + len], out_b);
            }
        }
    }

    // --- Stage 2: Optimized in-register interleaving network (fused len = 8, 4, 2) ---
    // At this point k equals 32
    int z_idx8 = k; 
    int z_idx4 = 0;
    int z_idx2 = 0;

    for (int j = 0; j < 512; j += 16) {
        __m256i v = _mm256_load_si256((__m256i*)&r[j]);
        
        // [Layer: len = 8]
        __m256i z_vec = _mm256_set1_epi16(zetas[z_idx8++]);
        __m256i v_right = _mm256_permute2x128_si256(v, v, 0x11); 
        __m256i t = fqmul_avx(v_right, z_vec, q_vec, qinv_vec);
        __m256i v_left = _mm256_permute2x128_si256(v, v, 0x00);
        __m256i res_add = _mm256_add_epi16(v_left, t);
        __m256i res_sub = _mm256_sub_epi16(v_left, t);
        v = _mm256_blend_epi32(res_add, res_sub, 0xF0);

        // [Layer: len = 4]
        z_vec = _mm256_load_si256((__m256i*)&zetas_ntt_len4[z_idx4]);
        z_idx4 += 16;
        v_right = _mm256_shuffle_epi32(v, 0xEE); 
        t = fqmul_avx(v_right, z_vec, q_vec, qinv_vec);
        v_left = _mm256_shuffle_epi32(v, 0x44);
        res_add = _mm256_add_epi16(v_left, t);
        res_sub = _mm256_sub_epi16(v_left, t);
        v = _mm256_blend_epi32(res_add, res_sub, 0xCC);

        // [Layer: len = 2]
        z_vec = _mm256_load_si256((__m256i*)&zetas_ntt_len2[z_idx2]);
        z_idx2 += 16;
        v_right = _mm256_shuffle_epi32(v, 0xF5); 
        t = fqmul_avx(v_right, z_vec, q_vec, qinv_vec);
        v_left = _mm256_shuffle_epi32(v, 0xA0);
        res_add = _mm256_add_epi16(v_left, t);
        res_sub = _mm256_sub_epi16(v_left, t);
        v = _mm256_blend_epi32(res_add, res_sub, 0xAA);
        
    // After 3 layers of unreduced butterfly ops, coefficients reach at most 4q = 30724 (< 32767)
        // Perform a single Barrett reduction before writing back to memory!
        v = barrett_reduce_avx(v, v_vec, q_vec);
        _mm256_store_si256((__m256i*)&r[j], v);
    }
}

// ====================================================================
// Inverse NTT (q = 7681, N = 512)
// ====================================================================
void invntt_avx_7681(int16_t *r) {
    __m256i q_vec = _mm256_set1_epi16(Q_7681);
    __m256i qinv_vec = _mm256_set1_epi16(QINV_7681);
    __m256i v_vec = _mm256_set1_epi16(V_7681);

    // --- Stage 1: Optimized in-register interleaving network (fused len = 2, 4, 8) ---
    int zeta_idx2 = 0;
    int zeta_idx4 = 0;
    // INTT uses reversed Zeta order; len=8 maps to zetas indices 63 down to 32
    int z_idx8 = 63; 

    for (int j = 0; j < 512; j += 16) {
        __m256i v = _mm256_load_si256((__m256i*)&r[j]);

        // [Layer: len = 2] (INTT does add/sub first, then multiply by Zeta)
        __m256i z_vec = _mm256_load_si256((__m256i*)&zetas_invntt_len2[zeta_idx2]);
        zeta_idx2 += 16;
        __m256i v_left  = _mm256_shuffle_epi32(v, 0xA0);
        __m256i v_right = _mm256_shuffle_epi32(v, 0xF5);
        __m256i res_add = _mm256_add_epi16(v_left, v_right);
        __m256i res_sub = _mm256_sub_epi16(v_right, v_left);
        __m256i t = fqmul_avx(res_sub, z_vec, q_vec, qinv_vec);
        v = _mm256_blend_epi32(res_add, t, 0xAA);

        // [Layer: len = 4]
        z_vec = _mm256_load_si256((__m256i*)&zetas_invntt_len4[zeta_idx4]);
        zeta_idx4 += 16;
        v_left  = _mm256_shuffle_epi32(v, 0x44);
        v_right = _mm256_shuffle_epi32(v, 0xEE);
        res_add = _mm256_add_epi16(v_left, v_right);
        res_sub = _mm256_sub_epi16(v_right, v_left);
        t = fqmul_avx(res_sub, z_vec, q_vec, qinv_vec);
        v = _mm256_blend_epi32(res_add, t, 0xCC);
        
        // [Safety]: First 2 layers push coefficients to 4q; reduce now to prevent 8q overflow
        v = barrett_reduce_avx(v, v_vec, q_vec);

        // [Layer: len = 8]
        z_vec = _mm256_set1_epi16(zetas[z_idx8--]);
        v_left  = _mm256_permute2x128_si256(v, v, 0x00);
        v_right = _mm256_permute2x128_si256(v, v, 0x11);
        res_add = _mm256_add_epi16(v_left, v_right);
        res_sub = _mm256_sub_epi16(v_right, v_left);
        t = fqmul_avx(res_sub, z_vec, q_vec, qinv_vec);
        v = _mm256_blend_epi32(res_add, t, 0xF0);

        v = barrett_reduce_avx(v, v_vec, q_vec);
        _mm256_store_si256((__m256i*)&r[j], v);
    }

    // --- Stage 2: Wide-span upper network (len = 16, 32, 64, 128, 256) ---
    // z_idx8 finishes exactly at 31, seamless handoff to the next loop
    int k = 31;
    for (int len = 16; len <= 256; len <<= 1) {
        for (int start = 0; start < 512; start += 2 * len) {
            __m256i z_vec = _mm256_set1_epi16(zetas[k--]);

            for (int j = start; j < start + len; j += 16) {
                __m256i a = _mm256_load_si256((__m256i*)&r[j]);
                __m256i b = _mm256_load_si256((__m256i*)&r[j + len]);

                __m256i res_add = _mm256_add_epi16(a, b);
                __m256i res_sub = _mm256_sub_epi16(b, a);

                res_add = barrett_reduce_avx(res_add, v_vec, q_vec);
                res_sub = barrett_reduce_avx(res_sub, v_vec, q_vec);

                __m256i t = fqmul_avx(res_sub, z_vec, q_vec, qinv_vec);

                _mm256_store_si256((__m256i*)&r[j], res_add);
                _mm256_store_si256((__m256i*)&r[j + len], t);
            }
        }
    }

    // --- Stage 3: INTT final scaling (multiply by inverse of N) ---
    // f = 256^-1 * 2^32 mod 7681 = 1912
    __m256i f_vec = _mm256_set1_epi16(1912);
    for (int j = 0; j < 512; j += 16) {
        __m256i v = _mm256_load_si256((__m256i*)&r[j]);
        v = fqmul_avx(v, f_vec, q_vec, qinv_vec);
        _mm256_store_si256((__m256i*)&r[j], v);
    }
}
#endif
/*************************************************
* Name:        fqmul
*
* Description: Multiplication followed by Montgomery reduction
*
* Arguments:   - int16_t a: first factor
*              - int16_t b: second factor
*
* Returns 16-bit integer congruent to a*b*R^{-1} mod q
**************************************************/
static int16_t fqmul(int16_t a, int16_t b) {
  return montgomery_reduce((int32_t)a*b);
}

/*************************************************
* Name:        ntt
*
* Description: Inplace number-theoretic transform (NTT) in Rq.
*              input is in standard order, output is in bitreversed order
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void ntt(int16_t *r) {
  unsigned int len, start, j, k;
  int16_t t, zeta;

  k = 1;
#if (COMPASS_KEM_Q == 3329) && (COMPASS_KEM_N == 256)
  for(len = COMPASS_KEM_N>>1; len >= 2; len >>= 1) {
    for(start = 0; start < COMPASS_KEM_N; start = j + len) {
      zeta = zetas[k++];
      for(j = start; j < start + len; j++) {
        t = fqmul(zeta, r[j + len]);
        r[j + len] = r[j] - t;
        r[j] = r[j] + t;
      }
    }
  }
#elif (COMPASS_KEM_Q == 7681) && (COMPASS_KEM_N == 512)
  for(len = COMPASS_KEM_N>>1; len >= 2; len >>= 1) {
    for(start = 0; start < COMPASS_KEM_N; start = j + len) {
      zeta = zetas[k++];
      for(j = start; j < start + len; j++) {
        t = fqmul(zeta, r[j + len]);
        r[j + len] = barrett_reduce(r[j] - t);
        r[j] = barrett_reduce(r[j] + t);
      }
    }
  }
#endif
}

/*************************************************
* Name:        invntt_tomont
*
* Description: Inplace inverse number-theoretic transform in Rq and
*              multiplication by Montgomery factor 2^16.
*              Input is in bitreversed order, output is in standard order
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void invntt(int16_t *r) {
  unsigned int start, len, j, k;
  int16_t t, zeta;
#if (COMPASS_KEM_Q == 3329) && (COMPASS_KEM_N == 256)
  const int16_t f = 1441; // 128^-1 * 2^32 mod 3329
  k = (COMPASS_KEM_N>>1)-1;
  for(len = 2; len <= (COMPASS_KEM_N>>1); len <<= 1) {
    for(start = 0; start < COMPASS_KEM_N; start = j + len) {
      zeta = zetas[k--];
      for(j = start; j < start + len; j++) {
        t = r[j];
        r[j] = barrett_reduce(t + r[j + len]);
        r[j + len] = r[j + len] - t;
        r[j + len] = fqmul(zeta, r[j + len]);
      }
    }
  }
#elif (COMPASS_KEM_Q == 7681) && (COMPASS_KEM_N == 512)
  const int16_t f = 1912; // 256^-1 * 2^32 mod 7681
  k = (COMPASS_KEM_N>>1)-1;
  for(len = 2; len <= (COMPASS_KEM_N>>1); len <<= 1) {
    for(start = 0; start < COMPASS_KEM_N; start = j + len) {
      zeta = zetas[k--];
      for(j = start; j < start + len; j++) {
        t = r[j];
        r[j] = barrett_reduce(t + r[j + len]);
        r[j + len] = barrett_reduce(r[j + len] - t);
        r[j + len] = fqmul(zeta, r[j + len]);
      }
    }
  }
#else
  #error "Unsupported parameters for invntt scaling factor!"
#endif

  for(j = 0; j < COMPASS_KEM_N; j++)
    r[j] = fqmul(r[j], f);
}

/*************************************************
* Name:        basemul
*
* Description: Multiplication of polynomials in Zq[X]/(X^2-zeta)
*              used for multiplication of elements in Rq in NTT domain
*
* Arguments:   - int16_t r[2]: pointer to the output polynomial
*              - const int16_t a[2]: pointer to the first factor
*              - const int16_t b[2]: pointer to the second factor
*              - int16_t zeta: integer defining the reduction polynomial
**************************************************/
void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta)
{
  r[0]  = fqmul(a[1], b[1]);
  r[0]  = fqmul(r[0], zeta);
  r[0] += fqmul(a[0], b[0]);
  r[1]  = fqmul(a[0], b[1]);
  r[1] += fqmul(a[1], b[0]);
}
