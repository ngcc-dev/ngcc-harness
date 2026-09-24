#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "params.h"
#include "inverse.h"
#include "reduce.h"
#include "ntt.h"

int16_t zetas_base_nega[128] = {
  3447, 10, 3291, 166, 1382, 2075, 125, 3332, 440, 3017, 390, 3067, 1418, 2039, 1414, 2043, 812, 2645, 1034, 2423, 2554, 903, 221, 3236, 2299, 1158, 2902, 555, 1705, 1752, 647, 2810, 1101, 2356, 1683, 1774, 2024, 1433, 1794, 1663, 3411, 46, 2002, 1455, 826, 2631, 575, 2882, 2555, 902, 929, 2528, 2970, 487, 904, 2553, 1661, 1796, 608, 2849, 686, 2771, 1708, 1749, 253, 3204, 2817, 640, 2371, 1086, 2023, 1434, 2696, 761, 504, 2953, 2843, 614, 870, 2587, 2964, 493, 113, 3344, 3141, 316, 977, 2480, 950, 2507, 1942, 1515, 76, 3381, 1953, 1504, 2912, 545, 1324, 2133, 2722, 735, 1627, 1830, 3238, 219, 513, 2944, 1227, 2230, 1009, 2448, 2770, 687, 1041, 2416, 913, 2544, 3402, 55, 2572, 885, 2594, 863, 1312, 2145, 2420, 1037
};

/* Based on the reference implementation of NTRU Prime (NIST 3rd round submission)
 * by Daniel J. Bernstein, Chitchanok Chuengsatiansup, Tanja Lange, Christine van Vredendaal.
 * It can be used for q = 641.
 * */

void uint32_divmod_uint14(uint32_t *y, uint16_t *r, uint32_t x, uint16_t m)
{
    uint32_t w = 0x80000000;
    uint32_t qpart;
    uint32_t mask;

    w /= m;

    *y = 0;
    qpart = (x * (uint64_t)w) >> 31;
    x -= qpart * m;
    *y += qpart;

    qpart = (x * (uint64_t)w) >> 31;
    x -= qpart * m;
    *y += qpart;

    x -= m;
    *y += 1;
    mask = -(x >> 31);
    x += mask & (uint32_t)m;
    *y += mask;

    *r = x;
}

void int32_divmod_uint14(int32_t *y, uint16_t *r, int32_t x, uint16_t m)
{
    uint32_t uq, uq2;
    uint16_t ur, ur2;
    uint32_t mask;

    uint32_divmod_uint14(&uq, &ur, 0x80000000 + (uint32_t)x, m);
    uint32_divmod_uint14(&uq2, &ur2, 0x80000000, m);

    ur -= ur2;
    uq -= uq2;

    mask = -(uint32_t)(ur >> 15);
    ur += mask & m;
    uq += mask;
    *r = ur;
    *y = uq;
}

uint16_t int32_mod_uint14(int32_t x, uint16_t m)
{
    int32_t y;
    uint16_t r;
    int32_divmod_uint14(&y, &r, x, m);
    return r;
}

int16_t fq_freeze(int32_t x)
{

    const int16_t half_q = (DTRU_Q - 1) >> 1;
    return int32_mod_uint14(x + half_q, DTRU_Q) - half_q;
}

int int16_nonzero_mask(int16_t x)
{
    uint16_t u = x;
    uint32_t w = u;
    w = -w;
    w >>= 31;
    return -w;
}

int int16_negative_mask(int16_t x)
{
    uint16_t u = x;
    u >>= 15;
    return -(int)u;
}

int rq_inverse(int16_t finv[ROOT_DIMENSION], const int16_t f[ROOT_DIMENSION], const int16_t zeta)
{
    int16_t Phi[ROOT_DIMENSION + 1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, zeta};
    int16_t V[ROOT_DIMENSION + 1] = {0};
    int16_t S[ROOT_DIMENSION + 1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int16_t F[ROOT_DIMENSION + 1] = {f[15], f[14], f[13], f[12], f[11], f[10], f[9], f[8], f[7], f[6], f[5], f[4], f[3], f[2], f[1], f[0], 0};
    int i, loop, swap, t;
    int Delta = 1;
    int32_t Phi0, F0;
    int16_t scale;

    for (loop = 0; loop < 2 * ROOT_DIMENSION - 1; ++loop)
    {
        for (i = ROOT_DIMENSION; i > 0; --i)
            V[i] = V[i - 1];
        V[0] = 0;

        swap = int16_negative_mask(-Delta) & int16_nonzero_mask(F[0]);

        for (i = 0; i < ROOT_DIMENSION + 1; ++i)
        {
            t = swap & (Phi[i] ^ F[i]);
            Phi[i] ^= t;
            F[i] ^= t;
            t = swap & (V[i] ^ S[i]);
            V[i] ^= t;
            S[i] ^= t;
        }

        Delta ^= swap & (Delta ^ -Delta);
        Delta++;

        Phi0 = Phi[0];
        F0 = F[0];

        for (i = 0; i < ROOT_DIMENSION + 1; ++i)
            F[i] = fq_freeze(Phi0 * F[i] - F0 * Phi[i]);
        for (i = 0; i < ROOT_DIMENSION; ++i)
            F[i] = F[i + 1];
        F[ROOT_DIMENSION] = 0;

        for (i = 0; i < ROOT_DIMENSION + 1; ++i)
            S[i] = fq_freeze(Phi0 * S[i] - F0 * V[i]);
    }

    scale = Phi[0];
    scale += (scale >> 15) & DTRU_Q;
    scale = fqinv(scale);
    for (i = 0; i < ROOT_DIMENSION; ++i)
        finv[i] = fq_freeze(scale * (int32_t)V[ROOT_DIMENSION - 1 - i]);

    return int16_nonzero_mask(Delta);
}

#define CAL_S(a, i, j) fqmul(a[i], a[j])
#define CAL_D(a, b, x, y, d) (fqmul((a[x] + a[y]), (b[x] + b[y])) - d[x] - d[y])

static void basemul4(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta)
{
  int16_t d[4];
  for (int i = 0; i < 4; i++)
    d[i] = fqmul(a[i], b[i]);

  c[0] = barrett_reduce(d[0] + fqmul((CAL_D(a, b, 1, 3, d) + d[2]), zeta));
  c[1] = barrett_reduce(CAL_D(a, b, 0, 1, d) + fqmul(CAL_D(a, b, 2, 3, d), zeta));
  c[2] = barrett_reduce(CAL_D(a, b, 0, 2, d) + d[1] + fqmul(d[3], zeta));
  c[3] = barrett_reduce(CAL_D(a, b, 0, 3, d) + CAL_D(a, b, 1, 2, d));
}

static void basemul8(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta)
{
  int16_t d[8];
  for (int i = 0; i < 8; i++)
    d[i] = fqmul(a[i], b[i]);

  c[0] = barrett_reduce(d[0] + fqmul((CAL_D(a, b, 1, 7, d) + CAL_D(a, b, 2, 6, d) + CAL_D(a, b, 3, 5, d) + d[4]), zeta));
  c[1] = barrett_reduce(CAL_D(a, b, 0, 1, d) + fqmul((CAL_D(a, b, 2, 7, d) + CAL_D(a, b, 3, 6, d) + CAL_D(a, b, 4, 5, d)), zeta));
  c[2] = barrett_reduce(CAL_D(a, b, 0, 2, d) + d[1] + fqmul((CAL_D(a, b, 3, 7, d) + CAL_D(a, b, 4, 6, d) + d[5]), zeta));
  c[3] = barrett_reduce(CAL_D(a, b, 0, 3, d) + CAL_D(a, b, 1, 2, d) + fqmul((CAL_D(a, b, 4, 7, d) + CAL_D(a, b, 5, 6, d)), zeta));
  c[4] = barrett_reduce(CAL_D(a, b, 0, 4, d) + CAL_D(a, b, 1, 3, d) + d[2] + fqmul((CAL_D(a, b, 5, 7, d) + d[6]), zeta));
  c[5] = barrett_reduce(CAL_D(a, b, 0, 5, d) + CAL_D(a, b, 1, 4, d) + CAL_D(a, b, 2, 3, d)) + fqmul(CAL_D(a, b, 6, 7, d), zeta);
  c[6] = barrett_reduce(CAL_D(a, b, 0, 6, d) + CAL_D(a, b, 1, 5, d) + CAL_D(a, b, 2, 4, d)) + d[3] + fqmul(d[7], zeta);
  c[7] = barrett_reduce(CAL_D(a, b, 0, 7, d) + CAL_D(a, b, 1, 6, d) + CAL_D(a, b, 2, 5, d) + CAL_D(a, b, 3, 4, d));
}

int baseinv_opt(int16_t r[4], const int16_t a[4], int16_t zeta)
{
	int16_t t0, t1, t2, t3;
	
	t0 = montgomery_reduce((int32_t)a[2]*a[2] - 2*(int32_t)a[1]*a[3]);                     // R^-1
	t1 = montgomery_reduce((int32_t)a[3]*a[3]);                                            // R^-1
	t0 = montgomery_reduce((int32_t)a[0]*a[0] + (int32_t)t0*zeta);                         // R^-1
	t1 = montgomery_reduce((int32_t)a[1]*a[1] + (int32_t)t1*zeta - 2*(int32_t)a[0]*a[2]);  // R^-1
	t2 = montgomery_reduce((int32_t)t1*zeta);                                              // R^-1
	
	t3 = montgomery_reduce((int32_t)t0*t0 - (int32_t)t1*t2);  // R^-3

	if (t3 == 0) return 1;

	r[0] = montgomery_reduce((int32_t)a[0]*t0 + (int32_t)a[2]*t2); // R^-2
	r[1] = montgomery_reduce((int32_t)a[3]*t2 + (int32_t)a[1]*t0); // R^-2
	r[2] = montgomery_reduce((int32_t)a[2]*t0 + (int32_t)a[0]*t1); // R^-2
	r[3] = montgomery_reduce((int32_t)a[1]*t1 + (int32_t)a[3]*t0); // R^-2

	t3 = fqinv(t3); // R^3

	r[0] =  montgomery_reduce((int32_t)r[0]*t3); // R^0
	r[1] = -montgomery_reduce((int32_t)r[1]*t3); // R^0
	r[2] =  montgomery_reduce((int32_t)r[2]*t3); // R^0
	r[3] = -montgomery_reduce((int32_t)r[3]*t3); // R^0

	return 0;
}

static int rq_inverse_recursive8(int16_t b[8], const int16_t a[8], const int16_t zeta)
{
    int16_t a0[4], a1[4], c0[4], c1[4], b0[4], b1[4];
    unsigned int i;
    int r;

    a0[0] = CAL_S(a,0,0)+fqmul(zeta,2*CAL_S(a,2,6)+CAL_S(a,4,4)-2*CAL_S(a,1,7)-2*CAL_S(a,3,5));
    a0[1] = 2*CAL_S(a,0,2)-CAL_S(a,1,1)+fqmul(zeta,2*CAL_S(a,4,6)-2*CAL_S(a,3,7)-CAL_S(a,5,5));
    a0[2] = 2*CAL_S(a,0,4)+CAL_S(a,2,2)-2*CAL_S(a,1,3)+fqmul(zeta,CAL_S(a,6,6)-2*CAL_S(a,5,7));
    a0[3] = 2*CAL_S(a,0,6)+2*CAL_S(a,2,4)-CAL_S(a,3,3)-2*CAL_S(a,1,5)-fqmul(zeta,CAL_S(a,7,7));

    for (i = 0; i < 4; i++)
        a0[i] = barrett_reduce(a0[i]);

    r = baseinv_opt(a1, a0, zeta); // a1 = a0^{-1} mod (x^4 - zeta) with mont

    for (i = 0; i < 4; i++)
    {
        c0[i] = a[2 * i];
        c1[i] = -a[2 * i + 1];
    }
    
    basemul4(b0, c0, a1, zeta);
    basemul4(b1, c1, a1, zeta);

    for (i = 0; i < 4; i++)
    {
        b[2 * i] = b0[i];
        b[2 * i + 1] = b1[i];
    }

    return r;
}

int rq_inverse_recursive16(int16_t b[16], const int16_t a[16], const int16_t zeta)
{
    int16_t d[16], a0[8], a1[8], c0[8], c1[8], b0[8], b1[8];
    unsigned int i;
    int r;

    for (i = 0; i < 8; i++)
    {
        d[i] = a[2 * i];
        d[i + 8] = a[2 * i + 1];
    }
    basemul8(a0, d, d, zeta);
    basemul8(a1, d + 8, d + 8, zeta);

    a0[0] -= fqmul(zeta, a1[7]);
    for (i = 1; i < 8; i++)
        a0[i] -= a1[i - 1];

    r = rq_inverse_recursive8(a1, a0, zeta);

    for (i = 0; i < 8; i++)
    {
        c0[i] = a[2 * i];
        c1[i] = -a[2 * i + 1];
    }
    basemul8(b0, c0, a1, zeta);
    basemul8(b1, c1, a1, zeta);
    for (i = 0; i < 8; i++)
    {
        b[2 * i] = b0[i];
        b[2 * i + 1] = b1[i];
    }

    return r;
}
