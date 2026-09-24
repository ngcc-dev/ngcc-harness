/**
 * \file brqc.h
 * \brief Function declarations for the BRQC.PKE public key encryption scheme
 *
 * BRQC.PKE is an IND-CPA secure public key encryption scheme based on the
 * Ideal Blockwise Rank Syndrome Decoding (IBRD) problem. It encodes plaintexts
 * using Gabidulin codes and relies on blockwise rank metric errors for security.
 * See algorithm specification, Figure 2.
 */

#ifndef BRQC_PKE_H
#define BRQC_PKE_H

#include "rbc_vec.h"
#include "rbc_qre.h"

void brqc_pke_keygen(uint8_t* pk, uint8_t* sk);
void brqc_pke_encrypt(rbc_qre u, rbc_qre v, const rbc_vec m, uint8_t* theta, const uint8_t* pk);
void brqc_pke_decrypt(rbc_vec m, const rbc_qre u, const rbc_qre v, const uint8_t* sk);

#endif