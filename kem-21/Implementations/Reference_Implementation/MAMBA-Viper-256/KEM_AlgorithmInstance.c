#include "KEM_AlgorithmInstance.h"
#include "api.h"
#include "viper_api_rng.h"
#define EXPECTED_PK 1312ULL
#define EXPECTED_SK 2912ULL
#define EXPECTED_SS 32ULL
#define EXPECTED_CT 1472ULL
unsigned long long kem_get_pk_len_bytes(void) { return CRYPTO_PUBLICKEYBYTES; }
unsigned long long kem_get_sk_len_bytes(void) { return CRYPTO_SECRETKEYBYTES; }
unsigned long long kem_get_ss_len_bytes(void) { return CRYPTO_BYTES; }
unsigned long long kem_get_ct_len_bytes(void) { return CRYPTO_CIPHERTEXTBYTES; }
typedef char viper_pk_len_must_match[(CRYPTO_PUBLICKEYBYTES == EXPECTED_PK) ? 1 : -1];
typedef char viper_sk_len_must_match[(CRYPTO_SECRETKEYBYTES == EXPECTED_SK) ? 1 : -1];
typedef char viper_ss_len_must_match[(CRYPTO_BYTES == EXPECTED_SS) ? 1 : -1];
typedef char viper_ct_len_must_match[(CRYPTO_CIPHERTEXTBYTES == EXPECTED_CT) ? 1 : -1];
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes) {
  if (!pk || !pk_len_bytes || !sk || !sk_len_bytes) return -1;
  viper_api_rng_clear_status();
  if (crypto_kem_keypair(pk, sk) != 0) return -2;
  if (viper_api_rng_status() != 0) return -8;
  *pk_len_bytes = CRYPTO_PUBLICKEYBYTES; *sk_len_bytes = CRYPTO_SECRETKEYBYTES; return 0;
}
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *ss, unsigned long long *ss_len_bytes, unsigned char *ct, unsigned long long *ct_len_bytes) {
  if (!pk || !ss || !ss_len_bytes || !ct || !ct_len_bytes) return -1;
  if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES) return -3;
  viper_api_rng_clear_status();
  if (crypto_kem_enc(ct, ss, pk) != 0) return -4;
  if (viper_api_rng_status() != 0) return -8;
  *ss_len_bytes = CRYPTO_BYTES; *ct_len_bytes = CRYPTO_CIPHERTEXTBYTES; return 0;
}
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *ct, unsigned long long ct_len_bytes, unsigned char *ss, unsigned long long *ss_len_bytes) {
  if (!sk || !ct || !ss || !ss_len_bytes) return -1;
  if (sk_len_bytes != CRYPTO_SECRETKEYBYTES) return -5;
  if (ct_len_bytes != CRYPTO_CIPHERTEXTBYTES) return -6;
  if (crypto_kem_dec(ss, ct, sk) != 0) return -7;
  *ss_len_bytes = CRYPTO_BYTES; return 0;
}
