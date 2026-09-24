#include <stdio.h>
#include <stdlib.h>
#include "../api.h"
#include "../drng.h"
#include "speed_print.h"
#include "cpucycles.h"
#include "../params.h"
#define MLEN 64
#define NTESTS  9000//(1<<30)//10000//5000
DRNG_ctx drng_algorithm;
int main(void)
{
  unsigned long long i;
  int ret;
  unsigned long long smlen;
  unsigned char m[MLEN];
  unsigned char sm[CRYPTO_BYTES];
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sk[CRYPTO_SECRETKEYBYTES];
  unsigned long long *t0, *t1, *t2;
  unsigned long long pk_len_bytes;
  unsigned long long sk_len_bytes;

  t0 = (unsigned long long *)calloc(NTESTS, sizeof(unsigned long long));
  t1 = (unsigned long long *)calloc(NTESTS, sizeof(unsigned long long));
  t2 = (unsigned long long *)calloc(NTESTS, sizeof(unsigned long long));

  printf("\nSecurity Level 3 (256bits)....");
  printf("\nK::%d-L::%d\n",K,L);

  for(i = 0; i < NTESTS; ++i) {
    //printf("\n...Count: %d...", i);
    //randombytes(m, MLEN);
    if(get_random_number(&drng_algorithm, m, MLEN * 8) != 0) {
    fprintf(stderr, "Error: random generation failed\n");
    exit(EXIT_FAILURE);
  }

    t0[i] = cpucycles();
    sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    t0[i] = cpucycles() - t0[i];

    t1[i] = cpucycles();
    sig_sign(sk, sk_len_bytes, m, MLEN, sm, &smlen);
    t1[i] = cpucycles() - t1[i];

    t2[i] = cpucycles();
    ret = sig_verify(pk,pk_len_bytes, sm, smlen, m, MLEN);
    t2[i] = cpucycles() - t2[i];

    if(ret) {
      printf("\nVerification failed\n");
      return -1;
    }

//    if(mlen != MLEN) {
//      printf("\nMessage lengths don't match\n");
//      return -1;
//    }

//    for(j = 0; j < mlen; ++j) {
//      if(m[j] != m2[j]) {
//        printf("\nMessages don't match\n");
//        return -1;
//      }
//    }
  }

  print_results("keygen:", t0, NTESTS);
  print_results("sign: ", t1, NTESTS);
  print_results("verify: ", t2, NTESTS);

  free(t0);
  free(t1);
  free(t2);

  return 0;
}
