/*
 * Breakdown benchmark for RRLWR SIGN.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpucycles.h"
#include "drng.h"
#include "sign_test.h"

#define NUMBER_OF_TESTS 1000
#define RNG_SEED_LENGTH 32
#define BENCHMARK_MESSAGE_LEN 32

DRNG_ctx drng_algorithm;

static uint64_t total_cycles[NUMBER_OF_TESTS];
static uint64_t stage_cycles[SIGN_TEST_STAGE_COUNT][NUMBER_OF_TESTS];
static uint64_t rounds_cycles[NUMBER_OF_TESTS];

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
  memset(rounds_cycles, 0, sizeof(rounds_cycles));
}

static unsigned int stage_indent(unsigned int stage)
{
  switch(stage) {
  case SIGN_TEST_KEYGEN_A_UNIFORM_BASE:
  case SIGN_TEST_KEYGEN_A_PREPARE_P1:
  case SIGN_TEST_KEYGEN_A_PREPARE_P2:
  case SIGN_TEST_KEYGEN_MUL_AS1_NTT_X2:
  case SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_X2:
    return 2;
  case SIGN_TEST_KEYGEN_AWIN_BASE_SHAKE128X4:
  case SIGN_TEST_KEYGEN_AWIN_BASE_UNPACK:
  case SIGN_TEST_KEYGEN_MUL_AS1_NTT_P1:
  case SIGN_TEST_KEYGEN_MUL_AS1_NTT_P2:
  case SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_P1:
  case SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_P2:
  case SIGN_TEST_KEYGEN_MUL_AS1_CRT:
    return 4;
  default:
    return 0;
  }
}

static void save_stage_cycles(unsigned int iter)
{
  for(unsigned int stage = 0; stage < SIGN_TEST_STAGE_COUNT; stage++) {
    stage_cycles[stage][iter] = sign_test_cycles[stage];
  }
  rounds_cycles[iter] = sign_test_rejection_rounds;
}

static void print_breakdown(const char *label)
{
  uint64_t total_med = median_local(total_cycles, NUMBER_OF_TESTS);
  uint64_t total_avg = average_local(total_cycles, NUMBER_OF_TESTS);

  printf("%s total\n", label);
  printf("median: %llu cycles/ticks\n", (unsigned long long)total_med);
  printf("average: %llu cycles/ticks\n\n", (unsigned long long)total_avg);

  printf("%-32s %14s %14s %9s\n", "stage", "median", "average", "avg%");
  for(unsigned int stage = 0; stage < SIGN_TEST_STAGE_COUNT; stage++) {
    uint64_t med = median_local(stage_cycles[stage], NUMBER_OF_TESTS);
    uint64_t avg = average_local(stage_cycles[stage], NUMBER_OF_TESTS);

    if(med == 0 && avg == 0) {
      continue;
    }

    printf("%*s%-*s %14llu %14llu %8.2f%%\n",
           stage_indent(stage),
           "",
           32 - stage_indent(stage),
           sign_test_stage_name(stage),
           (unsigned long long)med,
           (unsigned long long)avg,
           total_avg == 0 ? 0.0 : (100.0 * (double)avg) / (double)total_avg);
  }

  if(average_local(rounds_cycles, NUMBER_OF_TESTS) != 0) {
    printf("%-32s %14llu %14llu\n",
           "rejection_rounds",
           (unsigned long long)median_local(rounds_cycles, NUMBER_OF_TESTS),
           (unsigned long long)average_local(rounds_cycles, NUMBER_OF_TESTS));
  }

  printf("\n");
}

int main(void)
{
  unsigned char sk[RRLWR_SIGN_SK_LEN];
  unsigned char pk[RRLWR_SIGN_PK_LEN];
  unsigned char sn[RRLWR_SIGN_SIG_LEN];
  unsigned char m[BENCHMARK_MESSAGE_LEN] = {0};
  unsigned long long pk_len_bytes;
  unsigned long long sk_len_bytes;
  unsigned long long sn_len_bytes;
  unsigned long long m_len_bytes = BENCHMARK_MESSAGE_LEN;
  uint64_t overhead;
  const unsigned char seed[RNG_SEED_LENGTH] = {0};

  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);
  overhead = cpucycles_overhead();

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    uint64_t start = cpucycles();
    if(sig_keygen_test(pk, &pk_len_bytes, sk, &sk_len_bytes)) {
      return -1;
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("keygen");

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    uint64_t start = cpucycles();
    if(sig_sign_test(sk, sk_len_bytes, m, m_len_bytes, sn, &sn_len_bytes)) {
      return -1;
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("sign");

  clear_results();
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    uint64_t start = cpucycles();
    if(sig_verify_test(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes)) {
      return -1;
    }
    total_cycles[i] = cpucycles() - start - overhead;
    save_stage_cycles(i);
  }
  print_breakdown("verify");

  return 0;
}
