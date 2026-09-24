// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "api.h"
#include "randombytes.h"
#include "hal.h"
#include "drng.h"

#include <string.h>

/* The mupq SimpleTest harness expects exactly 30 "OK" lines and no "ERROR".
 * We spend that budget on three sub-tests, mirroring the upstream KEM test:
 *   positive round-trips + tampered-message negatives + corrupted-key negative.
 * N_POS + N_MSG + N_SK must equal 30. */
#define N_POS 12   /* positive round-trips        */
#define N_MSG 12   /* tampered-message negatives  */
#define N_SK   6   /* corrupted-key negatives     */

#define SEED_LEN_BYTES 64
#define GUARD 8

/* The AFS-KEX protocol layer draws its randomness from this DRNG context
 * (declared extern in KEX_AlgorithmInstance.c). The harness owns and seeds it. */
DRNG_ctx drng_algorithm;

static const unsigned char canary[GUARD] =
  { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };

/* Each buffer carries GUARD canary bytes on both ends; PTR(name) points at the
 * usable region. Kept static (off the stack: C512 keys are several KiB). */
#define DEF(name, sz)  static unsigned char name##_b[(sz) + 2 * GUARD]; \
                       enum { name##_SZ = (sz) };
#define PTR(name)      (name##_b + GUARD)

DEF(pka, CRYPTO_KEX_PUBLICKEYBYTES)  DEF(pkb, CRYPTO_KEX_PUBLICKEYBYTES)
DEF(ska, CRYPTO_KEX_SECRETKEYBYTES)  DEF(skb, CRYPTO_KEX_SECRETKEYBYTES)
DEF(pka0, CRYPTO_KEX_PUBLICKEYBYTES) DEF(pkb0, CRYPTO_KEX_PUBLICKEYBYTES)
DEF(sta, CRYPTO_KEX_STATEBYTES)      DEF(stb, CRYPTO_KEX_STATEBYTES)
DEF(m1, CRYPTO_KEX_MSGBYTES)         DEF(m2, CRYPTO_KEX_MSGBYTES)
DEF(m3, CRYPTO_KEX_MSGBYTES)         DEF(m4, CRYPTO_KEX_MSGBYTES)
DEF(ssa, CRYPTO_KEX_SSBYTES)         DEF(ssb, CRYPTO_KEX_SSBYTES)

static unsigned char *const guarded[] = {
  pka_b, pkb_b, ska_b, skb_b, pka0_b, pkb0_b, sta_b, stb_b,
  m1_b, m2_b, m3_b, m4_b, ssa_b, ssb_b,
};
static const unsigned guarded_sz[] = {
  pka_SZ, pkb_SZ, ska_SZ, skb_SZ, pka0_SZ, pkb0_SZ, sta_SZ, stb_SZ,
  m1_SZ, m2_SZ, m3_SZ, m4_SZ, ssa_SZ, ssb_SZ,
};
#define NGUARD (sizeof(guarded) / sizeof(guarded[0]))

static void set_canaries(void)
{
  for (unsigned k = 0; k < NGUARD; k++) {
    memcpy(guarded[k], canary, GUARD);
    memcpy(guarded[k] + GUARD + guarded_sz[k], canary, GUARD);
  }
}

static int canaries_ok(void)
{
  for (unsigned k = 0; k < NGUARD; k++)
    if (memcmp(guarded[k], canary, GUARD) ||
        memcmp(guarded[k] + GUARD + guarded_sz[k], canary, GUARD))
      return 0;
  return 1;
}

static void reseed_drng(void)
{
  unsigned char seed[SEED_LEN_BYTES];
  /* fresh random seed each run so the functional test exercises many inputs */
  randombytes(seed, SEED_LEN_BYTES);
  init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);
}

static void flip_byte(unsigned char *p, unsigned long long len)
{
  unsigned char r[2];
  unsigned long long pos;
  if (len == 0) return;
  randombytes(r, 2);
  pos = ((unsigned long long)r[0] | ((unsigned long long)r[1] << 8)) % len;
  p[pos] ^= 0x55; /* guarantees the byte actually changes */
}

enum { T_NONE = 0, T_M1, T_M2, T_M3, T_SKB };

/* Run one full 4-pass protocol, optionally injecting a fault at `tamper`.
 * Returns: 1 = completed and ssa==ssb; 0 = completed but ssa!=ssb;
 *          <0 = a pass aborted (authentication / verification failure).
 * Note: kex_generate_pass4_msg_b returns 1 on success, so faults are detected
 * with `< 0`, never `!= 0`. */
static int protocol_run(int tamper)
{
  unsigned long long pka_len = 0, ska_len = 0, pkb_len = 0, skb_len = 0;
  unsigned long long sta_len = 0, stb_len = 0, ssa_len = 0, ssb_len = 0;
  unsigned long long m1_len = 0, m2_len = 0, m3_len = 0, m4_len = 0;
  int rtn;

  reseed_drng();
  if ((rtn = kex_init_a(PTR(pka), &pka_len, PTR(ska), &ska_len, PTR(sta), &sta_len)) < 0) return rtn;
  if ((rtn = kex_init_b(PTR(pkb), &pkb_len, PTR(skb), &skb_len, PTR(stb), &stb_len)) < 0) return rtn;
  memcpy(PTR(pka0), PTR(pka), (size_t)pka_len);
  memcpy(PTR(pkb0), PTR(pkb), (size_t)pkb_len);

  if ((rtn = kex_generate_pass1_msg_a(PTR(ska), ska_len, PTR(pkb), pkb_len, PTR(sta), &sta_len, PTR(m1), &m1_len)) < 0) return rtn;
  if (tamper == T_M1)  flip_byte(PTR(m1), m1_len);
  if (tamper == T_SKB) randombytes(PTR(skb), (size_t)skb_len);

  if ((rtn = kex_generate_pass2_msg_b(PTR(skb), skb_len, PTR(pka), pka_len, PTR(m1), m1_len, PTR(stb), &stb_len, PTR(m2), &m2_len)) < 0) return rtn;
  if (tamper == T_M2)  flip_byte(PTR(m2), m2_len);

  if ((rtn = kex_generate_pass3_msg_a(PTR(ska), ska_len, PTR(pkb), pkb_len, PTR(m2), m2_len, PTR(sta), &sta_len, PTR(m3), &m3_len)) < 0) return rtn;
  /* pass3 may clobber the pkb buffer; restore before later use */
  memcpy(PTR(pkb), PTR(pkb0), (size_t)pkb_len);
  if (tamper == T_M3)  flip_byte(PTR(m3), m3_len);

  if ((rtn = kex_generate_pass4_msg_b(PTR(skb), skb_len, PTR(pka), pka_len, PTR(m3), m3_len, PTR(stb), &stb_len, PTR(m4), &m4_len)) < 0) return rtn;
  /* pass4 may clobber the pka buffer; restore before later use */
  memcpy(PTR(pka), PTR(pka0), (size_t)pka_len);

  if ((rtn = kex_derive_ss_a(PTR(ska), ska_len, PTR(pkb), pkb_len, PTR(m4), m4_len, PTR(sta), sta_len, PTR(ssa), &ssa_len)) < 0) return rtn;
  if ((rtn = kex_derive_ss_b(PTR(skb), skb_len, PTR(pka), pka_len, PTR(m1), m1_len, PTR(stb), stb_len, PTR(ssb), &ssb_len)) < 0) return rtn;

  return (ssa_len == ssb_len && memcmp(PTR(ssa), PTR(ssb), (size_t)ssa_len) == 0) ? 1 : 0;
}

static int ss_nonzero(void)
{
  for (unsigned i = 0; i < CRYPTO_KEX_SSBYTES; i++)
    if (PTR(ssa)[i] != 0) return 1;
  return 0;
}

int main(void)
{
  int i;
  hal_setup(CLOCK_FAST);

  hal_send_str("==========================");

  /* 1) Positive: parties agree, key is non-degenerate, no buffer overrun. */
  for (i = 0; i < N_POS; i++) {
    set_canaries();
    int r = protocol_run(T_NONE);
    if (r == 1 && ss_nonzero() && canaries_ok())  hal_send_str("OK KEYS\n");
    else if (r == 1 && !ss_nonzero())             hal_send_str("ERROR degenerate ss\n");
    else if (r == 1 && !canaries_ok())            hal_send_str("ERROR canary\n");
    else                                          hal_send_str("ERROR KEYS\n");
    hal_send_str("+");
  }

  /* 2) Tampered message: flip a byte in m1/m2/m3 in turn. A mutually
   *    authenticated KEX must NOT let both parties accept the same key:
   *    either a pass aborts (<0) or the derived secrets differ (0). */
  for (i = 0; i < N_MSG; i++) {
    int t = (i % 3 == 0) ? T_M1 : (i % 3 == 1) ? T_M2 : T_M3;
    int r = protocol_run(t);
    if (r <= 0)  hal_send_str("OK tampered msg\n");
    else         hal_send_str("ERROR forged msg accepted\n");
    hal_send_str("+");
  }

  /* 3) Corrupted key: randomize skb before B decapsulates m1. */
  for (i = 0; i < N_SK; i++) {
    int r = protocol_run(T_SKB);
    if (r <= 0)  hal_send_str("OK invalid skb\n");
    else         hal_send_str("ERROR invalid skb accepted\n");
    hal_send_str("+");
  }

  hal_send_str("#");
  return 0;
}
