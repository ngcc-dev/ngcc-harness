#include <stdint.h>
#include "params.h"
#include "ntt.h"

#define NTRE_R             711 /* R = 2^16 mod q */
#define NTRE_RINV           62 /* R^-1 mod q */
#define NTRE_RSQ          -114 /* R^2 mod q */
#define NTRE_QINV        -1567 /* q^-1 mod 2^16 */

#define NTRE_OMEGA       -609 /* (omega * R) mod q */
#define NTRE_ZMINUSZ5INV  169 /* (z - z^5)^-1 * R mod q */

#define NTRE_NINV        -753 /* (n/d)^-1 * R mod q */
#define NTRE_2NINV       1087 /* 2 * (n/d)^-1 * R mod q */
#define NTRE_QM2        2591U /* q - 2 */

const int16_t zetas[216] = {
      711,   102, -1012,  -456,  -328,  1012,  -314,  1251,
    -1024,    16,  -317,  -661,   -41,   502,    81,  -319,
     -413,  1024,   912,  1137,   728,   512,  1218,  -408,
     -496,   966,   -32,  1256,  -253,  1268, -1004,   506,
     -483,   164,   638,  -941, -1268,  -204,   671, -1296,
     -124,  -912,  1281,   634,    -1,    62,  -251,  1004,
      941,   826,   228,   -91,   182,    32,   260,   278,
       18,  -877,   279,  1156,   435,  -233,  -668,  -144,
     -513,  -369,  1124,   763,  -361,   158,   887,   729,
      359, -1152,  1082,   295,  1213,   918,  -646,  1264,
     -683,  1116,  -562,   915,  -932,   -79,   853,    72,
    -1040, -1112,   459,  1149,   690,   955,   323,  -632,
     1117,   576,  -541,  -334,  1079, -1180,  -130,  -139,
       -9,  -858,  1157,  -578,   917,   919,    76,   243,
     1160,   843,  1033,   744,  -610,  -108,   925, -1239,
     -171, -1195,   -17,    48,  -123, -1178,  1088,  -479,
       93,   195,  1283,  -572,   957,   501,   823,  -951,
        6,  -322,   439, -1087,  -942,  1287,  -867,  -145,
      649,   608,  1035, -1092,  -443,  -427,   864,   306,
     -460,  -379,   485,   766,  -384,  -136,  -948,  -984,
     1225,   812,  1220,   216,   743,  -115,  1105,  -527,
       34,   -96,   246,  -237,  -203,  -342,   273,   907,
      755,  -759,  -486,   152,   947,  -691,   -12,   644,
    -1002,  -679,  -709,    19,  -859,   290,  -419,   878,
       27,  1144,   958,  -417,  -390,   186,   920,   758,
     -970,  1061,  -612,  -865,  -697,  -625,   143,   969,
      272,  -768,   886,   854, -1216, -1295,  -409,  -523
};

const int16_t base_zetas[108] = {
      917,   919,    76,   243,  1160,   843,  1033,   744,
     -610,  -108,   925, -1239,  -171, -1195,   -17,    48,
     -123, -1178,  1088,  -479,    93,   195,  1283,  -572,
      957,   501,   823,  -951,     6,  -322,   439, -1087,
     -942,  1287,  -867,  -145,   649,   608,  1035, -1092,
     -443,  -427,   864,   306,  -460,  -379,   485,   766,
     -384,  -136,  -948,  -984,  1225,   812,  1220,   216,
      743,  -115,  1105,  -527,    34,   -96,   246,  -237,
     -203,  -342,   273,   907,   755,  -759,  -486,   152,
      947,  -691,   -12,   644, -1002,  -679,  -709,    19,
     -859,   290,  -419,   878,    27,  1144,   958,  -417,
     -390,   186,   920,   758,  -970,  1061,  -612,  -865,
     -697,  -625,   143,   969,   272,  -768,   886,   854,
    -1216, -1295,  -409,  -523
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

    for (int step = NTRE_N / 6; step >= 12; step = step / 3) {
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

    for (int step = 6; step >= 3; step >>= 1) {
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
    int k = 215;

    for (int step = 3; step <= 6; step <<= 1) {
        for (int start = 0; start < NTRE_N; start += (step << 1)) {
            zeta1 = zetas[k--];

            for (int i = start; i < start + step; i++) {
                t1 = r[i + step];
                r[i + step] = fqmul(zeta1, t1 - r[i]);
                r[i       ] = barrett_reduce(r[i] + t1);
            }
        }
    }

    for (int step = 12; step <= NTRE_N / 6; step = 3 * step) {
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

int baseinv(int16_t r[3], const int16_t a[3], int16_t zeta)
{
    int16_t t;

    r[0] = montgomery_reduce(a[1] * a[2]);
    r[1] = montgomery_reduce(a[2] * a[2]);
    r[2] = montgomery_reduce(a[1] * a[1] - a[0] * a[2]);
    r[0] = montgomery_reduce(a[0] * a[0] - r[0] * zeta);
    r[1] = montgomery_reduce(r[1] * zeta - a[0] * a[1]);

    t    = montgomery_reduce(r[2] * a[1] + r[1] * a[2]);
    t    = montgomery_reduce(t * zeta + r[0] * a[0]);

    if (t == 0) return 1;

    t = fqinv(t);

    r[0] = montgomery_reduce(r[0] * t);
    r[1] = montgomery_reduce(r[1] * t);
    r[2] = montgomery_reduce(r[2] * t);

    return 0;
}

void basemul(int16_t r[3], const int16_t a[3], const int16_t b[3], int16_t zeta)
{
    r[0] = montgomery_reduce(a[2] * b[1] + a[1] * b[2]);
    r[1] = montgomery_reduce(a[2] * b[2]);

    r[0] = montgomery_reduce(r[0] * zeta + a[0] * b[0]);
    r[1] = montgomery_reduce(r[1] * zeta + a[0] * b[1] + a[1] * b[0]);
    r[2] = montgomery_reduce(a[2] * b[0] + a[1] * b[1] + a[0] * b[2]);

    r[0] = montgomery_reduce(r[0] * NTRE_RSQ);
    r[1] = montgomery_reduce(r[1] * NTRE_RSQ);
    r[2] = montgomery_reduce(r[2] * NTRE_RSQ);
}

void basemul_add(int16_t r[3], const int16_t a[3], const int16_t b[3], const int16_t c[3], int16_t zeta)
{
    r[0] = montgomery_reduce(a[2] * b[1] + a[1] * b[2]);
    r[1] = montgomery_reduce(a[2] * b[2]);

    r[0] = montgomery_reduce(r[0] * zeta + a[0] * b[0]);
    r[1] = montgomery_reduce(r[1] * zeta + a[0] * b[1] + a[1] * b[0]);
    r[2] = montgomery_reduce(a[2] * b[0] + a[1] * b[1] + a[0] * b[2]);

    r[0] = montgomery_reduce(c[0] * NTRE_R + r[0] * NTRE_RSQ);
    r[1] = montgomery_reduce(c[1] * NTRE_R + r[1] * NTRE_RSQ);
    r[2] = montgomery_reduce(c[2] * NTRE_R + r[2] * NTRE_RSQ);
}
