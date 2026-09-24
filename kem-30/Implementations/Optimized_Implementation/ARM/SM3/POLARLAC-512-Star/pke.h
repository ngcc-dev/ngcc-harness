/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the POLARLAC public-key encryption layer for the optimized POLARLAC-512 instance.
*/

#ifndef PKE_H
#define PKE_H

#include <stdint.h>
#include "params.h"

/// @brief Generate a Polar-LAC.PKE key pair from an input seed.
/// @param[out] pk Base address of the public key output buffer.
/// @param[out] sk Base address of the secret key output buffer.
/// @param[in] seed Base address of the input seed byte array.
/// @return 0 on success; otherwise a negative error code.
int PKE_KeyGen(uint8_t *pk, uint8_t *sk, const uint8_t *seed);

/// @brief Encrypt a fixed-length message under a public key and randomness seed.
/// @param[out] c Base address of the ciphertext output buffer.
/// @param[in] pk Base address of the public key input buffer.
/// @param[in] m Base address of the plaintext message input buffer.
/// @param[in] seed Base address of the seed used to sample `r`, `e1`, and `e2`.
/// @return None.
void PKE_Encrypt(uint8_t *c, const uint8_t *pk, const uint8_t *m, const uint8_t *seed);

/// @brief Decrypt a ciphertext under the packed secret key.
/// @param[out] m Base address of the recovered message output buffer.
/// @param[in] c Base address of the ciphertext input buffer.
/// @param[in] sk Base address of the secret key input buffer.
/// @return None.
void PKE_Decrypt(uint8_t *m, const uint8_t *c, const uint8_t *sk);

#endif
