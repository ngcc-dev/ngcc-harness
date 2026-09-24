#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "polyvec.h"
#include "cbd.h"
#include "auxfunc.h"

void gen_a(
    polyvec *a, 
    const uint8_t seed[SCABBARD_SYMBYTES])
{
    size_t i, j;

    uint8_t extseed[SCABBARD_SYMBYTES + 2];
    memcpy(extseed, seed, SCABBARD_SYMBYTES);

    uint8_t buf[SCABBARD_POLYBYTES];
    
    for (i = 0; i < SCABBARD_L; i++) {
        for (j = 0; j < SCABBARD_L; j++) {
            extseed[SCABBARD_SYMBYTES + 0] = i;
            extseed[SCABBARD_SYMBYTES + 1] = j;

            pseudoXOF(SCABBARD_POLYBYTES * 8, extseed, (SCABBARD_SYMBYTES + 2) * 8, buf);

            poly_modq_frombytes(&a[i].vec[j], buf);
        }
    }
}

void gen_s(
    polyvec *s, 
    const uint8_t seed[SCABBARD_SYMBYTES])
{
    size_t i; 

    uint8_t extseed[SCABBARD_SYMBYTES + 1];
    memcpy(extseed, seed, SCABBARD_SYMBYTES);

    uint8_t buf[SCABBARD_CBD_POLYBYTES];

    for (i = 0; i < SCABBARD_L; i++) {
        extseed[SCABBARD_SYMBYTES + 0] = i;

        pseudoXOF(SCABBARD_CBD_POLYBYTES * 8, extseed, (SCABBARD_SYMBYTES + 1) * 8, buf);
        
        cbd(&s->vec[i], buf);
        
    }
}

void matrix_vector_mul(
    polyvec *r,
    const polyvec *a,
    const polyvec *b,
    int16_t transpose)
{
    size_t i, j;

    uint16_t b_weighted[SCABBARD_L][3][3][N_SM_16];
    uint16_t a_weighted[3][3][N_SM_16];
    uint16_t ab_acc[3][3][N_SM_16_RES];

    polyvec_TC_evaluate(b_weighted, b);
    for (i = 0; i < SCABBARD_L; i++) {
        memset(ab_acc, 0, sizeof(ab_acc));

        for (j = 0; j < SCABBARD_L; j++) {
            if (transpose == 1) {
                poly_TC_evaluate(a_weighted, &a[j].vec[i]);
            } else {
                poly_TC_evaluate(a_weighted, &a[i].vec[j]);
            }
            poly_TC_pointwise_acc(ab_acc, a_weighted, b_weighted[j]);

            if (j == SCABBARD_L - 1) {
                poly_TC_interpolate(&r->vec[i], ab_acc);
            }
        }
    }
}

void inner_prod(
    poly *r,
    const polyvec *a,
    const polyvec *b)
{
    size_t i;

    uint16_t b_weighted[SCABBARD_L][3][3][N_SM_16];
    uint16_t a_weighted[3][3][N_SM_16];
    uint16_t ab_acc[3][3][N_SM_16_RES] = {0};

    polyvec_TC_evaluate(b_weighted, b);
    for (i = 0; i < SCABBARD_L; i++) {
        poly_TC_evaluate(a_weighted, &a->vec[i]);
        poly_TC_pointwise_acc(ab_acc, a_weighted, b_weighted[i]);

        if (i == SCABBARD_L - 1) {
            poly_TC_interpolate(r, ab_acc);
        }
    }
}

void polyvec_modp_tobytes(
    uint8_t r[SCABBARD_POLYVECCOMPRESSEDBYTES],
    const polyvec *a)
{
    size_t i;

    for (i = 0; i < SCABBARD_L; i++) {
        poly_modp_tobytes(r + i * SCABBARD_P_POLYBYTES, &a->vec[i]);
    }
}

void polyvec_modp_frombytes(
    polyvec *r,
    const uint8_t a[SCABBARD_POLYVECCOMPRESSEDBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_L; i++) {
        poly_modp_frombytes(&r->vec[i], a + i * SCABBARD_P_POLYBYTES);
    }
}

void polyvec_s_tobytes(
    uint8_t r[SCABBARD_S_POLYVECBYTES],
    const polyvec *s)
{
    size_t i;

    for (i = 0; i < SCABBARD_L; i++) {
        poly_s_tobytes(r + i * SCABBARD_S_POLYBYTES, &s->vec[i]);
    }
}

void polyvec_s_frombytes(
    polyvec *s,
    const uint8_t r[SCABBARD_S_POLYVECBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_L; i++) {
        poly_s_frombytes(&s->vec[i], r + i * SCABBARD_S_POLYBYTES);
    }
}

void polyvec_TC_evaluate(
    uint16_t b_weighted[SCABBARD_L][3][3][N_SM_16],
    const polyvec *b)
{
    poly_TC_evaluate(b_weighted[0], &b->vec[0]);
    poly_TC_evaluate(b_weighted[1], &b->vec[1]);
    poly_TC_evaluate(b_weighted[2], &b->vec[2]);
    poly_TC_evaluate(b_weighted[3], &b->vec[3]);
    poly_TC_evaluate(b_weighted[4], &b->vec[4]);
    poly_TC_evaluate(b_weighted[5], &b->vec[5]);
    poly_TC_evaluate(b_weighted[6], &b->vec[6]);
    poly_TC_evaluate(b_weighted[7], &b->vec[7]);
    poly_TC_evaluate(b_weighted[8], &b->vec[8]);
    
}