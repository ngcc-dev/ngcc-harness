#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"

static int16_t center_mod_q(int32_t a)
{
    a %= NTRE_Q;
    if (a < 0)
        a += NTRE_Q;
    if (a > NTRE_Q / 2)
        a -= NTRE_Q;
    return (int16_t)a;
}

static uint16_t freeze_u12(int32_t a)
{
    a %= NTRE_Q;
    if (a < 0)
        a += NTRE_Q;
    return (uint16_t)a;
}

void poly_tobytes(uint8_t r[NTRE_POLYBYTES], const poly *a)
{
    for (size_t i = 0; i < NTRE_N / 2; i++) {
        uint16_t t0 = freeze_u12(a->coeffs[2 * i]);
        uint16_t t1 = freeze_u12(a->coeffs[2 * i + 1]);

        r[3 * i + 0] = (uint8_t)t0;
        r[3 * i + 1] = (uint8_t)((t0 >> 8) | (t1 << 4));
        r[3 * i + 2] = (uint8_t)(t1 >> 4);
    }
}

void poly_frombytes(poly *r, const uint8_t a[NTRE_POLYBYTES])
{
    for (size_t i = 0; i < NTRE_N / 2; i++) {
        r->coeffs[2 * i] =
            (int16_t)(((uint16_t)a[3 * i] | ((uint16_t)a[3 * i + 1] << 8)) & 0x0fff);
        r->coeffs[2 * i + 1] =
            (int16_t)(((uint16_t)a[3 * i + 1] >> 4) | ((uint16_t)a[3 * i + 2] << 4));
    }
}

void poly_cbd1(poly *r, const uint8_t buf[NTRE_SAMPLEBYTES])
{
    for (size_t i = 0; i < NTRE_N / 8; i++) {
        uint8_t lo = buf[i];
        uint8_t hi = buf[i + NTRE_N / 8];

        for (size_t j = 0; j < 8; j++) {
            r->coeffs[8 * i + j] = (int16_t)((lo & 1U) - (hi & 1U));
            lo >>= 1;
            hi >>= 1;
        }
    }
}

void poly_cbd1_prime(poly *r,
                     const uint8_t msg[NTRE_MSGBYTES],
                     const uint8_t coins[NTRE_ERROR_RANDOMBYTES])
{
    for (size_t i = 0; i < NTRE_N / 8; i++) {
        uint8_t m = msg[i];
        uint8_t c = coins[i];

        for (size_t j = 0; j < 8; j++) {
            uint8_t beta0 = m & 1U;
            uint8_t beta1 = c & 1U;
            r->coeffs[8 * i + j] = beta0 ? (int16_t)(1 - 2 * (int16_t)beta1) : 0;
            m >>= 1;
            c >>= 1;
        }
    }
}

void poly_msg_mod2_to_bytes(uint8_t msg[NTRE_MSGBYTES], const poly *a)
{
    memset(msg, 0, NTRE_MSGBYTES);

    for (size_t i = 0; i < NTRE_N; i++) {
        int16_t c = center_mod_q(a->coeffs[i]);
        msg[i >> 3] |= (uint8_t)((c & 1) << (i & 7));
    }
}

void poly_ntt(poly *r)    { ntt(r->coeffs); }
void poly_invntt(poly *r) { invntt(r->coeffs); }

int poly_baseinv(poly *r, const poly *a)
{
#if NTRE_D == 3
    for (size_t i = 0; i < NTRE_N / 6; i++) {
        if (baseinv(r->coeffs + 6 * i, a->coeffs + 6 * i, base_zetas[i])) {
            memset(r, 0, sizeof(*r));
            return 1;
        }
        if (baseinv(r->coeffs + 6 * i + 3, a->coeffs + 6 * i + 3, -base_zetas[i])) {
            memset(r, 0, sizeof(*r));
            return 1;
        }
    }
#elif NTRE_D == 4
    for (size_t i = 0; i < NTRE_N / 8; i++) {
        if (baseinv(r->coeffs + 8 * i, a->coeffs + 8 * i, base_zetas[i])) {
            memset(r, 0, sizeof(*r));
            return 1;
        }
        if (baseinv(r->coeffs + 8 * i + 4, a->coeffs + 8 * i + 4, -base_zetas[i])) {
            memset(r, 0, sizeof(*r));
            return 1;
        }
    }
#else
#error "Unsupported NTRE_D"
#endif
    return 0;
}

void poly_basemul(poly *r, const poly *a, const poly *b)
{
#if NTRE_D == 3
    for (size_t i = 0; i < NTRE_N / 6; i++) {
        basemul(r->coeffs + 6 * i,
                a->coeffs + 6 * i,
                b->coeffs + 6 * i,
                base_zetas[i]);
        basemul(r->coeffs + 6 * i + 3,
                a->coeffs + 6 * i + 3,
                b->coeffs + 6 * i + 3,
                -base_zetas[i]);
    }
#elif NTRE_D == 4
    for (size_t i = 0; i < NTRE_N / 8; i++) {
        basemul(r->coeffs + 8 * i,
                a->coeffs + 8 * i,
                b->coeffs + 8 * i,
                base_zetas[i]);
        basemul(r->coeffs + 8 * i + 4,
                a->coeffs + 8 * i + 4,
                b->coeffs + 8 * i + 4,
                -base_zetas[i]);
    }
#endif
}

void poly_basemul_add(poly *r, const poly *a, const poly *b, const poly *c)
{
#if NTRE_D == 3
    for (size_t i = 0; i < NTRE_N / 6; i++) {
        basemul_add(r->coeffs + 6 * i,
                    a->coeffs + 6 * i,
                    b->coeffs + 6 * i,
                    c->coeffs + 6 * i,
                    base_zetas[i]);
        basemul_add(r->coeffs + 6 * i + 3,
                    a->coeffs + 6 * i + 3,
                    b->coeffs + 6 * i + 3,
                    c->coeffs + 6 * i + 3,
                    -base_zetas[i]);
    }
#elif NTRE_D == 4
    for (size_t i = 0; i < NTRE_N / 8; i++) {
        basemul_add(r->coeffs + 8 * i,
                    a->coeffs + 8 * i,
                    b->coeffs + 8 * i,
                    c->coeffs + 8 * i,
                    base_zetas[i]);
        basemul_add(r->coeffs + 8 * i + 4,
                    a->coeffs + 8 * i + 4,
                    b->coeffs + 8 * i + 4,
                    c->coeffs + 8 * i + 4,
                    -base_zetas[i]);
    }
#endif
}

void poly_sub(poly *r, const poly *a, const poly *b)
{
    for (size_t i = 0; i < NTRE_N; i++)
        r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
}

void poly_double(poly *r, const poly *a)
{
    for (size_t i = 0; i < NTRE_N; i++)
        r->coeffs[i] = (int16_t)(2 * a->coeffs[i]);
}

void poly_double_add_one(poly *r, const poly *a)
{
    poly_double(r, a);
    r->coeffs[0] = (int16_t)(r->coeffs[0] + 1);
}
