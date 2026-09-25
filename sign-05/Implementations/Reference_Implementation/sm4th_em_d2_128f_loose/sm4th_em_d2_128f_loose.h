/*
Based on sig_impl.h, this file defines the SM4th-EM-d2-s variant.
Even-Mansour construction: SM4_x(k) XOR k = y
  pk = (x, y) where x is the SM4 key (public) and y is the OWF output
  sk = (x, k) where k is the SM4 input (secret)
*/

#ifndef SM4TH_EM_D2_F_LOOSE_H
#define SM4TH_EM_D2_F_LOOSE_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

/**
 * Size of the public key in bytes.
 */
#define SM4TH_EM_D2_F_LOOSE_PUBLIC_KEY_SIZE 32
/**
 * Size of the private key in bytes.
 */
#define SM4TH_EM_D2_F_LOOSE_PRIVATE_KEY_SIZE 32
/**
 * Size of the signature in bytes.
 */
static inline size_t sm4th_em_d2_128f_loose_signature_size(void) {
  return (size_t)sm4th_params_em_128f()->sig_size;
}
#define SM4TH_EM_D2_F_LOOSE_SIGNATURE_SIZE (sm4th_em_d2_128f_loose_signature_size())

/**
 * Unpacked private key with pre-computed OWF output and witness.
 */
typedef struct {
  uint8_t owf_input[16];   /* sk_input = SM4 key   (public) */
  uint8_t owf_key[128 / 8]; /* sk_key   = SM4 input (secret) */
  uint8_t owf_output[16];  /* SM4_x(k) XOR k       (public) */
  uint8_t* witness;
} sm4th_em_d2_128f_loose_unpacked_private_key_t;

/* Signature main API */

int sm4th_em_d2_128f_loose_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* sk_key, const uint8_t* sk_input);
int sm4th_em_d2_128f_loose_sign_with_randomness(const uint8_t* sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len);
int sm4th_em_d2_128f_loose_verify(const uint8_t* pk, const uint8_t* message, size_t message_len, const uint8_t* signature, size_t signature_len);

int sm4th_em_d2_128f_loose_validate_keypair(const uint8_t* pk, const uint8_t* sk);
void sm4th_em_d2_128f_loose_clear_private_key(uint8_t* key);
int sm4th_em_d2_128f_loose_unpack_private_key(sm4th_em_d2_128f_loose_unpacked_private_key_t* unpacked_sk, const uint8_t* sk);
void sm4th_em_d2_128f_loose_clear_unpacked_private_key(sm4th_em_d2_128f_loose_unpacked_private_key_t* key);
int sm4th_em_d2_128f_loose_unpacked_sign_with_randomness(const sm4th_em_d2_128f_loose_unpacked_private_key_t* unpacked_sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len);

#endif
