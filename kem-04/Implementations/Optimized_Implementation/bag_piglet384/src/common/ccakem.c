#include "ccakem.h"

#include <stdlib.h>
#include <string.h>

#include "auxfunc.h"
#include "bag_piglet.h"
#include "ffi_vec.h"
#include "randombytes.h"

static const uint8_t KEM_KEYGEN_DOMAIN = 0x01;
static const uint8_t KEM_ID_DOMAIN = 0x02;
static const uint8_t KEM_G_DOMAIN = 0x03;
static const uint8_t KEM_K_DOMAIN = 0x04;

static void sm3_xof(uint8_t *output, const uint8_t *input,
                    size_t input_len, size_t output_len) {
  pseudoXOF(8ULL * output_len, input, 8ULL * input_len, output);
}

static void sm3_xof_domain(uint8_t *output, size_t output_len,
                           const uint8_t *input, size_t input_len,
                           uint8_t domain) {
  uint8_t *domain_input = (uint8_t *) malloc(input_len + 1);
  if(domain_input == NULL) {
    abort();
  }
  memcpy(domain_input, input, input_len);
  domain_input[input_len] = domain;
  sm3_xof(output, domain_input, input_len + 1, output_len);
  free(domain_input);
}

static void derive_secret_key_material(
  uint8_t pke_seed[SEEDEXPANDER_SEED_BYTES],
  uint8_t fallback[PARAMS_PREKEY_SIZE],
  const uint8_t master_seed[CCAKEM_SK_SIZE]) {
  uint8_t expanded[SEEDEXPANDER_SEED_BYTES + PARAMS_PREKEY_SIZE];

  sm3_xof_domain(expanded, sizeof(expanded), master_seed, CCAKEM_SK_SIZE,
                 KEM_KEYGEN_DOMAIN);
  memcpy(pke_seed, expanded, SEEDEXPANDER_SEED_BYTES);
  memcpy(fallback, expanded + SEEDEXPANDER_SEED_BYTES,
         PARAMS_PREKEY_SIZE);
}

static void derive_key_material(
  uint8_t kr[PARAMS_PREKEY_SIZE + PARAMS_RAND_SIZE],
  const uint8_t pk[CCAKEM_PK_SIZE],
  const uint8_t message[CPAPKE_PT_SIZE],
  const uint8_t salt[PARAMS_SALT_SIZE]) {
  uint8_t id[PARAMS_ID_SIZE];
  uint8_t input[PARAMS_ID_SIZE + CPAPKE_PT_SIZE + PARAMS_SALT_SIZE];

  sm3_xof_domain(id, sizeof(id), pk, CCAKEM_PK_SIZE, KEM_ID_DOMAIN);
  memcpy(input, id, sizeof(id));
  memcpy(input + sizeof(id), message, CPAPKE_PT_SIZE);
  memcpy(input + sizeof(id) + CPAPKE_PT_SIZE, salt, PARAMS_SALT_SIZE);
  sm3_xof_domain(kr, PARAMS_PREKEY_SIZE + PARAMS_RAND_SIZE,
                 input, sizeof(input), KEM_G_DOMAIN);
}

static void derive_session_key(
  uint8_t key[CCAKEM_KEY_SIZE],
  const uint8_t secret[PARAMS_PREKEY_SIZE],
  const uint8_t ct[CPAPKE_CT_SIZE]) {
  uint8_t input[PARAMS_PREKEY_SIZE + CPAPKE_CT_SIZE];

  memcpy(input, secret, PARAMS_PREKEY_SIZE);
  memcpy(input + PARAMS_PREKEY_SIZE, ct, CPAPKE_CT_SIZE);
  sm3_xof_domain(key, CCAKEM_KEY_SIZE, input, sizeof(input), KEM_K_DOMAIN);
}

static uint8_t ciphertexts_equal(const uint8_t *left,
                                 const uint8_t *right, size_t size) {
  uint8_t difference = 0;
  for(size_t i = 0; i < size; ++i) {
    difference |= left[i] ^ right[i];
  }
  return (uint8_t) (((((uint16_t) difference) - 1U) >> 8) & 1U);
}

void ccakem_keygen(uint8_t pk[CCAKEM_PK_SIZE],
                   uint8_t sk[CCAKEM_SK_SIZE],
                   const unsigned char *seed) {
  uint8_t pke_seed[SEEDEXPANDER_SEED_BYTES];
  uint8_t fallback[PARAMS_PREKEY_SIZE];
  uint8_t pke_sk[CPAPKE_SK_SIZE];

  if(seed == NULL) {
    CryptoRandomBytes(sk, CCAKEM_SK_SIZE);
  } else {
    memcpy(sk, seed, CCAKEM_SK_SIZE);
  }
  derive_secret_key_material(pke_seed, fallback, sk);
  bag_piglet_pke_keygen(pk, pke_sk, pke_seed);
}

void ccakem_encaps(uint8_t key[CCAKEM_KEY_SIZE],
                   uint8_t ct[CCAKEM_CT_SIZE],
                   const uint8_t pk[CCAKEM_PK_SIZE],
                   const unsigned char *seed) {
  uint8_t message[CPAPKE_PT_SIZE];
  uint8_t kr[PARAMS_PREKEY_SIZE + PARAMS_RAND_SIZE];
  uint8_t salt[PARAMS_SALT_SIZE];

  message_random2(message, PARAM_K);
  CryptoRandomBytes(salt, sizeof(salt));
  derive_key_material(kr, pk, message, salt);
  bag_piglet_pke_encrypt(message, ct, pk, kr + PARAMS_PREKEY_SIZE);
  memcpy(ct + CPAPKE_CT_SIZE, salt, PARAMS_SALT_SIZE);
  derive_session_key(key, kr, ct);
  (void) seed;
}

void ccakem_decaps(uint8_t key[CCAKEM_KEY_SIZE],
                   const uint8_t sk[CCAKEM_SK_SIZE],
                   const uint8_t ct[CCAKEM_CT_SIZE]) {
  uint8_t message[CPAPKE_PT_SIZE];
  uint8_t kr[PARAMS_PREKEY_SIZE + PARAMS_RAND_SIZE];
  uint8_t check_ct[CPAPKE_CT_SIZE];
  uint8_t pke_seed[SEEDEXPANDER_SEED_BYTES];
  uint8_t fallback[PARAMS_PREKEY_SIZE];
  uint8_t selected_secret[PARAMS_PREKEY_SIZE];
  uint8_t pk[CCAKEM_PK_SIZE];
  uint8_t pke_sk[CPAPKE_SK_SIZE];
  const uint8_t *salt = ct + CPAPKE_CT_SIZE;

  derive_secret_key_material(pke_seed, fallback, sk);
  bag_piglet_pke_keygen(pk, pke_sk, pke_seed);
  bag_piglet_pke_decrypt(message, ct, pke_sk);
  derive_key_material(kr, pk, message, salt);
  bag_piglet_pke_encrypt(message, check_ct, pk,
                         kr + PARAMS_PREKEY_SIZE);

  const uint8_t valid_mask = (uint8_t) (0U - ciphertexts_equal(
    ct, check_ct, CPAPKE_CT_SIZE));
  for(size_t i = 0; i < PARAMS_PREKEY_SIZE; ++i) {
    selected_secret[i] = (kr[i] & valid_mask) |
                         (fallback[i] & (uint8_t) ~valid_mask);
  }
  derive_session_key(key, selected_secret, check_ct);
}
