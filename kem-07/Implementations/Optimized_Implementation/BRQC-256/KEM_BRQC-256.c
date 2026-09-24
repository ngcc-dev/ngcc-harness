/**
 * \file KEM_BRQC-256.c
 * \brief API_PKC KEM programming interface implementation for the BRQC KEM scheme
 *
 * Bridges the API_PKC standard KEM interface to the internal BRQC KEM implementation.
 */

#include "KEM_BRQC-256.h"
#include "brqc.h"
#include "drng.h"
#include "parameters.h"
#include "parsing.h"

// Deterministic random number generator context (defined and initialized in KAT_KEM.c)
extern DRNG_ctx drng_algorithm;

// Forward declarations: internal BRQC KEM functions (defined in kem.c)
int brqc_keygen(unsigned char *pk, unsigned char *sk);
int brqc_encaps(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int brqc_decaps(unsigned char *ss, const unsigned char *ct,
                const unsigned char *sk);

unsigned long long kem_get_pk_len_bytes() { return BRQC_PUBLIC_KEY_BYTES; }

unsigned long long kem_get_sk_len_bytes() { return BRQC_SECRET_KEY_BYTES; }

unsigned long long kem_get_ss_len_bytes() { return BRQC_SHARED_SECRET_BYTES; }

unsigned long long kem_get_ct_len_bytes() { return BRQC_CIPHERTEXT_BYTES; }

/**
 * @brief Key generation: calls internal brqc_keygen and outputs key lengths
 */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
  int ret = brqc_keygen(pk, sk);
  if (ret == 0) {
    *pk_len_bytes = BRQC_PUBLIC_KEY_BYTES;
    *sk_len_bytes = BRQC_SECRET_KEY_BYTES;
  }
  return ret;
}

/**
 * @brief Encapsulation: calls internal brqc_encaps and outputs shared secret and ciphertext lengths
 */
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
  (void)pk_len_bytes; // Fixed length, unused
  int ret = brqc_encaps(ct, ss, pk);
  if (ret == 0) {
    *ss_len_bytes = BRQC_SHARED_SECRET_BYTES;
    *ct_len_bytes = BRQC_CIPHERTEXT_BYTES;
  }
  return ret;
}

/**
 * @brief Decapsulation: calls internal brqc_decaps and outputs shared secret length
 */
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
  (void)sk_len_bytes; // Fixed length, unused
  (void)ct_len_bytes; // Fixed length, unused
  int ret = brqc_decaps(ss, ct, sk);
  if (ret == 0) {
    *ss_len_bytes = BRQC_SHARED_SECRET_BYTES;
  }
  return ret;
}
