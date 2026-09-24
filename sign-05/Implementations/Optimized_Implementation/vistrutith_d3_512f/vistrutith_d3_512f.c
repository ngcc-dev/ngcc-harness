#include "vistrutith_d3_512f.h"
#include "compat.h"
#include "sig_impl.h"
#include "params.h"
#include "owf.h"
#include "utils.h"
#include "utils_vistrutah/vistrutith_witness.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// memory layout of the public key: OWF input || OWF output
// split into two parts
#define PK_INPUT(pk) (pk)
#define PK_OUTPUT(pk) (&pk[64])

// memory layout of the extended secret key: OWF input || OWF key
// &sk[64] is the 512-bit OWF key
// sk[0..63] is the OWF input
#define SK_INPUT(sk) (sk)
#define SK_KEY(sk) (&sk[64])

int vistrutith_d3_512f_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* sk_key, const uint8_t* sk_input) {

  memcpy(SK_KEY(sk), sk_key, 512 / 8);
  memcpy(SK_INPUT(sk), sk_input, 64);

  // compute AES output as the PK_OUTPUT, using
  vistrutith_d3_512f_owf(SK_KEY(sk), SK_INPUT(sk), PK_OUTPUT(pk));
  // copy AES input to public key
  memcpy(PK_INPUT(pk), SK_INPUT(sk), 64);

  // declassify public key
  return 0;
}

// given sk, generate extended witness and store owf_key and owf_input into unpacked_sk
int vistrutith_d3_512f_unpack_private_key(vistrutith_d3_512f_unpacked_private_key_t* unpacked_sk, const uint8_t* sk) {
  if (!unpacked_sk || !sk) {
    return -1;
  }

  // copy OWF input and key to unpacked_sk
  memcpy(unpacked_sk->owf_input, SK_INPUT(sk), sizeof(unpacked_sk->owf_input));
  memcpy(unpacked_sk->owf_key, SK_KEY(sk), sizeof(unpacked_sk->owf_key));

  vistrutith_d3_512f_owf(SK_KEY(sk), SK_INPUT(sk), unpacked_sk->owf_output);
  // declassify OWF output

  // generate extended witness using sk: owf_key, owf_input
  const params_t* params = vistrutith_params_d3_512f();
  const unsigned int witness_bytes = params->ell / 8;
  unpacked_sk->witness = malloc(witness_bytes);
  assert(unpacked_sk->witness);
  vistrutith_extend_witness(params, unpacked_sk->witness, unpacked_sk->owf_key,
                            unpacked_sk->owf_input);
  assert(witness_bytes == VISTRUTAH_WITNESS_TOTAL_BYTES);

  return 0;
}

// check if the keypair is valid
int vistrutith_d3_512f_validate_keypair(const uint8_t* pk, const uint8_t* sk) {
  if (!sk || !pk) {
    return -1;
  }

  if ((ptr_get_bit(SK_KEY(sk), 0) & ptr_get_bit(SK_KEY(sk), 1)) != 0) {
    return 1;
  }

  uint8_t pk_check[VISTRUTITH_D3_512F_PUBLIC_KEY_SIZE];
  vistrutith_d3_512f_owf(SK_KEY(sk), SK_INPUT(sk), PK_OUTPUT(pk_check));
  memcpy(PK_INPUT(pk_check), SK_INPUT(sk), 64);

  return timingsafe_bcmp(pk_check, pk, sizeof(pk_check)) == 0 ? 0 : 2;
}

int vistrutith_d3_512f_sign_with_randomness(const uint8_t* sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len) {
  if (!sk || !signature || !signature_len || *signature_len < VISTRUTITH_D3_512F_SIGNATURE_SIZE || (!rho && rho_len) || (!message && message_len)) {
    return -1;
  }

  vistrutith_d3_512f_unpacked_private_key_t unpacked_sk;
  int ret = vistrutith_d3_512f_unpack_private_key(&unpacked_sk, sk);
  // as long as ret!=0, this would enter into the if branch
  // if ret==0, then continue to sign
  if (ret) {
    vistrutith_d3_512f_clear_unpacked_private_key(&unpacked_sk);
    return ret;
  }

  ret = vistrutith_d3_512f_unpacked_sign_with_randomness(&unpacked_sk, message, message_len, rho, rho_len, signature, signature_len);
  vistrutith_d3_512f_clear_unpacked_private_key(&unpacked_sk);
  return ret;
}

int vistrutith_d3_512f_unpacked_sign_with_randomness(const vistrutith_d3_512f_unpacked_private_key_t* sk, const uint8_t* message, size_t message_len, const uint8_t* rho, size_t rho_len, uint8_t* signature, size_t* signature_len) {
  if (!sk || !signature || !signature_len || *signature_len < VISTRUTITH_D3_512F_SIGNATURE_SIZE || (!message && message_len)) {
    return -1;
  }

  const params_t* params = vistrutith_params_d3_512f();
  vistrutith_sign_opt_d3_512f(params, signature, message, message_len, sk->owf_key,
                              sk->owf_input, sk->owf_output, sk->witness, rho,
                              rho_len);
  *signature_len = VISTRUTITH_D3_512F_SIGNATURE_SIZE;

  return 0;
}

int vistrutith_d3_512f_verify(const uint8_t* pk, const uint8_t* message, size_t message_len, const uint8_t* signature, size_t signature_len) {
  if (!pk || !signature || signature_len != VISTRUTITH_D3_512F_SIGNATURE_SIZE || (!message && message_len)) {
    return -1;
  }

  const params_t* params = vistrutith_params_d3_512f();
  return vistrutith_verify_opt_d3_512f(params, message, message_len, signature, PK_INPUT(pk),
                                       PK_OUTPUT(pk));
}

void vistrutith_d3_512f_clear_private_key(uint8_t* key) {
  explicit_bzero(key, VISTRUTITH_D3_512F_PRIVATE_KEY_SIZE);
}

void vistrutith_d3_512f_clear_unpacked_private_key(vistrutith_d3_512f_unpacked_private_key_t* key) {
  if (!key) return;
  if (key->witness) {
    const params_t* params = vistrutith_params_d3_512f();
    const unsigned int witness_bytes = params->ell / 8;
    explicit_bzero(key->witness, witness_bytes);
    free(key->witness);
    key->witness = NULL;
  }
  explicit_bzero(key, sizeof(*key));
}
