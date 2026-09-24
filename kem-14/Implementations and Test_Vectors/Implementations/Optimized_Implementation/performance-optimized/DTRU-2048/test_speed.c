#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "api.h"
#include "params.h"
#include "cpucycles.h"
#include "speed.h"

#define NTESTS 10000

uint64_t t[NTESTS];

void test_speed_kem()
{
  printf("\n");

  printf("DTRU-%d-%d-KEM\n\n", DTRU_N, DTRU_Q);
  unsigned int i;
  unsigned char k1[DTRU_SHAREDKEYBYTES], k2[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
  }
  print_results("dtru_kem_keygen: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_enc(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts);
  }
  print_results("dtru_kem_encaps: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_dec(sk, sk_byts, ct, ct_byts, k2, &ss_byts2);
  }
  print_results("dtru_kem_decaps: ", t, NTESTS);

}

void test_speed_kem_avx2()
{
  printf("\n");

  printf("DTRU-%d-%d-KEM-avx2\n\n", DTRU_N, DTRU_Q);
  unsigned int i;
  unsigned char k1[DTRU_SHAREDKEYBYTES], k2[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_keygen_avx2(pk, &pk_byts, sk, &sk_byts);
  }
  print_results("dtru_kem_keygen: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_enc_avx2(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts);
  }
  print_results("dtru_kem_encaps: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_dec_avx2(sk, sk_byts, ct, ct_byts, k2, &ss_byts2);
  }
  print_results("dtru_kem_decaps: ", t, NTESTS);

}
int main()
{
  test_speed_kem();
  test_speed_kem_avx2();
  return 0;
}
