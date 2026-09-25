/*

SM4th-EM-d2-s: Even-Mansour construction with SM4.
  OWF: SM4_x(k) XOR k = y
  pk = (x, y)  — x is the SM4 key (public), y = SM4_x(k) XOR k
  sk = (x, k)  — k is the SM4 plaintext (secret)

Memory layout:
  sk[0..15]  = sk_input = x (SM4 key, public)
  sk[16..31] = sk_key   = k (SM4 input, secret)

  owf_input  = sk_input = x (SM4 key, public)
  owf_key    = sk_key   = k (SM4 input, secret)
  owf_output = SM4_x(k) XOR k
*/

#include "sm4th_em_d2_128s_loose.h"
#include "sm4.h"
#include "compat.h"
#include "sig_impl.h"
#include "params.h"
#include "owf.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// memory layout of the public key:
// EM: SM4 key (public x) || OWF output (SM4_x(k) XOR k)
#define PK_INPUT(pk) (pk)
#define PK_OUTPUT(pk) (&pk[16])

// memory layout of the secret key:
// EM: SM4 key (public x) || SM4 input (secret k)
#define SK_INPUT(sk) (sk)
#define SK_KEY(sk) (&sk[16])
#define EM_SECRET_WITNESS_VALID(input) ((ptr_get_bit((input), 0) & ptr_get_bit((input), 1)) == 0)

int sm4th_em_d2_128s_loose_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* sk_key, const uint8_t* sk_input) {
  if (!EM_SECRET_WITNESS_VALID(sk_key)) {
    return 1;
  }

  memcpy(SK_KEY(sk), sk_key, 128 / 8);
  memcpy(SK_INPUT(sk), sk_input, 16);

  // owf: SM4_x(k) XOR k, where x=sk_input (SM4 key), k=sk_key (SM4 input)
  sm4th_em_d2_128s_loose_owf(SK_KEY(sk), SK_INPUT(sk), PK_OUTPUT(pk));
  memcpy(PK_INPUT(pk), SK_INPUT(sk), 16);

  return 0;
}

int sm4th_em_d2_128s_loose_unpack_private_key(sm4th_em_d2_128s_loose_unpacked_private_key_t* unpacked_sk, const uint8_t* sk) {
  if (!unpacked_sk || !sk) {
    return -1;
  }

  if (!EM_SECRET_WITNESS_VALID(SK_KEY(sk))) {
    return 1;
  }

  // owf_input = sk_input = SM4 key (public x)
  // owf_key   = sk_key   = SM4 input (secret k)
  memcpy(unpacked_sk->owf_input, SK_INPUT(sk), sizeof(unpacked_sk->owf_input));
  memcpy(unpacked_sk->owf_key, SK_KEY(sk), sizeof(unpacked_sk->owf_key));

  sm4th_em_d2_128s_loose_owf(SK_KEY(sk), SK_INPUT(sk), unpacked_sk->owf_output);

  // For EM witness: sm4_extend_witness(params, w, owf_key, owf_input)
  //   owf_key = k (SM4 input, secret) → used as "key" argument
  //   owf_input = x (SM4 key, public) → used as "in" argument
  const params_t* params = sm4th_params_em_128s();
  const unsigned int witness_bytes = params->ell / 8;
  unpacked_sk->witness = malloc(witness_bytes);
  assert(unpacked_sk->witness);
  sm4_extend_witness(params, unpacked_sk->witness, unpacked_sk->owf_key, unpacked_sk->owf_input);

  return 0;
}

int sm4th_em_d2_128s_loose_validate_keypair(const uint8_t* pk, const uint8_t* sk) {
  if (!sk || !pk) {
    return -1;
  }

  if (!EM_SECRET_WITNESS_VALID(SK_KEY(sk))) {
    return 1;
  }

  uint8_t pk_check[32];
  sm4th_em_d2_128s_loose_owf(SK_KEY(sk), SK_INPUT(sk), PK_OUTPUT(pk_check));
  memcpy(PK_INPUT(pk_check), SK_INPUT(sk), 16);

  return timingsafe_bcmp(pk_check, pk, sizeof(pk_check)) == 0 ? 0 : 2;
}

int sm4th_em_d2_128s_loose_sign_with_randomness(const uint8_t* sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len) {
  if (!sk || !signature || !signature_len || *signature_len < SM4TH_EM_D2_S_LOOSE_SIGNATURE_SIZE || (!rho && rho_len) || (!message && message_len)) {
    return -1;
  }

  sm4th_em_d2_128s_loose_unpacked_private_key_t unpacked_sk;
  int ret = sm4th_em_d2_128s_loose_unpack_private_key(&unpacked_sk, sk);
  if (ret) {
    sm4th_em_d2_128s_loose_clear_unpacked_private_key(&unpacked_sk);
    return ret;
  }

  ret = sm4th_em_d2_128s_loose_unpacked_sign_with_randomness(&unpacked_sk, message, message_len, rho, rho_len, signature, signature_len);
  sm4th_em_d2_128s_loose_clear_unpacked_private_key(&unpacked_sk);
  return ret;
}

int sm4th_em_d2_128s_loose_unpacked_sign_with_randomness(const sm4th_em_d2_128s_loose_unpacked_private_key_t* sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len) {
  if (!sk || !signature || !signature_len || *signature_len < SM4TH_EM_D2_S_LOOSE_SIGNATURE_SIZE || (!message && message_len)) {
    return -1;
  }

  const params_t* params = sm4th_params_em_128s();
  sm4th_sign(params, signature, message, message_len, sk->owf_key, sk->owf_input, sk->owf_output,
             sk->witness, rho, rho_len);
  *signature_len = SM4TH_EM_D2_S_LOOSE_SIGNATURE_SIZE;

  return 0;
}

int sm4th_em_d2_128s_loose_verify(const uint8_t* pk, const uint8_t* message, size_t message_len, const uint8_t* signature, size_t signature_len) {
  if (!pk || !signature || signature_len != SM4TH_EM_D2_S_LOOSE_SIGNATURE_SIZE || (!message && message_len)) {
    return -1;
  }

  const params_t* params = sm4th_params_em_128s();
  return sm4th_verify(params, message, message_len, signature, PK_INPUT(pk), PK_OUTPUT(pk));
}

void sm4th_em_d2_128s_loose_clear_private_key(uint8_t* key) {
  explicit_bzero(key, SM4TH_EM_D2_S_LOOSE_PRIVATE_KEY_SIZE);
}

void sm4th_em_d2_128s_loose_clear_unpacked_private_key(sm4th_em_d2_128s_loose_unpacked_private_key_t* key) {
  if (!key) return;
  if (key->witness) {
    const params_t* params = sm4th_params_em_128s();
    const unsigned int witness_bytes = params->ell / 8;
    explicit_bzero(key->witness, witness_bytes);
    free(key->witness);
    key->witness = NULL;
  }
  explicit_bzero(key, sizeof(*key));
}
