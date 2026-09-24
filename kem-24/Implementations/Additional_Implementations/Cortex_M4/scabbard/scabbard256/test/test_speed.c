#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "../KEM_scabbard256.h"
#include "../drng.h"
#include "cpucycles.h"
#include "speed_print.h"

#define NTESTS 10000//0
#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

uint64_t t[NTESTS];

int main(void)
{
  size_t i;

  unsigned char *nonce;
	// DRNG_ctx for generating seed
	DRNG_ctx drng_seed;
  unsigned char *seed, *ss, *ss1, *ct, *pk, *sk;
	unsigned long long pk_len_bytes, sk_len_bytes, ss_len_bytes, ct_len_bytes;
	int rtn;

  pk_len_bytes = kem_get_pk_len_bytes();
	sk_len_bytes = kem_get_sk_len_bytes();
	ss_len_bytes = kem_get_ss_len_bytes();
	ct_len_bytes = kem_get_ct_len_bytes();
	pk = (unsigned char *)calloc(pk_len_bytes, sizeof(unsigned char));
	sk = (unsigned char *)calloc(sk_len_bytes, sizeof(unsigned char));
	ss = (unsigned char *)calloc(ss_len_bytes, sizeof(unsigned char));
	ss1 = (unsigned char *)calloc(ss_len_bytes, sizeof(unsigned char));
	ct = (unsigned char *)calloc(ct_len_bytes, sizeof(unsigned char));
	seed = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));

  // generate seed using drng_seed
  get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
  // init drng_algorithm using seed
	init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

  for(i=0;i<NTESTS;i++) {

    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss1, &ss_len_bytes);
    for (size_t j = 0; j < ss_len_bytes; j++)
    {
      if(ss[j] != ss1[j])
        {
          printf("%ld Error\n", i);
            return 1;
        }
    }
  }
  
  for(i=0;i<NTESTS;i++) {
    t[i] = cpucycles();
    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
  }
  print_results("scabbard256_kem_keygen: ", t, NTESTS);

  for(i=0;i<NTESTS;i++) {
    t[i] = cpucycles();
    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
  }
  print_results("scabbard256_kem_enc: ", t, NTESTS);

  for(i=0;i<NTESTS;i++) {
    t[i] = cpucycles();
    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss1, &ss_len_bytes);
  }
  print_results("scabbard256_kem_dec: ", t, NTESTS);

  return 0;
}
