#ifndef SAMPLE_H
#define SAMPLE_H

#include "poly.h"

void poly_sample_f(poly *f, uint8_t *seed, uint8_t nonce);

void poly_sample_g(poly *g, uint8_t *seed, uint8_t nonce);

void poly_sample_r(poly *r, uint8_t *coins, uint8_t nonce);

void poly_sample_m(poly *m, uint8_t *coins, uint8_t nonce);

void poly_get_noisem(poly *r, const uint8_t msg[32], const uint8_t *seed, uint8_t nonce);

void poly_binomial_dist1(poly *r, const uint8_t *seed, uint8_t nonce);

void poly_binomial_dist2(poly *r, const uint8_t *seed, uint8_t nonce);

void poly_bias8_ternary(poly *r, const uint8_t *seed, uint8_t nonce);

#endif
