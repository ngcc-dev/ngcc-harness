#ifndef VISTRUTITH_D3_512S_H
#define VISTRUTITH_D3_512S_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

/**
 * Size of the public key in bytes.
 */
#define VISTRUTITH_D3_512S_PUBLIC_KEY_SIZE 128
/**
 * Size of the private key in bytes.
 */
#define VISTRUTITH_D3_512S_PRIVATE_KEY_SIZE 128
/**
 * Size of the signature in bytes.
 */
// Signature size is derived from the parameter set. Use the accessor below
// to obtain the runtime value; the macro wraps the accessor for backward
// compatibility in existing code paths.
static inline size_t vistrutith_d3_512s_signature_size(void) {
  return (size_t)vistrutith_params_d3_512s()->sig_size;
}
#define VISTRUTITH_D3_512S_SIGNATURE_SIZE (vistrutith_d3_512s_signature_size())

/**
 * Unpacked private key with pre-computed OWF output and witness.
 */
typedef struct {
  uint8_t owf_input[64];
  uint8_t owf_key[512 / 8];
  uint8_t owf_output[64];
  // witness_size = param->ell / 8,
  uint8_t* witness;
} vistrutith_d3_512s_unpacked_private_key_t;

/* Signature main API */

int vistrutith_d3_512s_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* sk_key, const uint8_t* sk_input);
int vistrutith_d3_512s_sign_with_randomness(const uint8_t* sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len);
// int vistrutith_d3_512s_sign(const uint8_t* sk, const uint8_t* message, size_t message_len, uint8_t* signature, size_t* signature_len);
int vistrutith_d3_512s_verify(const uint8_t* pk, const uint8_t* message, size_t message_len, const uint8_t* signature, size_t signature_len);

// Additional utility functions to unpack sk for split usage 
int vistrutith_d3_512s_validate_keypair(const uint8_t* pk, const uint8_t* sk);
void vistrutith_d3_512s_clear_private_key(uint8_t* key);
int vistrutith_d3_512s_unpack_private_key(vistrutith_d3_512s_unpacked_private_key_t* unpacked_sk, const uint8_t* sk);
void vistrutith_d3_512s_clear_unpacked_private_key(vistrutith_d3_512s_unpacked_private_key_t* key);
int vistrutith_d3_512s_unpacked_sign_with_randomness(const vistrutith_d3_512s_unpacked_private_key_t* unpacked_sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len);

#endif
