// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
//
// ICCS-format KAT generator for AFS-KEX, designed to run on the target
// (physical board or QEMU) and stream the test vectors over the HAL UART.
//
// The deterministic SM3-DRNG (drng.c) seeds both the per-record seed stream
// (drng_seed) and the per-record KEX randomness (drng_algorithm), so the output
// is fully reproducible and matches, byte for byte, the layout produced by the
// ICCS reference KAT_KEX.c on the host:
//
//     ====================        <- start marker, consumed by the host runner
//     Count = 0
//     Seed_Len = 64
//     Seed = ...
//     Pass_Num = 4
//     PKa_Len = ...
//     PKa = ...
//     SKa_Len = ...  / SKa = ...
//     Init_Sta_Len = ... / Init_Sta = ...
//     PKb_Len/PKb, SKb_Len/SKb, Init_Stb_Len/Init_Stb
//     Pass1_Sta_Len/Pass1_Sta, M1_Len/M1
//     Pass2_Stb_Len/Pass2_Stb, M2_Len/M2
//     Pass3_Sta_Len/Pass3_Sta, M3_Len/M3
//     Pass4_Stb_Len/Pass4_Stb, M4_Len/M4
//     SS_Len = ... / SS = ...
//     <blank line>
//     ... (10 records) ...
//     #                           <- end marker, consumed by the host runner
//
// The host-side kat.py captures everything between the markers and writes it to
// the scheme's Test_Vectors/KAT_KEX_AFS_KEX_C*.txt file.

#include "api.h"
#include "hal.h"
#include "drng.h"
#include "KEX_AlgorithmInstance.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SEED_LEN_BYTES 64
#define NTESTS 10

// The single DRNG instance for the KEX protocol; KEX_AlgorithmInstance.c and
// symmetric-iccs.c reference it via "extern".
DRNG_ctx drng_algorithm;

// Single reusable line buffer. SKa is the largest object we ever hex-encode.
static char line[2 * CRYPTO_KEX_SECRETKEYBYTES + 64];

static unsigned char pka[CRYPTO_KEX_PUBLICKEYBYTES], pkb[CRYPTO_KEX_PUBLICKEYBYTES];
static unsigned char ska[CRYPTO_KEX_SECRETKEYBYTES], skb[CRYPTO_KEX_SECRETKEYBYTES];
static unsigned char sta[CRYPTO_KEX_STATEBYTES], stb[CRYPTO_KEX_STATEBYTES];
static unsigned char m1[CRYPTO_KEX_MSGBYTES], m2[CRYPTO_KEX_MSGBYTES];
static unsigned char m3[CRYPTO_KEX_MSGBYTES], m4[CRYPTO_KEX_MSGBYTES];
static unsigned char ssa[CRYPTO_KEX_SSBYTES], ssb[CRYPTO_KEX_SSBYTES];

static void send_hex(const char *id, const unsigned char *msg, unsigned long long len)
{
  int n = sprintf(line, "%s", id);
  for (unsigned long long i = 0; i < len; i++)
    n += sprintf(line + n, "%02X", msg[i]);
  hal_send_str(line);
}

static void send_uint(const char *id, unsigned long long value)
{
  sprintf(line, "%s%llu", id, value);
  hal_send_str(line);
}

static void fail(const char *msg)
{
  hal_send_str(msg);
  hal_send_str("#");
}

int main(void)
{
  unsigned char nonce[SEED_LEN_BYTES];
  unsigned char seed[SEED_LEN_BYTES];
  DRNG_ctx drng_seed;

  unsigned long long pass;
  unsigned long long pka_len = 0, ska_len = 0, sta_len = 0;
  unsigned long long pkb_len = 0, skb_len = 0, stb_len = 0;
  unsigned long long m1_len = 0, m2_len = 0, m3_len = 0, m4_len = 0;
  unsigned long long ssa_len = 0, ssb_len = 0;
  unsigned char *ma, *mb;
  unsigned long long ma_len, mb_len;
  int rtn;

  hal_setup(CLOCK_FAST);

  // Start marker: at least four '=' so the host runner can locate the body.
  hal_send_str("==========================");

  // The seed stream is itself driven by a DRNG instantiated from the fixed
  // nonce "seedseed..." (64 bytes), exactly like the ICCS reference harness.
  for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
    memcpy(nonce + 4 * i, "seed", 4);
  init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);

  pass = kex_get_passes_num();

  for (int i = 0; i < NTESTS; i++)
  {
    send_uint("Count = ", (unsigned long long)i);

    get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
    send_uint("Seed_Len = ", SEED_LEN_BYTES);
    send_hex("Seed = ", seed, SEED_LEN_BYTES);

    // (Re)seed the KEX randomness for this record.
    init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

    send_uint("Pass_Num = ", pass);

    // ---- Initialise by the initiator ----
    rtn = kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len);
    if (rtn < 0) { fail("ERROR: kex_init_a"); return -1; }
    send_uint("PKa_Len = ", pka_len);
    send_hex("PKa = ", pka, pka_len);
    send_uint("SKa_Len = ", ska_len);
    send_hex("SKa = ", ska, ska_len);
    send_uint("Init_Sta_Len = ", sta_len);
    send_hex("Init_Sta = ", sta, sta_len);

    // ---- Initialise by the responder ----
    rtn = kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len);
    if (rtn < 0) { fail("ERROR: kex_init_b"); return -1; }
    send_uint("PKb_Len = ", pkb_len);
    send_hex("PKb = ", pkb, pkb_len);
    send_uint("SKb_Len = ", skb_len);
    send_hex("SKb = ", skb, skb_len);
    send_uint("Init_Stb_Len = ", stb_len);
    send_hex("Init_Stb = ", stb, stb_len);

    ma = NULL; mb = NULL; ma_len = 0; mb_len = 0;

    for (;;)
    {
      // Pass 1: initiator
      rtn = kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, &sta_len, m1, &m1_len);
      if (rtn < 0) { fail("ERROR: kex_generate_pass1_msg_a"); return -1; }
      send_uint("Pass1_Sta_Len = ", sta_len);
      send_hex("Pass1_Sta = ", sta, sta_len);
      send_uint("M1_Len = ", m1_len);
      send_hex("M1 = ", m1, m1_len);
      ma = m1; ma_len = m1_len;
      if (rtn == 1) { if (pass != 1) { fail("ERROR: pass"); return -1; } break; }

      // Pass 2: responder
      rtn = kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, &stb_len, m2, &m2_len);
      if (rtn < 0) { fail("ERROR: kex_generate_pass2_msg_b"); return -1; }
      send_uint("Pass2_Stb_Len = ", stb_len);
      send_hex("Pass2_Stb = ", stb, stb_len);
      send_uint("M2_Len = ", m2_len);
      send_hex("M2 = ", m2, m2_len);
      mb = m2; mb_len = m2_len;
      if (rtn == 1) { if (pass != 2) { fail("ERROR: pass"); return -1; } break; }

      // Pass 3: initiator
      rtn = kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m2, m2_len, sta, &sta_len, m3, &m3_len);
      if (rtn < 0) { fail("ERROR: kex_generate_pass3_msg_a"); return -1; }
      send_uint("Pass3_Sta_Len = ", sta_len);
      send_hex("Pass3_Sta = ", sta, sta_len);
      send_uint("M3_Len = ", m3_len);
      send_hex("M3 = ", m3, m3_len);
      ma = m3; ma_len = m3_len;
      if (rtn == 1) { if (pass != 3) { fail("ERROR: pass"); return -1; } break; }

      // Pass 4: responder
      rtn = kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m3, m3_len, stb, &stb_len, m4, &m4_len);
      if (rtn < 0) { fail("ERROR: kex_generate_pass4_msg_b"); return -1; }
      send_uint("Pass4_Stb_Len = ", stb_len);
      send_hex("Pass4_Stb = ", stb, stb_len);
      send_uint("M4_Len = ", m4_len);
      send_hex("M4 = ", m4, m4_len);
      mb = m4; mb_len = m4_len;
      if (rtn == 1) { if (pass != 4) { fail("ERROR: pass"); return -1; } break; }

      break;
    }

    rtn = kex_derive_ss_a(ska, ska_len, pkb, pkb_len, mb, mb_len, sta, sta_len, ssa, &ssa_len);
    if (rtn < 0) { fail("ERROR: kex_derive_ss_a"); return -1; }
    rtn = kex_derive_ss_b(skb, skb_len, pka, pka_len, ma, ma_len, stb, stb_len, ssb, &ssb_len);
    if (rtn < 0) { fail("ERROR: kex_derive_ss_b"); return -1; }

    if (ssa_len != ssb_len || memcmp(ssa, ssb, ssa_len) != 0)
    {
      fail("ERROR: shared secret mismatch");
      return -1;
    }
    send_uint("SS_Len = ", ssa_len);
    send_hex("SS = ", ssa, ssa_len);

    // Blank line separates records (matches the reference layout).
    hal_send_str("");
  }

  // End marker.
  hal_send_str("#");
  return 0;
}
