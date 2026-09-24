/**
 * \file bra.h
 * \brief Function declarations for the BRA.PKE public key encryption scheme
 *
 * BRA.PKE is an IND-CPA secure public key encryption scheme based on the
 * Ideal Blockwise Rank Syndrome Decoding (IBRD) problem. It encodes plaintexts
 * using Extended Gabidulin codes and relies on blockwise rank metric errors for security.
 * See algorithm specification, Figure 2.
 */

#ifndef BRA_PKE_H
#define BRA_PKE_H

#include "rbc_vec.h"
#include "rbc_qre.h"

void bra_pke_keygen(uint8_t* pk, uint8_t* sk);
void bra_pke_encrypt(rbc_qre u, rbc_qre v, const rbc_vec m, uint8_t* theta, const uint8_t* pk);
void bra_pke_decrypt(rbc_vec m, const rbc_qre u, const rbc_qre v, const uint8_t* sk);

#endif