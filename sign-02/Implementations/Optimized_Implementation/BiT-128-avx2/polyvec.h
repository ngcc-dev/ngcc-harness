/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#ifndef POLYVEC_H
#define POLYVEC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct {
  poly vec[BIT_K];
} polyveck;

typedef struct {
  poly vec[BIT_L];
} polyvecl;

typedef struct {
  poly vec[BIT_L + 1];
} polyvecm1;

typedef struct {
  poly vec[BIT_Y];
} polyvecy;

typedef struct {
    polyvecl vec[BIT_K];
} poly_matrix;

typedef struct {
  poly_ntt vec[BIT_K];
} polyveck_ntt;

typedef struct {
  poly_ntt vec[BIT_L];
} polyvecl_ntt;

typedef struct {
    polyvecl_ntt vec[BIT_K];
} poly_matrix_ntt;

void poly_matrix_expand_ntt(poly_matrix_ntt *A_ntt, const unsigned char *seed);
void polyvec_sample_S1_avx(polyvecl *s, polyveck *e, const uint8_t *seed, uint16_t *nonce);
void poly_matrix_mul_vector_ntt(polyveck *result, const poly_matrix_ntt *A_ntt, const polyvecl_ntt *v_ntt);

void polyvecl_to_ntt(polyvecl_ntt *result, const polyvecl *v);
void polyvecy_y1_to_ntt(polyvecl_ntt *result, const polyvecy *y);
void polyveck_to_ntt(polyveck_ntt *result, const polyveck *v);
void polyveck_from_ntt(polyveck *result, const polyveck_ntt *v_ntt);

void polyveck_add(polyveck *result, const polyveck *a, const polyveck *b);
void polyveck_add_eta1(polyveck *result, const polyveck *a, const polyveck *eta1);
void polyveck_decompose_b(polyveck *b1, polyveck *b0, const polyveck *b);
void polyveck_b1_scaled_to_ntt(polyveck_ntt *b1_ntt, const polyveck *b1);

void polyvecl_mul_challenge_no_reduce(polyvecl *result, const polyvecl *v, const sparse_challenge *c);
void polyveck_mul_challenge_no_reduce(polyveck *result, const polyveck *v, const sparse_challenge *c);

void polyveck_compute_w_ntt(polyveck *w, const poly_matrix_ntt *A_0_ntt, const polyveck_ntt *b_ntt, const polyvecl_ntt *y1_ntt, const poly_ntt *y0_ntt, const polyvecy *y);
void polyveck_highbits(polyveck *w1, const polyveck *w);
void polyveck_pack_w1(unsigned char *packed, const polyveck *w1);
void polyveck_make_hint_compressed(polyveck *h, const polyveck *w1, const polyveck *w, const polyveck *z2, const polyveck *b0c, const poly *c, uint8_t b);

void polyveck_compute_w_hat_prime_ntt(polyveck *w_hat_prime, const poly_matrix_ntt *A_0_ntt, const polyveck_ntt *b1_ntt, const polyvecl_ntt *z1_tail_ntt, const poly_ntt *z0_ntt, const poly *z0, const poly *c_poly);
void polyveck_compute_w_prime(polyveck *w_prime, const polyveck *w_hat_prime, const polyveck *h);
#endif
