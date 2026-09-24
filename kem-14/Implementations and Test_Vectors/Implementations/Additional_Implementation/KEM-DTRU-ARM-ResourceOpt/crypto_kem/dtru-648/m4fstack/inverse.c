#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "params.h"
#include "inverse.h"
#include "reduce.h"
#include "ntt.h"

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
    int16_t Phi[ROOT_DIMENSION + 1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, zeta};
    int16_t V[ROOT_DIMENSION + 1] = {0};
    int16_t S[ROOT_DIMENSION + 1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int16_t F[ROOT_DIMENSION + 1] = {f[8], f[7], f[6], f[5], f[4], f[3], f[2], f[1], f[0], 0};
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

int base_inv_opt(int16_t finv[3], const int16_t f[3], const int16_t zeta)
{
    int16_t t[3], d, invd;

    t[0] = fqmul(f[0], f[0]) - fqmul(fqmul(f[1], f[2]), zeta);
    t[1] = fqmul(fqmul(f[2], f[2]), zeta) - fqmul(f[0], f[1]);
    t[2] = fqmul(f[1], f[1]) - fqmul(f[0], f[2]);
    d = fqmul(t[0], f[0]) + fqmul(t[1], fqmul(f[2], zeta)) + fqmul(t[2], fqmul(f[1], zeta));
    invd = fqinv(d);

    if (invd == 0)
        return 1;

    finv[0] = fqmul(invd, t[0]);
    finv[1] = fqmul(invd, t[1]);
    finv[2] = fqmul(invd, t[2]);

    return 0;
}

int rq_inverse_opt(int16_t finv[ROOT_DIMENSION], const int16_t f[ROOT_DIMENSION], const int16_t zeta)
{
    int16_t f0[3], f1[3], f2[3];
    int16_t f0_sq[3], f1_sq[3], f2_sq[3];
    int16_t f1f2[3], f0f1[3], f0f2[3];
    int16_t N0[3], N1[3], N2[3];
    int16_t N0f0[3], N1f2[3], N2f1[3];
    int16_t D[3], invD[3];
    int16_t g0[3], g1[3], g2[3];

    f0[0] = f[0];
    f0[1] = f[3];
    f0[2] = f[6];
    f1[0] = f[1];
    f1[1] = f[4];
    f1[2] = f[7];
    f2[0] = f[2];
    f2[1] = f[5];
    f2[2] = f[8];

    basemul3(f0_sq, f0, f0, zeta);
    basemul3(f1_sq, f1, f1, zeta);
    basemul3(f2_sq, f2, f2, zeta);
    basemul3(f0f1, f0, f1, zeta);
    basemul3(f0f2, f0, f2, zeta);
    basemul3(f1f2, f1, f2, zeta);

    N0[0] = f0_sq[0] - fqmul(zeta, f1f2[2]);
    N0[1] = f0_sq[1] - f1f2[0];
    N0[2] = f0_sq[2] - f1f2[1];

    N1[0] = fqmul(zeta, f2_sq[2]) - f0f1[0];
    N1[1] = f2_sq[0] - f0f1[1];
    N1[2] = f2_sq[1] - f0f1[2];

    N2[0] = f1_sq[0] - f0f2[0];
    N2[1] = f1_sq[1] - f0f2[1];
    N2[2] = f1_sq[2] - f0f2[2];

    basemul3(N0f0, N0, f0, zeta);
    basemul3(N1f2, N1, f2, zeta);
    basemul3(N2f1, N2, f1, zeta);

    D[0] = barrett_reduce(N0f0[0] + fqmul(N1f2[2] + N2f1[2], zeta));
    D[1] = barrett_reduce(N0f0[1] + N1f2[0] + N2f1[0]);
    D[2] = barrett_reduce(N0f0[2] + N1f2[1] + N2f1[1]);

    if (base_inv_opt(invD, D, zeta) != 0)
        return 1;

    basemul3(g0, N0, invD, zeta);
    basemul3(g1, N1, invD, zeta);
    basemul3(g2, N2, invD, zeta);

    finv[0] = g0[0];
    finv[1] = g1[0];
    finv[2] = g2[0];
    finv[3] = g0[1];
    finv[4] = g1[1];
    finv[5] = g2[1];
    finv[6] = g0[2];
    finv[7] = g1[2];
    finv[8] = g2[2];

    return 0;
}
