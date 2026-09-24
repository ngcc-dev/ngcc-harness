#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "poly.h"
#include "minal.h"


void poly_modq_frombytes(
    poly *r, 
    const uint8_t buf[SCABBARD_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        r->coeffs[4 * i + 0] = ((buf[7 * i + 0] & 0xff) | ((buf[7 * i + 1] & 0x3f) << 8)) & 0x3fff;
		r->coeffs[4 * i + 1] = (((buf[7 * i + 1] >> 6) & 0x03) | (((buf[7 * i + 2] & 0xff) << 2) & 0xff) | (((buf[7 * i + 3] & 0xf) << 10) & 0x0f)) & 0x3fff;
        r->coeffs[4 * i + 2] = (((buf[7 * i + 3] >> 4) & 0x0F) | (((buf[7 * i + 4] & 0xff) << 4) & 0xff) | (((buf[7 * i + 5] & 0x3) << 12) & 0x03)) & 0x3fff;
        r->coeffs[4 * i + 3] = (((buf[7 * i + 5] >> 2) & 0x3F) | (((buf[7 * i + 6] & 0xff) << 6) & 0xff)) & 0x3fff;
    }
}

void poly_modp_tobytes(
    uint8_t r[SCABBARD_P_POLYBYTES], 
    const poly *a)
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        r[5 * i + 0] = (a->coeffs[4 * i + 0] & (0xff));
		r[5 * i + 1] = ((a->coeffs[4 * i + 0] >> 8) & 0x03) | ((a->coeffs[4 * i + 1] & 0x3f) << 2);
		r[5 * i + 2] = ((a->coeffs[4 * i + 1] >> 6) & 0x0f) | ((a->coeffs[4 * i + 2] & 0x0f) << 4);
		r[5 * i + 3] = ((a->coeffs[4 * i + 2] >> 4) & 0x3f) | ((a->coeffs[4 * i + 3] & 0x03) << 6);
		r[5 * i + 4] = ((a->coeffs[4 * i + 3] >> 2) & 0xff);
    }
}

uint8_t poly_modp_tobytes_cmp(
    const uint8_t r[SCABBARD_P_POLYBYTES],
    const poly *a)
{
    size_t i;
    uint8_t fail = 0;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        fail |= r[5 * i + 0] ^ (uint8_t)(a->coeffs[4 * i + 0] & 0xff);
        fail |= r[5 * i + 1] ^ (uint8_t)(((a->coeffs[4 * i + 0] >> 8) & 0x03) |
                                          ((a->coeffs[4 * i + 1] & 0x3f) << 2));
        fail |= r[5 * i + 2] ^ (uint8_t)(((a->coeffs[4 * i + 1] >> 6) & 0x0f) |
                                          ((a->coeffs[4 * i + 2] & 0x0f) << 4));
        fail |= r[5 * i + 3] ^ (uint8_t)(((a->coeffs[4 * i + 2] >> 4) & 0x3f) |
                                          ((a->coeffs[4 * i + 3] & 0x03) << 6));
        fail |= r[5 * i + 4] ^ (uint8_t)((a->coeffs[4 * i + 3] >> 2) & 0xff);
    }

    return fail;
}


void poly_modp_frombytes(
    poly *r, 
    const uint8_t buf[SCABBARD_P_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        r->coeffs[4 * i + 0] = (buf[5 * i + 0] & (0xff)) | ((uint16_t)buf[5 * i + 1] & 0x03) << 8;
        r->coeffs[4 * i + 1] = ((buf[5 * i + 1] >> 2) & (0x3f)) | (((uint16_t)buf[5 * i + 2] & 0x0f) << 6);
        r->coeffs[4 * i + 2] = ((buf[5 * i + 2] >> 4) & (0x0f)) | (((uint16_t)buf[5 * i + 3] & 0x3f) << 4);
        r->coeffs[4 * i + 3] = ((buf[5 * i + 3] >> 6) & (0x03)) | (((uint16_t)buf[5 * i + 4] & 0xff) << 2);
    }
}

void poly_s_tobytes(
    uint8_t r[SCABBARD_S_POLYBYTES],
    const poly *s)
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 2; i++) {
        r[i] = (s->coeffs[2 * i] & 0x0F) | ((s->coeffs[2 * i + 1] & 0x0F) << 4);
    }
}

void poly_s_frombytes(
    poly *r,
    const uint8_t s[SCABBARD_S_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 2; i++) {
        r->coeffs[2 * i + 0] = (((s[i] & 0x0F) ^ 0x08) - 0x08) & SCABBARD_P;
        r->coeffs[2 * i + 1] = (((s[i] >> 4) ^ 0x08) - 0x08) & SCABBARD_P;
    }
}

void poly_m_tobytes(
    uint8_t r[SCABBARD_POLYCOMPRESSEDBYTES],
    const poly *m)
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 8; i++) {
        r[5 * i + 0]= (m->coeffs[8 * i + 0] & 0x1f) | ( (m->coeffs[8 * i + 1] & 0x07)<<5 );
		r[5 * i + 1]= ((m->coeffs[8 * i + 1] >> 3 ) & 0x03)  | ( (m->coeffs[8 * i + 2] & 0x1f)<<2 ) | ( (m->coeffs[8 * i + 3] & 0x01)<<7 );
		r[5 * i + 2]= ((m->coeffs[8 * i + 3] >> 1 ) & 0x0f)  | ( (m->coeffs[8 * i + 4] & 0x0f)<<4 );
		r[5 * i + 3]= ((m->coeffs[8 * i + 4] >> 4 ) & 0x01)  | ( (m->coeffs[8 * i + 5] & 0x1f)<<1 ) | ( (m->coeffs[8 * i + 6] & 0x03)<<6 );
		r[5 * i + 4]= ((m->coeffs[8 * i + 6] >> 2 ) & 0x07)  | ( (m->coeffs[8 * i + 7] & 0x1f)<<3 );
    }
}

uint8_t poly_m_tobytes_cmp(
    const uint8_t r[SCABBARD_POLYCOMPRESSEDBYTES],
    const poly *m)
{
    size_t i;
    uint8_t fail = 0;

    for (i = 0; i < SCABBARD_N / 8; i++) {
        fail |= r[5 * i + 0] ^ (uint8_t)((m->coeffs[8 * i + 0] & 0x1f) |
                                          ((m->coeffs[8 * i + 1] & 0x07) << 5));
        fail |= r[5 * i + 1] ^ (uint8_t)(((m->coeffs[8 * i + 1] >> 3) & 0x03) |
                                          ((m->coeffs[8 * i + 2] & 0x1f) << 2) |
                                          ((m->coeffs[8 * i + 3] & 0x01) << 7));
        fail |= r[5 * i + 2] ^ (uint8_t)(((m->coeffs[8 * i + 3] >> 1) & 0x0f) |
                                          ((m->coeffs[8 * i + 4] & 0x0f) << 4));
        fail |= r[5 * i + 3] ^ (uint8_t)(((m->coeffs[8 * i + 4] >> 4) & 0x01) |
                                          ((m->coeffs[8 * i + 5] & 0x1f) << 1) |
                                          ((m->coeffs[8 * i + 6] & 0x03) << 6));
        fail |= r[5 * i + 4] ^ (uint8_t)(((m->coeffs[8 * i + 6] >> 2) & 0x07) |
                                          ((m->coeffs[8 * i + 7] & 0x1f) << 3));
    }

    return fail;
}

void poly_m_frombytes(
    poly *r,
    const uint8_t m[SCABBARD_POLYCOMPRESSEDBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 8; i++) {
        r->coeffs[8 * i + 0] = (m[5 * i + 0])&0x1f;
		r->coeffs[8 * i + 1] = ( ( (m[5 * i + 0])>>5 )&0x07) | ( ( (m[5 * i + 1])&0x03)<<3 );
		r->coeffs[8 * i + 2] = ( ( (m[5 * i + 1])>>2 )&0x1f);
		r->coeffs[8 * i + 3] = ( ( (m[5 * i + 1])>>7 )&0x01) | ( ( (m[5 * i + 2])&0x0f)<<1 );
		r->coeffs[8 * i + 4] = ( ( (m[5 * i + 2])>>4 )&0x0f) | ( ( (m[5 * i + 3])&0x01)<<4 );
		r->coeffs[8 * i + 5] = ( ( (m[5 * i + 3])>>1 )&0x1f);
		r->coeffs[8 * i + 6] = ( ( (m[5 * i + 3])>>6 )&0x03) | ( ( (m[5 * i + 4])&0x07)<<2 );
		r->coeffs[8 * i + 7] = ( (m[5 * i + 4]>>3)&0x1f );

    }
}

void poly_tomsg(
    uint8_t msg[SCABBARD_INDCPA_MSGBYTES],
    const poly *v)
{
    unsigned int i, j;
    int16_t codeword[2];
    uint32_t base = 0;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        msg[i] = 0;
        for (j = 0; j < 8; j += 4) {
            codeword[0] = (int16_t)v->coeffs[base];
            codeword[1] = (int16_t)v->coeffs[base + SCABBARD_N / 2];

            msg[i] |= (uint8_t)(minal_b2_code_decode(codeword) << j);
            base++;
        }
    }
}

void poly_frommsg(
    poly *r,
    const uint8_t msg[SCABBARD_INDCPA_MSGBYTES])
{
    unsigned int i, j;
    int16_t codeword[2];
    uint8_t msg_bits[4];
    size_t base = 0;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        for (j = 0; j < 8; j += 4) {
            msg_bits[0] = (msg[i] >> (j + 3)) & 1;
            msg_bits[1] = (msg[i] >> (j + 2)) & 1;
            msg_bits[2] = (msg[i] >> (j + 1)) & 1;
            msg_bits[3] = (msg[i] >> (j + 0)) & 1;

            minal_b2_code_encode(codeword, msg_bits);

            r->coeffs[base] = (uint16_t)codeword[0];
            r->coeffs[base + SCABBARD_N / 2] = (uint16_t)codeword[1];
            base++;
        }
    }
}

/* Polynomial Multiplication */

void poly_TC_evaluate(
    uint16_t b_weighted[3][3][N_SM_16],
    const uint16_t *b)
{
    size_t i;
    uint16_t bw0[N_SM], bw1[N_SM], bw2[N_SM];
    const uint16_t *b0 = b;
    const uint16_t *b1 = b + N_SM;

    for (i = 0; i < N_SM; i++) {
        bw2[i] = b0[i];
        bw0[i] = b1[i];
        bw1[i] = (uint16_t)(b0[i] + b1[i]);
    }

    for (i = 0; i < N_SM_16; i++) {
        b_weighted[0][2][i] = bw0[i];
        b_weighted[0][0][i] = bw0[i + N_SM_16];
        b_weighted[0][1][i] = (uint16_t)(bw0[i] + bw0[i + N_SM_16]);

        b_weighted[1][2][i] = bw1[i];
        b_weighted[1][0][i] = bw1[i + N_SM_16];
        b_weighted[1][1][i] = (uint16_t)(bw1[i] + bw1[i + N_SM_16]);

        b_weighted[2][2][i] = bw2[i];
        b_weighted[2][0][i] = bw2[i + N_SM_16];
        b_weighted[2][1][i] = (uint16_t)(bw2[i] + bw2[i + N_SM_16]);
    }
}

void poly_TC_pointwise_acc(
    uint16_t acc[3][3][N_SM_16_RES],
    const uint16_t a_weighted[3][3][N_SM_16],
    const uint16_t b_weighted[3][3][N_SM_16])
{
    size_t i, j;

    for (i = 0; i < N_SM_16; i++) {
        for (j = 0; j < N_SM_16; j++) {
            acc[0][0][i + j] = (uint16_t)(acc[0][0][i + j] + a_weighted[0][0][i] * b_weighted[0][0][j]);
            acc[0][1][i + j] = (uint16_t)(acc[0][1][i + j] + a_weighted[0][1][i] * b_weighted[0][1][j]);
            acc[0][2][i + j] = (uint16_t)(acc[0][2][i + j] + a_weighted[0][2][i] * b_weighted[0][2][j]);
            acc[1][0][i + j] = (uint16_t)(acc[1][0][i + j] + a_weighted[1][0][i] * b_weighted[1][0][j]);
            acc[1][1][i + j] = (uint16_t)(acc[1][1][i + j] + a_weighted[1][1][i] * b_weighted[1][1][j]);
            acc[1][2][i + j] = (uint16_t)(acc[1][2][i + j] + a_weighted[1][2][i] * b_weighted[1][2][j]);
            acc[2][0][i + j] = (uint16_t)(acc[2][0][i + j] + a_weighted[2][0][i] * b_weighted[2][0][j]);
            acc[2][1][i + j] = (uint16_t)(acc[2][1][i + j] + a_weighted[2][1][i] * b_weighted[2][1][j]);
            acc[2][2][i + j] = (uint16_t)(acc[2][2][i + j] + a_weighted[2][2][i] * b_weighted[2][2][j]);
        }
    }
}

void poly_TC_interpolate(
    uint16_t *res,
    const uint16_t acc[3][3][N_SM_16_RES],
    uint16_t mod_mask)
{
    size_t i;
    uint16_t r0, r1, r2;
    uint16_t w0[N_SM_RES] = {0}, w1[N_SM_RES] = {0}, w2[N_SM_RES] = {0};
    uint16_t c[2 * SCABBARD_N] = {0};

    for (i = 0; i < N_SM_16_RES; i++) {
        r0 = (uint16_t)(acc[0][1][i] - acc[0][0][i] - acc[0][2][i]);
        r1 = (uint16_t)(acc[1][1][i] - acc[1][0][i] - acc[1][2][i]);
        r2 = (uint16_t)(acc[2][1][i] - acc[2][0][i] - acc[2][2][i]);

        w0[i] = (uint16_t)(w0[i] + acc[0][2][i]);
        w1[i] = (uint16_t)(w1[i] + acc[1][2][i]);
        w2[i] = (uint16_t)(w2[i] + acc[2][2][i]);

        w0[i + N_SM_16] = (uint16_t)(w0[i + N_SM_16] + r0);
        w1[i + N_SM_16] = (uint16_t)(w1[i + N_SM_16] + r1);
        w2[i + N_SM_16] = (uint16_t)(w2[i + N_SM_16] + r2);

        w0[i + N_SM] = (uint16_t)(w0[i + N_SM] + acc[0][0][i]);
        w1[i + N_SM] = (uint16_t)(w1[i + N_SM] + acc[1][0][i]);
        w2[i + N_SM] = (uint16_t)(w2[i + N_SM] + acc[2][0][i]);
    }

    for (i = 0; i < N_SM_RES; i++) {
        r0 = w0[i];
        r1 = w1[i];
        r2 = w2[i];
        r1 = (uint16_t)(r1 - (uint16_t)(r0 + r2));

        c[i] = (uint16_t)(c[i] + r2);
        c[i + N_SM] = (uint16_t)(c[i + N_SM] + r1);
        c[i + SCABBARD_N] = (uint16_t)(c[i + SCABBARD_N] + r0);
    }

    for (i = 0; i < SCABBARD_N; i++) {
        res[i] = (uint16_t)((c[i] - c[i + SCABBARD_N]) & mod_mask);
    }
}

void poly_TC_mul_64(
    const uint16_t *a,
    const uint16_t *b,
    uint16_t *res,
    uint16_t mod_mask)
{
    uint16_t a_weighted[3][3][N_SM_16];
    uint16_t b_weighted[3][3][N_SM_16];
    uint16_t acc[3][3][N_SM_16_RES];

    poly_TC_evaluate(a_weighted, a);
    poly_TC_evaluate(b_weighted, b);
    memset(acc, 0, sizeof(acc));
    poly_TC_pointwise_acc(acc, a_weighted, b_weighted);
    poly_TC_interpolate(res, acc, mod_mask);
}
