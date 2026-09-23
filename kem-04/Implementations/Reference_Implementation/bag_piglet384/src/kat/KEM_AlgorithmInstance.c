#include "KEM_AlgorithmInstance.h"

#include <stddef.h>

#include "ccakem.h"
#include "ffi_field.h"
#include "parameters.h"

static void ensure_field_initialized(void) {
  static int initialized = 0;

  if(!initialized) {
    ffi_field_init();
    initialized = 1;
  }
}

unsigned long long kem_get_pk_len_bytes(void) {
  return CCAKEM_PK_SIZE;
}

unsigned long long kem_get_sk_len_bytes(void) {
  return CCAKEM_SK_SIZE;
}

unsigned long long kem_get_ss_len_bytes(void) {
  return CCAKEM_KEY_SIZE;
}

unsigned long long kem_get_ct_len_bytes(void) {
  return CCAKEM_CT_SIZE;
}

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
  ensure_field_initialized();
  ccakem_keygen(pk, sk, NULL);
  *pk_len_bytes = kem_get_pk_len_bytes();
  *sk_len_bytes = kem_get_sk_len_bytes();
  return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
  ensure_field_initialized();
  ccakem_encaps(ss, ct, pk, NULL);
  *ss_len_bytes = kem_get_ss_len_bytes();
  *ct_len_bytes = kem_get_ct_len_bytes();
  (void) pk_len_bytes;
  return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
  ensure_field_initialized();
  ccakem_decaps(ss, sk, ct);
  *ss_len_bytes = kem_get_ss_len_bytes();
  (void) sk_len_bytes;
  (void) ct_len_bytes;
  return 0;
}
