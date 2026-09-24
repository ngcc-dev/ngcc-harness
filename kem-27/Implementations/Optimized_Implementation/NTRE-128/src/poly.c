#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"

void poly_double(poly *r, const poly *a);  /* in asm/add.s */
void poly_sub(poly *r, const poly *a, const poly *b);  /* in asm/add.s */

void poly_tobytes(uint8_t r[NTRE_POLYBYTES], const poly *a)
{
    for (size_t i = 0; i < NTRE_N / 2; i++) {
        int32_t t0 = a->coeffs[2 * i];
        int32_t t1 = a->coeffs[2 * i + 1];
        t0 %= NTRE_Q; if (t0 < 0) t0 += NTRE_Q;
        t1 %= NTRE_Q; if (t1 < 0) t1 += NTRE_Q;
        uint16_t u0 = (uint16_t)t0;
        uint16_t u1 = (uint16_t)t1;
        r[3 * i + 0] = (uint8_t)u0;
        r[3 * i + 1] = (uint8_t)((u0 >> 8) | (u1 << 4));
        r[3 * i + 2] = (uint8_t)(u1 >> 4);
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
        r->coeffs[8*i + 0] = (int16_t)((lo & 1U) - (hi & 1U));
        r->coeffs[8*i + 1] = (int16_t)(((lo>>1) & 1U) - ((hi>>1) & 1U));
        r->coeffs[8*i + 2] = (int16_t)(((lo>>2) & 1U) - ((hi>>2) & 1U));
        r->coeffs[8*i + 3] = (int16_t)(((lo>>3) & 1U) - ((hi>>3) & 1U));
        r->coeffs[8*i + 4] = (int16_t)(((lo>>4) & 1U) - ((hi>>4) & 1U));
        r->coeffs[8*i + 5] = (int16_t)(((lo>>5) & 1U) - ((hi>>5) & 1U));
        r->coeffs[8*i + 6] = (int16_t)(((lo>>6) & 1U) - ((hi>>6) & 1U));
        r->coeffs[8*i + 7] = (int16_t)(((lo>>7) & 1U) - ((hi>>7) & 1U));
    }
}

void poly_cbd1_prime(poly *r,
                     const uint8_t msg[NTRE_MSGBYTES],
                     const uint8_t coins[NTRE_ERROR_RANDOMBYTES])
{
    for (size_t i = 0; i < NTRE_N / 8; i++) {
        uint8_t m = msg[i];
        uint8_t c = coins[i];
        r->coeffs[8*i + 0] = (m & 1U) ? (int16_t)(1 - 2*(int16_t)(c & 1U)) : 0;
        r->coeffs[8*i + 1] = ((m>>1) & 1U) ? (int16_t)(1 - 2*(int16_t)((c>>1) & 1U)) : 0;
        r->coeffs[8*i + 2] = ((m>>2) & 1U) ? (int16_t)(1 - 2*(int16_t)((c>>2) & 1U)) : 0;
        r->coeffs[8*i + 3] = ((m>>3) & 1U) ? (int16_t)(1 - 2*(int16_t)((c>>3) & 1U)) : 0;
        r->coeffs[8*i + 4] = ((m>>4) & 1U) ? (int16_t)(1 - 2*(int16_t)((c>>4) & 1U)) : 0;
        r->coeffs[8*i + 5] = ((m>>5) & 1U) ? (int16_t)(1 - 2*(int16_t)((c>>5) & 1U)) : 0;
        r->coeffs[8*i + 6] = ((m>>6) & 1U) ? (int16_t)(1 - 2*(int16_t)((c>>6) & 1U)) : 0;
        r->coeffs[8*i + 7] = ((m>>7) & 1U) ? (int16_t)(1 - 2*(int16_t)((c>>7) & 1U)) : 0;
    }
}

static int16_t center_mod_q(int32_t a)
{
    a %= NTRE_Q;
    if (a < 0) a += NTRE_Q;
    if (a > NTRE_Q / 2) a -= NTRE_Q;
    return (int16_t)a;
}

void poly_msg_mod2_to_bytes(uint8_t msg[NTRE_MSGBYTES], const poly *a)
{
    for (size_t i = 0; i < NTRE_MSGBYTES; i++) {
        uint8_t b = 0;
        for (size_t j = 0; j < 8; j++) {
            int16_t c = center_mod_q(a->coeffs[8 * i + j]);
            b |= (uint8_t)((c & 1) << j);
        }
        msg[i] = b;
    }
}

void poly_ntt(poly *r)    { ntt(r->coeffs); }
void poly_invntt(poly *r) { invntt(r->coeffs); }

int poly_baseinv(poly *r, const poly *a)
{
    if (baseinv_batch(r->coeffs, a->coeffs)) {
        memset(r, 0, sizeof(*r));
        return 1;
    }
    return 0;
}

void poly_basemul(poly *r, const poly *a, const poly *b)
{
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
}

void poly_basemul_add(poly *r, const poly *a, const poly *b, const poly *c)
{
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
}

void poly_double_add_one(poly *r, const poly *a)
{
    poly_double(r, a);
    r->coeffs[0] = (int16_t)(r->coeffs[0] + 1);
}
