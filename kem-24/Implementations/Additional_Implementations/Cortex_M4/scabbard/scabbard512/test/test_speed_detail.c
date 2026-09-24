#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../KEM_scabbard512.h"
#include "../drng.h"
#include "../auxfunc.h"
#include "cpucycles.h"
#include "speed_print.h"

#define NTESTS 10
#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

uint64_t t[NTESTS];
uint64_t t_pseudo[NTESTS];

static int cmp_u64(const void *a, const void *b)
{
    uint64_t va = *(const uint64_t *)a;
    uint64_t vb = *(const uint64_t *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static uint64_t median_raw(uint64_t *l, size_t llen)
{
    qsort(l, llen, sizeof(uint64_t), cmp_u64);
    if (llen % 2) return l[llen / 2];
    return (l[llen / 2 - 1] + l[llen / 2]) / 2;
}

static uint64_t average_raw(const uint64_t *t, size_t tlen)
{
    uint64_t acc = 0;
    for (size_t i = 0; i < tlen; i++) {
        acc += t[i];
    }
    return acc / tlen;
}

static void print_results_raw(const char *s, uint64_t *t, size_t tlen)
{
    uint64_t *copy = (uint64_t *)malloc(tlen * sizeof(uint64_t));
    if (copy == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed in print_results_raw.\n");
        return;
    }
    memcpy(copy, t, tlen * sizeof(uint64_t));
    printf("%s\n", s);
    printf("median: %llu cycles/ticks\n", (unsigned long long)median_raw(copy, tlen));
    printf("average: %llu cycles/ticks\n\n", (unsigned long long)average_raw(t, tlen));
    free(copy);
}

static uint64_t sum_cycles(const uint64_t *arr, size_t len)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += arr[i];
    }
    return sum;
}

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
  
  // printf("No Error\n");

  reset_pseudoXOF_cycles();
  for(i=0;i<NTESTS;i++) {
    uint64_t before = get_pseudoXOF_cycles();
    t[i] = cpucycles();
    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    t_pseudo[i] = get_pseudoXOF_cycles() - before;
  }
  print_results("scabbard512_kem_keygen: ", t, NTESTS);
  print_results_raw("scabbard512_pseudoXOF_keygen: ", t_pseudo, NTESTS);
  printf("scabbard512_pseudoXOF_percent_keygen: %.2f%%\n\n",
         100.0 * (double)sum_cycles(t_pseudo, NTESTS) / (double)sum_cycles(t, NTESTS - 1));

  reset_pseudoXOF_cycles();
  for(i=0;i<NTESTS;i++) {
    uint64_t before = get_pseudoXOF_cycles();
    t[i] = cpucycles();
    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    t_pseudo[i] = get_pseudoXOF_cycles() - before;
  }
  print_results("scabbard512_kem_enc: ", t, NTESTS);
  print_results_raw("scabbard512_pseudoXOF_enc: ", t_pseudo, NTESTS);
  printf("scabbard512_pseudoXOF_percent_enc: %.2f%%\n\n",
         100.0 * (double)sum_cycles(t_pseudo, NTESTS) / (double)sum_cycles(t, NTESTS - 1));

  reset_pseudoXOF_cycles();
  for(i=0;i<NTESTS;i++) {
    uint64_t before = get_pseudoXOF_cycles();
    t[i] = cpucycles();
    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss1, &ss_len_bytes);
    t_pseudo[i] = get_pseudoXOF_cycles() - before;
  }
  print_results("scabbard512_kem_dec: ", t, NTESTS);
  print_results_raw("scabbard512_pseudoXOF_dec: ", t_pseudo, NTESTS);
  printf("scabbard512_pseudoXOF_percent_dec: %.2f%%\n",
         100.0 * (double)sum_cycles(t_pseudo, NTESTS) / (double)sum_cycles(t, NTESTS - 1));

  return 0;
}
