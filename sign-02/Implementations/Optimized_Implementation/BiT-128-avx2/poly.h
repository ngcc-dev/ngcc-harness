/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

static inline int32_t ct_abs(int32_t x) {
    int32_t mask = x >> 31;
    return (x ^ mask) - mask;
}

static inline int32_t ct_max(int32_t x, int32_t y) {
    int32_t diff = x - y;
    int32_t mask = diff >> 31;
    return x - (diff & mask);
}

typedef struct {
  int16_t coeffs[BIT_N];
} __attribute__((aligned(32))) poly;

typedef struct {
  int16_t coeffs[BIT_N];
} __attribute__((aligned(32))) poly_ntt;

typedef struct {
  uint8_t pos[BIT_TAU];
  int8_t sign[BIT_TAU];
} sparse_challenge;

void poly_sample_S1(poly *a, const uint8_t *seed, uint16_t *nonce);
void poly_sample_S1_x4(poly *p0, poly *p1, poly *p2, poly *p3,
                       const uint8_t *seed, uint16_t *nonce);
void poly_sample_S1_x2(poly *p0, poly *p1,
                       const uint8_t *seed, uint16_t *nonce);
void poly_challenge(poly *a, const unsigned char challenge[BIT_CHALLENGEBYTES]);
void poly_challenge_to_sparse(sparse_challenge *s, const poly *c);
void poly_cneg(poly *result, uint8_t b);
void poly_add(poly *result, const poly *a, const poly *b);
void poly_add_eta1(poly *result, const poly *a, const poly *eta1);
void poly_sub(poly *result, const poly *a, const poly *b);
void poly_to_ntt(poly_ntt *result, const poly *a);
void poly_from_ntt(poly *result, const poly_ntt *a);
void poly_ntt_basemul_raw(poly_ntt *result, const poly_ntt *a, const poly_ntt *b);
void poly_ntt_basemul_acc_raw(poly_ntt *result, const poly_ntt *a, const poly_ntt *b);
void poly_ntt_montgomery_lift(poly_ntt *result);
void poly_ntt_zero(poly_ntt *result);
void poly_mul_challenge_no_reduce(poly *result, const poly *a, const sparse_challenge *c);
void poly_highbits(poly *w1, const poly *w);
void poly_decompose_b(poly *b1, poly *b0, const poly *b);
void decompose_hint(int16_t *highbits, int16_t r);
void poly_pack_w1(unsigned char *r, const poly *a);
int poly_check_reject_highbits_w1_sparse(const poly *w1, const poly *w0, const sparse_challenge *c);
#include "reduce.h"

#endif
