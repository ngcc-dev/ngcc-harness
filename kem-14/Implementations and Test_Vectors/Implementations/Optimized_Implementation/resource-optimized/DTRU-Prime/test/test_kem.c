#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../params.h"
#include "../api.h"
#define NTESTS 1000

void test_kem()
{
  unsigned int i, j;
  unsigned char k1[DTRU_SHAREDKEYBYTES], k2[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  for (i = 0; i < NTESTS; i++)
  {
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
    kem_enc(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts);
    kem_dec(sk, sk_byts, ct, ct_byts, k2, &ss_byts2);

    for (j = 0; j < DTRU_SHAREDKEYBYTES; j++)
      if (k1[j] != k2[j])
      {
        printf("Round %d. Failure: Keys dont match: %hhx != %hhx!\n", i, k1[j], k2[j]);
        return;
      }
    printf("KEM test %d passed!\n", i+1);
  }

  printf("DTRU-%d-KEM is correct!\n", DTRU_N);

  printf("Test %d times.\n\n", NTESTS);
  printf("DTRU_N = %d, DTRU_Q = %d, DTRU_Q2 = %d\n", DTRU_N, DTRU_Q, DTRU_Q2);
  printf("KEM size:  sk = %d bytes, pk = %d bytes, ct = %d bytes\n\n",
         DTRU_KEM_SECRETKEYBYTES, DTRU_KEM_PUBLICKEYBYTES, DTRU_KEM_CIPHERTEXTBYTES);
}

int main()
{
  test_kem();
  return 0;
}
