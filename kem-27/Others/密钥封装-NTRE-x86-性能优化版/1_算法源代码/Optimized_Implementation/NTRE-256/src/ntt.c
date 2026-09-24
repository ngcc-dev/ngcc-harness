#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "ntt.h"

#define NTRE_R            1362
#define NTRE_RINV         -529
#define NTRE_RSQ          -168
#define NTRE_QINV        11885
#define NTRE_OMEGA         959
#define NTRE_ZMINUSZ5INV  -121
#define NTRE_NINV         -590
#define NTRE_2NINV       -1180
#define NTRE_QM2         2915U

const int16_t base_zetas[162] = {
    1064, 1342,  278, 1247,   54,-1193,  724,-1302,
     891,  512,-1372, 1033,-1001, -303,  698,  151,
    -473, -624, -280,  568,  848,  593, 1214,  621,
     -37, -425, -388, 1085,  716, -369,-1204,-1058,
     146,  508,  553,   45,  -92,  520,  612,-1347,
    1399, -171, 1363, -348, 1206, -132, -649, -517,
    1238,  739, -499,  941,    8, -933, -947, 1421,
    -549,-1151,  418,-1348,-1094,  -31, 1063,  290,
   -1005,-1295, -510,-1049, -539,-1316,  336,-1265,
    -979, -681,  298,  917, -110,-1027,  902, -913,
    1102,-1191, -751,  440,-1426, -691,  735,  999,
    -193,-1192, 1335,-1458,  124,  871,  150, -721,
   -1230, 1245, -442,  774, -570,-1344,-1160, 1103,
    -654, -877, 1279, -761,  647,   21, -626,-1339,
     466,-1112,  846, -216,-1062,  421, -604,-1025,
     869, -346,-1215, 1087, 1212,  125, 1365,  148,
   -1217, 1120,  645, -475,  545,  978,  433,  180,
     885,  705,-1423,   53,-1441,-1018, 1315, -584,
   -1010,  382, 1392,  368,  837,  469, -446,  238,
     684, -815, -847,  -32,  528, -321, -849,  882,
     -39, -921
};

static const int16_t zetas[324] = {
    1362, -596, 220, -1091, 713, -220, -836, -819,
    1098, -387, 791, 907, -672, -221, 1078, 729,
    580, -1098, -108, -801, -556, -90, 1135, 1432,
    489, 696, 1221, -184, 776, -119, 946, 1034,
    -1396, -441, 1024, 16, 119, -313, -1224, -789,
    505, 108, -1106, 851, -292, 915, -747, -946,
    -16, -1242, 998, -74, -264, -1221, -1182, -1223,
    73, -1219, 916, -1153, 4, 287, 1209, 360,
    66, 106, 306, 133, 603, 736, -785, 476,
    1222, -300, 349, 884, -256, 1, 139, 359,
    -1013, -1369, 835, -711, 424, 1447, -194, -919,
    1162, 880, 168, -1290, 1189, -866, -145, 296,
    1184, 1252, -927, 239, -883, -432, 149, -487,
    551, -743, 1000, 867, 1382, -174, -880, 46,
    -533, 759, 711, 1200, -229, -619, -1395, -4,
    427, -1436, 1442, -358, -247, -73, -106, 1002,
    -1410, -969, 1168, 55, 642, -674, 64, 547,
    -1075, -1184, -476, 826, -938, -956, 897, -1189,
    692, -942, -867, -958, -250, -1222, 432, -1148,
    -693, -1440, 1294, -424, 961, -532, 950, -27,
    -187, 1013, 1064, 1342, 278, 1247, 54, -1193,
    724, -1302, 891, 512, -1372, 1033, -1001, -303,
    698, 151, -473, -624, -280, 568, 848, 593,
    1214, 621, -37, -425, -388, 1085, 716, -369,
    -1204, -1058, 146, 508, 553, 45, -92, 520,
    612, -1347, 1399, -171, 1363, -348, 1206, -132,
    -649, -517, 1238, 739, -499, 941, 8, -933,
    -947, 1421, -549, -1151, 418, -1348, -1094, -31,
    1063, 290, -1005, -1295, -510, -1049, -539, -1316,
    336, -1265, -979, -681, 298, 917, -110, -1027,
    902, -913, 1102, -1191, -751, 440, -1426, -691,
    735, 999, -193, -1192, 1335, -1458, 124, 871,
    150, -721, -1230, 1245, -442, 774, -570, -1344,
    -1160, 1103, -654, -877, 1279, -761, 647, 21,
    -626, -1339, 466, -1112, 846, -216, -1062, 421,
    -604, -1025, 869, -346, -1215, 1087, 1212, 125,
    1365, 148, -1217, 1120, 645, -475, 545, 978,
    433, 180, 885, 705, -1423, 53, -1441, -1018,
    1315, -584, -1010, 382, 1392, 368, 837, 469,
    -446, 238, 684, -815, -847, -32, 528, -321,
    -849, 882, -39, -921
};

static inline int16_t mont_reduce(int32_t a)
{
    int16_t t = (int16_t)a * NTRE_QINV;
    t = (a - (int32_t)t * NTRE_Q) >> 16;
    return t;
}

static inline int16_t barrett_reduce(int16_t a)
{
    int16_t t;
    const int16_t v = ((1 << 26) + NTRE_Q / 2) / NTRE_Q;
    t = ((int32_t)v * a + (1 << 25)) >> 26;
    t *= NTRE_Q;
    return a - t;
}

static inline int16_t fqmul(int16_t a, int16_t b)
{
    return mont_reduce((int32_t)a * b);
}

static inline int16_t fqinv(int16_t a)
{
    int16_t r = NTRE_R;
    int16_t base = fqmul(a, NTRE_RSQ);
    for (int i = 11; i >= 0; i--) {
        r = fqmul(r, r);
        if ((NTRE_QM2 >> i) & 1U)
            r = fqmul(r, base);
    }
    return fqmul(r, 1);
}

int baseinv(int16_t r[4], const int16_t a[4], int16_t zeta)
{
    int16_t t0, t1, t2, t3;

    t0 = mont_reduce((int32_t)a[2] * a[2] - 2 * (int32_t)a[1] * a[3]);
    t1 = mont_reduce((int32_t)a[3] * a[3]);
    t0 = mont_reduce((int32_t)a[0] * a[0] + (int32_t)t0 * zeta);
    t1 = mont_reduce((int32_t)a[1] * a[1] + (int32_t)t1 * zeta - 2 * (int32_t)a[0] * a[2]);
    t2 = mont_reduce((int32_t)t1 * zeta);
    t3 = mont_reduce((int32_t)t0 * t0 - (int32_t)t1 * t2);

    if (t3 == 0) return 1;

    r[0] = mont_reduce((int32_t)a[0] * t0 + (int32_t)a[2] * t2);
    r[1] = mont_reduce((int32_t)a[3] * t2 + (int32_t)a[1] * t0);
    r[2] = mont_reduce((int32_t)a[2] * t0 + (int32_t)a[0] * t1);
    r[3] = mont_reduce((int32_t)a[1] * t1 + (int32_t)a[3] * t0);

    t3 = fqinv(t3);

    r[0] =  mont_reduce((int32_t)r[0] * t3);
    r[1] = -mont_reduce((int32_t)r[1] * t3);
    r[2] =  mont_reduce((int32_t)r[2] * t3);
    r[3] = -mont_reduce((int32_t)r[3] * t3);

    return 0;
}

static int baseinv_precompute(int16_t r[4], const int16_t a[4],
                              int16_t zeta, int16_t *den)
{
    int16_t t0, t1, t2, t3;

    t0 = mont_reduce((int32_t)a[2] * a[2] - 2 * (int32_t)a[1] * a[3]);
    t1 = mont_reduce((int32_t)a[3] * a[3]);
    t0 = mont_reduce((int32_t)a[0] * a[0] + (int32_t)t0 * zeta);
    t1 = mont_reduce((int32_t)a[1] * a[1] + (int32_t)t1 * zeta - 2 * (int32_t)a[0] * a[2]);
    t2 = mont_reduce((int32_t)t1 * zeta);
    t3 = mont_reduce((int32_t)t0 * t0 - (int32_t)t1 * t2);

    r[0] = mont_reduce((int32_t)a[0] * t0 + (int32_t)a[2] * t2);
    r[1] = mont_reduce((int32_t)a[3] * t2 + (int32_t)a[1] * t0);
    r[2] = mont_reduce((int32_t)a[2] * t0 + (int32_t)a[0] * t1);
    r[3] = mont_reduce((int32_t)a[1] * t1 + (int32_t)a[3] * t0);

    *den = t3;
    return t3 == 0;
}

int baseinv_batch(int16_t r[NTRE_N], const int16_t a[NTRE_N])
{
    enum { BLOCKS = NTRE_N / 4 };
    int16_t den[BLOCKS];
    int16_t prefix[BLOCKS];
    size_t k = 0;

    for (size_t i = 0; i < NTRE_N / 8; i++) {
        if (baseinv_precompute(r + 8 * i, a + 8 * i, base_zetas[i], &den[k++]))
            return 1;
        if (baseinv_precompute(r + 8 * i + 4, a + 8 * i + 4, -base_zetas[i], &den[k++]))
            return 1;
    }

    prefix[0] = den[0];
    for (size_t i = 1; i < BLOCKS; i++)
        prefix[i] = fqmul(prefix[i - 1], den[i]);

    int16_t inv = fqinv(prefix[BLOCKS - 1]);
    for (size_t i = BLOCKS; i-- > 0;) {
        int16_t inv_i = i == 0 ? inv : fqmul(inv, prefix[i - 1]);
        int16_t *ri = r + 4 * i;

        inv = fqmul(inv, den[i]);
        ri[0] =  mont_reduce((int32_t)ri[0] * inv_i);
        ri[1] = -mont_reduce((int32_t)ri[1] * inv_i);
        ri[2] =  mont_reduce((int32_t)ri[2] * inv_i);
        ri[3] = -mont_reduce((int32_t)ri[3] * inv_i);
    }

    return 0;
}

void basemul(int16_t r[4], const int16_t a[4], const int16_t b[4], int16_t zeta)
{
    r[0] = mont_reduce((int32_t)a[1] * b[3] + (int32_t)a[2] * b[2] + (int32_t)a[3] * b[1]);
    r[1] = mont_reduce((int32_t)a[2] * b[3] + (int32_t)a[3] * b[2]);
    r[2] = mont_reduce((int32_t)a[3] * b[3]);

    r[0] = mont_reduce((int32_t)r[0] * zeta + (int32_t)a[0] * b[0]);
    r[1] = mont_reduce((int32_t)r[1] * zeta + (int32_t)a[0] * b[1] + (int32_t)a[1] * b[0]);
    r[2] = mont_reduce((int32_t)r[2] * zeta + (int32_t)a[0] * b[2] + (int32_t)a[1] * b[1] + (int32_t)a[2] * b[0]);
    r[3] = mont_reduce((int32_t)a[0] * b[3] + (int32_t)a[1] * b[2] + (int32_t)a[2] * b[1] + (int32_t)a[3] * b[0]);

    r[0] = mont_reduce((int32_t)r[0] * NTRE_RSQ);
    r[1] = mont_reduce((int32_t)r[1] * NTRE_RSQ);
    r[2] = mont_reduce((int32_t)r[2] * NTRE_RSQ);
    r[3] = mont_reduce((int32_t)r[3] * NTRE_RSQ);
}

void basemul_add(int16_t r[4], const int16_t a[4], const int16_t b[4], const int16_t c[4], int16_t zeta)
{
    r[0] = mont_reduce((int32_t)a[1] * b[3] + (int32_t)a[2] * b[2] + (int32_t)a[3] * b[1]);
    r[1] = mont_reduce((int32_t)a[2] * b[3] + (int32_t)a[3] * b[2]);
    r[2] = mont_reduce((int32_t)a[3] * b[3]);

    r[0] = mont_reduce((int32_t)r[0] * zeta + (int32_t)a[0] * b[0]);
    r[1] = mont_reduce((int32_t)r[1] * zeta + (int32_t)a[0] * b[1] + (int32_t)a[1] * b[0]);
    r[2] = mont_reduce((int32_t)r[2] * zeta + (int32_t)a[0] * b[2] + (int32_t)a[1] * b[1] + (int32_t)a[2] * b[0]);
    r[3] = mont_reduce((int32_t)a[0] * b[3] + (int32_t)a[1] * b[2] + (int32_t)a[2] * b[1] + (int32_t)a[3] * b[0]);

    r[0] = mont_reduce((int32_t)c[0] * NTRE_R + (int32_t)r[0] * NTRE_RSQ);
    r[1] = mont_reduce((int32_t)c[1] * NTRE_R + (int32_t)r[1] * NTRE_RSQ);
    r[2] = mont_reduce((int32_t)c[2] * NTRE_R + (int32_t)r[2] * NTRE_RSQ);
    r[3] = mont_reduce((int32_t)c[3] * NTRE_R + (int32_t)r[3] * NTRE_RSQ);
}

void ntt(int16_t r[NTRE_N])
{
    int16_t t1, t2, t3;
    int16_t zeta1, zeta2;
    int k = 1;

    zeta1 = zetas[k++];
    {
        int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
        __m256i zqinv = _mm256_set1_epi16(zq);
        __m256i zeta  = _mm256_set1_epi16(zeta1);
        __m256i q     = _mm256_set1_epi16(NTRE_Q);

        int i;
        for (i = 0; i < NTRE_N / 2 - 8; i += 16) {
            __m256i lo = _mm256_loadu_si256((__m256i *)&r[i]);
            __m256i hi = _mm256_loadu_si256((__m256i *)&r[i + NTRE_N / 2]);

            __m256i t_lo = _mm256_mullo_epi16(hi, zqinv);
            __m256i t_hi = _mm256_mulhi_epi16(hi, zeta);
            t_lo = _mm256_mulhi_epi16(t_lo, q);
            __m256i t = _mm256_sub_epi16(t_hi, t_lo);

            __m256i new_lo = _mm256_add_epi16(lo, t);
            __m256i new_hi = _mm256_add_epi16(lo, hi);
            new_hi = _mm256_sub_epi16(new_hi, t);

            _mm256_storeu_si256((__m256i *)&r[i], new_lo);
            _mm256_storeu_si256((__m256i *)&r[i + NTRE_N / 2], new_hi);
        }
        for (; i < NTRE_N / 2; i++) {
            int16_t t1 = fqmul(zeta1, r[i + NTRE_N / 2]);
            r[i + NTRE_N / 2] = r[i] + r[i + NTRE_N / 2] - t1;
            r[i             ] = r[i]                     + t1;
        }
    }

    {
        __m256i q    = _mm256_set1_epi16(NTRE_Q);
        __m256i w_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)NTRE_OMEGA * (uint16_t)NTRE_QINV)));
        __m256i w_v    = _mm256_set1_epi16(NTRE_OMEGA);

        for (int step = NTRE_N / 6; step >= 8; step = step / 3) {
            for (int start = 0; start < NTRE_N; start += 3 * step) {
                zeta1 = zetas[k++];
                zeta2 = zetas[k++];

                if (step >= 16) {
                    __m256i z1_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV)));
                    __m256i z1_v    = _mm256_set1_epi16(zeta1);
                    __m256i z2_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta2 * (uint16_t)NTRE_QINV)));
                    __m256i z2_v    = _mm256_set1_epi16(zeta2);

                    int i;
                    for (i = start; i < start + step - 8; i += 16) {
                        __m256i r0 = _mm256_loadu_si256((__m256i *)&r[i]);
                        __m256i r1 = _mm256_loadu_si256((__m256i *)&r[i + step]);
                        __m256i r2 = _mm256_loadu_si256((__m256i *)&r[i + 2*step]);

                        __m256i t1_lo = _mm256_mullo_epi16(r1, z1_qinv);
                        __m256i t1_hi = _mm256_mulhi_epi16(r1, z1_v);
                        t1_lo = _mm256_mulhi_epi16(t1_lo, q);
                        __m256i t1 = _mm256_sub_epi16(t1_hi, t1_lo);

                        __m256i t2_lo = _mm256_mullo_epi16(r2, z2_qinv);
                        __m256i t2_hi = _mm256_mulhi_epi16(r2, z2_v);
                        t2_lo = _mm256_mulhi_epi16(t2_lo, q);
                        __m256i t2 = _mm256_sub_epi16(t2_hi, t2_lo);

                        __m256i d12 = _mm256_sub_epi16(t1, t2);
                        __m256i t3_lo = _mm256_mullo_epi16(d12, w_qinv);
                        __m256i t3_hi = _mm256_mulhi_epi16(d12, w_v);
                        t3_lo = _mm256_mulhi_epi16(t3_lo, q);
                        __m256i t3 = _mm256_sub_epi16(t3_hi, t3_lo);

                        __m256i new_r2 = _mm256_sub_epi16(r0, t1);
                        new_r2 = _mm256_sub_epi16(new_r2, t3);
                        __m256i new_r1 = _mm256_sub_epi16(r0, t2);
                        new_r1 = _mm256_add_epi16(new_r1, t3);
                        __m256i new_r0 = _mm256_add_epi16(r0, t1);
                        new_r0 = _mm256_add_epi16(new_r0, t2);

                        _mm256_storeu_si256((__m256i *)&r[i], new_r0);
                        _mm256_storeu_si256((__m256i *)&r[i + step], new_r1);
                        _mm256_storeu_si256((__m256i *)&r[i + 2*step], new_r2);
                    }
                    for (; i < start + step; i++) {
                        t1 = fqmul(zeta1, r[i +     step]);
                        t2 = fqmul(zeta2, r[i + 2 * step]);
                        t3 = fqmul(NTRE_OMEGA, t1 - t2);
                        r[i + 2 * step] = r[i] - t1 - t3;
                        r[i +     step] = r[i] - t2 + t3;
                        r[i           ] = r[i] + t1 + t2;
                    }
                } else {
                    /* step == 8: use SSE, 8 elements per load */
                    __m128i z1q = _mm_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV)));
                    __m128i z1v = _mm_set1_epi16(zeta1);
                    __m128i z2q = _mm_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta2 * (uint16_t)NTRE_QINV)));
                    __m128i z2v = _mm_set1_epi16(zeta2);
                    __m128i sq  = _mm_set1_epi16(NTRE_Q);
                    __m128i wq  = _mm_set1_epi16((int16_t)((uint16_t)((uint16_t)NTRE_OMEGA * (uint16_t)NTRE_QINV)));
                    __m128i wv  = _mm_set1_epi16(NTRE_OMEGA);

                    __m128i r0 = _mm_loadu_si128((__m128i *)&r[start]);
                    __m128i r1 = _mm_loadu_si128((__m128i *)&r[start + 8]);
                    __m128i r2 = _mm_loadu_si128((__m128i *)&r[start + 16]);

                    __m128i t1l = _mm_mullo_epi16(r1, z1q);
                    __m128i t1h = _mm_mulhi_epi16(r1, z1v);
                    t1l = _mm_mulhi_epi16(t1l, sq);
                    __m128i t1 = _mm_sub_epi16(t1h, t1l);

                    __m128i t2l = _mm_mullo_epi16(r2, z2q);
                    __m128i t2h = _mm_mulhi_epi16(r2, z2v);
                    t2l = _mm_mulhi_epi16(t2l, sq);
                    __m128i t2 = _mm_sub_epi16(t2h, t2l);

                    __m128i d12 = _mm_sub_epi16(t1, t2);
                    __m128i t3l = _mm_mullo_epi16(d12, wq);
                    __m128i t3h = _mm_mulhi_epi16(d12, wv);
                    t3l = _mm_mulhi_epi16(t3l, sq);
                    __m128i t3 = _mm_sub_epi16(t3h, t3l);

                    __m128i nr2 = _mm_sub_epi16(r0, t1); nr2 = _mm_sub_epi16(nr2, t3);
                    __m128i nr1 = _mm_sub_epi16(r0, t2); nr1 = _mm_add_epi16(nr1, t3);
                    __m128i nr0 = _mm_add_epi16(r0, t1); nr0 = _mm_add_epi16(nr0, t2);

                    _mm_storeu_si128((__m128i *)&r[start], nr0);
                    _mm_storeu_si128((__m128i *)&r[start + 8], nr1);
                    _mm_storeu_si128((__m128i *)&r[start + 16], nr2);
                }
            }
        }
    }

    for (int step = 4; step >= 4; step >>= 1) {
        __m128i bar_v = _mm_set1_epi16(23005);
        __m128i bar_q = _mm_set1_epi16(NTRE_Q);
        for (int start = 0; start < NTRE_N; start += (step << 1)) {
            zeta1 = zetas[k++];
            int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
            __m128i zq_s = _mm_set1_epi16(zq);
            __m128i zv_s = _mm_set1_epi16(zeta1);

            __m128i all = _mm_loadu_si128((__m128i *)&r[start]);
            __m128i lo = all;
            __m128i hi = _mm_bsrli_si128(all, 8);

            /* t = fqmul(zeta, hi) */
            __m128i t_lo = _mm_mullo_epi16(hi, zq_s);
            __m128i t_hi = _mm_mulhi_epi16(hi, zv_s);
            t_lo = _mm_mulhi_epi16(t_lo, bar_q);
            __m128i t = _mm_sub_epi16(t_hi, t_lo);

            /* new_lo = barrett(lo + t), new_hi = barrett(lo - t) */
            __m128i nl = _mm_add_epi16(lo, t);
            __m128i nh = _mm_sub_epi16(lo, t);

            __m128i bt = _mm_mulhi_epi16(bar_v, nl);
            bt = _mm_srai_epi16(bt, 10);
            bt = _mm_mullo_epi16(bt, bar_q);
            nl = _mm_sub_epi16(nl, bt);

            bt = _mm_mulhi_epi16(bar_v, nh);
            bt = _mm_srai_epi16(bt, 10);
            bt = _mm_mullo_epi16(bt, bar_q);
            nh = _mm_sub_epi16(nh, bt);

            __m128i result = _mm_unpacklo_epi64(nl, nh);
            _mm_storeu_si128((__m128i *)&r[start], result);
        }
    }
}

void invntt(int16_t r[NTRE_N])
{
    int16_t t1, t2, t3;
    int16_t zeta1, zeta2;
    int k = 323;

    for (int step = 4; step <= 4; step <<= 1) {
        __m128i bar_v = _mm_set1_epi16(23005);
        __m128i bar_q = _mm_set1_epi16(NTRE_Q);
        for (int start = 0; start < NTRE_N; start += (step << 1)) {
            zeta1 = zetas[k--];
            int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
            __m128i zq_s = _mm_set1_epi16(zq);
            __m128i zv_s = _mm_set1_epi16(zeta1);

            __m128i all = _mm_loadu_si128((__m128i *)&r[start]);
            __m128i lo = all;
            __m128i hi = _mm_bsrli_si128(all, 8);

            /* t = fqmul(zeta, hi - lo) */
            __m128i diff = _mm_sub_epi16(hi, lo);
            __m128i t_lo = _mm_mullo_epi16(diff, zq_s);
            __m128i t_hi = _mm_mulhi_epi16(diff, zv_s);
            t_lo = _mm_mulhi_epi16(t_lo, bar_q);
            __m128i t = _mm_sub_epi16(t_hi, t_lo);

            /* nl = barrett(lo + hi), nh = t (fqmul result, already reduced) */
            __m128i sum = _mm_add_epi16(lo, hi);
            __m128i bt = _mm_mulhi_epi16(bar_v, sum);
            bt = _mm_srai_epi16(bt, 10);
            bt = _mm_mullo_epi16(bt, bar_q);
            __m128i nl = _mm_sub_epi16(sum, bt);

            __m128i result = _mm_unpacklo_epi64(nl, t);
            _mm_storeu_si128((__m128i *)&r[start], result);
        }
    }

    {
        __m256i q = _mm256_set1_epi16(NTRE_Q);
        __m256i w_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)NTRE_OMEGA * (uint16_t)NTRE_QINV)));
        __m256i w_v    = _mm256_set1_epi16(NTRE_OMEGA);
        __m256i bar_v  = _mm256_set1_epi16((int16_t)((1 << 26) / NTRE_Q));

        for (int step = 8; step <= NTRE_N / 6; step = 3 * step) {
            for (int start = 0; start < NTRE_N; start += 3 * step) {
                zeta2 = zetas[k--];
                zeta1 = zetas[k--];

                if (step >= 16) {
                    __m256i z1_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV)));
                    __m256i z1_v    = _mm256_set1_epi16(zeta1);
                    __m256i z2_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta2 * (uint16_t)NTRE_QINV)));
                    __m256i z2_v    = _mm256_set1_epi16(zeta2);

                    int i;
                    for (i = start; i < start + step - 8; i += 16) {
                        __m256i r0 = _mm256_loadu_si256((__m256i *)&r[i]);
                        __m256i r1 = _mm256_loadu_si256((__m256i *)&r[i + step]);
                        __m256i r2 = _mm256_loadu_si256((__m256i *)&r[i + 2*step]);

                        /* t1 = fqmul(omega, r1 - r0) */
                        __m256i d10 = _mm256_sub_epi16(r1, r0);
                        __m256i t1_lo = _mm256_mullo_epi16(d10, w_qinv);
                        __m256i t1_hi = _mm256_mulhi_epi16(d10, w_v);
                        t1_lo = _mm256_mulhi_epi16(t1_lo, q);
                        __m256i t1 = _mm256_sub_epi16(t1_hi, t1_lo);

                        /* t2 = fqmul(zeta1, r2 - r0 + t1) */
                        __m256i d20 = _mm256_sub_epi16(r2, r0);
                        __m256i a2 = _mm256_add_epi16(d20, t1);
                        __m256i t2_lo = _mm256_mullo_epi16(a2, z1_qinv);
                        __m256i t2_hi = _mm256_mulhi_epi16(a2, z1_v);
                        t2_lo = _mm256_mulhi_epi16(t2_lo, q);
                        __m256i t2 = _mm256_sub_epi16(t2_hi, t2_lo);

                        /* t3 = fqmul(zeta2, r2 - r1 - t1) */
                        __m256i d21 = _mm256_sub_epi16(r2, r1);
                        __m256i a3 = _mm256_sub_epi16(d21, t1);
                        __m256i t3_lo = _mm256_mullo_epi16(a3, z2_qinv);
                        __m256i t3_hi = _mm256_mulhi_epi16(a3, z2_v);
                        t3_lo = _mm256_mulhi_epi16(t3_lo, q);
                        __m256i t3 = _mm256_sub_epi16(t3_hi, t3_lo);

                        /* r0 = barrett_reduce(r0 + r1 + r2) */
                        __m256i sum = _mm256_add_epi16(r0, r1);
                        sum = _mm256_add_epi16(sum, r2);
                        __m256i bt = _mm256_mulhi_epi16(bar_v, sum);
                        bt = _mm256_srai_epi16(bt, 10);
                        bt = _mm256_mullo_epi16(bt, q);
                        __m256i new_r0 = _mm256_sub_epi16(sum, bt);

                        _mm256_storeu_si256((__m256i *)&r[i], new_r0);
                        _mm256_storeu_si256((__m256i *)&r[i + step], t2);
                        _mm256_storeu_si256((__m256i *)&r[i + 2*step], t3);
                    }
                    for (; i < start + step; i++) {
                        t1 = fqmul(NTRE_OMEGA, r[i + step] - r[i]);
                        t2 = fqmul(zeta1, r[i + 2*step] - r[i] + t1);
                        t3 = fqmul(zeta2, r[i + 2*step] - r[i+step] - t1);
                        r[i] = barrett_reduce(r[i] + r[i+step] + r[i+2*step]);
                        r[i+step] = t2;
                        r[i+2*step] = t3;
                    }
                } else {
                    /* step == 8: use SSE */
                    __m128i sq  = _mm_set1_epi16(NTRE_Q);
                    __m128i wq  = _mm_set1_epi16((int16_t)((uint16_t)((uint16_t)NTRE_OMEGA * (uint16_t)NTRE_QINV)));
                    __m128i wv  = _mm_set1_epi16(NTRE_OMEGA);
                    __m128i z1q = _mm_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV)));
                    __m128i z1v = _mm_set1_epi16(zeta1);
                    __m128i z2q = _mm_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta2 * (uint16_t)NTRE_QINV)));
                    __m128i z2v = _mm_set1_epi16(zeta2);
                    __m128i bv  = _mm_set1_epi16(23005);

                    __m128i r0 = _mm_loadu_si128((__m128i *)&r[start]);
                    __m128i r1 = _mm_loadu_si128((__m128i *)&r[start + 8]);
                    __m128i r2 = _mm_loadu_si128((__m128i *)&r[start + 16]);

                    __m128i d10 = _mm_sub_epi16(r1, r0);
                    __m128i t1l = _mm_mullo_epi16(d10, wq);
                    __m128i t1h = _mm_mulhi_epi16(d10, wv);
                    t1l = _mm_mulhi_epi16(t1l, sq);
                    __m128i t1 = _mm_sub_epi16(t1h, t1l);

                    __m128i d20 = _mm_sub_epi16(r2, r0);
                    __m128i a2 = _mm_add_epi16(d20, t1);
                    __m128i t2l = _mm_mullo_epi16(a2, z1q);
                    __m128i t2h = _mm_mulhi_epi16(a2, z1v);
                    t2l = _mm_mulhi_epi16(t2l, sq);
                    __m128i t2 = _mm_sub_epi16(t2h, t2l);

                    __m128i d21 = _mm_sub_epi16(r2, r1);
                    __m128i a3 = _mm_sub_epi16(d21, t1);
                    __m128i t3l = _mm_mullo_epi16(a3, z2q);
                    __m128i t3h = _mm_mulhi_epi16(a3, z2v);
                    t3l = _mm_mulhi_epi16(t3l, sq);
                    __m128i t3 = _mm_sub_epi16(t3h, t3l);

                    __m128i sum = _mm_add_epi16(r0, r1); sum = _mm_add_epi16(sum, r2);
                    __m128i bt = _mm_mulhi_epi16(bv, sum); bt = _mm_srai_epi16(bt, 10);
                    bt = _mm_mullo_epi16(bt, sq);
                    __m128i nr0 = _mm_sub_epi16(sum, bt);

                    _mm_storeu_si128((__m128i *)&r[start], nr0);
                    _mm_storeu_si128((__m128i *)&r[start + 8], t2);
                    _mm_storeu_si128((__m128i *)&r[start + 16], t3);
                }
            }
        }
    }

    {
        int16_t z5   = NTRE_ZMINUSZ5INV;
        int16_t nin  = NTRE_NINV;
        int16_t n2in = NTRE_2NINV;
        __m256i z5_qinv   = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)z5 * (uint16_t)NTRE_QINV)));
        __m256i z5_v      = _mm256_set1_epi16(z5);
        __m256i nin_qinv  = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)nin * (uint16_t)NTRE_QINV)));
        __m256i nin_v     = _mm256_set1_epi16(nin);
        __m256i n2in_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)n2in * (uint16_t)NTRE_QINV)));
        __m256i n2in_v    = _mm256_set1_epi16(n2in);
        __m256i q         = _mm256_set1_epi16(NTRE_Q);

        int i;
        for (i = 0; i < NTRE_N / 2 - 8; i += 16) {
            __m256i lo = _mm256_loadu_si256((__m256i *)&r[i]);
            __m256i hi = _mm256_loadu_si256((__m256i *)&r[i + NTRE_N / 2]);

            __m256i t1 = _mm256_add_epi16(lo, hi);
            __m256i diff = _mm256_sub_epi16(lo, hi);

            /* t2 = fqmul(z5, diff) */
            __m256i t2_lo = _mm256_mullo_epi16(diff, z5_qinv);
            __m256i t2_hi = _mm256_mulhi_epi16(diff, z5_v);
            t2_lo = _mm256_mulhi_epi16(t2_lo, q);
            __m256i t2 = _mm256_sub_epi16(t2_hi, t2_lo);

            /* r[i] = fqmul(nin, t1 - t2) */
            __m256i a1 = _mm256_sub_epi16(t1, t2);
            __m256i a1_lo = _mm256_mullo_epi16(a1, nin_qinv);
            __m256i a1_hi = _mm256_mulhi_epi16(a1, nin_v);
            a1_lo = _mm256_mulhi_epi16(a1_lo, q);
            __m256i new_lo = _mm256_sub_epi16(a1_hi, a1_lo);

            /* r[i+N/2] = fqmul(n2in, t2) */
            __m256i a2_lo = _mm256_mullo_epi16(t2, n2in_qinv);
            __m256i a2_hi = _mm256_mulhi_epi16(t2, n2in_v);
            a2_lo = _mm256_mulhi_epi16(a2_lo, q);
            __m256i new_hi = _mm256_sub_epi16(a2_hi, a2_lo);

            _mm256_storeu_si256((__m256i *)&r[i], new_lo);
            _mm256_storeu_si256((__m256i *)&r[i + NTRE_N / 2], new_hi);
        }
        for (; i < NTRE_N / 2; i++) {
            int16_t t1 = r[i] + r[i + NTRE_N / 2];
            int16_t t2 = fqmul(NTRE_ZMINUSZ5INV, r[i] - r[i + NTRE_N / 2]);
            r[i             ] = fqmul(NTRE_NINV,  t1 - t2);
            r[i + NTRE_N / 2] = fqmul(NTRE_2NINV, t2);
        }
    }
}
