/**
 * \file main_cmultiurag.c
 * \brief CMULTIURAG KEM demonstration program
 */

#include "drng.h"
#include "KEM_CMultiURAG-128.h"
#include "parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;

int main() {
  unsigned char nonce[64];
  unsigned char *pk, *sk, *ct, *ss1, *ss2;
  unsigned long long pk_len, sk_len, ct_len, ss_len, ss_len2;
  int ret;

  printf("\n");
  printf("*******************\n");
  printf("***** CMultiURAG-%d *****\n", CMULTIURAG_SECURITY);
  printf("*******************\n");

  printf("\n");
  printf("Q: %d   ", CMULTIURAG_PARAM_Q);
  printf("M: %d   ", CMULTIURAG_PARAM_M);
  printf("K: %d   ", CMULTIURAG_PARAM_K);
  printf("N: %d   ", CMULTIURAG_PARAM_N);
  printf("N1: %d   ", CMULTIURAG_PARAM_N1);
  printf("N2: %d   ", CMULTIURAG_PARAM_N2);
  printf("W1: %d   ", CMULTIURAG_PARAM_W1);
  printf("W2: %d   ", CMULTIURAG_PARAM_W2);
  printf("Sec: %d bits\n", CMULTIURAG_SECURITY);

  FILE *urandom = fopen("/dev/urandom", "r");
  if (urandom) {
    if (fread(nonce, 1, 64, urandom) != 64) {
      memset(nonce, 0x42, 64);
    }
    fclose(urandom);
  } else {
    memset(nonce, 0x42, 64);
  }
  init_random_number(&drng_algorithm, nonce, 64);

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

  ret = kem_keygen(pk, &pk_len, sk, &sk_len);
  if (ret != 0) {
    printf("kem_keygen failed, error code: %d\n", ret);
    return ret;
  }

  ret = kem_enc(pk, pk_len, ss1, &ss_len, ct, &ct_len);
  if (ret != 0) {
    printf("kem_enc failed, error code: %d\n", ret);
    return ret;
  }

  ret = kem_dec(sk, sk_len, ct, ct_len, ss2, &ss_len2);
  if (ret != 0) {
    printf("kem_dec failed, error code: %d\n", ret);
    return ret;
  }

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
