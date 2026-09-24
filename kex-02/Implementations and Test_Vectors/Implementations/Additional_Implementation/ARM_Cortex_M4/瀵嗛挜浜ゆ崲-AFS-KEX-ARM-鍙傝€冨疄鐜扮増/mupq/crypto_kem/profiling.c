// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "api.h"
#include "hal.h"
#include "sendfn.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// https://stackoverflow.com/a/1489985/1711232
#define PASTER(x, y) x##y
#define EVALUATOR(x, y) PASTER(x, y)
#define NAMESPACE(fun) EVALUATOR(MUPQ_NAMESPACE, fun)

// use different names so we can have empty namespaces
#define MUPQ_CRYPTO_BYTES           NAMESPACE(CRYPTO_BYTES)
#define MUPQ_CRYPTO_PUBLICKEYBYTES  NAMESPACE(CRYPTO_PUBLICKEYBYTES)
#define MUPQ_CRYPTO_SECRETKEYBYTES  NAMESPACE(CRYPTO_SECRETKEYBYTES)
#define MUPQ_CRYPTO_CIPHERTEXTBYTES NAMESPACE(CRYPTO_CIPHERTEXTBYTES)
#define MUPQ_CRYPTO_ALGNAME NAMESPACE(CRYPTO_ALGNAME)

#define MUPQ_crypto_kem_keypair NAMESPACE(crypto_kem_keypair)
#define MUPQ_crypto_kem_enc NAMESPACE(crypto_kem_enc)
#define MUPQ_crypto_kem_dec NAMESPACE(crypto_kem_dec)

#define printcycles(S, U) send_unsignedll((S), (U))

unsigned long long hash_cycles;

/* Extra per-function profiling counters. Updated only by schemes that
 * insert PROFILE_FUNCTIONS probes for these symbols (e.g. bw-kem-*).
 * For schemes without probes, they remain 0 — harmless. */
unsigned long long pseudoxof_cycles;
unsigned long long pseudohash_cycles;
unsigned long long ntt_cycles;
unsigned long long invntt_cycles;
unsigned long long basemul_cycles;
unsigned long long reduce_cycles;
unsigned long long bw32enc_cycles;
unsigned long long bw32dec_cycles;
unsigned long long compress_cycles;
unsigned long long decompress_cycles;
unsigned long long tobytes_cycles;
unsigned long long frombytes_cycles;
unsigned long long frommsg_cycles;
unsigned long long tomsg_cycles;
unsigned long long getnoise_eta1_cycles;
unsigned long long getnoise_eta2_cycles;

static void reset_counters(void)
{
  hash_cycles = 0;
  pseudoxof_cycles = 0;
  pseudohash_cycles = 0;
  ntt_cycles = 0;
  invntt_cycles = 0;
  basemul_cycles = 0;
  reduce_cycles = 0;
  bw32enc_cycles = 0;
  bw32dec_cycles = 0;
  compress_cycles = 0;
  decompress_cycles = 0;
  tobytes_cycles = 0;
  frombytes_cycles = 0;
  frommsg_cycles = 0;
  tomsg_cycles = 0;
  getnoise_eta1_cycles = 0;
  getnoise_eta2_cycles = 0;
}

static void print_counters(const char *op)
{
  char buf[64];
  (void)op;
  /* hash_cycles preserved for backwards compatibility with existing parser */
  snprintf(buf, sizeof(buf), "%s hash cycles:", op);          printcycles(buf, hash_cycles);
  snprintf(buf, sizeof(buf), "%s pseudohash cycles:", op);   printcycles(buf, pseudohash_cycles);
  snprintf(buf, sizeof(buf), "%s pseudoxof cycles:", op);     printcycles(buf, pseudoxof_cycles);
  snprintf(buf, sizeof(buf), "%s ntt cycles:", op);           printcycles(buf, ntt_cycles);
  snprintf(buf, sizeof(buf), "%s invntt cycles:", op);        printcycles(buf, invntt_cycles);
  snprintf(buf, sizeof(buf), "%s basemul cycles:", op);       printcycles(buf, basemul_cycles);
  snprintf(buf, sizeof(buf), "%s reduce cycles:", op);        printcycles(buf, reduce_cycles);
  snprintf(buf, sizeof(buf), "%s bw32enc cycles:", op);       printcycles(buf, bw32enc_cycles);
  snprintf(buf, sizeof(buf), "%s bw32dec cycles:", op);       printcycles(buf, bw32dec_cycles);
  snprintf(buf, sizeof(buf), "%s compress cycles:", op);      printcycles(buf, compress_cycles);
  snprintf(buf, sizeof(buf), "%s decompress cycles:", op);    printcycles(buf, decompress_cycles);
  snprintf(buf, sizeof(buf), "%s tobytes cycles:", op);       printcycles(buf, tobytes_cycles);
  snprintf(buf, sizeof(buf), "%s frombytes cycles:", op);     printcycles(buf, frombytes_cycles);
  snprintf(buf, sizeof(buf), "%s frommsg cycles:", op);       printcycles(buf, frommsg_cycles);
  snprintf(buf, sizeof(buf), "%s tomsg cycles:", op);         printcycles(buf, tomsg_cycles);
  snprintf(buf, sizeof(buf), "%s getnoise_eta1 cycles:", op); printcycles(buf, getnoise_eta1_cycles);
  snprintf(buf, sizeof(buf), "%s getnoise_eta2 cycles:", op); printcycles(buf, getnoise_eta2_cycles);
}

int main(void)
{
  unsigned char key_a[MUPQ_CRYPTO_BYTES], key_b[MUPQ_CRYPTO_BYTES];
  unsigned char sk[MUPQ_CRYPTO_SECRETKEYBYTES];
  unsigned char pk[MUPQ_CRYPTO_PUBLICKEYBYTES];
  unsigned char ct[MUPQ_CRYPTO_CIPHERTEXTBYTES];
  unsigned long long t0, t1;
  int i;

  hal_setup(CLOCK_BENCHMARK);

  hal_send_str("==========================");

  for(i=0;i<MUPQ_ITERATIONS; i++)
  {
    // Key-pair generation
    reset_counters();
    t0 = hal_get_time();
    MUPQ_crypto_kem_keypair(pk, sk);
    t1 = hal_get_time();
    printcycles("keypair cycles:", t1-t0);
    print_counters("keypair");

    // Encapsulation
    reset_counters();
    t0 = hal_get_time();
    MUPQ_crypto_kem_enc(ct, key_a, pk);
    t1 = hal_get_time();
    printcycles("encaps cycles:", t1-t0);
    print_counters("encaps");

    // Decapsulation
    reset_counters();
    t0 = hal_get_time();
    MUPQ_crypto_kem_dec(key_b, ct, sk);
    t1 = hal_get_time();
    printcycles("decaps cycles:", t1-t0);
    print_counters("decaps");

    if (memcmp(key_a, key_b, MUPQ_CRYPTO_BYTES)) {
      hal_send_str("ERROR KEYS\n");
    }
    else {
      hal_send_str("OK KEYS\n");
    }
    hal_send_str("+");
  }
  hal_send_str("#");
  return 0;
}
