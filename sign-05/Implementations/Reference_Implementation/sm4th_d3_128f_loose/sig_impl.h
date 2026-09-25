/*
 * The main sign and verify functions for sm4th_d3_128f_loose signature scheme
 * Accelerated by xor_u_tilde_masked_batch4 and chall3_batch4_sm3_hash
 */

#ifndef SIG_IMPL_H
#define SIG_IMPL_H

#include <stdint.h>
#include <stddef.h>

#include "params.h"

void sm4th_sign(const params_t* params, uint8_t* sig, const uint8_t* msg, size_t msglen,
                const uint8_t* owf_key, const uint8_t* owf_input, const uint8_t* owf_output,
                const uint8_t* witness, const uint8_t* rho, size_t rholen);

int sm4th_verify(const params_t* params, const uint8_t* msg, size_t msglen,
                 const uint8_t* sig, const uint8_t* owf_input, const uint8_t* owf_output);

#endif
