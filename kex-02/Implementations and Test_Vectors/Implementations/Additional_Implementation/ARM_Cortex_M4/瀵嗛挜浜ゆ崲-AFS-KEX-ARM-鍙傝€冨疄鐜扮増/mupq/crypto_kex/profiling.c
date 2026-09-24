// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "api.h"
#include "hal.h"
#include "sendfn.h"
#include "drng.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SEED_LEN_BYTES 64
#define printcycles(S, U) send_unsignedll((S), (U))

DRNG_ctx drng_algorithm;

/* Per-function profiling counters, updated by PROFILE_FUNCTIONS probes inside
 * the optimized core (m4fspeed). For schemes without probes they stay 0. */
unsigned long long hash_cycles;
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
  hash_cycles = pseudoxof_cycles = pseudohash_cycles = 0;
  ntt_cycles = invntt_cycles = basemul_cycles = reduce_cycles = 0;
  bw32enc_cycles = bw32dec_cycles = compress_cycles = decompress_cycles = 0;
  tobytes_cycles = frombytes_cycles = frommsg_cycles = tomsg_cycles = 0;
  getnoise_eta1_cycles = getnoise_eta2_cycles = 0;
}

static void print_counters(const char *op)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "%s hash cycles:", op);          printcycles(buf, hash_cycles);
  snprintf(buf, sizeof(buf), "%s pseudohash cycles:", op);    printcycles(buf, pseudohash_cycles);
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

static unsigned char g_seed[SEED_LEN_BYTES];
static void reseed_drng(void) { init_random_number(&drng_algorithm, g_seed, SEED_LEN_BYTES); }

/* pka0/pkb0 restore the buffers the protocol clobbers (pass3 overwrites pkb,
 * pass4 overwrites pka), outside the timed/counted regions, so each iteration
 * stays a single coherent 4-pass transcript (cf. speed.c). */
static unsigned char pka[CRYPTO_KEX_PUBLICKEYBYTES], pkb[CRYPTO_KEX_PUBLICKEYBYTES];
static unsigned char ska[CRYPTO_KEX_SECRETKEYBYTES], skb[CRYPTO_KEX_SECRETKEYBYTES];
static unsigned char pka0[CRYPTO_KEX_PUBLICKEYBYTES], pkb0[CRYPTO_KEX_PUBLICKEYBYTES];
static unsigned char sta[CRYPTO_KEX_STATEBYTES], stb[CRYPTO_KEX_STATEBYTES];
static unsigned char m1[CRYPTO_KEX_MSGBYTES], m2[CRYPTO_KEX_MSGBYTES];
static unsigned char m3[CRYPTO_KEX_MSGBYTES], m4[CRYPTO_KEX_MSGBYTES];
static unsigned char ssa[CRYPTO_KEX_SSBYTES], ssb[CRYPTO_KEX_SSBYTES];

int main(void)
{
  unsigned long long pka_len = 0, ska_len = 0, pkb_len = 0, skb_len = 0;
  unsigned long long sta_len = 0, stb_len = 0, ssa_len = 0, ssb_len = 0;
  unsigned long long m1_len = 0, m2_len = 0, m3_len = 0, m4_len = 0;
  unsigned long long t0, t1;
  int i;

  for (i = 0; i < SEED_LEN_BYTES / 4; i++)
    memcpy(g_seed + 4 * i, "seed", 4);

  hal_setup(CLOCK_BENCHMARK);
  hal_send_str("==========================");

  /* Seed the DRNG once and let it stream across iterations, so each iteration
   * profiles a different transcript (mirrors the KEM profiling.c using fresh
   * randomness per iteration). The stream never runs dry; the fixed start seed
   * keeps the whole run reproducible. */
  reseed_drng();

  for (i = 0; i < MUPQ_ITERATIONS; i++) {
    /* One coherent 4-pass transcript per iteration; reset/time/print the per-
     * function PROFILE counters around each of the 5 heavy stages. init_b is
     * identical to init_a and derive_ss_* are plain memcpy, so they run untimed
     * to keep the transcript coherent without redundant profiling. */
    reset_counters(); t0 = hal_get_time();
    kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len);
    t1 = hal_get_time(); printcycles("kex_init_a cycles:", t1 - t0); print_counters("kex_init_a");

    kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len);
    memcpy(pka0, pka, (size_t)pka_len);
    memcpy(pkb0, pkb, (size_t)pkb_len);

    reset_counters(); t0 = hal_get_time();
    kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, &sta_len, m1, &m1_len);
    t1 = hal_get_time(); printcycles("kex_generate_pass1_msg_a cycles:", t1 - t0); print_counters("kex_generate_pass1_msg_a");

    reset_counters(); t0 = hal_get_time();
    kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, &stb_len, m2, &m2_len);
    t1 = hal_get_time(); printcycles("kex_generate_pass2_msg_b cycles:", t1 - t0); print_counters("kex_generate_pass2_msg_b");

    reset_counters(); t0 = hal_get_time();
    kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m2, m2_len, sta, &sta_len, m3, &m3_len);
    t1 = hal_get_time(); printcycles("kex_generate_pass3_msg_a cycles:", t1 - t0); print_counters("kex_generate_pass3_msg_a");

    /* pass3 clobbered pkb; restore (untimed) before pass4. */
    memcpy(pkb, pkb0, (size_t)pkb_len);

    reset_counters(); t0 = hal_get_time();
    kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m3, m3_len, stb, &stb_len, m4, &m4_len);
    t1 = hal_get_time(); printcycles("kex_generate_pass4_msg_b cycles:", t1 - t0); print_counters("kex_generate_pass4_msg_b");

    /* pass4 clobbered pka; restore (untimed) before derive. */
    memcpy(pka, pka0, (size_t)pka_len);

    kex_derive_ss_a(ska, ska_len, pkb, pkb_len, m4, m4_len, sta, sta_len, ssa, &ssa_len);
    kex_derive_ss_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, stb_len, ssb, &ssb_len);

    /* Genuine check: ssa/ssb come from this same coherent run. */
    if (ssa_len == ssb_len && memcmp(ssa, ssb, (size_t)ssa_len) == 0)
      hal_send_str("OK KEYS\n");
    else
      hal_send_str("ERROR KEYS\n");
    hal_send_str("+");
  }
  hal_send_str("#");
  return 0;
}
