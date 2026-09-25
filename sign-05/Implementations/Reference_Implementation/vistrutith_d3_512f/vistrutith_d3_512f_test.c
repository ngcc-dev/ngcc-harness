// A simple test program for the vistrutith_d3_512f signature scheme

#include "vistrutith_d3_512f_AlgorithmInstance.h"

#include "drng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED_LEN_BYTES 64

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
DRNG_ctx drng_algorithm;

int main(void) {
  unsigned char* nonce1;
  unsigned char* nonce2;
  // DRNG_ctx for generating seed
  DRNG_ctx drng_seed;
  // DRNG_ctx for generating message
  DRNG_ctx drng_msg;

  // init drng_seed using nonce1
  nonce1 = (unsigned char*)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
  for (int i = 0; i < SEED_LEN_BYTES / 4; i++) {
    memcpy(nonce1 + 4 * i, "seed", 4);
  }
  init_random_number(&drng_seed, nonce1, SEED_LEN_BYTES);
  // init drng_msg using nonce2
  nonce2 = (unsigned char*)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
  for (int i = 0; i < SEED_LEN_BYTES / 3; i++) {
    memcpy(nonce2 + 3 * i, "msg", 3);
  }
  memcpy(nonce2 + SEED_LEN_BYTES - 1, "m", 1);
  init_random_number(&drng_msg, nonce2, SEED_LEN_BYTES);

  unsigned char* seed;
  unsigned char* m;
  unsigned char* sn;
  unsigned char* pk;
  unsigned char* sk;
  unsigned long long pk_len_bytes = sig_get_pk_len_bytes();
  unsigned long long sk_len_bytes = sig_get_sk_len_bytes();
  unsigned long long sn_len_bytes = sig_get_sn_len_bytes();

  int m_len_bytes = 56;
  pk = (unsigned char*)calloc(pk_len_bytes, sizeof(unsigned char));
  sk = (unsigned char*)calloc(sk_len_bytes, sizeof(unsigned char));
  sn = (unsigned char*)calloc(sn_len_bytes, sizeof(unsigned char));
  seed = (unsigned char*)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
  m = (unsigned char*)calloc(128, sizeof(unsigned char));

  get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
  // generate message using drng_msg
  get_random_number(&drng_msg, m, m_len_bytes * 8);
  init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

  if (sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes) != 0) {
    fprintf(stderr, "sig_keygen failed\n");
    return 1;
  }

  if (sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, &sn_len_bytes) != 0) {
    fprintf(stderr, "sig_sign failed\n");
    return 1;
  }

  if (sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes) != 0) {
    fprintf(stderr, "sig_verify failed\n");
    return 1;
  }

  printf("vistrutith_d3_512f_test: ok\n");
  return 0;
}
