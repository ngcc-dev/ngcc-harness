/*
 * Breakdown benchmark for RRLWR KEM.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpucycles.h"
#include "drng.h"
#include "pke_test.h"
#include "kem_test.h"

#define NUMBER_OF_TESTS 1000
#define RNG_SEED_LENGTH 32

DRNG_ctx drng_algorithm;

static uint64_t total_cycles[NUMBER_OF_TESTS];
static uint64_t stage_cycles[KEM_TEST_STAGE_COUNT][NUMBER_OF_TESTS];

static int cmp_uint64_local(const void *a, const void *b)
{
  uint64_t av = *(const uint64_t *)a;
  uint64_t bv = *(const uint64_t *)b;

  if(av < bv) return -1;
  if(av > bv) return 1;
  return 0;
}

static uint64_t average_local(const uint64_t *x, size_t n)
{
  uint64_t acc = 0;

  for(size_t i = 0; i < n; i++) {
    acc += x[i];
  }

  return acc / n;
}

static uint64_t median_local(const uint64_t *x, size_t n)
{
  uint64_t tmp[NUMBER_OF_TESTS];

  memcpy(tmp, x, n * sizeof(tmp[0]));
  qsort(tmp, n, sizeof(tmp[0]), cmp_uint64_local);

  if(n & 1) {
    return tmp[n / 2];
  }

  return (tmp[n / 2 - 1] + tmp[n / 2]) / 2;
}

static void clear_results(void)
{
  memset(total_cycles, 0, sizeof(total_cycles));
  memset(stage_cycles, 0, sizeof(stage_cycles));
}

static unsigned int stage_indent(unsigned int stage)
{
  switch(stage) {
  case KEM_TEST_PKE_KEYGEN_A_UNIFORM_BASE:
  case KEM_TEST_PKE_KEYGEN_A_PREPARE:
  case KEM_TEST_PKE_ENC_A_UNIFORM_BASE:
  case KEM_TEST_PKE_ENC_A_PREPARE:
  case KEM_TEST_PKE_ENC_UNPACK_B_BASE:
  case KEM_TEST_PKE_ENC_UNPACK_B_PREPARE:
  case KEM_TEST_PKE_DEC_BP_TO_AWIN_BASE:
  case KEM_TEST_PKE_DEC_BP_TO_AWIN_PREPARE:
    return 2;
  default:
    return 0;
  }
}

static void save_stage_cycles(unsigned int iter)
{
  for(unsigned int stage = 0; stage < KEM_TEST_STAGE_COUNT; stage++) {
    stage_cycles[stage][iter] = kem_test_cycles[stage];
  }
}

static void print_breakdown(const char *label)
{
  uint64_t total_med = median_local(total_cycles, NUMBER_OF_TESTS);
  uint64_t total_avg = average_local(total_cycles, NUMBER_OF_TESTS);

  printf("%s total\n", label);
  printf("median: %llu cycles/ticks\n", (unsigned long long)total_med);
  printf("average: %llu cycles/ticks\n\n", (unsigned long long)total_avg);

  printf("%-40s %14s %14s %9s\n", "stage", "median", "average", "avg%");
  for(unsigned int stage = 0; stage < KEM_TEST_STAGE_COUNT; stage++) {
    uint64_t med = median_local(stage_cycles[stage], NUMBER_OF_TESTS);
    uint64_t avg = average_local(stage_cycles[stage], NUMBER_OF_TESTS);
    unsigned int indent = stage_indent(stage);

    if(med == 0 && avg == 0) {
      continue;
    }

    printf("%*s%-*s %14llu %14llu %8.2f%%\n",
           indent,
           "",
           40 - indent,
           kem_test_stage_name(stage),
           (unsigned long long)med,
           (unsigned long long)avg,
           total_avg == 0 ? 0.0 : (100.0 * (double)avg) / (double)total_avg);
  }

  printf("\n");
}

static void measure_pke_keygen(const unsigned char seedA[RRLWR_PKE_SEED_A_LEN],
                               const unsigned char seedS[RRLWR_SEED_S_LEN])
{
  unsigned char sk[RRLWR_PKE_SK_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN];
  uint64_t overhead = cpucycles_overhead();

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    kem_test_reset_cycles();
    uint64_t start = cpucycles();
    if(pke_keygen_test(pk, sk, seedA, seedS)) {
      exit(-1);
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("pke_keygen");
}

static void measure_pke_encrypt(const unsigned char seedA[RRLWR_PKE_SEED_A_LEN],
                                const unsigned char seedS[RRLWR_SEED_S_LEN],
                                const unsigned char seedSp[RRLWR_SEED_S_LEN],
                                const unsigned char m[RRLWR_PKE_MESSAGE_LEN])
{
  unsigned char sk[RRLWR_PKE_SK_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN];
  unsigned char ct[RRLWR_PKE_CT_LEN];
  uint64_t overhead = cpucycles_overhead();

  pke_keygen(pk, sk, seedA, seedS);

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    kem_test_reset_cycles();
    uint64_t start = cpucycles();
    if(pke_encrypt_test(ct, pk, m, seedSp)) {
      exit(-1);
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("pke_encrypt");
}

static void measure_pke_decrypt(const unsigned char seedA[RRLWR_PKE_SEED_A_LEN],
                                const unsigned char seedS[RRLWR_SEED_S_LEN],
                                const unsigned char seedSp[RRLWR_SEED_S_LEN],
                                const unsigned char m[RRLWR_PKE_MESSAGE_LEN])
{
  unsigned char sk[RRLWR_PKE_SK_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN];
  unsigned char ct[RRLWR_PKE_CT_LEN];
  unsigned char mp[RRLWR_PKE_MESSAGE_LEN];
  uint64_t overhead = cpucycles_overhead();

  pke_keygen(pk, sk, seedA, seedS);
  pke_encrypt(ct, pk, m, seedSp);

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    kem_test_reset_cycles();
    uint64_t start = cpucycles();
    if(pke_decrypt_test(mp, ct, sk)) {
      exit(-1);
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("pke_decrypt");
}

static void measure_kem_keygen(void)
{
  unsigned char sk[RRLWR_KEM_SK_LEN];
  unsigned char pk[RRLWR_KEM_PK_LEN];
  unsigned long long pk_len_bytes;
  unsigned long long sk_len_bytes;
  uint64_t overhead = cpucycles_overhead();

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    kem_test_reset_cycles();
    uint64_t start = cpucycles();
    if(kem_keygen_test(pk, &pk_len_bytes, sk, &sk_len_bytes)) {
      exit(-1);
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("kem_keygen");
}

static void measure_kem_encaps(unsigned char pk[RRLWR_KEM_PK_LEN],
                               unsigned long long pk_len_bytes)
{
  unsigned char ss[RRLWR_KEM_SS_LEN];
  unsigned char ct[RRLWR_KEM_CT_LEN];
  unsigned long long ss_len_bytes;
  unsigned long long ct_len_bytes;
  uint64_t overhead = cpucycles_overhead();

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    kem_test_reset_cycles();
    uint64_t start = cpucycles();
    if(kem_enc_test(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes)) {
      exit(-1);
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("kem_encaps");
}

static void measure_kem_decaps(unsigned char sk[RRLWR_KEM_SK_LEN],
                               unsigned long long sk_len_bytes,
                               unsigned char ct[RRLWR_KEM_CT_LEN],
                               unsigned long long ct_len_bytes)
{
  unsigned char ss[RRLWR_KEM_SS_LEN];
  unsigned long long ss_len_bytes;
  uint64_t overhead = cpucycles_overhead();

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    kem_test_reset_cycles();
    uint64_t start = cpucycles();
    if(kem_dec_test(sk, sk_len_bytes, ct, ct_len_bytes, ss, &ss_len_bytes)) {
      exit(-1);
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("kem_decaps");
}

int main(void)
{
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
  unsigned char seedS[RRLWR_SEED_S_LEN];
  unsigned char seedSp[RRLWR_SEED_S_LEN];
  unsigned char m[RRLWR_PKE_MESSAGE_LEN];
  unsigned char kem_sk[RRLWR_KEM_SK_LEN];
  unsigned char kem_pk[RRLWR_KEM_PK_LEN];
  unsigned char kem_ct[RRLWR_KEM_CT_LEN];
  unsigned char ss[RRLWR_KEM_SS_LEN];
  unsigned long long kem_pk_len;
  unsigned long long kem_sk_len;
  unsigned long long kem_ct_len;
  unsigned long long ss_len;
  const unsigned char seed[RNG_SEED_LENGTH] = {0};

  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);
  GENERATE_RANDOM_BYTES(seedA, RRLWR_PKE_SEED_A_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedS, RRLWR_SEED_S_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedSp, RRLWR_SEED_S_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(m, RRLWR_PKE_MESSAGE_LEN, &drng_algorithm);

  measure_pke_keygen(seedA, seedS);
  measure_pke_encrypt(seedA, seedS, seedSp, m);
  measure_pke_decrypt(seedA, seedS, seedSp, m);

  kem_keygen(kem_pk, &kem_pk_len, kem_sk, &kem_sk_len);
  kem_enc(kem_pk, kem_pk_len, ss, &ss_len, kem_ct, &kem_ct_len);

  measure_kem_keygen();
  measure_kem_encaps(kem_pk, kem_pk_len);
  measure_kem_decaps(kem_sk, kem_sk_len, kem_ct, kem_ct_len);

  return 0;
}
