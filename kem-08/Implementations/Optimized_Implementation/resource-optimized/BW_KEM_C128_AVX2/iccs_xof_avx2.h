#ifndef ICCS_XOF_AVX2_H
#define ICCS_XOF_AVX2_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

void iccs_pseudoxof8_bytes(uint8_t *out[8], size_t outlen, const uint8_t *msg[8], size_t msglen);
void iccs_pseudoxof4_bytes(uint8_t *out[4], size_t outlen, const uint8_t *msg[4], size_t msglen);
void kyber_iccs_prf4(uint8_t *out[4], size_t outlen, const uint8_t key[KYBER_SYMBYTES], uint8_t nonce0);
void kyber_iccs_xof_squeezeblocks4(uint8_t *out[4], size_t outblocks, const uint8_t seed[KYBER_SYMBYTES], const uint8_t x[4], const uint8_t y[4]);

#endif
