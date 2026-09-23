#include <stdint.h>
#include "params.h"
#include "ntt.h"

#define NTRE_R            1362  /* R = 2^16 mod q */
#define NTRE_RINV         -529  /* R^-1 mod q */
#define NTRE_RSQ          -168  /* R^2 mod q */
#define NTRE_QINV        11885  /* q^-1 mod 2^16 */

#define NTRE_OMEGA         959  /* (omega * R) mod q */
#define NTRE_ZMINUSZ5INV  -121  /* (z - z^5)^-1 * R mod q */

#define NTRE_NINV         -590  /* (n/d)^-1 * R mod q */
#define NTRE_2NINV       -1180  /* 2 * (n/d)^-1 * R mod q */
#define NTRE_QM2         2915U  /* q - 2 */

const int16_t zetas[324] = {
     1362,  -596,   220, -1091,   713,  -220,  -836,  -819,
     1098,  -387,   791,   907,  -672,  -221,  1078,   729,
      580, -1098,  -108,  -801,  -556,   -90,  1135,  1432,
      489,   696,  1221,  -184,   776,  -119,   946,  1034,
    -1396,  -441,  1024,    16,   119,  -313, -1224,  -789,
      505,   108, -1106,   851,  -292,   915,  -747,  -946,
      -16, -1242,   998,   -74,  -264, -1221, -1182, -1223,
       73, -1219,   916, -1153,     4,   287,  1209,   360,
       66,   106,   306,   133,   603,   736,  -785,   476,
     1222,  -300,   349,   884,  -256,     1,   139,   359,
    -1013, -1369,   835,  -711,   424,  1447,  -194,  -919,
     1162,   880,   168, -1290,  1189,  -866,  -145,   296,
     1184,  1252,  -927,   239,  -883,  -432,   149,  -487,
      551,  -743,  1000,   867,  1382,  -174,  -880,    46,
     -533,   759,   711,  1200,  -229,  -619, -1395,    -4,
      427, -1436,  1442,  -358,  -247,   -73,  -106,  1002,
    -1410,  -969,  1168,    55,   642,  -674,    64,   547,
    -1075, -1184,  -476,   826,  -938,  -956,   897, -1189,
      692,  -942,  -867,  -958,  -250, -1222,   432, -1148,
     -693, -1440,  1294,  -424,   961,  -532,   950,   -27,
     -187,  1013,  1064,  1342,   278,  1247,    54, -1193,
      724, -1302,   891,   512, -1372,  1033, -1001,  -303,
      698,   151,  -473,  -624,  -280,   568,   848,   593,
     1214,   621,   -37,  -425,  -388,  1085,   716,  -369,
    -1204, -1058,   146,   508,   553,    45,   -92,   520,
      612, -1347,  1399,  -171,  1363,  -348,  1206,  -132,
     -649,  -517,  1238,   739,  -499,   941,     8,  -933,
     -947,  1421,  -549, -1151,   418, -1348, -1094,   -31,
     1063,   290, -1005, -1295,  -510, -1049,  -539, -1316,
      336, -1265,  -979,  -681,   298,   917,  -110, -1027,
      902,  -913,  1102, -1191,  -751,   440, -1426,  -691,
      735,   999,  -193, -1192,  1335, -1458,   124,   871,
      150,  -721, -1230,  1245,  -442,   774,  -570, -1344,
    -1160,  1103,  -654,  -877,  1279,  -761,   647,    21,
     -626, -1339,   466, -1112,   846,  -216, -1062,   421,
     -604, -1025,   869,  -346, -1215,  1087,  1212,   125,
     1365,   148, -1217,  1120,   645,  -475,   545,   978,
      433,   180,   885,   705, -1423,    53, -1441, -1018,
     1315,  -584, -1010,   382,  1392,   368,   837,   469,
     -446,   238,   684,  -815,  -847,   -32,   528,  -321,
     -849,   882,   -39,  -921
};

const int16_t base_zetas[162] = {
     1064,  1342,   278,  1247,    54, -1193,   724, -1302,
      891,   512, -1372,  1033, -1001,  -303,   698,   151,
     -473,  -624,  -280,   568,   848,   593,  1214,   621,
      -37,  -425,  -388,  1085,   716,  -369, -1204, -1058,
      146,   508,   553,    45,   -92,   520,   612, -1347,
     1399,  -171,  1363,  -348,  1206,  -132,  -649,  -517,
     1238,   739,  -499,   941,     8,  -933,  -947,  1421,
     -549, -1151,   418, -1348, -1094,   -31,  1063,   290,
    -1005, -1295,  -510, -1049,  -539, -1316,   336, -1265,
     -979,  -681,   298,   917,  -110, -1027,   902,  -913,
     1102, -1191,  -751,   440, -1426,  -691,   735,   999,
     -193, -1192,  1335, -1458,   124,   871,   150,  -721,
    -1230,  1245,  -442,   774,  -570, -1344, -1160,  1103,
     -654,  -877,  1279,  -761,   647,    21,  -626, -1339,
      466, -1112,   846,  -216, -1062,   421,  -604, -1025,
      869,  -346, -1215,  1087,  1212,   125,  1365,   148,
    -1217,  1120,   645,  -475,   545,   978,   433,   180,
      885,   705, -1423,    53, -1441, -1018,  1315,  -584,
    -1010,   382,  1392,   368,   837,   469,  -446,   238,
      684,  -815,  -847,   -32,   528,  -321,  -849,   882,
      -39,  -921
};

static inline int16_t montgomery_reduce(int32_t a)
{
    int16_t t;
    t = (int16_t)a * NTRE_QINV;
    t = (a - (int32_t)t * NTRE_Q) >> 16;
    return t;
}

static inline int16_t barrett_reduce(int16_t a)
{
    int16_t t;
    const int16_t v = ((1 << 26) + NTRE_Q / 2) / NTRE_Q;
    t  = ((int32_t)v * a + (1 << 25)) >> 26;
    t *= NTRE_Q;
    return a - t;
}

static inline int16_t fqmul(int16_t a, int16_t b)
{
    return montgomery_reduce((int32_t)a * b);
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

void ntt(int16_t r[NTRE_N])
{
    int16_t t1, t2, t3;
    int16_t zeta1, zeta2;
    int k = 1;

    zeta1 = zetas[k++];

    for (int i = 0; i < NTRE_N / 2; i++) {
        t1 = fqmul(zeta1, r[i + NTRE_N / 2]);
        r[i + NTRE_N / 2] = r[i] + r[i + NTRE_N / 2] - t1;
        r[i             ] = r[i]                     + t1;
    }

    for (int step = NTRE_N / 6; step >= 8; step = step / 3) {
        for (int start = 0; start < NTRE_N; start += 3 * step) {
            zeta1 = zetas[k++];
            zeta2 = zetas[k++];

            for (int i = start; i < start + step; i++) {
                t1 = fqmul(zeta1, r[i +     step]);
                t2 = fqmul(zeta2, r[i + 2 * step]);
                t3 = fqmul(NTRE_OMEGA, t1 - t2);

                r[i + 2 * step] = r[i] - t1 - t3;
                r[i +     step] = r[i] - t2 + t3;
                r[i           ] = r[i] + t1 + t2;
            }
        }
    }

    for (int step = 4; step >= 4; step >>= 1) {
        for (int start = 0; start < NTRE_N; start += (step << 1)) {
            zeta1 = zetas[k++];

            for (int i = start; i < start + step; i++) {
                t1 = fqmul(zeta1, r[i + step]);
                r[i + step] = barrett_reduce(r[i] - t1);
                r[i       ] = barrett_reduce(r[i] + t1);
            }
        }
    }
}

void invntt(int16_t r[NTRE_N])
{
    int16_t t1, t2, t3;
    int16_t zeta1, zeta2;
    int k = 323;

    for (int step = 4; step <= 4; step <<= 1) {
        for (int start = 0; start < NTRE_N; start += (step << 1)) {
            zeta1 = zetas[k--];

            for (int i = start; i < start + step; i++) {
                t1 = r[i + step];
                r[i + step] = fqmul(zeta1, t1 - r[i]);
                r[i       ] = barrett_reduce(r[i] + t1);
            }
        }
    }

    for (int step = 8; step <= NTRE_N / 6; step = 3 * step) {
        for (int start = 0; start < NTRE_N; start += 3 * step) {
            zeta2 = zetas[k--];
            zeta1 = zetas[k--];

            for (int i = start; i < start + step; i++) {
                t1 = fqmul(NTRE_OMEGA, r[i +     step] - r[i]);
                t2 = fqmul(zeta1, r[i + 2 * step] - r[i]        + t1);
                t3 = fqmul(zeta2, r[i + 2 * step] - r[i + step] - t1);

                r[i           ] = barrett_reduce(r[i] + r[i + step] + r[i + 2 * step]);
                r[i +     step] = t2;
                r[i + 2 * step] = t3;
            }
        }
    }

    for (int i = 0; i < NTRE_N / 2; i++) {
        t1 = r[i] + r[i + NTRE_N / 2];
        t2 = fqmul(NTRE_ZMINUSZ5INV, r[i] - r[i + NTRE_N / 2]);

        r[i             ] = fqmul(NTRE_NINV,  t1 - t2);
        r[i + NTRE_N / 2] = fqmul(NTRE_2NINV, t2);
    }
}

int baseinv(int16_t r[4], const int16_t a[4], int16_t zeta)
{
    int16_t t0, t1, t2, t3;

    t0 = montgomery_reduce(a[2] * a[2] - 2 * a[1] * a[3]);
    t1 = montgomery_reduce(a[3] * a[3]);
    t0 = montgomery_reduce(a[0] * a[0] + t0 * zeta);
    t1 = montgomery_reduce(a[1] * a[1] + t1 * zeta - 2 * a[0] * a[2]);
    t2 = montgomery_reduce(t1 * zeta);

    t3 = montgomery_reduce(t0 * t0 - t1 * t2);

    if (t3 == 0) return 1;

    r[0] = montgomery_reduce(a[0] * t0 + a[2] * t2);
    r[1] = montgomery_reduce(a[3] * t2 + a[1] * t0);
    r[2] = montgomery_reduce(a[2] * t0 + a[0] * t1);
    r[3] = montgomery_reduce(a[1] * t1 + a[3] * t0);

    t3 = fqinv(t3);

    r[0] =  montgomery_reduce(r[0] * t3);
    r[1] = -montgomery_reduce(r[1] * t3);
    r[2] =  montgomery_reduce(r[2] * t3);
    r[3] = -montgomery_reduce(r[3] * t3);

    return 0;
}

void basemul(int16_t r[4], const int16_t a[4], const int16_t b[4], int16_t zeta)
{
    r[0] = montgomery_reduce(a[1] * b[3] + a[2] * b[2] + a[3] * b[1]);
    r[1] = montgomery_reduce(a[2] * b[3] + a[3] * b[2]);
    r[2] = montgomery_reduce(a[3] * b[3]);

    r[0] = montgomery_reduce(r[0] * zeta + a[0] * b[0]);
    r[1] = montgomery_reduce(r[1] * zeta + a[0] * b[1] + a[1] * b[0]);
    r[2] = montgomery_reduce(r[2] * zeta + a[0] * b[2] + a[1] * b[1] + a[2] * b[0]);
    r[3] = montgomery_reduce(a[0] * b[3] + a[1] * b[2] + a[2] * b[1] + a[3] * b[0]);

    r[0] = montgomery_reduce(r[0] * NTRE_RSQ);
    r[1] = montgomery_reduce(r[1] * NTRE_RSQ);
    r[2] = montgomery_reduce(r[2] * NTRE_RSQ);
    r[3] = montgomery_reduce(r[3] * NTRE_RSQ);
}

void basemul_add(int16_t r[4], const int16_t a[4], const int16_t b[4], const int16_t c[4], int16_t zeta)
{
    r[0] = montgomery_reduce(a[1] * b[3] + a[2] * b[2] + a[3] * b[1]);
    r[1] = montgomery_reduce(a[2] * b[3] + a[3] * b[2]);
    r[2] = montgomery_reduce(a[3] * b[3]);

    r[0] = montgomery_reduce(r[0] * zeta + a[0] * b[0]);
    r[1] = montgomery_reduce(r[1] * zeta + a[0] * b[1] + a[1] * b[0]);
    r[2] = montgomery_reduce(r[2] * zeta + a[0] * b[2] + a[1] * b[1] + a[2] * b[0]);
    r[3] = montgomery_reduce(a[0] * b[3] + a[1] * b[2] + a[2] * b[1] + a[3] * b[0]);

    r[0] = montgomery_reduce(c[0] * NTRE_R + r[0] * NTRE_RSQ);
    r[1] = montgomery_reduce(c[1] * NTRE_R + r[1] * NTRE_RSQ);
    r[2] = montgomery_reduce(c[2] * NTRE_R + r[2] * NTRE_RSQ);
    r[3] = montgomery_reduce(c[3] * NTRE_R + r[3] * NTRE_RSQ);
}
