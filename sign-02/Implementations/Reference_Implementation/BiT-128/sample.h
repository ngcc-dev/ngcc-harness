/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef SAMPLE_H
#define SAMPLE_H
#include "polyvec.h"

void poly_sample_triangular(int16_t *out, const uint8_t *seed, uint16_t *nonce);
void poly_sample_triangular_3bit(int16_t *out, const uint8_t *seed, uint16_t *nonce);
void poly_sample_triangular_11bit(int16_t *out, const uint8_t *seed, uint16_t *nonce);

/* Check rejection for z0 and z1 */
int check_reject_sample_z0z1(const polyvecy *z, const polyvecl *c_s, const poly *c_poly, const sparse_challenge *c_sparse, const unsigned char *seed, uint16_t *nonce);
/* Check rejection for z2 */
int check_reject_sample_z2   (const polyvecy *z, const polyveck *c_e, const unsigned char *seed, uint16_t *nonce);




int check_reject_norm(const polyvecm1 *z1, const polyveck *h);
int check_reject_hint_range(const polyveck *h);

#endif
