/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#ifndef SAMPLE_H
#define SAMPLE_H
#include "polyvec.h"

// Partial y-vector resampling — aligned with AVX2 Keccak lane widths:
//   y0 + y1 = 4 polynomials → single x4 call
//   y2       = 2 polynomials → single x2 call
void polyvecy_sample_y0y1_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce);
void polyvecy_sample_y2_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce);

// z0+z1 combined: x4 Keccak pre-fetch, per-poly early exit, fixed +4 nonce
int check_reject_sample_z0z1(const polyvecy *z, const polyvecl *c_s, const poly *c_poly, const sparse_challenge *c_sparse, const unsigned char *seed, uint16_t *nonce);
// z2:        x2 Keccak pre-fetch, per-poly early exit, fixed +2 nonce
int check_reject_sample_z2  (const polyvecy *z, const polyveck *c_e, const unsigned char *seed, uint16_t *nonce);

// w 的维度是 K (n)
int check_reject_highbits_w1_sparse(const poly *w1, const poly *w0, const sparse_challenge *c);

// z1 的维度是 L+1 (m+1), h 的维度是 K (n)
int check_reject_norm(const polyvecm1 *z1, const polyveck *h);
int check_reject_hint_range(const polyveck *h);

#endif
