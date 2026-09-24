// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "api.h"
#include "hal.h"
#include "sendfn.h"
#include "randombytes.h"
#include "drng.h"

#include <stdint.h>
#include <string.h>

#define SEED_LEN_BYTES 64

#define printcycles(S, U) send_unsignedll((S), (U))

/* The AFS-KEX protocol layer draws its randomness from this DRNG context. */
DRNG_ctx drng_algorithm;

static unsigned char g_seed[SEED_LEN_BYTES];
static void reseed_drng(void)
{
  init_random_number(&drng_algorithm, g_seed, SEED_LEN_BYTES);
}

/* Working buffers (global to keep them off the stack: C512 keys are several KiB).
 * pka0/pkb0 hold pristine public keys so the buffers that the protocol clobbers
 * (pass3 overwrites pkb, pass4 overwrites pka) can be restored OUTSIDE the timed
 * regions, keeping a single coherent run per iteration. */
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
   * benchmarks a different transcript (mirrors the KEM speed.c using fresh
   * randomness per iteration). The stream never runs dry; the fixed start seed
   * keeps the whole run reproducible. */
  reseed_drng();

  for (i = 0; i < MUPQ_ITERATIONS; i++) {
    /* One coherent 4-pass transcript per iteration; time each call in place. */
    t0 = hal_get_time();
    kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len);
    t1 = hal_get_time();
    printcycles("kex_init_a cycles:", t1 - t0);

    t0 = hal_get_time();
    kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len);
    t1 = hal_get_time();
    printcycles("kex_init_b cycles:", t1 - t0);

    memcpy(pka0, pka, (size_t)pka_len);
    memcpy(pkb0, pkb, (size_t)pkb_len);

    t0 = hal_get_time();
    kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, &sta_len, m1, &m1_len);
    t1 = hal_get_time();
    printcycles("kex_generate_pass1_msg_a cycles:", t1 - t0);

    t0 = hal_get_time();
    kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, &stb_len, m2, &m2_len);
    t1 = hal_get_time();
    printcycles("kex_generate_pass2_msg_b cycles:", t1 - t0);

    t0 = hal_get_time();
    kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m2, m2_len, sta, &sta_len, m3, &m3_len);
    t1 = hal_get_time();
    printcycles("kex_generate_pass3_msg_a cycles:", t1 - t0);

    /* pass3 clobbered pkb; restore (untimed) before pass4/derive. */
    memcpy(pkb, pkb0, (size_t)pkb_len);

    t0 = hal_get_time();
    kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m3, m3_len, stb, &stb_len, m4, &m4_len);
    t1 = hal_get_time();
    printcycles("kex_generate_pass4_msg_b cycles:", t1 - t0);

    /* pass4 clobbered pka; restore (untimed) before derive. */
    memcpy(pka, pka0, (size_t)pka_len);

    t0 = hal_get_time();
    kex_derive_ss_a(ska, ska_len, pkb, pkb_len, m4, m4_len, sta, sta_len, ssa, &ssa_len);
    t1 = hal_get_time();
    printcycles("kex_derive_ss_a cycles:", t1 - t0);

    t0 = hal_get_time();
    kex_derive_ss_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, stb_len, ssb, &ssb_len);
    t1 = hal_get_time();
    printcycles("kex_derive_ss_b cycles:", t1 - t0);

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
