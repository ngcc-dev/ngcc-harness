/* sign-06-6: COMPASS-SIG's shake256 wrapper compresses a full 512-byte absorb
 * buffer to a 256-bit pseudoXOF value (symmetric-shake.c:151-167), so for a
 * message longer than 446 bytes mu = H(tr || pre || m) depends on m only
 * through D = pseudoXOF_256(tr || pre || m[0..446)) and the suffix m[446..].
 *
 *   full   : (unmodified sources) the library mu equals
 *            pseudoXOF(D || m[446..]) recomputed by hand; then, given two
 *            prefix counters, a signature on m1 must NOT verify on m2.
 *   reduced: (-DREDUCED, symmetric-shake.c compiled with
 *            -DpseudoXOF=wrap_pseudoXOF) only the absorb-compression output is
 *            truncated to RBITS bits; a birthday search finds two 446-byte
 *            prefixes with equal D, and a signature on m1 verifies on m2.
 * Output lines are parsed by reproduce_absorb_compression.sh. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "params.h"
#include "sign.h"
#include "symmetric.h"
#include "auxfunc.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
#define PFX 446 /* 512 - TRBYTES(64) - prelen(2) */
#define SFX 40

#ifdef REDUCED
#ifndef RBITS
#define RBITS 24
#endif
int wrap_pseudoXOF(unsigned long long ob, const unsigned char *m,
                   unsigned long long ib, unsigned char *o)
{
  int r = pseudoXOF(ob, m, ib, o);
  if (ob == 256 && ib == 512 * 8) /* the absorb-buffer compression only */
    for (int i = RBITS / 8; i < 32; i++) o[i] = 0;
  return r;
}
#define COMPRESS wrap_pseudoXOF
#else
#define COMPRESS pseudoXOF
#endif

static void lib_mu(uint8_t mu[CRHBYTES], const uint8_t *pk, const uint8_t *m, size_t mlen)
{
  uint8_t tr[TRBYTES], pre[2] = {0, 0};
  keccak_state st;
  shake256(tr, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  shake256_init(&st);
  shake256_absorb(&st, tr, TRBYTES);
  shake256_absorb(&st, pre, 2);
  shake256_absorb(&st, m, mlen);
  shake256_finalize(&st);
  shake256_squeeze(mu, CRHBYTES, &st);
}

static void prefix_block(uint8_t blk[512], const uint8_t tr[TRBYTES], uint32_t c)
{
  memcpy(blk, tr, TRBYTES);
  blk[64] = 0;
  blk[65] = 0;
  memset(blk + 66, 'A', PFX);
  memcpy(blk + 66, &c, 4);
}

static void message(uint8_t m[PFX + SFX], uint32_t c)
{
  static const char sfx[] = "common suffix appended to both messages!";
  memset(m, 'A', PFX);
  memcpy(m, &c, 4);
  memcpy(m + PFX, sfx, SFX);
}

static void transfer(const uint8_t *pk, const uint8_t *sk, uint32_t c1, uint32_t c2)
{
  static uint8_t sig[CRYPTO_BYTES];
  uint8_t m1[PFX + SFX], m2[PFX + SFX];
  size_t siglen;
  message(m1, c1);
  message(m2, c2);
  crypto_sign_signature(sig, &siglen, m1, sizeof m1, NULL, 0, sk);
  printf("messages differ: %s\n", memcmp(m1, m2, sizeof m1) ? "yes" : "no");
  printf("verify(sig, m1) = %d\n", crypto_sign_verify(sig, siglen, m1, sizeof m1, NULL, 0, pk));
  printf("verify(sig, m2) = %d\n", crypto_sign_verify(sig, siglen, m2, sizeof m2, NULL, 0, pk));
  m2[PFX + 3] ^= 1;
  printf("verify(sig, m2 with a suffix bit flipped) = %d\n",
         crypto_sign_verify(sig, siglen, m2, sizeof m2, NULL, 0, pk));
}

int main(int argc, char **argv)
{
  static uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
  uint8_t seed[48], tr[TRBYTES], m[PFX + SFX], blk[512], dg[32], mu1[CRHBYTES], mu2[CRHBYTES];
  uint8_t in2[32 + SFX];

  for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(3 * i + 1);
  init_random_number(&drng_algorithm, seed, 48);
  crypto_sign_keypair(pk, sk);
  shake256(tr, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);

  /* mu of a 486-byte message from the library path vs the 256-bit model */
  for (int i = 0; i < PFX + SFX; i++) m[i] = (uint8_t)(i * 7 + 11);
  lib_mu(mu1, pk, m, sizeof m);
  memcpy(blk, tr, TRBYTES);
  blk[64] = 0;
  blk[65] = 0;
  memcpy(blk + 66, m, PFX);
  COMPRESS(256, blk, 512 * 8, dg);
  memcpy(in2, dg, 32);
  memcpy(in2 + 32, m + PFX, SFX);
  pseudoXOF(CRHBYTES * 8, in2, (32 + SFX) * 8, mu2);
  printf("mode %d: mu(library) == pseudoXOF(pseudoXOF_256(tr||pre||m[0..446)) || m[446..]): %s\n",
         COMPASS_SIG_MODE, memcmp(mu1, mu2, CRHBYTES) == 0 ? "yes" : "no");
  m[PFX - 1] ^= 1; /* control: a change inside the compressed prefix changes mu */
  lib_mu(mu2, pk, m, sizeof m);
  printf("control: prefix bit flip changes mu: %s\n", memcmp(mu1, mu2, CRHBYTES) ? "yes" : "no");

#ifdef REDUCED
  {
    enum { TB = 1 << 24 };
    uint32_t *tab = calloc(TB, sizeof(uint32_t));
    uint32_t found = 0, c1 = 0, c2 = 0;
    if (!tab) return 2;
    for (uint32_t c = 1; c < (1u << 22) && !found; c++) {
      prefix_block(blk, tr, c);
      COMPRESS(256, blk, 512 * 8, dg);
      uint32_t key = (uint32_t)dg[0] | ((uint32_t)dg[1] << 8) | ((uint32_t)dg[2] << 16);
      if (tab[key]) { found = 1; c1 = tab[key]; c2 = c; }
      else tab[key] = c;
    }
    free(tab);
    printf("reduced %d-bit compression: collision %s between prefix counters %u %u\n",
           RBITS, found ? "found" : "NOT-found", c1, c2);
    if (!found) return 1;
    transfer(pk, sk, c1, c2);
  }
#else
  if (argc == 3) {
    uint32_t c1 = (uint32_t)strtoul(argv[1], NULL, 10), c2 = (uint32_t)strtoul(argv[2], NULL, 10);
    printf("full-width control with prefix counters %u %u\n", c1, c2);
    transfer(pk, sk, c1, c2);
  }
#endif
  return 0;
}
