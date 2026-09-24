/*
Based on sig_impl.h, this file defines the ublockith-d3-256s variant.
*/

#ifndef UBLOCKITH_EM_D3_256S_H
#define UBLOCKITH_EM_D3_256S_H

#include <stddef.h>
#include <stdint.h>

#include "params.h"

/**
 * Size of the public key in bytes.
 * Layout: OWF input (32B) || OWF output (32B).
 */
#define UBLOCKITH_EM_D3_256S_PUBLIC_KEY_SIZE 64
/**
 * Size of the private key in bytes.
 * Layout: OWF input (32B) || OWF key (32B).
 */
#define UBLOCKITH_EM_D3_256S_PRIVATE_KEY_SIZE 64

static inline size_t ublockith_em_d3_256s_signature_size(void) {
  return (size_t)ublockith_params_em_d3_256s()->sig_size;
}
#define UBLOCKITH_EM_D3_256S_SIGNATURE_SIZE (ublockith_em_d3_256s_signature_size())

typedef struct {
  uint8_t owf_input[32];
  uint8_t owf_key[32];
  uint8_t owf_output[32];
  uint8_t* witness;
} ublockith_em_d3_256s_unpacked_private_key_t;

int ublockith_em_d3_256s_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* sk_key,
                             const uint8_t* sk_input);
int ublockith_em_d3_256s_sign_with_randomness(const uint8_t* sk, const uint8_t* message,
                                           size_t message_len, const uint8_t* rho, size_t rho_len,
                                           uint8_t* signature, size_t* signature_len);
int ublockith_em_d3_256s_verify(const uint8_t* pk, const uint8_t* message, size_t message_len,
                             const uint8_t* signature, size_t signature_len);

int ublockith_em_d3_256s_validate_keypair(const uint8_t* pk, const uint8_t* sk);
void ublockith_em_d3_256s_clear_private_key(uint8_t* key);
int ublockith_em_d3_256s_unpack_private_key(ublockith_em_d3_256s_unpacked_private_key_t* unpacked_sk,
                                         const uint8_t* sk);
void ublockith_em_d3_256s_clear_unpacked_private_key(ublockith_em_d3_256s_unpacked_private_key_t* key);
int ublockith_em_d3_256s_unpacked_sign_with_randomness(
    const ublockith_em_d3_256s_unpacked_private_key_t* unpacked_sk, const uint8_t* message,
    size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature,
    size_t* signature_len);

#endif
