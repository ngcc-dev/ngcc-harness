// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "api.h"
#include "randombytes.h"
#include "hal.h"
#include "sendfn.h"
#include "drng.h"

#include <string.h>

#define SEED_LEN_BYTES 64
#define send_stack_usage(S, U) send_unsigned((S), (U))

DRNG_ctx drng_algorithm;

static unsigned char g_seed[SEED_LEN_BYTES];
static void reseed_drng(void)
{
  init_random_number(&drng_algorithm, g_seed, SEED_LEN_BYTES);
}

static unsigned char pka[CRYPTO_KEX_PUBLICKEYBYTES], pkb[CRYPTO_KEX_PUBLICKEYBYTES];
static unsigned char ska[CRYPTO_KEX_SECRETKEYBYTES], skb[CRYPTO_KEX_SECRETKEYBYTES];
static unsigned char pka0[CRYPTO_KEX_PUBLICKEYBYTES], pkb0[CRYPTO_KEX_PUBLICKEYBYTES];
static unsigned char sta[CRYPTO_KEX_STATEBYTES], stb[CRYPTO_KEX_STATEBYTES];
static unsigned char m1[CRYPTO_KEX_MSGBYTES], m2[CRYPTO_KEX_MSGBYTES];
static unsigned char m3[CRYPTO_KEX_MSGBYTES], m4[CRYPTO_KEX_MSGBYTES];
static unsigned char ssa[CRYPTO_KEX_SSBYTES], ssb[CRYPTO_KEX_SSBYTES];

static unsigned int s_init_a, s_init_b, s_p1, s_p2, s_p3, s_p4, s_da, s_db;

static int test_stack(void)
{
  unsigned long long pka_len = 0, ska_len = 0, pkb_len = 0, skb_len = 0;
  unsigned long long sta_len = 0, stb_len = 0, ssa_len = 0, ssb_len = 0;
  unsigned long long m1_len = 0, m2_len = 0, m3_len = 0, m4_len = 0;

  /* No reseed here: the DRNG is seeded once in main() and left to stream, so
   * each iteration runs a different transcript (mirrors the KEM stack.c using
   * fresh randomness per iteration). The stream never runs dry. */
  hal_spraystack(); kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len); s_init_a = hal_checkstack();
  hal_spraystack(); kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len); s_init_b = hal_checkstack();
  memcpy(pka0, pka, (size_t)pka_len);
  memcpy(pkb0, pkb, (size_t)pkb_len);

  hal_spraystack(); kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, &sta_len, m1, &m1_len); s_p1 = hal_checkstack();
  hal_spraystack(); kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, &stb_len, m2, &m2_len); s_p2 = hal_checkstack();
  hal_spraystack(); kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m2, m2_len, sta, &sta_len, m3, &m3_len); s_p3 = hal_checkstack();
  memcpy(pkb, pkb0, (size_t)pkb_len);
  hal_spraystack(); kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m3, m3_len, stb, &stb_len, m4, &m4_len); s_p4 = hal_checkstack();
  memcpy(pka, pka0, (size_t)pka_len);

  hal_spraystack(); kex_derive_ss_a(ska, ska_len, pkb, pkb_len, m4, m4_len, sta, sta_len, ssa, &ssa_len); s_da = hal_checkstack();
  hal_spraystack(); kex_derive_ss_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, stb_len, ssb, &ssb_len); s_db = hal_checkstack();

  if (ssa_len != ssb_len || memcmp(ssa, ssb, (size_t)ssa_len) != 0)
    return -1;

  send_stack_usage("kex_init_a stack usage:", s_init_a);
  send_stack_usage("kex_init_b stack usage:", s_init_b);
  send_stack_usage("kex_generate_pass1_msg_a stack usage:", s_p1);
  send_stack_usage("kex_generate_pass2_msg_b stack usage:", s_p2);
  send_stack_usage("kex_generate_pass3_msg_a stack usage:", s_p3);
  send_stack_usage("kex_generate_pass4_msg_b stack usage:", s_p4);
  send_stack_usage("kex_derive_ss_a stack usage:", s_da);
  send_stack_usage("kex_derive_ss_b stack usage:", s_db);
  hal_send_str("OK KEYS\n");
  return 0;
}

int main(void)
{
  int i;
  hal_setup(CLOCK_FAST);

  for (i = 0; i < SEED_LEN_BYTES / 4; i++)
    memcpy(g_seed + 4 * i, "seed", 4);
  reseed_drng();   /* seed once; the stream advances across iterations */

  hal_send_str("==========================");
  for (i = 0; i < MUPQ_ITERATIONS; i++) {
    if (test_stack())
      hal_send_str("ERROR KEYS\n");
    hal_send_str("+");
  }
  hal_send_str("#");
  return 0;
}
