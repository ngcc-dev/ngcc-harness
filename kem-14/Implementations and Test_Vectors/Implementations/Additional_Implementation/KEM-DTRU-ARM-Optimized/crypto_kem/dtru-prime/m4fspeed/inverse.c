#include <stdint.h>
#include "params.h"
#include "reduce.h"

/* Based on the reference implementation of NTRU Prime (NIST 3rd round submission)
 * by Daniel J. Bernstein, Chitchanok Chuengsatiansup, Tanja Lange, Christine van Vredendaal.
 * It can be used for q \in {4091, 4621, 4591, 7879}.
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

int rq_inverse(int16_t *finv, const int16_t *f)
{
    int16_t Phi[DTRU_N + 1], F[DTRU_N + 1], V[DTRU_N + 1], S[DTRU_N + 1];
    int i, loop, Delta, swap, t;
    int32_t Phi0, F0;
    int16_t scale;

    for (i = 0; i < DTRU_N; ++i){
        Phi[i] = 0;
        F[DTRU_N - 1 - i] = f[i];
        V[i] = 0;
        S[i] = 0;
    }  

    Phi[0] = 1;
    Phi[DTRU_N - 1] = Phi[DTRU_N] = -1;
    F[DTRU_N] = 0;
    Delta = 1;
    V[DTRU_N] = 0;
    S[0] = 1;
    S[DTRU_N] = 0;

    for (loop = 0; loop < 2 * DTRU_N - 1; ++loop)
    {
        for (i = DTRU_N; i > 0; --i)
            V[i] = V[i - 1];
        V[0] = 0;

        swap = int16_negative_mask(-Delta) & int16_nonzero_mask(F[0]);

        for (i = 0; i < DTRU_N + 1; ++i)
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

        for (i = 0; i < DTRU_N + 1; ++i)
            F[i] = fq_freeze(Phi0 * F[i] - F0 * Phi[i]);
        for (i = 0; i < DTRU_N; ++i)
            F[i] = F[i + 1];
        F[DTRU_N] = 0;

        for (i = 0; i < DTRU_N + 1; ++i)
            S[i] = fq_freeze(Phi0 * S[i] - F0 * V[i]);
    }

    scale = fqinv(Phi[0]);
    // scale = Phi[0];
    // scale += (scale >> 15) & DTRU_Q;
    for (i = 0; i < DTRU_N; ++i)
        finv[i] = fq_freeze(scale * (int32_t)V[DTRU_N - 1 - i]);

    return int16_nonzero_mask(Delta);
}
