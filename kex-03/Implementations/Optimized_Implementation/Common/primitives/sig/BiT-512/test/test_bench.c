/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../SIG_AlgorithmInstance.h"
#include "../params.h"
#include "../drng.h"
#include "cpucycles.h"
#include "speed_print.h"
#include "test_common.h"

#ifndef NTESTS
#define NTESTS 10000
#endif

#define MESSAGE_BYTES 64

uint64_t t[NTESTS];

int main(void) {
  uint8_t pk[BIT_PUBLICKEYBYTES];
  uint8_t sk[BIT_SECRETKEYBYTES];
  uint8_t seed[55];
  uint8_t *msgs = NULL;
  uint8_t *sigs = NULL;
  unsigned long long *siglens = NULL;
  unsigned long long pk_len = 0;
  unsigned long long sk_len = 0;
  unsigned int i;

  msgs = (uint8_t *)malloc((size_t)NTESTS * MESSAGE_BYTES);
  sigs = (uint8_t *)malloc((size_t)NTESTS * BIT_SIGNBYTES);
  siglens = (unsigned long long *)malloc((size_t)NTESTS * sizeof(*siglens));
  if (msgs == NULL || sigs == NULL || siglens == NULL) {
    fprintf(stderr, "allocation failed\n");
    free(msgs);
    free(sigs);
    free(siglens);
    return 1;
  }

  test_fill_seed(seed, sizeof(seed), 0, 0);
  if (init_random_number(&test_rng, seed, sizeof(seed)) != 0) {
    fprintf(stderr, "init_random_number failed\n");
    free(msgs);
    free(sigs);
    free(siglens);
    return 1;
  }

  printf("BiT-512 cpucycles benchmark\n");
  printf("NTESTS: %d\n\n", NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0) {
      fprintf(stderr, "keypair failed\n");
      free(msgs);
      free(sigs);
      free(siglens);
      return 1;
    }
  }
  print_results("Keypair:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    test_fill_message(msgs + (size_t)i * MESSAGE_BYTES, MESSAGE_BYTES, 1, i);
  }

  test_fill_seed(seed, sizeof(seed), 2, 0);
  if (init_random_number(&test_rng, seed, sizeof(seed)) != 0) {
    fprintf(stderr, "init_random_number failed\n");
    free(msgs);
    free(sigs);
    free(siglens);
    return 1;
  }

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    if (sig_sign(sk, sk_len,
                 msgs + (size_t)i * MESSAGE_BYTES, MESSAGE_BYTES,
                 sigs + (size_t)i * BIT_SIGNBYTES,
                 &siglens[i]) != 0) {
      fprintf(stderr, "sign failed\n");
      free(msgs);
      free(sigs);
      free(siglens);
      return 1;
    }
  }
  print_results("Sign:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    if (sig_verify(pk, pk_len,
                   sigs + (size_t)i * BIT_SIGNBYTES,
                   siglens[i],
                   msgs + (size_t)i * MESSAGE_BYTES,
                   MESSAGE_BYTES) != 0) {
      fprintf(stderr, "verify failed\n");
      free(msgs);
      free(sigs);
      free(siglens);
      return 1;
    }
  }
  print_results("Verify:", t, NTESTS);

  free(msgs);
  free(sigs);
  free(siglens);
  return 0;
}
