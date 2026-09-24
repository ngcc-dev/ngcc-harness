#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "ntt.h"

#define NTRE_R             711
#define NTRE_RINV           62
#define NTRE_RSQ          -114
#define NTRE_QINV        -1567
#define NTRE_OMEGA        -609
#define NTRE_ZMINUSZ5INV   169
#define NTRE_NINV         -753
#define NTRE_2NINV        1087
#define NTRE_QM2         2591U

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

static const int16_t zetas[216] = {
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

int baseinv(int16_t r[3], const int16_t a[3], int16_t zeta)
{
    int16_t t;

    r[0] = mont_reduce((int32_t)a[1] * a[2]);
    r[1] = mont_reduce((int32_t)a[2] * a[2]);
    r[2] = mont_reduce((int32_t)a[1] * a[1] - (int32_t)a[0] * a[2]);
    r[0] = mont_reduce((int32_t)a[0] * a[0] - (int32_t)r[0] * zeta);
    r[1] = mont_reduce((int32_t)r[1] * zeta - (int32_t)a[0] * a[1]);

    t    = mont_reduce((int32_t)r[2] * a[1] + (int32_t)r[1] * a[2]);
    t    = mont_reduce((int32_t)t * zeta + (int32_t)r[0] * a[0]);

    if (t == 0) return 1;

    t = fqinv(t);

    r[0] = mont_reduce((int32_t)r[0] * t);
    r[1] = mont_reduce((int32_t)r[1] * t);
    r[2] = mont_reduce((int32_t)r[2] * t);

    return 0;
}

static int baseinv_precompute(int16_t r[3], const int16_t a[3],
                              int16_t zeta, int16_t *den)
{
    int16_t t;

    r[0] = mont_reduce((int32_t)a[1] * a[2]);
    r[1] = mont_reduce((int32_t)a[2] * a[2]);
    r[2] = mont_reduce((int32_t)a[1] * a[1] - (int32_t)a[0] * a[2]);
    r[0] = mont_reduce((int32_t)a[0] * a[0] - (int32_t)r[0] * zeta);
    r[1] = mont_reduce((int32_t)r[1] * zeta - (int32_t)a[0] * a[1]);

    t    = mont_reduce((int32_t)r[2] * a[1] + (int32_t)r[1] * a[2]);
    t    = mont_reduce((int32_t)t * zeta + (int32_t)r[0] * a[0]);

    *den = t;
    return t == 0;
}

int baseinv_batch(int16_t r[NTRE_N], const int16_t a[NTRE_N])
{
    enum { BLOCKS = NTRE_N / 3 };
    int16_t den[BLOCKS];
    int16_t prefix[BLOCKS];
    size_t k = 0;

    for (size_t i = 0; i < NTRE_N / 6; i++) {
        if (baseinv_precompute(r + 6 * i, a + 6 * i, base_zetas[i], &den[k++]))
            return 1;
        if (baseinv_precompute(r + 6 * i + 3, a + 6 * i + 3, -base_zetas[i], &den[k++]))
            return 1;
    }

    prefix[0] = den[0];
    for (size_t i = 1; i < BLOCKS; i++)
        prefix[i] = fqmul(prefix[i - 1], den[i]);

    int16_t inv = fqinv(prefix[BLOCKS - 1]);
    for (size_t i = BLOCKS; i-- > 0;) {
        int16_t inv_i = i == 0 ? inv : fqmul(inv, prefix[i - 1]);
        int16_t *ri = r + 3 * i;

        inv = fqmul(inv, den[i]);
        ri[0] = mont_reduce((int32_t)ri[0] * inv_i);
        ri[1] = mont_reduce((int32_t)ri[1] * inv_i);
        ri[2] = mont_reduce((int32_t)ri[2] * inv_i);
    }

    return 0;
}

void basemul(int16_t r[3], const int16_t a[3], const int16_t b[3], int16_t zeta)
{
    r[0] = mont_reduce((int32_t)a[2] * b[1] + (int32_t)a[1] * b[2]);
    r[1] = mont_reduce((int32_t)a[2] * b[2]);

    r[0] = mont_reduce((int32_t)r[0] * zeta + (int32_t)a[0] * b[0]);
    r[1] = mont_reduce((int32_t)r[1] * zeta + (int32_t)a[0] * b[1] + (int32_t)a[1] * b[0]);
    r[2] = mont_reduce((int32_t)a[2] * b[0] + (int32_t)a[1] * b[1] + (int32_t)a[0] * b[2]);

    r[0] = mont_reduce((int32_t)r[0] * NTRE_RSQ);
    r[1] = mont_reduce((int32_t)r[1] * NTRE_RSQ);
    r[2] = mont_reduce((int32_t)r[2] * NTRE_RSQ);
}

void basemul_add(int16_t r[3], const int16_t a[3], const int16_t b[3], const int16_t c[3], int16_t zeta)
{
    r[0] = mont_reduce((int32_t)a[2] * b[1] + (int32_t)a[1] * b[2]);
    r[1] = mont_reduce((int32_t)a[2] * b[2]);

    r[0] = mont_reduce((int32_t)r[0] * zeta + (int32_t)a[0] * b[0]);
    r[1] = mont_reduce((int32_t)r[1] * zeta + (int32_t)a[0] * b[1] + (int32_t)a[1] * b[0]);
    r[2] = mont_reduce((int32_t)a[2] * b[0] + (int32_t)a[1] * b[1] + (int32_t)a[0] * b[2]);

    r[0] = mont_reduce((int32_t)c[0] * NTRE_R + (int32_t)r[0] * NTRE_RSQ);
    r[1] = mont_reduce((int32_t)c[1] * NTRE_R + (int32_t)r[1] * NTRE_RSQ);
    r[2] = mont_reduce((int32_t)c[2] * NTRE_R + (int32_t)r[2] * NTRE_RSQ);
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
        for (i = 0; i + 16 <= NTRE_N / 2; i += 16) {
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
        __m256i w_qinv = _mm256_set1_epi16((int16_t)((uint32_t)(uint16_t)NTRE_OMEGA * (uint32_t)(uint16_t)NTRE_QINV));
        __m256i w_v    = _mm256_set1_epi16(NTRE_OMEGA);

        for (int step = NTRE_N / 6; step >= 12; step = step / 3) {
            for (int start = 0; start < NTRE_N; start += 3 * step) {
                zeta1 = zetas[k++];
                zeta2 = zetas[k++];

                if (step >= 16) {
                    __m256i z1_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV)));
                    __m256i z1_v    = _mm256_set1_epi16(zeta1);
                    __m256i z2_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta2 * (uint16_t)NTRE_QINV)));
                    __m256i z2_v    = _mm256_set1_epi16(zeta2);

                    int i;
                    for (i = start; i + 16 <= start + step; i += 16) {
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
                    /* step == 12: scalar */
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

    {
        __m256i q = _mm256_set1_epi16(NTRE_Q);
        __m256i w_qinv = _mm256_set1_epi16((int16_t)((uint32_t)(uint16_t)NTRE_OMEGA * (uint32_t)(uint16_t)NTRE_QINV));
        __m256i w_v    = _mm256_set1_epi16(NTRE_OMEGA);
        __m256i bar_v  = _mm256_set1_epi16((int16_t)((1 << 26) / NTRE_Q));

        for (int step = 12; step <= NTRE_N / 6; step = 3 * step) {
            for (int start = 0; start < NTRE_N; start += 3 * step) {
                zeta2 = zetas[k--];
                zeta1 = zetas[k--];

                if (step >= 16) {
                    __m256i z1_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta1 * (uint16_t)NTRE_QINV)));
                    __m256i z1_v    = _mm256_set1_epi16(zeta1);
                    __m256i z2_qinv = _mm256_set1_epi16((int16_t)((uint16_t)((uint16_t)zeta2 * (uint16_t)NTRE_QINV)));
                    __m256i z2_v    = _mm256_set1_epi16(zeta2);

                    int i;
                    for (i = start; i + 16 <= start + step; i += 16) {
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
                    /* step == 12: scalar */
                    for (int i = start; i < start + step; i++) {
                        t1 = fqmul(NTRE_OMEGA, r[i + step] - r[i]);
                        t2 = fqmul(zeta1, r[i + 2*step] - r[i] + t1);
                        t3 = fqmul(zeta2, r[i + 2*step] - r[i+step] - t1);
                        r[i] = barrett_reduce(r[i] + r[i+step] + r[i+2*step]);
                        r[i+step] = t2;
                        r[i+2*step] = t3;
                    }
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
        for (i = 0; i + 16 <= NTRE_N / 2; i += 16) {
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
