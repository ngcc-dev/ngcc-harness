#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "ntt.h"

#define NTRE_R            -147
#define NTRE_RINV         -682
#define NTRE_RSQ           867
#define NTRE_QINV        12929
#define NTRE_OMEGA        1033
#define NTRE_ZMINUSZ5INV  1665
#define NTRE_NINV          882
#define NTRE_2NINV       -1693
#define NTRE_QM2         3455U

const int16_t base_zetas[288] = {
     -289,   649,  1573,  -530,   -72,  -879,  -900,  1112,
     1698,   852,   483,   279,  -510,  1552,   539, -1342,
     1429,  1169, -1151,  -944, -1211,    52,   419,   650,
     -537,   -74, -1527,  -925,  1505,  -784,  -201,   571,
     -116,  -840, -1450,  -129,  1574, -1238, -1067, -1647,
     1483,   964,  -476,  1679,   202, -1279,  -932,  -431,
      -59,   288,   991,   143,  1337,   622,  1156,   861,
     -706, -1417,  1546,  1301,  1116,  1525,   122,    49,
      208,  1387,  -857, -1676,  -319,  1147,  1198, -1219,
      321,   894, -1173,   804,  -243,  -806, -1309,   296,
    -1495,   618,   326,   811,  -516, -1114,   464,   -97,
    -1659,  -808, -1724,   271,  -198, -1553,   982,  -399,
     -969,  1566,   -13, -1167,   572,  -507,   236, -1152,
     -814, -1007,   196,  -488, -1710,   730,  -633, -1246,
     1131,  1276, -1419, -1335,   210,   -29,  -832,  1366,
      233,   972,  1184, -1678,  -241,  1235, -1284,  -119,
     -999, -1393,  -388,  1601,  -213, -1304,  -934,   985,
      702,   792, -1596,  -471,  1084,   -18,  -278,  -225,
     1064,   314,  -528,   468,  -967,   150,    12, -1582,
    -1682,  1648,  -283,  -142, -1376,  -666,    85, -1411,
      995,   768,   338,  -771, -1044,  -646,   778, -1161,
      422, -1474, -1639, -1140,  -481, -1695,  -827,  1283,
     1707,  1394, -1133,   140,  1454,   754,   890,  -946,
      856, -1073,   329, -1313,  -648,  -997, -1186,  -363,
      600,   411,   586,   -48, -1585, -1345,  -799, -1256,
      793, -1410,  1270,  -340,  -568,  1132,  -186,   322,
      873,   719, -1187,   345,   373, -1352,  -523,   385,
      134, -1533,  1675,  -149, -1103,  -358, -1688, -1018,
     -441,  1098,  -327,  -103,   560,  1075,    86,  1338,
     -531,  -865, -1452,  1287,  1662, -1316,    33,   835,
     1534,  -574, -1567,  -261,  -192,  1113,  1057, -1644,
     1071, -1185,  1288,   744, -1360, -1623,   285, -1274,
     1506, -1492,  1540, -1365,  1380,  1291,   -35,   581,
    -1432,   955,  -615,  -162,  -596,   214,  -536,  -782,
      843,  1217, -1562,  -344,  -412,  1308, -1693,  -935,
     1650,   266,  -117,  -132,  1691, -1106, -1333,     3
};

static const int16_t zetas[576] = {
     -147,   886,   460,  1265, -1510,  -460,   107,  1145,
      161,   976,  -727,  1155,   556,  1307,  1612,  -157,
     -773,  -161,  -392,    58,   450, -1078, -1569,  -491,
    -1236, -1722,  -486, -1556, -1463,    93,  -822,   298,
     1120,  -532,  -909,  -377,   566,   284, -1085,   726,
     1292,  1369,   234,   264,  -473,  -445,  -211,   737,
     -172,   781,  -391,   268,    96,  1172,   952,    99,
     1119,  -599,   353, -1020,  1664,   725, -1443, -1628,
       36, -1289,  -839,  -592, -1244,  -783, -1622,   652,
     -966,  -558, -1704,   -61,  -838, -1300,  -104,  1035,
    -1593,   862,  -899,   404, -1111, -1608,  1669,   642,
     -928,   194, -1229, -1032,  -576,  -118,  -286, -1475,
     -265,   942, -1584,  1404,  1266,  -965, -1460,    37,
      905, -1195,  -787,   619,   881, -1488, -1087, -1315,
       70, -1162,   875,  -697,  -791,    -6, -1245,   -75,
     -658,  -831, -1311,  1712,  -170,  -635,  1332,   705,
    -1536, -1467,  1542,   676,  1598,  -945,  -767,   287,
      -81, -1421,   716,  1251,  1261,  -882,   206,  -654,
     -333,   688,  1023,  1686,   -71, -1587,   841,  -824,
     1230,   324,  1547,   593,  1072,  1564,  -428, -1192,
     1343,  -169,  1231,  -384,  1148,  -389,   522,   323,
      917,   680,  -637,  1586,   372,  -644,  1193, -1136,
     -553,   883, -1727, -1062,   -66, -1670,  -825,  -133,
     1046,  -770,  -753,   746, -1438, -1711,  -690,  1083,
     -179, -1177,  -509,   844,  1654,   891,   -67,  -962,
      -24,  -293,  -300,  1523,  -628, -1329,  -936, -1056,
     1677, -1565, -1508,  -549,   669,   -43,  -280,  1191,
      365,   855,  -623, -1412,  -244,   -98,   407, -1225,
       -9,  -542,  1616,   139,  1493,   798,  -351,  -396,
     1061, -1019,  1163,  -638,   683,   416,  -105, -1714,
     -403, -1607,   148, -1074,   402, -1142,  1568,  -447,
    -1657,  1233, -1699,  -144, -1298,  -578,  1060,  -311,
    -1323,  -163,  -981,  -309,  1680,  -232,   258,   557,
     1089,  -101,  1513,   466,  -889,   238,   987,  -482,
     -472, -1153,  1014,  1144,   325,  1519, -1123,   -26,
    -1589,  1487,  -849,  -426,  -671,  1459,   255,  -776,
     -289,   649,  1573,  -530,   -72,  -879,  -900,  1112,
     1698,   852,   483,   279,  -510,  1552,   539, -1342,
     1429,  1169, -1151,  -944, -1211,    52,   419,   650,
     -537,   -74, -1527,  -925,  1505,  -784,  -201,   571,
     -116,  -840, -1450,  -129,  1574, -1238, -1067, -1647,
     1483,   964,  -476,  1679,   202, -1279,  -932,  -431,
      -59,   288,   991,   143,  1337,   622,  1156,   861,
     -706, -1417,  1546,  1301,  1116,  1525,   122,    49,
      208,  1387,  -857, -1676,  -319,  1147,  1198, -1219,
      321,   894, -1173,   804,  -243,  -806, -1309,   296,
    -1495,   618,   326,   811,  -516, -1114,   464,   -97,
    -1659,  -808, -1724,   271,  -198, -1553,   982,  -399,
     -969,  1566,   -13, -1167,   572,  -507,   236, -1152,
     -814, -1007,   196,  -488, -1710,   730,  -633, -1246,
     1131,  1276, -1419, -1335,   210,   -29,  -832,  1366,
      233,   972,  1184, -1678,  -241,  1235, -1284,  -119,
     -999, -1393,  -388,  1601,  -213, -1304,  -934,   985,
      702,   792, -1596,  -471,  1084,   -18,  -278,  -225,
     1064,   314,  -528,   468,  -967,   150,    12, -1582,
    -1682,  1648,  -283,  -142, -1376,  -666,    85, -1411,
      995,   768,   338,  -771, -1044,  -646,   778, -1161,
      422, -1474, -1639, -1140,  -481, -1695,  -827,  1283,
     1707,  1394, -1133,   140,  1454,   754,   890,  -946,
      856, -1073,   329, -1313,  -648,  -997, -1186,  -363,
      600,   411,   586,   -48, -1585, -1345,  -799, -1256,
      793, -1410,  1270,  -340,  -568,  1132,  -186,   322,
      873,   719, -1187,   345,   373, -1352,  -523,   385,
      134, -1533,  1675,  -149, -1103,  -358, -1688, -1018,
     -441,  1098,  -327,  -103,   560,  1075,    86,  1338,
     -531,  -865, -1452,  1287,  1662, -1316,    33,   835,
     1534,  -574, -1567,  -261,  -192,  1113,  1057, -1644,
     1071, -1185,  1288,   744, -1360, -1623,   285, -1274,
     1506, -1492,  1540, -1365,  1380,  1291,   -35,   581,
    -1432,   955,  -615,  -162,  -596,   214,  -536,  -782,
      843,  1217, -1562,  -344,  -412,  1308, -1693,  -935,
     1650,   266,  -117,  -132,  1691, -1106, -1333,     3
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

        for (int step = NTRE_N / 6; step >= 128; step = step / 3) {
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
                    /* step == 8: use SSE */
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

    for (int step = 64; step >= 4; step >>= 1) {
        if (step >= 16) {
            __m256i bar_v = _mm256_set1_epi16((int16_t)(((1 << 26) + NTRE_Q / 2) / NTRE_Q));
            __m256i bar_q = _mm256_set1_epi16(NTRE_Q);
            for (int start = 0; start < NTRE_N; start += (step << 1)) {
                zeta1 = zetas[k++];
                int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
                __m256i zqinv = _mm256_set1_epi16(zq);
                __m256i zeta  = _mm256_set1_epi16(zeta1);

                int i;
                for (i = start; i < start + step - 8; i += 16) {
                    __m256i lo = _mm256_loadu_si256((__m256i *)&r[i]);
                    __m256i hi = _mm256_loadu_si256((__m256i *)&r[i + step]);

                    __m256i t_lo = _mm256_mullo_epi16(hi, zqinv);
                    __m256i t_hi = _mm256_mulhi_epi16(hi, zeta);
                    t_lo = _mm256_mulhi_epi16(t_lo, bar_q);
                    __m256i t = _mm256_sub_epi16(t_hi, t_lo);

                    __m256i nl = _mm256_add_epi16(lo, t);
                    __m256i nh = _mm256_sub_epi16(lo, t);

                    __m256i bt = _mm256_mulhi_epi16(bar_v, nl);
                    bt = _mm256_srai_epi16(bt, 10);
                    bt = _mm256_mullo_epi16(bt, bar_q);
                    nl = _mm256_sub_epi16(nl, bt);

                    bt = _mm256_mulhi_epi16(bar_v, nh);
                    bt = _mm256_srai_epi16(bt, 10);
                    bt = _mm256_mullo_epi16(bt, bar_q);
                    nh = _mm256_sub_epi16(nh, bt);

                    _mm256_storeu_si256((__m256i *)&r[i], nl);
                    _mm256_storeu_si256((__m256i *)&r[i + step], nh);
                }
                for (; i < start + step; i++) {
                    t1 = fqmul(zeta1, r[i + step]);
                    r[i + step] = barrett_reduce(r[i] - t1);
                    r[i       ] = barrett_reduce(r[i] + t1);
                }
            }
        } else if (step == 8) {
            __m128i bar_v = _mm_set1_epi16((int16_t)(((1 << 26) + NTRE_Q / 2) / NTRE_Q));
            __m128i bar_q = _mm_set1_epi16(NTRE_Q);
            for (int start = 0; start < NTRE_N; start += (step << 1)) {
                zeta1 = zetas[k++];
                int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
                __m128i zq_s = _mm_set1_epi16(zq);
                __m128i zv_s = _mm_set1_epi16(zeta1);

                __m128i lo = _mm_loadu_si128((__m128i *)&r[start]);
                __m128i hi = _mm_loadu_si128((__m128i *)&r[start + 8]);

                __m128i t_lo = _mm_mullo_epi16(hi, zq_s);
                __m128i t_hi = _mm_mulhi_epi16(hi, zv_s);
                t_lo = _mm_mulhi_epi16(t_lo, bar_q);
                __m128i t = _mm_sub_epi16(t_hi, t_lo);

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

                _mm_storeu_si128((__m128i *)&r[start], nl);
                _mm_storeu_si128((__m128i *)&r[start + 8], nh);
            }
        } else {
            /* step == 4: use split trick */
            __m128i bar_v = _mm_set1_epi16((int16_t)(((1 << 26) + NTRE_Q / 2) / NTRE_Q));
            __m128i bar_q = _mm_set1_epi16(NTRE_Q);
            for (int start = 0; start < NTRE_N; start += (step << 1)) {
                zeta1 = zetas[k++];
                int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
                __m128i zq_s = _mm_set1_epi16(zq);
                __m128i zv_s = _mm_set1_epi16(zeta1);

                __m128i all = _mm_loadu_si128((__m128i *)&r[start]);
                __m128i lo = all;
                __m128i hi = _mm_bsrli_si128(all, 8);

                __m128i t_lo = _mm_mullo_epi16(hi, zq_s);
                __m128i t_hi = _mm_mulhi_epi16(hi, zv_s);
                t_lo = _mm_mulhi_epi16(t_lo, bar_q);
                __m128i t = _mm_sub_epi16(t_hi, t_lo);

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
}

void invntt(int16_t r[NTRE_N])
{
    int16_t t1, t2, t3;
    int16_t zeta1, zeta2;
    int k = 575;

    for (int step = 4; step <= 64; step <<= 1) {
        if (step >= 16) {
            __m256i bar_v = _mm256_set1_epi16((int16_t)(((1 << 26) + NTRE_Q / 2) / NTRE_Q));
            __m256i bar_q = _mm256_set1_epi16(NTRE_Q);
            for (int start = 0; start < NTRE_N; start += (step << 1)) {
                zeta1 = zetas[k--];
                int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
                __m256i zqinv = _mm256_set1_epi16(zq);
                __m256i zeta  = _mm256_set1_epi16(zeta1);

                int i;
                for (i = start; i < start + step - 8; i += 16) {
                    __m256i lo = _mm256_loadu_si256((__m256i *)&r[i]);
                    __m256i hi = _mm256_loadu_si256((__m256i *)&r[i + step]);

                    __m256i diff = _mm256_sub_epi16(hi, lo);
                    __m256i t_lo = _mm256_mullo_epi16(diff, zqinv);
                    __m256i t_hi = _mm256_mulhi_epi16(diff, zeta);
                    t_lo = _mm256_mulhi_epi16(t_lo, bar_q);
                    __m256i t = _mm256_sub_epi16(t_hi, t_lo);

                    __m256i sum = _mm256_add_epi16(lo, hi);
                    __m256i bt = _mm256_mulhi_epi16(bar_v, sum);
                    bt = _mm256_srai_epi16(bt, 10);
                    bt = _mm256_mullo_epi16(bt, bar_q);
                    __m256i nl = _mm256_sub_epi16(sum, bt);

                    _mm256_storeu_si256((__m256i *)&r[i], nl);
                    _mm256_storeu_si256((__m256i *)&r[i + step], t);
                }
                for (; i < start + step; i++) {
                    t1 = r[i + step];
                    r[i + step] = fqmul(zeta1, t1 - r[i]);
                    r[i       ] = barrett_reduce(r[i] + t1);
                }
            }
        } else if (step == 8) {
            __m128i bar_v = _mm_set1_epi16((int16_t)(((1 << 26) + NTRE_Q / 2) / NTRE_Q));
            __m128i bar_q = _mm_set1_epi16(NTRE_Q);
            for (int start = 0; start < NTRE_N; start += (step << 1)) {
                zeta1 = zetas[k--];
                int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
                __m128i zq_s = _mm_set1_epi16(zq);
                __m128i zv_s = _mm_set1_epi16(zeta1);

                __m128i lo = _mm_loadu_si128((__m128i *)&r[start]);
                __m128i hi = _mm_loadu_si128((__m128i *)&r[start + 8]);

                __m128i diff = _mm_sub_epi16(hi, lo);
                __m128i t_lo = _mm_mullo_epi16(diff, zq_s);
                __m128i t_hi = _mm_mulhi_epi16(diff, zv_s);
                t_lo = _mm_mulhi_epi16(t_lo, bar_q);
                __m128i t = _mm_sub_epi16(t_hi, t_lo);

                __m128i sum = _mm_add_epi16(lo, hi);
                __m128i bt = _mm_mulhi_epi16(bar_v, sum);
                bt = _mm_srai_epi16(bt, 10);
                bt = _mm_mullo_epi16(bt, bar_q);
                __m128i nl = _mm_sub_epi16(sum, bt);

                _mm_storeu_si128((__m128i *)&r[start], nl);
                _mm_storeu_si128((__m128i *)&r[start + 8], t);
            }
        } else {
            /* step == 4: use split trick */
            __m128i bar_v = _mm_set1_epi16((int16_t)(((1 << 26) + NTRE_Q / 2) / NTRE_Q));
            __m128i bar_q = _mm_set1_epi16(NTRE_Q);
            for (int start = 0; start < NTRE_N; start += (step << 1)) {
                zeta1 = zetas[k--];
                int16_t zq = (int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV));
                __m128i zq_s = _mm_set1_epi16(zq);
                __m128i zv_s = _mm_set1_epi16(zeta1);

                __m128i all = _mm_loadu_si128((__m128i *)&r[start]);
                __m128i lo = all;
                __m128i hi = _mm_bsrli_si128(all, 8);

                __m128i diff = _mm_sub_epi16(hi, lo);
                __m128i t_lo = _mm_mullo_epi16(diff, zq_s);
                __m128i t_hi = _mm_mulhi_epi16(diff, zv_s);
                t_lo = _mm_mulhi_epi16(t_lo, bar_q);
                __m128i t = _mm_sub_epi16(t_hi, t_lo);

                __m128i sum = _mm_add_epi16(lo, hi);
                __m128i bt = _mm_mulhi_epi16(bar_v, sum);
                bt = _mm_srai_epi16(bt, 10);
                bt = _mm_mullo_epi16(bt, bar_q);
                __m128i nl = _mm_sub_epi16(sum, bt);

                __m128i result = _mm_unpacklo_epi64(nl, t);
                _mm_storeu_si128((__m128i *)&r[start], result);
            }
        }
    }

    {
        __m256i q = _mm256_set1_epi16(NTRE_Q);
        __m256i w_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)NTRE_OMEGA * (uint16_t)NTRE_QINV)));
        __m256i w_v    = _mm256_set1_epi16(NTRE_OMEGA);
        __m256i bar_v  = _mm256_set1_epi16((int16_t)((1 << 26) / NTRE_Q));

        for (int step = 128; step <= NTRE_N / 6; step = 3 * step) {
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

                        __m256i d10 = _mm256_sub_epi16(r1, r0);
                        __m256i t1_lo = _mm256_mullo_epi16(d10, w_qinv);
                        __m256i t1_hi = _mm256_mulhi_epi16(d10, w_v);
                        t1_lo = _mm256_mulhi_epi16(t1_lo, q);
                        __m256i t1 = _mm256_sub_epi16(t1_hi, t1_lo);

                        __m256i d20 = _mm256_sub_epi16(r2, r0);
                        __m256i a2 = _mm256_add_epi16(d20, t1);
                        __m256i t2_lo = _mm256_mullo_epi16(a2, z1_qinv);
                        __m256i t2_hi = _mm256_mulhi_epi16(a2, z1_v);
                        t2_lo = _mm256_mulhi_epi16(t2_lo, q);
                        __m256i t2 = _mm256_sub_epi16(t2_hi, t2_lo);

                        __m256i d21 = _mm256_sub_epi16(r2, r1);
                        __m256i a3 = _mm256_sub_epi16(d21, t1);
                        __m256i t3_lo = _mm256_mullo_epi16(a3, z2_qinv);
                        __m256i t3_hi = _mm256_mulhi_epi16(a3, z2_v);
                        t3_lo = _mm256_mulhi_epi16(t3_lo, q);
                        __m256i t3 = _mm256_sub_epi16(t3_hi, t3_lo);

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
                    __m128i bv  = _mm_set1_epi16((int16_t)((1 << 26) / NTRE_Q));

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

            __m256i t2_lo = _mm256_mullo_epi16(diff, z5_qinv);
            __m256i t2_hi = _mm256_mulhi_epi16(diff, z5_v);
            t2_lo = _mm256_mulhi_epi16(t2_lo, q);
            __m256i t2 = _mm256_sub_epi16(t2_hi, t2_lo);

            __m256i a1 = _mm256_sub_epi16(t1, t2);
            __m256i a1_lo = _mm256_mullo_epi16(a1, nin_qinv);
            __m256i a1_hi = _mm256_mulhi_epi16(a1, nin_v);
            a1_lo = _mm256_mulhi_epi16(a1_lo, q);
            __m256i new_lo = _mm256_sub_epi16(a1_hi, a1_lo);

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
