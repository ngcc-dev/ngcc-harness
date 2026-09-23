#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "KEM_AlgorithmInstance.h"
#include "params.h"

DRNG_ctx drng_algorithm;

void test_bandwidth(void)
{
  unsigned long long pk_len, sk_len, ct_len, ss_len;

  unsigned long long pk_byts = kem_get_pk_len_bytes();
  unsigned long long sk_byts = kem_get_sk_len_bytes();
  unsigned long long ct_byts = kem_get_ct_len_bytes();
  unsigned long long ss_byts = kem_get_ss_len_bytes();

  unsigned char *pk = (unsigned char *)malloc(pk_byts);
  unsigned char *sk = (unsigned char *)malloc(sk_byts);

  if (kem_keygen(pk, &pk_len, sk, &sk_len))
  {
    printf("ERROR: kem_keygen failed\n");
    free(pk);
    free(sk);
    return;
  }

  unsigned char *ss = (unsigned char *)malloc(ss_byts);
  unsigned char *ct = (unsigned char *)malloc(ct_byts);

  if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len))
  {
    printf("ERROR: kem_enc failed\n");
  }

  printf("=== DTRU-%d-%d KEM Bandwidth ===\n", DTRU_N, DTRU_Q);
  printf("Public Key:     %llu Bytes\n", pk_len);
  printf("Private Key:    %llu Bytes\n", sk_len);
  printf("Shared Secret:  %llu Bytes\n", ss_len);
  printf("Ciphertext:     %llu Bytes\n", ct_len);
  printf("Key Exchange Rounds: 1\n");
  printf("\n");

  free(pk);
  free(sk);
  free(ss);
  free(ct);
}

int main(void)
{
  test_bandwidth();
  return 0;
}
