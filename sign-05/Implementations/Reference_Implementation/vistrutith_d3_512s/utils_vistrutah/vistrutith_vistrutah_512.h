/*
 * QuickSilver wrapper for Vistrutith-512 constraints.
 */

#ifndef VISTRUTITH_VISTRUTAH_512_H
#define VISTRUTITH_VISTRUTAH_512_H

#include <stdint.h>

#include "params.h"

void vistrutith_512_prover(const params_t* params, uint8_t* a0_tilde, uint8_t* a1_tilde,
                           uint8_t* a2_tilde, const uint8_t* w, const uint8_t* u, uint8_t** V,
                           const uint8_t* owf_in, const uint8_t* owf_out,
                           const uint8_t* chall_2);

void vistrutith_512_verifier(const params_t* params, uint8_t* a0_tilde, const uint8_t* d,
                             uint8_t** Q, const uint8_t* owf_in, const uint8_t* owf_out,
                             const uint8_t* chall_2, const uint8_t* chall_3,
                             const uint8_t* a1_tilde, const uint8_t* a2_tilde);

#endif
