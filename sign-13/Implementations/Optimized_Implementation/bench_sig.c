#define _POSIX_C_SOURCE 199309L

#include "SIG_AlgorithmInstance.h"
#include "drng.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

#define BENCH_SEED_LEN_BYTES 64
#define BENCH_MESSAGE_LEN_BYTES 64

DRNG_ctx drng_algorithm;

static uint64_t now_ns(void) {
  struct timespec ts;
#if defined(CLOCK_MONOTONIC_RAW)
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint64_t now_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
  return __rdtsc();
#else
  return 0;
#endif
}

static void init_bench_drng(const char* label) {
  unsigned char seed[BENCH_SEED_LEN_BYTES];
  const size_t label_len = strlen(label);
  for (size_t i = 0; i < BENCH_SEED_LEN_BYTES; ++i) {
    seed[i] = (unsigned char)(label[i % label_len] + (unsigned char)i);
  }
  init_random_number(&drng_algorithm, seed, BENCH_SEED_LEN_BYTES);
}

static void fill_message(unsigned char* msg) {
  for (size_t i = 0; i < BENCH_MESSAGE_LEN_BYTES; ++i) {
    msg[i] = (unsigned char)(0xa5u ^ (unsigned char)i);
  }
}

static unsigned long long parse_iterations(int argc, char** argv) {
  if (argc < 2) {
    return 100;
  }

  char* end = NULL;
  const unsigned long long iterations = strtoull(argv[1], &end, 10);
  if (end == argv[1] || *end != '\0' || iterations == 0) {
    fprintf(stderr, "usage: %s [iterations]\n", argv[0]);
    exit(2);
  }
  return iterations;
}

int main(int argc, char** argv) {
  const unsigned long long iterations = parse_iterations(argc, argv);
  const unsigned long long pk_len = sig_get_pk_len_bytes();
  const unsigned long long sk_len = sig_get_sk_len_bytes();
  const unsigned long long sig_len = sig_get_sn_len_bytes();

  unsigned char* pk = (unsigned char*)malloc(pk_len);
  unsigned char* sk = (unsigned char*)malloc(sk_len);
  unsigned char* sig = (unsigned char*)malloc(sig_len);
  unsigned char msg[BENCH_MESSAGE_LEN_BYTES];
  if (pk == NULL || sk == NULL || sig == NULL) {
    fprintf(stderr, "allocation failed\n");
    free(pk);
    free(sk);
    free(sig);
    return 2;
  }
  fill_message(msg);

  unsigned long long actual_pk_len = 0;
  unsigned long long actual_sk_len = 0;
  unsigned long long actual_sig_len = 0;

  init_bench_drng("bench-keygen-warmup");
  if (sig_keygen(pk, &actual_pk_len, sk, &actual_sk_len) != 0 ||
      actual_pk_len != pk_len || actual_sk_len != sk_len) {
    fprintf(stderr, "keygen warmup failed\n");
    free(pk);
    free(sk);
    free(sig);
    return 1;
  }

  init_bench_drng("bench-keygen");
  uint64_t t0 = now_ns();
  uint64_t c0 = now_cycles();
  for (unsigned long long i = 0; i < iterations; ++i) {
    if (sig_keygen(pk, &actual_pk_len, sk, &actual_sk_len) != 0) {
      fprintf(stderr, "keygen failed at iteration %llu\n", i);
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
  }
  uint64_t c1 = now_cycles();
  uint64_t t1 = now_ns();
  const uint64_t keygen_ns = t1 - t0;
  const uint64_t keygen_cycles = c1 - c0;

  init_bench_drng("bench-sign-key");
  if (sig_keygen(pk, &actual_pk_len, sk, &actual_sk_len) != 0) {
    fprintf(stderr, "keygen for sign failed\n");
    free(pk);
    free(sk);
    free(sig);
    return 1;
  }

  init_bench_drng("bench-sign");
  t0 = now_ns();
  c0 = now_cycles();
  for (unsigned long long i = 0; i < iterations; ++i) {
    if (sig_sign(sk, sk_len, msg, BENCH_MESSAGE_LEN_BYTES, sig, &actual_sig_len) != 0 ||
        actual_sig_len != sig_len) {
      fprintf(stderr, "sign failed at iteration %llu\n", i);
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
  }
  c1 = now_cycles();
  t1 = now_ns();
  const uint64_t sign_ns = t1 - t0;
  const uint64_t sign_cycles = c1 - c0;

  init_bench_drng("bench-verify-signature");
  if (sig_sign(sk, sk_len, msg, BENCH_MESSAGE_LEN_BYTES, sig, &actual_sig_len) != 0 ||
      actual_sig_len != sig_len) {
    fprintf(stderr, "signature for verify failed\n");
    free(pk);
    free(sk);
    free(sig);
    return 1;
  }

  t0 = now_ns();
  c0 = now_cycles();
  for (unsigned long long i = 0; i < iterations; ++i) {
    if (sig_verify(pk, pk_len, sig, sig_len, msg, BENCH_MESSAGE_LEN_BYTES) != 0) {
      fprintf(stderr, "verify failed at iteration %llu\n", i);
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
  }
  c1 = now_cycles();
  t1 = now_ns();
  const uint64_t verify_ns = t1 - t0;
  const uint64_t verify_cycles = c1 - c0;

  const double keygen_avg_ms = (double)keygen_ns / (double)iterations / 1000000.0;
  const double sign_avg_ms = (double)sign_ns / (double)iterations / 1000000.0;
  const double verify_avg_ms = (double)verify_ns / (double)iterations / 1000000.0;
  const double keygen_avg_cycles = (double)keygen_cycles / (double)iterations;
  const double sign_avg_cycles = (double)sign_cycles / (double)iterations;
  const double verify_avg_cycles = (double)verify_cycles / (double)iterations;

  printf("%s,%llu,%u,%llu,%llu,%llu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
         ALGORITHM_INSTANCE,
         iterations,
         BENCH_MESSAGE_LEN_BYTES,
         pk_len,
         sk_len,
         sig_len,
         keygen_avg_ms,
         sign_avg_ms,
         verify_avg_ms,
         keygen_avg_cycles,
         sign_avg_cycles,
         verify_avg_cycles,
         1000.0 / keygen_avg_ms,
         1000.0 / sign_avg_ms,
         1000.0 / verify_avg_ms);

  free(pk);
  free(sk);
  free(sig);
  return 0;
}
