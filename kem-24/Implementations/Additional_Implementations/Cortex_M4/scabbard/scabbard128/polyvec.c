#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "polyvec.h"
#include "cbd.h"
#include "auxfunc.h"
#include "poly_mul.h"

#if defined(SCABBARD_USE_ASM_MUL) && SCABBARD_USE_ASM_MUL
#define SCABBARD_HAVE_ASM_MUL 1
#else
#define SCABBARD_HAVE_ASM_MUL 0
#endif

#if !SCABBARD_HAVE_ASM_MUL
static void polyvec_TC_evaluate_packed(
    uint16_t b_weighted[SCABBARD_L][3][3][N_SM_16],
    const uint8_t *b_packed)
{
    size_t i, j;
    poly b_i;

    for (i = 0; i < SCABBARD_L; i++) {
        for (j = 0; j < SCABBARD_N / 2; j++) {
            b_i.coeffs[2 * j] = (uint16_t)(((b_packed[i * SCABBARD_S_POLYBYTES + j] & 0x0F) ^ 0x08) - 0x08);
            b_i.coeffs[2 * j + 1] = (uint16_t)(((b_packed[i * SCABBARD_S_POLYBYTES + j] >> 4) ^ 0x08) - 0x08);
        }
        poly_TC_evaluate(b_weighted[i], b_i.coeffs);
    }
}
#endif

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

void gen_s_packed(
    uint8_t s[SCABBARD_INDCPA_SECRETKEYBYTES],
    const uint8_t seed[SCABBARD_SYMBYTES])
{
    size_t i, j;
    uint8_t extseed[SCABBARD_SYMBYTES + 1];
    uint8_t buf[SCABBARD_CBD_POLYBYTES];
    poly tmp;

    memcpy(extseed, seed, SCABBARD_SYMBYTES);
    for (i = 0; i < SCABBARD_L; i++) {
        extseed[SCABBARD_SYMBYTES] = (uint8_t)i;
        pseudoXOF(SCABBARD_CBD_POLYBYTES * 8, extseed, (SCABBARD_SYMBYTES + 1) * 8, buf);
        cbd(&tmp, buf);

        for (j = 0; j < SCABBARD_N / 2; j++) {
            s[i * SCABBARD_S_POLYBYTES + j] =
                (uint8_t)((tmp.coeffs[2 * j] & 0x0F) |
                          ((tmp.coeffs[2 * j + 1] & 0x0F) << 4));
        }
    }
}

void matrix_vector_mul_tobytes(
    uint8_t *out,
    const uint8_t seed[SCABBARD_SYMBYTES],
    const uint8_t *s_packed,
    int16_t transpose,
    uint16_t rounding_const)
{
#if SCABBARD_HAVE_ASM_MUL
    size_t i, j, k;
    uint8_t extseed[SCABBARD_SYMBYTES + 2];
    uint8_t buf[SCABBARD_POLYBYTES];
    poly a_ij;
    uint16_t acc[SCABBARD_N];

    memcpy(extseed, seed, SCABBARD_SYMBYTES);

    for (i = 0; i < SCABBARD_L; i++) {
        memset(acc, 0, sizeof(acc));

        for (j = 0; j < SCABBARD_L; j++) {
            extseed[SCABBARD_SYMBYTES + 0] = transpose ? (uint8_t)j : (uint8_t)i;
            extseed[SCABBARD_SYMBYTES + 1] = transpose ? (uint8_t)i : (uint8_t)j;
            pseudoXOF(SCABBARD_POLYBYTES * 8, extseed, (SCABBARD_SYMBYTES + 2) * 8, buf);
            poly_modq_frombytes(&a_ij, buf);

            poly_mul_64_sch_acc(a_ij.coeffs, s_packed + j * SCABBARD_S_POLYBYTES, acc, SCABBARD_Q);
        }

        for (k = 0; k < SCABBARD_N; k++) {
            a_ij.coeffs[k] = (uint16_t)(((acc[k] + rounding_const) & (uint16_t)SCABBARD_Q) >>
                                        (SCABBARD_EQ - SCABBARD_EP));
        }
        poly_modp_tobytes(out + i * SCABBARD_P_POLYBYTES, &a_ij);
    }
#else
    size_t i, j, k;
    uint8_t extseed[SCABBARD_SYMBYTES + 2];
    uint8_t buf[SCABBARD_POLYBYTES];
    poly a_ij;
    uint16_t s_weighted[SCABBARD_L][3][3][N_SM_16];
    uint16_t a_weighted[3][3][N_SM_16];
    uint16_t ab_acc[3][3][N_SM_16_RES];

    memcpy(extseed, seed, SCABBARD_SYMBYTES);
    polyvec_TC_evaluate_packed(s_weighted, s_packed);

    for (i = 0; i < SCABBARD_L; i++) {
        memset(ab_acc, 0, sizeof(ab_acc));

        for (j = 0; j < SCABBARD_L; j++) {
            extseed[SCABBARD_SYMBYTES + 0] = transpose ? (uint8_t)j : (uint8_t)i;
            extseed[SCABBARD_SYMBYTES + 1] = transpose ? (uint8_t)i : (uint8_t)j;
            pseudoXOF(SCABBARD_POLYBYTES * 8, extseed, (SCABBARD_SYMBYTES + 2) * 8, buf);
            poly_modq_frombytes(&a_ij, buf);

            poly_TC_evaluate(a_weighted, a_ij.coeffs);
            poly_TC_pointwise_acc(ab_acc, a_weighted, s_weighted[j]);
        }

        poly_TC_interpolate(a_ij.coeffs, ab_acc, SCABBARD_Q);
        for (k = 0; k < SCABBARD_N; k++) {
            a_ij.coeffs[k] = (uint16_t)(((a_ij.coeffs[k] + rounding_const) & (uint16_t)SCABBARD_Q) >>
                                        (SCABBARD_EQ - SCABBARD_EP));
        }
        poly_modp_tobytes(out + i * SCABBARD_P_POLYBYTES, &a_ij);
    }
#endif
}

uint8_t matrix_vector_mul_tobytes_cmp(
    const uint8_t *out,
    const uint8_t seed[SCABBARD_SYMBYTES],
    const uint8_t *s_packed,
    int16_t transpose,
    uint16_t rounding_const)
{
#if SCABBARD_HAVE_ASM_MUL
    size_t i, j, k;
    uint8_t extseed[SCABBARD_SYMBYTES + 2];
    uint8_t buf[SCABBARD_POLYBYTES];
    uint8_t fail = 0;
    poly a_ij;
    uint16_t acc[SCABBARD_N];

    memcpy(extseed, seed, SCABBARD_SYMBYTES);

    for (i = 0; i < SCABBARD_L; i++) {
        memset(acc, 0, sizeof(acc));

        for (j = 0; j < SCABBARD_L; j++) {
            extseed[SCABBARD_SYMBYTES + 0] = transpose ? (uint8_t)j : (uint8_t)i;
            extseed[SCABBARD_SYMBYTES + 1] = transpose ? (uint8_t)i : (uint8_t)j;
            pseudoXOF(SCABBARD_POLYBYTES * 8, extseed, (SCABBARD_SYMBYTES + 2) * 8, buf);
            poly_modq_frombytes(&a_ij, buf);

            poly_mul_64_sch_acc(a_ij.coeffs, s_packed + j * SCABBARD_S_POLYBYTES, acc, SCABBARD_Q);
        }

        for (k = 0; k < SCABBARD_N; k++) {
            a_ij.coeffs[k] = (uint16_t)(((acc[k] + rounding_const) & (uint16_t)SCABBARD_Q) >>
                                        (SCABBARD_EQ - SCABBARD_EP));
        }
        fail |= poly_modp_tobytes_cmp(out + i * SCABBARD_P_POLYBYTES, &a_ij);
    }

    return fail;
#else
    size_t i, j, k;
    uint8_t extseed[SCABBARD_SYMBYTES + 2];
    uint8_t buf[SCABBARD_POLYBYTES];
    uint8_t fail = 0;
    poly a_ij;
    uint16_t s_weighted[SCABBARD_L][3][3][N_SM_16];
    uint16_t a_weighted[3][3][N_SM_16];
    uint16_t ab_acc[3][3][N_SM_16_RES];

    memcpy(extseed, seed, SCABBARD_SYMBYTES);
    polyvec_TC_evaluate_packed(s_weighted, s_packed);

    for (i = 0; i < SCABBARD_L; i++) {
        memset(ab_acc, 0, sizeof(ab_acc));

        for (j = 0; j < SCABBARD_L; j++) {
            extseed[SCABBARD_SYMBYTES + 0] = transpose ? (uint8_t)j : (uint8_t)i;
            extseed[SCABBARD_SYMBYTES + 1] = transpose ? (uint8_t)i : (uint8_t)j;
            pseudoXOF(SCABBARD_POLYBYTES * 8, extseed, (SCABBARD_SYMBYTES + 2) * 8, buf);
            poly_modq_frombytes(&a_ij, buf);

            poly_TC_evaluate(a_weighted, a_ij.coeffs);
            poly_TC_pointwise_acc(ab_acc, a_weighted, s_weighted[j]);
        }

        poly_TC_interpolate(a_ij.coeffs, ab_acc, SCABBARD_Q);
        for (k = 0; k < SCABBARD_N; k++) {
            a_ij.coeffs[k] = (uint16_t)(((a_ij.coeffs[k] + rounding_const) & (uint16_t)SCABBARD_Q) >>
                                        (SCABBARD_EQ - SCABBARD_EP));
        }
        fail |= poly_modp_tobytes_cmp(out + i * SCABBARD_P_POLYBYTES, &a_ij);
    }

    return fail;
#endif
}

void inner_prod_packed(
    poly *r,
    const uint8_t *a_packed,
    const uint8_t *s_packed)
{
#if SCABBARD_HAVE_ASM_MUL
    size_t i;
    size_t k;
    poly a_i;
    uint16_t acc[SCABBARD_N];

    memset(acc, 0, sizeof(acc));

    for (i = 0; i < SCABBARD_L; i++) {
        poly_modp_frombytes(&a_i, a_packed + i * SCABBARD_P_POLYBYTES);
        poly_mul_64_sch_acc(a_i.coeffs, s_packed + i * SCABBARD_S_POLYBYTES, acc, SCABBARD_P);
    }

    for (k = 0; k < SCABBARD_N; k++) {
        r->coeffs[k] = (uint16_t)(acc[k] & SCABBARD_P);
    }
#else
    size_t i;
    poly a_i;
    uint16_t s_weighted[SCABBARD_L][3][3][N_SM_16];
    uint16_t a_weighted[3][3][N_SM_16];
    uint16_t ab_acc[3][3][N_SM_16_RES];

    polyvec_TC_evaluate_packed(s_weighted, s_packed);
    memset(ab_acc, 0, sizeof(ab_acc));

    for (i = 0; i < SCABBARD_L; i++) {
        poly_modp_frombytes(&a_i, a_packed + i * SCABBARD_P_POLYBYTES);
        poly_TC_evaluate(a_weighted, a_i.coeffs);
        poly_TC_pointwise_acc(ab_acc, a_weighted, s_weighted[i]);
    }

    poly_TC_interpolate(r->coeffs, ab_acc, SCABBARD_P);
#endif
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
