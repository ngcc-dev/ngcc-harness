/*
Based on sig_impl.h, this file defines variants of sm4th_s/f.
*/

#ifndef SM4TH_D3_S_LOOSE_H
#define SM4TH_D3_S_LOOSE_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

/**
 * Size of the public key in bytes.
 */
#define SM4TH_D3_S_LOOSE_PUBLIC_KEY_SIZE 32
/**
 * Size of the private key in bytes.
 */
#define SM4TH_D3_S_LOOSE_PRIVATE_KEY_SIZE 32
/**
 * Size of the signature in bytes.
 */
// Signature size is derived from the parameter set. Use the accessor below
// to obtain the runtime value; the macro wraps the accessor for backward
// compatibility in existing code paths.
static inline size_t sm4th_d3_128s_loose_signature_size(void) {
  return (size_t)sm4th_params_d3_s()->sig_size;
}
#define SM4TH_D3_S_LOOSE_SIGNATURE_SIZE (sm4th_d3_128s_loose_signature_size())

/**
 * Unpacked private key with pre-computed OWF output and witness.
 */
typedef struct {
  uint8_t owf_input[16];
  uint8_t owf_key[128 / 8];
  uint8_t owf_output[16];
  // witness_size = param->ell / 8,
  uint8_t* witness;
} sm4th_d3_128s_loose_unpacked_private_key_t;

/* Signature main API */

int sm4th_d3_128s_loose_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* sk_key, const uint8_t* sk_input);
int sm4th_d3_128s_loose_sign_with_randomness(const uint8_t* sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len);
// int sm4th_d3_128s_loose_sign(const uint8_t* sk, const uint8_t* message, size_t message_len, uint8_t* signature, size_t* signature_len);
int sm4th_d3_128s_loose_verify(const uint8_t* pk, const uint8_t* message, size_t message_len, const uint8_t* signature, size_t signature_len);

// Additional utility functions to unpack sk for split usage 
int sm4th_d3_128s_loose_validate_keypair(const uint8_t* pk, const uint8_t* sk);
void sm4th_d3_128s_loose_clear_private_key(uint8_t* key);
int sm4th_d3_128s_loose_unpack_private_key(sm4th_d3_128s_loose_unpacked_private_key_t* unpacked_sk, const uint8_t* sk);
void sm4th_d3_128s_loose_clear_unpacked_private_key(sm4th_d3_128s_loose_unpacked_private_key_t* key);
int sm4th_d3_128s_loose_unpacked_sign_with_randomness(const sm4th_d3_128s_loose_unpacked_private_key_t* unpacked_sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len);

#endif

