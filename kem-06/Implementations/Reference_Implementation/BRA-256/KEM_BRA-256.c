/**
 * \file KEM_BRA-256.c
 * \brief API_PKC KEM programming interface implementation for the BRA KEM scheme
 *
 * Bridges the API_PKC standard KEM interface to the internal BRA KEM implementation.
 */

#include "KEM_BRA-256.h"
#include "bra.h"
#include "drng.h"
#include "parameters.h"
#include "parsing.h"

// Deterministic random number generator context (defined and initialized in KAT_KEM.c)
extern DRNG_ctx drng_algorithm;

// Forward declarations: internal BRA KEM functions (defined in kem.c)
int bra_keygen(unsigned char *pk, unsigned char *sk);
int bra_encaps(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int bra_decaps(unsigned char *ss, const unsigned char *ct,
                const unsigned char *sk);

unsigned long long kem_get_pk_len_bytes() { return BRA_PUBLIC_KEY_BYTES; }

unsigned long long kem_get_sk_len_bytes() { return BRA_SECRET_KEY_BYTES; }

unsigned long long kem_get_ss_len_bytes() { return BRA_SHARED_SECRET_BYTES; }

unsigned long long kem_get_ct_len_bytes() { return BRA_CIPHERTEXT_BYTES; }

/**
 * @brief Key generation: calls internal bra_keygen and outputs key lengths
 */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
  int ret = bra_keygen(pk, sk);
  if (ret == 0) {
    *pk_len_bytes = BRA_PUBLIC_KEY_BYTES;
    *sk_len_bytes = BRA_SECRET_KEY_BYTES;
  }
  return ret;
}

/**
 * @brief Encapsulation: calls internal bra_encaps and outputs shared secret and ciphertext lengths
 */
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
  (void)pk_len_bytes; // Fixed length, unused
  int ret = bra_encaps(ct, ss, pk);
  if (ret == 0) {
    *ss_len_bytes = BRA_SHARED_SECRET_BYTES;
    *ct_len_bytes = BRA_CIPHERTEXT_BYTES;
  }
  return ret;
}

/**
 * @brief Decapsulation: calls internal bra_decaps and outputs shared secret length
 */
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
  (void)sk_len_bytes; // Fixed length, unused
  (void)ct_len_bytes; // Fixed length, unused
  int ret = bra_decaps(ss, ct, sk);
  if (ret == 0) {
    *ss_len_bytes = BRA_SHARED_SECRET_BYTES;
  }
  return ret;
}
