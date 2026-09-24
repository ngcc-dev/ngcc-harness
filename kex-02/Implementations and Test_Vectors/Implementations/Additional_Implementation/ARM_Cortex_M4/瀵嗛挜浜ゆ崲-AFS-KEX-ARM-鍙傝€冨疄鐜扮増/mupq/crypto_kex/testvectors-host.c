// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "api.h"
#include "randombytes.h"
#include "drng.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define NTESTS 2
#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

/* The KEX path draws ALL its randomness from drng_algorithm (fixed seed below):
 * that is the sole source of determinism for these vectors. The surf
 * implementation only resolves the KEM core's randombytes() reference (its
 * non-derand keypair/enc, never called on the KEX path). Identical sequence to
 * the device testvectors build. */
typedef uint32_t uint32;
static uint32 seed_rb[32] = { 3,1,4,1,5,9,2,6,5,3,5,8,9,7,9,3,2,3,8,4,6,2,6,4,3,3,8,3,2,7,9,5 };
static uint32 in_rb[12];
static uint32 out_rb[8];
static int outleft_rb = 0;
#define ROTATE(x,b) (((x) << (b)) | ((x) >> (32 - (b))))
#define MUSH(i,b) x = t[i] += (((x ^ seed_rb[i]) + sum) ^ ROTATE(x,b));
static void surf(void)
{
  uint32 t[12]; uint32 x; uint32 sum = 0;
  int r, i, loop;
  for (i = 0; i < 12; ++i) t[i] = in_rb[i] ^ seed_rb[12 + i];
  for (i = 0; i < 8; ++i) out_rb[i] = seed_rb[24 + i];
  x = t[11];
  for (loop = 0; loop < 2; ++loop) {
    for (r = 0; r < 16; ++r) {
      sum += 0x9e3779b9;
      MUSH(0,5) MUSH(1,7) MUSH(2,9) MUSH(3,13)
      MUSH(4,5) MUSH(5,7) MUSH(6,9) MUSH(7,13)
      MUSH(8,5) MUSH(9,7) MUSH(10,9) MUSH(11,13)
    }
    for (i = 0; i < 8; ++i) out_rb[i] ^= t[i + 4];
  }
}
int randombytes(uint8_t *x, size_t xlen)
{
  while (xlen > 0) {
    if (!outleft_rb) {
      if (!++in_rb[0]) if (!++in_rb[1]) if (!++in_rb[2]) ++in_rb[3];
      surf();
      outleft_rb = 8;
    }
    *x = out_rb[--outleft_rb];
    ++x; --xlen;
  }
  return 0;
}

static void printbytes(const unsigned char *x, unsigned long long xlen)
{
  unsigned long long i, off = 0;
  char outs[2 * 64 + 1];
  for (i = 0; i < xlen; i++) {
    sprintf(outs + off, "%02x", x[i]);
    off += 2;
    if (off >= 2 * 64) { outs[off] = 0; puts(outs); off = 0; }
  }
  if (off) { outs[off] = 0; puts(outs); }
}

static unsigned char pka[CRYPTO_KEX_PUBLICKEYBYTES], pkb[CRYPTO_KEX_PUBLICKEYBYTES];
static unsigned char ska[CRYPTO_KEX_SECRETKEYBYTES], skb[CRYPTO_KEX_SECRETKEYBYTES];
static unsigned char pka0[CRYPTO_KEX_PUBLICKEYBYTES], pkb0[CRYPTO_KEX_PUBLICKEYBYTES];
static unsigned char sta[CRYPTO_KEX_STATEBYTES], stb[CRYPTO_KEX_STATEBYTES];
static unsigned char m1[CRYPTO_KEX_MSGBYTES], m2[CRYPTO_KEX_MSGBYTES];
static unsigned char m3[CRYPTO_KEX_MSGBYTES], m4[CRYPTO_KEX_MSGBYTES];
static unsigned char ssa[CRYPTO_KEX_SSBYTES], ssb[CRYPTO_KEX_SSBYTES];

int main(void)
{
  unsigned char seed[SEED_LEN_BYTES];
  /* Init to 0: some passes (e.g. C128 pass4) legitimately produce no message
   * and do not write *m_len, so the buffer length must default to 0. */
  unsigned long long pka_len = 0, ska_len = 0, pkb_len = 0, skb_len = 0;
  unsigned long long sta_len = 0, stb_len = 0, ssa_len = 0, ssb_len = 0;
  unsigned long long m1_len = 0, m2_len = 0, m3_len = 0, m4_len = 0;
  int i, j;

  for (i = 0; i < SEED_LEN_BYTES / 4; i++)
    memcpy(seed + 4 * i, "seed", 4);
  init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

  for (i = 0; i < NTESTS; i++) {
    kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len);
    kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len);
    memcpy(pka0, pka, (size_t)pka_len);
    memcpy(pkb0, pkb, (size_t)pkb_len);
    /* Print public AND secret keys: a secret-key (csk) divergence between ref
     * and m4fspeed must be caught by the vector itself, not only via the
     * functional ssa==ssb check (cf. the C512 csk bug). */
    printbytes(pka, pka_len);
    printbytes(ska, ska_len);
    printbytes(pkb, pkb_len);
    printbytes(skb, skb_len);

    kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, &sta_len, m1, &m1_len);
    kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, &stb_len, m2, &m2_len);
    kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m2, m2_len, sta, &sta_len, m3, &m3_len);
    memcpy(pkb, pkb0, (size_t)pkb_len);
    kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m3, m3_len, stb, &stb_len, m4, &m4_len);
    memcpy(pka, pka0, (size_t)pka_len);
    printbytes(m1, m1_len);
    printbytes(m2, m2_len);
    printbytes(m3, m3_len);
    printbytes(m4, m4_len);

    kex_derive_ss_a(ska, ska_len, pkb, pkb_len, m4, m4_len, sta, sta_len, ssa, &ssa_len);
    kex_derive_ss_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, stb_len, ssb, &ssb_len);
    printbytes(ssa, ssa_len);
    printbytes(ssb, ssb_len);

    for (j = 0; (unsigned long long)j < ssa_len; j++) {
      if (ssa[j] != ssb[j]) { puts("ERROR"); return -1; }
    }
  }
  return 0;
}
