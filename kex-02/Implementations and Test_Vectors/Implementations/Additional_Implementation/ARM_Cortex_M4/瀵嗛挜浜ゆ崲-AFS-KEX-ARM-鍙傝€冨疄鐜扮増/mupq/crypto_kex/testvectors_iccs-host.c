// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
//
// Host-side counterpart of testvectors_iccs.c. It produces the exact same
// ICCS-format KAT body, but prints it to stdout instead of the HAL UART and
// without the start/end markers. kat.py runs this on the host to obtain a
// trusted reference, then checks the vectors generated on the target against
// it. The reference output itself is never written to disk.

#include "api.h"
#include "randombytes.h"
#include "drng.h"
#include "KEX_AlgorithmInstance.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED_LEN_BYTES 64
#define NTESTS 10

// The single DRNG instance for the KEX protocol; KEX_AlgorithmInstance.c and
// symmetric-iccs.c reference it via "extern".
DRNG_ctx drng_algorithm;

// The scheme library references randombytes() through the non-derand
// crypto_kem_keypair/enc, which the ICCS KEX path never calls. Provide a
// definition so the host link succeeds; it must never actually run.
int randombytes(uint8_t *out, size_t outlen)
{
  (void)out;
  (void)outlen;
  fprintf(stderr, "ERROR: randombytes() must not be called in the ICCS KAT\n");
  abort();
}

static void print_hex(const char *id, const unsigned char *msg, unsigned long long len)
{
  fputs(id, stdout);
  for (unsigned long long i = 0; i < len; i++)
    printf("%02X", msg[i]);
  putchar('\n');
}

static void print_uint(const char *id, unsigned long long value)
{
  printf("%s%llu\n", id, value);
}

int main(void)
{
  unsigned char nonce[SEED_LEN_BYTES];
  unsigned char seed[SEED_LEN_BYTES];
  DRNG_ctx drng_seed;

  static unsigned char pka[CRYPTO_KEX_PUBLICKEYBYTES], pkb[CRYPTO_KEX_PUBLICKEYBYTES];
  static unsigned char ska[CRYPTO_KEX_SECRETKEYBYTES], skb[CRYPTO_KEX_SECRETKEYBYTES];
  static unsigned char sta[CRYPTO_KEX_STATEBYTES], stb[CRYPTO_KEX_STATEBYTES];
  static unsigned char m1[CRYPTO_KEX_MSGBYTES], m2[CRYPTO_KEX_MSGBYTES];
  static unsigned char m3[CRYPTO_KEX_MSGBYTES], m4[CRYPTO_KEX_MSGBYTES];
  static unsigned char ssa[CRYPTO_KEX_SSBYTES], ssb[CRYPTO_KEX_SSBYTES];

  unsigned long long pass;
  unsigned long long pka_len = 0, ska_len = 0, sta_len = 0;
  unsigned long long pkb_len = 0, skb_len = 0, stb_len = 0;
  unsigned long long m1_len = 0, m2_len = 0, m3_len = 0, m4_len = 0;
  unsigned long long ssa_len = 0, ssb_len = 0;
  unsigned char *ma, *mb;
  unsigned long long ma_len, mb_len;
  int rtn;

  for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
    memcpy(nonce + 4 * i, "seed", 4);
  init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);

  pass = kex_get_passes_num();

  for (int i = 0; i < NTESTS; i++)
  {
    print_uint("Count = ", (unsigned long long)i);

    get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
    print_uint("Seed_Len = ", SEED_LEN_BYTES);
    print_hex("Seed = ", seed, SEED_LEN_BYTES);

    init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

    print_uint("Pass_Num = ", pass);

    rtn = kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len);
    if (rtn < 0) { fputs("ERROR: kex_init_a\n", stderr); return -1; }
    print_uint("PKa_Len = ", pka_len);
    print_hex("PKa = ", pka, pka_len);
    print_uint("SKa_Len = ", ska_len);
    print_hex("SKa = ", ska, ska_len);
    print_uint("Init_Sta_Len = ", sta_len);
    print_hex("Init_Sta = ", sta, sta_len);

    rtn = kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len);
    if (rtn < 0) { fputs("ERROR: kex_init_b\n", stderr); return -1; }
    print_uint("PKb_Len = ", pkb_len);
    print_hex("PKb = ", pkb, pkb_len);
    print_uint("SKb_Len = ", skb_len);
    print_hex("SKb = ", skb, skb_len);
    print_uint("Init_Stb_Len = ", stb_len);
    print_hex("Init_Stb = ", stb, stb_len);

    ma = NULL; mb = NULL; ma_len = 0; mb_len = 0;

    for (;;)
    {
      rtn = kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, &sta_len, m1, &m1_len);
      if (rtn < 0) { fputs("ERROR: kex_generate_pass1_msg_a\n", stderr); return -1; }
      print_uint("Pass1_Sta_Len = ", sta_len);
      print_hex("Pass1_Sta = ", sta, sta_len);
      print_uint("M1_Len = ", m1_len);
      print_hex("M1 = ", m1, m1_len);
      ma = m1; ma_len = m1_len;
      if (rtn == 1) { if (pass != 1) { fputs("ERROR: pass\n", stderr); return -1; } break; }

      rtn = kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, &stb_len, m2, &m2_len);
      if (rtn < 0) { fputs("ERROR: kex_generate_pass2_msg_b\n", stderr); return -1; }
      print_uint("Pass2_Stb_Len = ", stb_len);
      print_hex("Pass2_Stb = ", stb, stb_len);
      print_uint("M2_Len = ", m2_len);
      print_hex("M2 = ", m2, m2_len);
      mb = m2; mb_len = m2_len;
      if (rtn == 1) { if (pass != 2) { fputs("ERROR: pass\n", stderr); return -1; } break; }

      rtn = kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m2, m2_len, sta, &sta_len, m3, &m3_len);
      if (rtn < 0) { fputs("ERROR: kex_generate_pass3_msg_a\n", stderr); return -1; }
      print_uint("Pass3_Sta_Len = ", sta_len);
      print_hex("Pass3_Sta = ", sta, sta_len);
      print_uint("M3_Len = ", m3_len);
      print_hex("M3 = ", m3, m3_len);
      ma = m3; ma_len = m3_len;
      if (rtn == 1) { if (pass != 3) { fputs("ERROR: pass\n", stderr); return -1; } break; }

      rtn = kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m3, m3_len, stb, &stb_len, m4, &m4_len);
      if (rtn < 0) { fputs("ERROR: kex_generate_pass4_msg_b\n", stderr); return -1; }
      print_uint("Pass4_Stb_Len = ", stb_len);
      print_hex("Pass4_Stb = ", stb, stb_len);
      print_uint("M4_Len = ", m4_len);
      print_hex("M4 = ", m4, m4_len);
      mb = m4; mb_len = m4_len;
      if (rtn == 1) { if (pass != 4) { fputs("ERROR: pass\n", stderr); return -1; } break; }

      break;
    }

    rtn = kex_derive_ss_a(ska, ska_len, pkb, pkb_len, mb, mb_len, sta, sta_len, ssa, &ssa_len);
    if (rtn < 0) { fputs("ERROR: kex_derive_ss_a\n", stderr); return -1; }
    rtn = kex_derive_ss_b(skb, skb_len, pka, pka_len, ma, ma_len, stb, stb_len, ssb, &ssb_len);
    if (rtn < 0) { fputs("ERROR: kex_derive_ss_b\n", stderr); return -1; }

    if (ssa_len != ssb_len || memcmp(ssa, ssb, ssa_len) != 0)
    {
      fputs("ERROR: shared secret mismatch\n", stderr);
      return -1;
    }
    print_uint("SS_Len = ", ssa_len);
    print_hex("SS = ", ssa, ssa_len);

    putchar('\n');
  }

  return 0;
}
