#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct{
  poly vec[KEM_K];
} polyvec;

typedef struct{
  nttpoly vec[KEM_K];
} nttpolyvec;

#define polyvec_uniform POLYMUL_NAMESPACE(_polyvec_uniform)
void polyvec_uniform(polyvec *r, const uint8_t seed[POLYMUL_SYMBYTES], uint16_t nonce);
#define polyvec_noise POLYMUL_NAMESPACE(_polyvec_noise)
void polyvec_noise(polyvec *r, const uint8_t seed[POLYMUL_SYMBYTES], uint16_t nonce);

#define polyvec_ntt POLYMUL_NAMESPACE(_polyvec_ntt)
void polyvec_ntt(nttpolyvec *r, const polyvec *a, const int16_t *pdata);
#define polyvec_invntt_tomont POLYMUL_NAMESPACE(_polyvec_invntt_tomont)
void polyvec_invntt_tomont(nttpolyvec *r, const nttpolyvec *a, const int16_t *pdata);

#define polyvec_basemul_acc_montgomery POLYMUL_NAMESPACE(_polyvec_basemul_acc_montgomery)
void polyvec_basemul_acc_montgomery(nttpoly *r, const nttpolyvec *a, const nttpolyvec *b, const int16_t *pdata);

#define polyvec_crt POLYMUL_NAMESPACE(_polyvec_crt)
void polyvec_crt(polyvec *r, const nttpolyvec *a, const nttpolyvec *b);

#define polyvec_matrix_vector_mul POLYMUL_NAMESPACE(_polyvec_matrix_vector_mul)
void polyvec_matrix_vector_mul(polyvec *t, const polyvec a[KEM_K], const polyvec *s, int transpose);

#define polyvec_iprod POLYMUL_NAMESPACE(_polyvec_iprod)
void polyvec_iprod(poly *r, const polyvec *a, const polyvec *b);

void saber_matrix_vector_mul(polyvec *r, const polyvec a[KEM_K], const polyvec *v);
void saber_iprod(poly *r, const polyvec *a, const polyvec *b);

#endif
