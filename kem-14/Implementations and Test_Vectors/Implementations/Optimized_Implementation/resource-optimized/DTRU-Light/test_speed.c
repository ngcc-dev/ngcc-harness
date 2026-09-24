#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "api.h"
#include "params.h"
#include "cpucycles.h"
#include "speed.h"

#ifndef NTESTS
#define NTESTS 10000
#endif

uint64_t t[NTESTS];

static int test_speed_kem(void)
{
  printf("\n");

  printf("DTRU-%d-%d-KEM\n\n", DTRU_N, DTRU_Q);
  unsigned int i;
  unsigned char k1[DTRU_SHAREDKEYBYTES], k2[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1 = 0, ss_byts2 = 0, ct_byts = 0, pk_byts = 0, sk_byts = 0;

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    if (kem_keygen(pk, &pk_byts, sk, &sk_byts) != 0)
    {
      fprintf(stderr, "kem_keygen failed at iteration %u\n", i);
      return 1;
    }
  }
  print_results("dtru_kem_keygen: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    if (kem_enc(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts) != 0)
    {
      fprintf(stderr, "kem_enc failed at iteration %u\n", i);
      return 1;
    }
  }
  print_results("dtru_kem_encaps: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    if (kem_dec(sk, sk_byts, ct, ct_byts, k2, &ss_byts2) != 0)
    {
      fprintf(stderr, "kem_dec failed at iteration %u\n", i);
      return 1;
    }
  }
  print_results("dtru_kem_decaps: ", t, NTESTS);

  return 0;
}

int main(void)
{
  return test_speed_kem();
}
