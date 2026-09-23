/**
 * \file KEM_CMultiURAG-512.c
 * \brief API_PKC KEM programming interface implementation for the CMULTIURAG KEM scheme
 */

#include "KEM_CMultiURAG-512.h"
#include "cmultiurag.h"
#include "drng.h"
#include "parameters.h"
#include "parsing.h"

extern DRNG_ctx drng_algorithm;

int cmultiurag_keygen(unsigned char *pk, unsigned char *sk);
int cmultiurag_encaps(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int cmultiurag_decaps(unsigned char *ss, const unsigned char *ct,
                const unsigned char *sk);

unsigned long long kem_get_pk_len_bytes() { return CMULTIURAG_PUBLIC_KEY_BYTES; }

unsigned long long kem_get_sk_len_bytes() { return CMULTIURAG_SECRET_KEY_BYTES; }

unsigned long long kem_get_ss_len_bytes() { return CMULTIURAG_SHARED_SECRET_BYTES; }

unsigned long long kem_get_ct_len_bytes() { return CMULTIURAG_CIPHERTEXT_BYTES; }

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
  int ret = cmultiurag_keygen(pk, sk);
  if (ret == 0) {
    *pk_len_bytes = CMULTIURAG_PUBLIC_KEY_BYTES;
    *sk_len_bytes = CMULTIURAG_SECRET_KEY_BYTES;
  }
  return ret;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
  (void)pk_len_bytes;
  int ret = cmultiurag_encaps(ct, ss, pk);
  if (ret == 0) {
    *ss_len_bytes = CMULTIURAG_SHARED_SECRET_BYTES;
    *ct_len_bytes = CMULTIURAG_CIPHERTEXT_BYTES;
  }
  return ret;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
  (void)sk_len_bytes;
  (void)ct_len_bytes;
  int ret = cmultiurag_decaps(ss, ct, sk);
  if (ret == 0) {
    *ss_len_bytes = CMULTIURAG_SHARED_SECRET_BYTES;
  }
  return ret;
}
