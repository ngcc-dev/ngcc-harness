#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "drng.h"
#include "sign.h"
#include "packing.h"
#include "polyvec.h"

#define MLEN 59
#define CTXLEN 14
#define NTESTS 100000
#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

int main(void) {
  size_t i;
  size_t j;
  int ret;
  uint8_t b;
  uint8_t ctx[CTXLEN] = {0};
  const uint8_t *ctxp = NULL;
  size_t ctxlen = 0;
  uint8_t m[MLEN];
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];
  uint8_t sig[CRYPTO_BYTES];
  size_t siglen = 0;
  DRNG_ctx drng_seed;
  DRNG_ctx drng_msg;

  memcpy(ctx, "test_OPS", sizeof("test_OPS") - 1);

  {
    uint8_t nonce1[SEED_LEN_BYTES];
    uint8_t nonce2[SEED_LEN_BYTES];
    for(size_t k = 0; k + 4 <= SEED_LEN_BYTES; k += 4)
      memcpy(nonce1 + k, "seed", 4);
    for(size_t k = 0; k + 3 <= SEED_LEN_BYTES; k += 3)
      memcpy(nonce2 + k, "msg", 3);
    nonce2[SEED_LEN_BYTES - 1] = (uint8_t)'m';
    init_random_number(&drng_seed, nonce1, SEED_LEN_BYTES);
    init_random_number(&drng_msg, nonce2, SEED_LEN_BYTES);
  }

  for(i = 0; i < NTESTS; i++) {
    uint8_t seed[SEED_LEN_BYTES];
    get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8ULL);
    init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);
    get_random_number(&drng_msg, m, MLEN * 8ULL);

    crypto_sign_keypair(pk, sk);
    ret = crypto_sign_signature(sig, &siglen, m, MLEN, ctxp, ctxlen, sk);
    if(ret != 0) {
      fprintf(stderr, "Signing failed\n");
      return 1;
    }
    if(siglen != CRYPTO_BYTES) {
      fprintf(stderr, "Signature length wrong\n");
      return 1;
    }

    ret = crypto_sign_verify(sig, siglen, m, MLEN, ctxp, ctxlen, pk);
    if(ret != 0) {
      fprintf(stderr, "Verification failed at i=%llu (ret=%d)\n",
              (unsigned long long)i, ret);
      fprintf(stderr, "seed=");
      for(size_t k = 0; k < SEED_LEN_BYTES; k++)
        fprintf(stderr, "%02x", seed[k]);
      fprintf(stderr, "\n");
      fprintf(stderr, "m=");
      for(size_t k = 0; k < MLEN; k++)
        fprintf(stderr, "%02x", m[k]);
      fprintf(stderr, "\n");
      return 1;
    }

    get_random_number(&drng_algorithm, (uint8_t *)&j, (unsigned long long)sizeof(j) * 8ULL);
    do {
      get_random_number(&drng_algorithm, &b, 8ULL);
    } while(!b);
    sig[j % CRYPTO_BYTES] ^= b;

    ret = crypto_sign_verify(sig, siglen, m, MLEN, ctxp, ctxlen, pk);
    if(ret == 0) {
      fprintf(stderr, "Trivial forgeries possible\n");
      return 1;
    }
  }

  printf("CRYPTO_PUBLICKEYBYTES = %d\n", CRYPTO_PUBLICKEYBYTES);
  printf("CRYPTO_SECRETKEYBYTES = %d\n", CRYPTO_SECRETKEYBYTES);
  printf("CRYPTO_BYTES = %d\n", CRYPTO_BYTES);
  printf("OK\n");
  return 0;
}
