/*
 *  The QuickSilver implementation of the SM4-based SM4th signature scheme
 * Optimized via bit matrix transposition and precomputation for byte-to-field mapping, and also with a more compact proof format.
 */

#ifndef SM4TH_SM4_128_H
#define SM4TH_SM4_128_H

#include <stdint.h>

#include "params.h"
#include "utils_sm4/sm4.h"


void sm4_128_prover(const params_t* params, uint8_t* a0_tilde, uint8_t* a1_tilde,
                    uint8_t* a2_tilde, const uint8_t* w, const uint8_t* u, uint8_t** V,
                    const uint8_t* owf_in, const uint8_t* owf_out,
                    const uint8_t* chall_2);

void sm4_128_verifier(const params_t* params, uint8_t* a0_tilde, const uint8_t* d,
                      uint8_t** Q, const uint8_t* owf_in, const uint8_t* owf_out,
                      const uint8_t* chall_2, const uint8_t* chall_3,
                      const uint8_t* a1_tilde, const uint8_t* a2_tilde);

#endif
