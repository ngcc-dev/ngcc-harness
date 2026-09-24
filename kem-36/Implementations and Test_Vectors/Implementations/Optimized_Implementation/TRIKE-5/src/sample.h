#ifndef SAMPLE_H
#define SAMPLE_H

#include "trike_types.h"

// Generates the secret key polynomials from a seed.
void generate_secret_key(uint8_t *h0, uint8_t *h1, uint8_t *h2, uint32_t *h0_idx, uint32_t *h1_idx, uint32_t *h2_idx, const uint8_t *seed);

// Generates hash-derived vectors from a seed.
void generate_hash_vectors(uint8_t *t1, uint8_t *t2, uint8_t *r1, const uint8_t *seed);

// Generates an error vector from the message and public key r2.
void generate_error_vector(uint8_t *e0, uint8_t *e1, uint8_t *e2, const uint8_t *msg, const uint8_t *r2);

#endif