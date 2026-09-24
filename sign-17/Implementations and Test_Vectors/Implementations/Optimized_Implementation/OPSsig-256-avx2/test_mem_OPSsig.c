#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#include "params.h"
#include "drng.h"
#include "sign.h"

#define MEM_ITERS 100
#define MEM_MSG_BYTES 64
#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

static unsigned long long read_status_value_bytes(const char *key) {
  FILE *fp = fopen("/proc/self/status", "r");
  char line[256];
  char parsed_key[256];
  unsigned long long value = 0;

  if(fp == NULL)
    return 0;

  while(fgets(line, sizeof(line), fp) != NULL) {
    unsigned long long kb = 0;

    if(sscanf(line, "%255[^:]: %llu kB", parsed_key, &kb) == 2 &&
       strcmp(parsed_key, key) == 0) {
      value = kb * 1024ULL;
      break;
    }
  }

  fclose(fp);
  return value;
}

int main(void) {
  uint8_t seed[SEED_LEN_BYTES];
  uint8_t msg[MEM_MSG_BYTES];
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];
  uint8_t sig[CRYPTO_BYTES];
  size_t siglen = 0;
  struct rusage usage;
  unsigned long long vmpeak_bytes;
  unsigned long long vmhwm_bytes;
  unsigned long long peak_rss_bytes;
  size_t i;

  for(i = 0; i < SEED_LEN_BYTES; i++)
    seed[i] = (uint8_t)(0xA5u ^ (unsigned)i);

  init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);
  get_random_number(&drng_algorithm, msg, (unsigned long long)sizeof(msg) * 8ULL);

  for(i = 0; i < MEM_ITERS; i++) {
    if(crypto_sign_keypair(pk, sk) != 0) {
      fprintf(stderr, "keypair failed at iteration %zu\n", i);
      return 1;
    }
    if(crypto_sign_signature(sig, &siglen, msg, sizeof(msg), NULL, 0, sk) != 0) {
      fprintf(stderr, "sign failed at iteration %zu\n", i);
      return 1;
    }
    if(crypto_sign_verify(sig, siglen, msg, sizeof(msg), NULL, 0, pk) != 0) {
      fprintf(stderr, "verify failed at iteration %zu\n", i);
      return 1;
    }
  }

  if(getrusage(RUSAGE_SELF, &usage) != 0) {
    fprintf(stderr, "getrusage failed\n");
    return 1;
  }

  peak_rss_bytes = (unsigned long long)usage.ru_maxrss * 1024ULL;
  vmpeak_bytes = read_status_value_bytes("VmPeak");
  vmhwm_bytes = read_status_value_bytes("VmHWM");

  printf("metric,value\n");
  printf("iterations,%d\n", MEM_ITERS);
  printf("message_bytes,%d\n", MEM_MSG_BYTES);
  printf("public_key_bytes,%d\n", CRYPTO_PUBLICKEYBYTES);
  printf("secret_key_bytes,%d\n", CRYPTO_SECRETKEYBYTES);
  printf("signature_bytes,%d\n", CRYPTO_BYTES);
  printf("peak_rss_bytes,%llu\n", peak_rss_bytes);
  printf("vmpeak_bytes,%llu\n", vmpeak_bytes);
  printf("vmhwm_bytes,%llu\n", vmhwm_bytes);

  return 0;
}
