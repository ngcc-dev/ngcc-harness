#ifndef POLYVEC_H
#define POLYVEC_H

#include "params.h"
#include "poly.h"

typedef struct{
  poly vec[PARAM_K];
} polyvec;


void polyvec_compress10(uint8_t *r, const polyvec *a);
void polyvec_decompress10(polyvec *r, const unsigned char *a);

void polyvec_pk_rnd_decompress(polyvec *r, const uint8_t *a,const uint8_t *seed);

void polyvec_tobytes(uint8_t *r, const polyvec *a);
void polyvec_frombytes(polyvec *r, const uint8_t *a);

void polyvec_ntt(polyvec *r);
void polyvec_invntt(polyvec *r);

void polyvec_pointwise_acc(poly *r, const polyvec *a, const polyvec *b);

void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b);
void polyvec_addq(polyvec *r);
void polyvec_caddq(polyvec *r);
void polyvec_ss_getnoise(polyvec *r, const uint8_t *seed, uint8_t nonce);
void polyvec_ee_getnoise(polyvec *r, const uint8_t *seed, uint8_t nonce);
#endif
