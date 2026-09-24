#include <stdint.h>

#include "params.h"
#include "ntt.h"
#include "reduce.h"

/* Precomputed zeta values for NTT */
int16_t zetas[128] = {
       1,   -16,    64,     4,    -8,   128,     2,   -32,
    -121,  -120,   -34,    30,   -60,   -68,    15,    17,
      81,   -11,    44,    67,   123,    88,   -95,   -22,
     -35,    46,    73,   117,    23,  -111,   -70,    92,
       9,   113,    62,    36,   -72,   124,    18,   -31,
     -61,   -52,   -49,    13,   -26,   -98,  -122,  -104,
     -42,   -99,  -118,    89,    79,    21,   -84,    59,
     -58,  -100,  -114,    25,   -50,    29,  -116,    57,
       3,   -48,   -65,    12,   -24,   127,     6,   -96,
    -106,  -103,  -102,    90,    77,    53,    45,    51,
     -14,   -33,  -125,   -56,   112,     7,   -28,   -66,
    -105,  -119,   -38,    94,    69,   -76,    47,    19,
      27,    82,   -71,   108,    41,   115,    54,   -93,
      74,   101,   110,    39,   -78,   -37,  -109,   -55,
    -126,   -40,   -97,    10,   -20,    63,     5,   -80,
      83,   -43,   -85,    75,   107,    87,   -91,   -86,
};


#if LORE_N == 512

/* ==================== NTT for N=512 ==================== */

#if LORE_LEVEL == 4

/*************************************************
* Reference implementation path for LORE_LEVEL == 4
**************************************************/
static void basemul4(int16_t r[4], const int16_t a[4], const int16_t b[4], int16_t zeta) {
    int32_t c[7] = {0};
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            c[i + j] += (int32_t)fqmul(a[i], b[j]);
        }
    }
    r[0] = (int16_t)(c[0] + fqmul((int16_t)c[4], zeta));
    r[1] = (int16_t)(c[1] + fqmul((int16_t)c[5], zeta));
    r[2] = (int16_t)(c[2] + fqmul((int16_t)c[6], zeta));
    r[3] = (int16_t)c[3];
}

void ntt(int16_t r[512]) {
    unsigned int len, start, j, k;
    int16_t t, zeta;
    k = 1;
    for(len = 256; len >= 4; len >>= 1) {
        for(start = 0; start < 512; start = j + len) {
            zeta = zetas[k++];
            for(j = start; j < start + len; j++) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
}

void invntt_tomont(int16_t r[512]) {
    unsigned int len, start, j, k;
    int16_t t, zeta;
    const int16_t f = 255;
    k = 127;
    for(len = 4; len <= 256; len <<= 1) {
        for(start = 0; start < 512; start = j + len) {
            zeta = zetas[k--];
            for(j = start; j < start + len; j++) {
                t = r[j];
                r[j] = barrett_reduce(t + r[j + len]);
                r[j + len] = r[j + len] - t;
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    for(j = 0; j < 512; j++) {
        r[j] = fqmul(r[j], f);
    }
}

void poly_mul_ntt(int16_t r[512], const int16_t a[512], const int16_t b[512]) {
    for(int i = 0; i < 64; i++) {
        int16_t zeta = zetas[64 + i];
        basemul4(&r[8 * i],     &a[8 * i],     &b[8 * i],     zeta);
        basemul4(&r[8 * i + 4], &a[8 * i + 4], &b[8 * i + 4], -zeta);
    }
}

#else

/*************************************************
* Optimized implementation path for LORE_LEVEL != 4
**************************************************/
static inline int16_t fqmul_scalar(int16_t a, int16_t b)
{
    return (int16_t)montgomery_reduce((int64_t)a * b);
}

static inline void butterfly_fwd(int16_t *a, int16_t *b, int16_t zeta)
{
    const int16_t u = *a;
    const int16_t t = fqmul_scalar(zeta, *b);
    *a = (int16_t)(u + t);
    *b = (int16_t)(u - t);
}

static inline void butterfly_inv(int16_t *a, int16_t *b, int16_t zeta)
{
    const int16_t u = *a;
    const int16_t v = *b;
    *a = barrett_reduce((int16_t)(u + v));
    *b = fqmul_scalar(zeta, (int16_t)(v - u));
}

static inline void basemul4(int16_t r[4],
                            const int16_t a[4],
                            const int16_t b[4],
                            int16_t zeta)
{
    const int16_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
    const int16_t b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];

    const int32_t c0 = fqmul_scalar(a0, b0);
    const int32_t c1 = fqmul_scalar(a0, b1) + fqmul_scalar(a1, b0);
    const int32_t c2 = fqmul_scalar(a0, b2) + fqmul_scalar(a1, b1) + fqmul_scalar(a2, b0);
    const int32_t c3 = fqmul_scalar(a0, b3) + fqmul_scalar(a1, b2) + fqmul_scalar(a2, b1) + fqmul_scalar(a3, b0);
    const int32_t c4 = fqmul_scalar(a1, b3) + fqmul_scalar(a2, b2) + fqmul_scalar(a3, b1);
    const int32_t c5 = fqmul_scalar(a2, b3) + fqmul_scalar(a3, b2);
    const int32_t c6 = fqmul_scalar(a3, b3);

    r[0] = (int16_t)(c0 + fqmul_scalar((int16_t)c4, zeta));
    r[1] = (int16_t)(c1 + fqmul_scalar((int16_t)c5, zeta));
    r[2] = (int16_t)(c2 + fqmul_scalar((int16_t)c6, zeta));
    r[3] = (int16_t)c3;
}

void ntt(int16_t r[512])
{
    unsigned int len, start, k = 1;

    for (len = 256; len >= 4; len >>= 1) {
        for (start = 0; start < 512; start += (len << 1)) {
            const int16_t zeta = zetas[k++];
            int16_t *lo = r + start;
            int16_t *hi = lo + len;
            int16_t *end = lo + len;

            while (lo < end) {
                butterfly_fwd(lo, hi, zeta);
                ++lo;
                ++hi;
            }
        }
    }
}

void invntt_tomont(int16_t r[512])
{
    unsigned int len, start, k = 127;
    const int16_t f = 255;

    for (len = 4; len <= 256; len <<= 1) {
        for (start = 0; start < 512; start += (len << 1)) {
            const int16_t zeta = zetas[k--];
            int16_t *lo = r + start;
            int16_t *hi = lo + len;
            int16_t *end = lo + len;

            while (lo < end) {
                butterfly_inv(lo, hi, zeta);
                ++lo;
                ++hi;
            }
        }
    }

    for (unsigned int j = 0; j < 512; ++j) {
        r[j] = fqmul_scalar(r[j], f);
    }
}

void poly_mul_ntt(int16_t r[512], const int16_t a[512], const int16_t b[512])
{
    for (unsigned int i = 0; i < 64; ++i) {
        const int16_t zeta = zetas[64 + i];
        basemul4(&r[8 * i],     &a[8 * i],     &b[8 * i],     zeta);
        basemul4(&r[8 * i + 4], &a[8 * i + 4], &b[8 * i + 4], (int16_t)-zeta);
    }
}

#endif /* LORE_LEVEL == 4 */

#elif LORE_N == 768

/* ==================== NTT for N=768 ==================== */

#if LORE_LEVEL == 4

/*************************************************
* Reference implementation path for LORE_LEVEL == 4
**************************************************/
static void basemul6(int16_t r[6], const int16_t a[6], const int16_t b[6], int16_t zeta) {
    int32_t c[11] = {0};
    for(int i = 0; i < 6; i++) {
        for(int j = 0; j < 6; j++) {
            c[i + j] += (int32_t)fqmul(a[i], b[j]);
        }
    }
    r[0] = (int16_t)(c[0] + fqmul((int16_t)c[6], zeta));
    r[1] = (int16_t)(c[1] + fqmul((int16_t)c[7], zeta));
    r[2] = (int16_t)(c[2] + fqmul((int16_t)c[8], zeta));
    r[3] = (int16_t)(c[3] + fqmul((int16_t)c[9], zeta));
    r[4] = (int16_t)(c[4] + fqmul((int16_t)c[10], zeta));
    r[5] = (int16_t)c[5];
}

void ntt(int16_t r[768]) {
    unsigned int len, start, j, k;
    int16_t t, zeta;
    k = 1;
    for(len = 384; len >= 6; len >>= 1) {
        for(start = 0; start < 768; start = j + len) {
            zeta = zetas[k++];
            for(j = start; j < start + len; j++) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
}

void invntt_tomont(int16_t r[768]) {
    unsigned int start, len, j, k;
    int16_t t, zeta;
    const int16_t f = 255;
    k = 127;
    for(len = 6; len <= 384; len <<= 1) {
        for(start = 0; start < 768; start = j + len) {
            zeta = zetas[k--];
            for(j = start; j < start + len; j++) {
                t = r[j];
                r[j] = barrett_reduce(t + r[j + len]);
                r[j + len] = r[j + len] - t;
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    for(j = 0; j < 768; j++) {
        r[j] = fqmul(r[j], f);
    }
}

void poly_mul_ntt(int16_t r[768], const int16_t a[768], const int16_t b[768]) {
    for(int i = 0; i < 64; i++) {
        int16_t zeta = zetas[64 + i];
        basemul6(&r[12 * i],     &a[12 * i],     &b[12 * i],     zeta);
        basemul6(&r[12 * i + 6], &a[12 * i + 6], &b[12 * i + 6], -zeta);
    }
}

#else

/*************************************************
* Optimized implementation path for LORE_LEVEL != 4
**************************************************/
static inline int16_t fqmul_scalar(int16_t a, int16_t b)
{
    return (int16_t)montgomery_reduce((int64_t)a * b);
}

static inline void butterfly_fwd(int16_t *a, int16_t *b, int16_t zeta)
{
    const int16_t u = *a;
    const int16_t t = fqmul_scalar(zeta, *b);
    *a = (int16_t)(u + t);
    *b = (int16_t)(u - t);
}

static inline void butterfly_inv(int16_t *a, int16_t *b, int16_t zeta)
{
    const int16_t u = *a;
    const int16_t v = *b;
    *a = barrett_reduce((int16_t)(u + v));
    *b = fqmul_scalar(zeta, (int16_t)(v - u));
}

static inline void basemul6(int16_t r[6],
                            const int16_t a[6],
                            const int16_t b[6],
                            int16_t zeta)
{
    const int16_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
    const int16_t b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5];

    const int32_t c0  = fqmul_scalar(a0, b0);
    const int32_t c1  = fqmul_scalar(a0, b1) + fqmul_scalar(a1, b0);
    const int32_t c2  = fqmul_scalar(a0, b2) + fqmul_scalar(a1, b1) + fqmul_scalar(a2, b0);
    const int32_t c3  = fqmul_scalar(a0, b3) + fqmul_scalar(a1, b2) + fqmul_scalar(a2, b1) + fqmul_scalar(a3, b0);
    const int32_t c4  = fqmul_scalar(a0, b4) + fqmul_scalar(a1, b3) + fqmul_scalar(a2, b2) + fqmul_scalar(a3, b1) + fqmul_scalar(a4, b0);
    const int32_t c5  = fqmul_scalar(a0, b5) + fqmul_scalar(a1, b4) + fqmul_scalar(a2, b3) + fqmul_scalar(a3, b2) + fqmul_scalar(a4, b1) + fqmul_scalar(a5, b0);
    const int32_t c6  = fqmul_scalar(a1, b5) + fqmul_scalar(a2, b4) + fqmul_scalar(a3, b3) + fqmul_scalar(a4, b2) + fqmul_scalar(a5, b1);
    const int32_t c7  = fqmul_scalar(a2, b5) + fqmul_scalar(a3, b4) + fqmul_scalar(a4, b3) + fqmul_scalar(a5, b2);
    const int32_t c8  = fqmul_scalar(a3, b5) + fqmul_scalar(a4, b4) + fqmul_scalar(a5, b3);
    const int32_t c9  = fqmul_scalar(a4, b5) + fqmul_scalar(a5, b4);
    const int32_t c10 = fqmul_scalar(a5, b5);

    r[0] = (int16_t)(c0 + fqmul_scalar((int16_t)c6,  zeta));
    r[1] = (int16_t)(c1 + fqmul_scalar((int16_t)c7,  zeta));
    r[2] = (int16_t)(c2 + fqmul_scalar((int16_t)c8,  zeta));
    r[3] = (int16_t)(c3 + fqmul_scalar((int16_t)c9,  zeta));
    r[4] = (int16_t)(c4 + fqmul_scalar((int16_t)c10, zeta));
    r[5] = (int16_t)c5;
}

void ntt(int16_t r[768])
{
    unsigned int len, start, k = 1;

    for (len = 384; len >= 6; len >>= 1) {
        for (start = 0; start < 768; start += (len << 1)) {
            const int16_t zeta = zetas[k++];
            int16_t *lo = r + start;
            int16_t *hi = lo + len;
            int16_t *end = lo + len;

            while (lo < end) {
                butterfly_fwd(lo, hi, zeta);
                ++lo;
                ++hi;
            }
        }
    }
}

void invntt_tomont(int16_t r[768])
{
    unsigned int len, start, k = 127;
    const int16_t f = 255;

    for (len = 6; len <= 384; len <<= 1) {
        for (start = 0; start < 768; start += (len << 1)) {
            const int16_t zeta = zetas[k--];
            int16_t *lo = r + start;
            int16_t *hi = lo + len;
            int16_t *end = lo + len;

            while (lo < end) {
                butterfly_inv(lo, hi, zeta);
                ++lo;
                ++hi;
            }
        }
    }

    for (unsigned int j = 0; j < 768; ++j) {
        r[j] = fqmul_scalar(r[j], f);
    }
}

void poly_mul_ntt(int16_t r[768], const int16_t a[768], const int16_t b[768])
{
    for (unsigned int i = 0; i < 64; ++i) {
        const int16_t zeta = zetas[64 + i];
        basemul6(&r[12 * i],     &a[12 * i],     &b[12 * i],     zeta);
        basemul6(&r[12 * i + 6], &a[12 * i + 6], &b[12 * i + 6], (int16_t)-zeta);
    }
}

#endif /* LORE_LEVEL == 4 */

#endif /* LORE_N == 512 / 768 */