#ifndef POLY_H
#define POLY_H

#include <immintrin.h>
#include <stdint.h>
#include "params.h"

typedef struct {
    int16_t coeffs[NTRE_N];
} poly __attribute__((aligned(32)));

void poly_tobytes(uint8_t r[NTRE_POLYBYTES], const poly *a);
void poly_frombytes(poly *r, const uint8_t a[NTRE_POLYBYTES]);

void poly_cbd1(poly *r, const uint8_t buf[NTRE_SAMPLEBYTES]);
void poly_cbd1_prime(poly *r,
                     const uint8_t msg[NTRE_MSGBYTES],
                     const uint8_t coins[NTRE_ERROR_RANDOMBYTES]);
void poly_msg_mod2_to_bytes(uint8_t msg[NTRE_MSGBYTES], const poly *a);

void poly_ntt(poly *r);
void poly_invntt(poly *r);
int  poly_baseinv(poly *r, const poly *a);
void poly_basemul(poly *r, const poly *a, const poly *b);
void poly_basemul_add(poly *r, const poly *a, const poly *b, const poly *c);
void poly_sub(poly *r, const poly *a, const poly *b);
void poly_double(poly *r, const poly *a);
void poly_double_add_one(poly *r, const poly *a);

#endif
