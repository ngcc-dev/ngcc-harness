#include "vistrutith_d3_512s_AlgorithmInstance.h"
#include "drng.h"
#include "sig_impl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/resource.h>
#include "bench_cycles.h"

/* Keep diagnostics visible so failures are not mistaken as "no output". */
#define BENCH_SILENT 0

#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

static double seconds_since(clock_t start, clock_t end) {
  return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

static long bench_peak_rss_kib(void) {
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != 0) {
    return -1;
  }
#if defined(__APPLE__)
  return (long)(usage.ru_maxrss / 1024L);
#else
  return (long)usage.ru_maxrss;
#endif
}

static void fill_signature_message(unsigned char* message,
                                   unsigned long long message_len,
                                   size_t iteration) {
  static const unsigned char base[] =
      "Chinith sm4th ublockith vistrutith 56-byte bench msg v1.";
  static const unsigned char filler[] =
      " sm4th ublockith vistrutith bench message ";
  const size_t base_len = sizeof(base) - 1;
  const size_t filler_len = sizeof(filler) - 1;
  size_t pos = 0;

  while (pos < message_len && pos < base_len) {
    message[pos] = base[pos];
    pos++;
  }
  while (pos < message_len) {
    message[pos] = filler[(pos + iteration) % filler_len];
    pos++;
  }
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);
  size_t iterations = 100;
  bench_iteration_stats_t iter_stats = {0};
  if (argc > 1) {
    const long parsed = strtol(argv[1], NULL, 10);
    if (parsed > 0) {
      iterations = (size_t)parsed;
    }
  }
  size_t message_len_step = 0u;
  if (argc > 2) {
    const long parsed = strtol(argv[2], NULL, 10);
    if (parsed >= 0) {
      message_len_step = (size_t)parsed;
    }
  }
  if (iterations <= 0) {
#if !BENCH_SILENT
    fprintf(stderr, "iterations must be > 0\n");
#endif
    bench_iteration_stats_free(&iter_stats);
    return 1;
  }

  unsigned char* nonce1 = (unsigned char*)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
  if (!nonce1) {
#if !BENCH_SILENT
    fprintf(stderr, "nonce allocation failed\n");
#endif
    free(nonce1);
    bench_iteration_stats_free(&iter_stats);
    return 1;
  }
  for (int i = 0; i < SEED_LEN_BYTES / 4; i++) {
    memcpy(nonce1 + 4 * i, "seed", 4);
  }
  DRNG_ctx drng_seed;
  if (init_random_number(&drng_seed, nonce1, SEED_LEN_BYTES) != 0) {
#if !BENCH_SILENT
    fprintf(stderr, "drng init failed\n");
#endif
    free(nonce1);
    bench_iteration_stats_free(&iter_stats);
    return 1;
  }

  const size_t max_message_len = 56 + message_len_step * (iterations - 1);
  unsigned char* seed = (unsigned char*)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
  unsigned char* message = (unsigned char*)calloc(max_message_len, sizeof(unsigned char));
  if (!seed || !message) {
#if !BENCH_SILENT
    fprintf(stderr, "seed and message buffer allocation failed\n");
#endif
    free(seed);
    free(message);
    free(nonce1);
    bench_iteration_stats_free(&iter_stats);
    return 1;
  }

  unsigned long long pk_len_bytes = sig_get_pk_len_bytes();
  unsigned long long sk_len_bytes = sig_get_sk_len_bytes();
  unsigned long long sn_len_bytes = sig_get_sn_len_bytes();
  unsigned char* pk = (unsigned char*)calloc(pk_len_bytes, sizeof(unsigned char));
  unsigned char* sk = (unsigned char*)calloc(sk_len_bytes, sizeof(unsigned char));
  unsigned char* sn = (unsigned char*)calloc(sn_len_bytes, sizeof(unsigned char));
  if (!pk || !sk || !sn) {
#if !BENCH_SILENT
    fprintf(stderr, "key/signature allocation failed\n");
#endif
    free(pk);
    free(sk);
    free(sn);
    free(seed);
    free(message);
    free(nonce1);
    bench_iteration_stats_free(&iter_stats);
    return 1;
  }

  unsigned long long message_len = 56;
  const int cycles_ok = bench_cycles_available();
  if (bench_iteration_stats_init(&iter_stats, iterations) != 0) {
    free(pk);
    free(sk);
    free(sn);
    free(seed);
    free(message);
    free(nonce1);
    bench_iteration_stats_free(&iter_stats);
    return 1;
  }

  /* silence internal VISTRUTITH status prints during bench */
  sig_set_verbose(0);

  for (size_t i = 0; i < iterations; ++i) {
    if (get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8) != 0) {
#if !BENCH_SILENT
      fprintf(stderr, "drng seed output failed\n");
#endif
      free(pk);
      free(sk);
      free(sn);
      free(seed);
      free(message);
      free(nonce1);
      bench_iteration_stats_free(&iter_stats);
      return 1;
    }
    fill_signature_message(message, message_len, i);

    if (init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES) != 0) {
#if !BENCH_SILENT
      fprintf(stderr, "drng algorithm init failed\n");
#endif
      free(pk);
      free(sk);
      free(sn);
      free(seed);
      free(message);
      free(nonce1);
      bench_iteration_stats_free(&iter_stats);
      return 1;
    }

    const uint64_t cyc_start_keygen = cycles_ok ? bench_read_cycles() : 0u;
    clock_t start = clock();
    if (sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes) != 0) {
#if !BENCH_SILENT
      fprintf(stderr, "sig_keygen failed\n");
#endif
      free(pk);
      free(sk);
      free(sn);
      free(seed);
      free(message);
      free(nonce1);
      bench_iteration_stats_free(&iter_stats);
      return 1;
    }
    clock_t end = clock();
    iter_stats.keygen_ms[i] = 1000.0 * seconds_since(start, end);
    const uint64_t cyc_end_keygen = cycles_ok ? bench_read_cycles() : 0u;
    if (cycles_ok && cyc_end_keygen >= cyc_start_keygen) {
      iter_stats.keygen_mcyc[i] =
          (long double)(cyc_end_keygen - cyc_start_keygen) / 1000000.0L;
    }

    const uint64_t cyc_start_sign = cycles_ok ? bench_read_cycles() : 0u;
    start = clock();
    if (sig_sign(sk, sk_len_bytes, message, message_len, sn, &sn_len_bytes) != 0) {
#if !BENCH_SILENT
      fprintf(stderr, "sig_sign failed\n");
#endif
      free(pk);
      free(sk);
      free(sn);
      free(seed);
      free(message);
      free(nonce1);
      bench_iteration_stats_free(&iter_stats);
      return 1;
    }
    end = clock();
    iter_stats.sign_ms[i] = 1000.0 * seconds_since(start, end);
    const uint64_t cyc_end_sign = cycles_ok ? bench_read_cycles() : 0u;
    if (cycles_ok && cyc_end_sign >= cyc_start_sign) {
      iter_stats.sign_mcyc[i] =
          (long double)(cyc_end_sign - cyc_start_sign) / 1000000.0L;
    }

    const uint64_t cyc_start_verify = cycles_ok ? bench_read_cycles() : 0u;
    start = clock();
    if (sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, message, message_len) != 0) {
#if !BENCH_SILENT
      fprintf(stderr, "sig_verify failed\n");
#endif
      free(pk);
      free(sk);
      free(sn);
      free(seed);
      free(message);
      free(nonce1);
      bench_iteration_stats_free(&iter_stats);
      return 1;
    }
    end = clock();
    iter_stats.verify_ms[i] = 1000.0 * seconds_since(start, end);
    const uint64_t cyc_end_verify = cycles_ok ? bench_read_cycles() : 0u;
    if (cycles_ok && cyc_end_verify >= cyc_start_verify) {
      iter_stats.verify_mcyc[i] =
          (long double)(cyc_end_verify - cyc_start_verify) / 1000000.0L;
    }

    message_len += message_len_step;
  }

  printf("vistrutith_d3_512s KAT bench (iterations=%zu)\n", iterations);
  printf("cycle source: %s\n", bench_cycles_source());
  printf("message length: 56 + %zu*i bytes%s\n", message_len_step,
         message_len_step == 0 ? " (fixed 56B)" : "");
  printf("message content: deterministic ASCII prefix \"Chinith sm4th ublockith vistrutith 56-byte bench msg v1.\"\n");
  printf("per-iteration ms/op stats (average / min / median / p90 / max)\n");
  bench_print_double_stats("keygen", iter_stats.keygen_ms, iterations);
  bench_print_double_stats("sign", iter_stats.sign_ms, iterations);
  bench_print_double_stats("verify", iter_stats.verify_ms, iterations);
  if (cycles_ok) {
    printf("per-iteration Mcyc/op stats (average / min / median / p90 / max)\n");
    bench_print_long_double_stats("keygen", iter_stats.keygen_mcyc, iterations);
    bench_print_long_double_stats("sign", iter_stats.sign_mcyc, iterations);
    bench_print_long_double_stats("verify", iter_stats.verify_mcyc, iterations);
  }
  printf("pk size: %llu bytes\n", pk_len_bytes);
  printf("sk size: %llu bytes\n", sk_len_bytes);
  printf("sig size: %llu bytes\n", sn_len_bytes);
  {
    const long peak_rss_kib = bench_peak_rss_kib();
    if (peak_rss_kib >= 0) {
      printf("peak rss: %ld KiB\n", peak_rss_kib);
    } else {
      printf("peak rss: N/A\n");
    }
  }

  bench_iteration_stats_free(&iter_stats);
  free(pk);
  free(sk);
  free(sn);
  free(seed);
  free(message);
  free(nonce1);
  return 0;
}
