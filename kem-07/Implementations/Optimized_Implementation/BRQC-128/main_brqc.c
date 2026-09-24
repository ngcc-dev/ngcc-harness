/**
 * \file main_brqc.c
 * \brief BRQC KEM demonstration program
 *
 * Demonstrates the complete KEM workflow through the API_PKC interface:
 * Key generation -> Encapsulation -> Decapsulation -> Shared secret verification.
 */

#include "drng.h"
#include "KEM_BRQC-128.h"
#include "parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Deterministic random number generator context (used internally by the KEM scheme)
DRNG_ctx drng_algorithm;

int main() {
  unsigned char nonce[64];
  unsigned char *pk, *sk, *ct, *ss1, *ss2;
  unsigned long long pk_len, sk_len, ct_len, ss_len, ss_len2;
  int ret;

  /* Print algorithm instance info */
  printf("\n");
  printf("*******************\n");
  printf("***** BRQC-%d *****\n", BRQC_SECURITY);
  printf("*******************\n");

  /* Print algorithm parameters */
  printf("\n");
  printf("Q: %d   ", BRQC_PARAM_Q);
  printf("M: %d   ", BRQC_PARAM_M);
  printf("K: %d   ", BRQC_PARAM_K);
  printf("N: %d   ", BRQC_PARAM_N);
  printf("W_x: %d   ", BRQC_PARAM_W_x);
  printf("W_y: %d   ", BRQC_PARAM_W_y);
  printf("W_r1: %d   ", BRQC_PARAM_W_r1);
  printf("W_r2: %d   ", BRQC_PARAM_W_r2);
  printf("W_e: %d   ", BRQC_PARAM_W_e);
  printf("Sec: %d bits\n", BRQC_SECURITY);

  /* Obtain true entropy from system secure random source as seed */
  FILE *urandom = fopen("/dev/urandom", "r");
  if (urandom) {
    if (fread(nonce, 1, 64, urandom) != 64) {
      memset(nonce, 0x42, 64); // Fall back to fixed value on read failure
    }
    fclose(urandom);
  } else {
    memset(nonce, 0x42, 64); // Fall back to fixed value if cannot open
  }
  init_random_number(&drng_algorithm, nonce, 64);

  /* Query data structure sizes and allocate memory */
  pk_len = kem_get_pk_len_bytes();
  sk_len = kem_get_sk_len_bytes();
  ct_len = kem_get_ct_len_bytes();
  ss_len = kem_get_ss_len_bytes();

  printf("\nPK: %llu bytes\n", pk_len);
  printf("SK: %llu bytes\n", sk_len);
  printf("CT: %llu bytes\n", ct_len);
  printf("SS: %llu bytes\n", ss_len);

  pk = (unsigned char *)calloc(pk_len, sizeof(unsigned char));
  sk = (unsigned char *)calloc(sk_len, sizeof(unsigned char));
  ct = (unsigned char *)calloc(ct_len, sizeof(unsigned char));
  ss1 = (unsigned char *)calloc(ss_len, sizeof(unsigned char));
  ss2 = (unsigned char *)calloc(ss_len, sizeof(unsigned char));

  /* Key generation */
  ret = kem_keygen(pk, &pk_len, sk, &sk_len);
  if (ret != 0) {
    printf("kem_keygen failed, error code: %d\n", ret);
    return ret;
  }

  /* Encapsulation (generate shared secret ss1 and ciphertext ct) */
  ret = kem_enc(pk, pk_len, ss1, &ss_len, ct, &ct_len);
  if (ret != 0) {
    printf("kem_enc failed, error code: %d\n", ret);
    return ret;
  }

  /* Decapsulation (recover shared secret ss2 from ciphertext ct) */
  ret = kem_dec(sk, sk_len, ct, ct_len, ss2, &ss_len2);
  if (ret != 0) {
    printf("kem_dec failed, error code: %d\n", ret);
    return ret;
  }

  /* Print and compare the two shared secrets */
  printf("\nEncaps secret: ");
  for (unsigned long long i = 0; i < ss_len; ++i)
    printf("%02x", ss1[i]);

  printf("\nDecaps secret: ");
  for (unsigned long long i = 0; i < ss_len2; ++i)
    printf("%02x", ss2[i]);

  if (memcmp(ss1, ss2, ss_len) == 0) {
    printf("\n\n[PASS] Shared secrets match!\n\n");
  } else {
    printf("\n\n[FAIL] Shared secrets do NOT match!\n\n");
  }

  free(pk);
  free(sk);
  free(ct);
  free(ss1);
  free(ss2);
  return 0;
}
