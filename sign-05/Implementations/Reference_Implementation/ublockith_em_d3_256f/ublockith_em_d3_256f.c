/*
Part of the codes are adapted from the UBLOCKITH implementation at https://ublockith.info/,
which uses SPDX-License-Identifier: MIT.
*/

#include "ublockith_em_d3_256f.h"

#include "compat.h"
#include "sig_impl.h"
#include "params.h"
#include "owf.h"
#include "utils_ublock/ublock.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// memory layout of the public key: OWF input || OWF output
#define PK_INPUT(pk) (pk)
#define PK_OUTPUT(pk) (&pk[32])

// memory layout of the private key: OWF input || OWF key
#define SK_INPUT(sk) (sk)
#define SK_KEY(sk) (&sk[32])
#define ptr_get_bit(value, index) (((value)[(index) / 8] >> ((index) % 8)) & 1)

int ublockith_em_d3_256f_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* sk_key,
                             const uint8_t* sk_input) {
  if ((ptr_get_bit(sk_key, 0) & ptr_get_bit(sk_key, 1)) != 0) {
    return 1;
  }

  memcpy(SK_KEY(sk), sk_key, 32);
  memcpy(SK_INPUT(sk), sk_input, 32);

  ublockith_em_d3_256f_owf(SK_KEY(sk), SK_INPUT(sk), PK_OUTPUT(pk));
  memcpy(PK_INPUT(pk), SK_INPUT(sk), 32);
  return 0;
}

int ublockith_em_d3_256f_unpack_private_key(ublockith_em_d3_256f_unpacked_private_key_t* unpacked_sk,
                                         const uint8_t* sk) {
  if (!unpacked_sk || !sk) {
    return -1;
  }
  if ((ptr_get_bit(SK_KEY(sk), 0) & ptr_get_bit(SK_KEY(sk), 1)) != 0) {
    return 1;
  }

  memcpy(unpacked_sk->owf_input, SK_INPUT(sk), sizeof(unpacked_sk->owf_input));
  memcpy(unpacked_sk->owf_key, SK_KEY(sk), sizeof(unpacked_sk->owf_key));

  ublockith_em_d3_256f_owf(SK_KEY(sk), SK_INPUT(sk), unpacked_sk->owf_output);

  const params_t* params = ublockith_params_em_d3_256f();
  const unsigned int witness_bytes = params->ell / 8;
  unpacked_sk->witness = malloc(witness_bytes);
  assert(unpacked_sk->witness);
  ublock_extend_witness(params, unpacked_sk->witness, unpacked_sk->owf_key, unpacked_sk->owf_input);
  return 0;
}

int ublockith_em_d3_256f_validate_keypair(const uint8_t* pk, const uint8_t* sk) {
  if (!sk || !pk) {
    return -1;
  }

  if ((ptr_get_bit(SK_KEY(sk), 0) & ptr_get_bit(SK_KEY(sk), 1)) != 0) {
    return 1;
  }

  uint8_t pk_check[UBLOCKITH_EM_D3_256F_PUBLIC_KEY_SIZE];
  ublockith_em_d3_256f_owf(SK_KEY(sk), SK_INPUT(sk), PK_OUTPUT(pk_check));
  memcpy(PK_INPUT(pk_check), SK_INPUT(sk), 32);
  return timingsafe_bcmp(pk_check, pk, sizeof(pk_check)) == 0 ? 0 : 2;
}

int ublockith_em_d3_256f_sign_with_randomness(const uint8_t* sk, const uint8_t* message,
                                           size_t message_len, const uint8_t* rho, size_t rho_len,
                                           uint8_t* signature, size_t* signature_len) {
  if (!sk || !signature || !signature_len || *signature_len < UBLOCKITH_EM_D3_256F_SIGNATURE_SIZE ||
      (!rho && rho_len) || (!message && message_len)) {
    return -1;
  }

  ublockith_em_d3_256f_unpacked_private_key_t unpacked_sk;
  int ret = ublockith_em_d3_256f_unpack_private_key(&unpacked_sk, sk);
  if (ret) {
    ublockith_em_d3_256f_clear_unpacked_private_key(&unpacked_sk);
    return ret;
  }

  ret = ublockith_em_d3_256f_unpacked_sign_with_randomness(&unpacked_sk, message, message_len, rho,
                                                         rho_len, signature, signature_len);
  ublockith_em_d3_256f_clear_unpacked_private_key(&unpacked_sk);
  return ret;
}

int ublockith_em_d3_256f_unpacked_sign_with_randomness(
    const ublockith_em_d3_256f_unpacked_private_key_t* sk, const uint8_t* message,
    size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature,
    size_t* signature_len) {
  if (!sk || !signature || !signature_len || *signature_len < UBLOCKITH_EM_D3_256F_SIGNATURE_SIZE ||
      (!message && message_len)) {
    return -1;
  }

  const params_t* params = ublockith_params_em_d3_256f();
  ublockith_sign(params, signature, message, message_len, sk->owf_key, sk->owf_input,
                 sk->owf_output, sk->witness, rho, rho_len);
  *signature_len = UBLOCKITH_EM_D3_256F_SIGNATURE_SIZE;
  return 0;
}

int ublockith_em_d3_256f_verify(const uint8_t* pk, const uint8_t* message, size_t message_len,
                             const uint8_t* signature, size_t signature_len) {
  if (!pk || !signature || signature_len != UBLOCKITH_EM_D3_256F_SIGNATURE_SIZE ||
      (!message && message_len)) {
    return -1;
  }

  const params_t* params = ublockith_params_em_d3_256f();
  return ublockith_verify(params, message, message_len, signature, PK_INPUT(pk), PK_OUTPUT(pk));
}

void ublockith_em_d3_256f_clear_private_key(uint8_t* key) {
  explicit_bzero(key, UBLOCKITH_EM_D3_256F_PRIVATE_KEY_SIZE);
}

void ublockith_em_d3_256f_clear_unpacked_private_key(ublockith_em_d3_256f_unpacked_private_key_t* key) {
  if (!key) {
    return;
  }
  if (key->witness) {
    const params_t* params = ublockith_params_em_d3_256f();
    const unsigned int witness_bytes = params->ell / 8;
    explicit_bzero(key->witness, witness_bytes);
    free(key->witness);
    key->witness = NULL;
  }
  explicit_bzero(key, sizeof(*key));
}
