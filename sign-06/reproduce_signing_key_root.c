/* sign-06-2 (extension): COMPASS-SIG-384/-512 signing nonce depends on a
 * 256-bit secret K only. Given (pk, m, sig) and a guess for the 32-byte
 * sk field K, an attacker recomputes rhoprime = SHAKE256(K || 0^32 || mu),
 * re-derives the mask y for nonce 0..63, checks c~ = H(mu || HighBits(A y))
 * against the signature, and on a match obtains z - y = c*s1.
 * The right K (from sk) must match; a K with one flipped bit must not.
 * Built and run by reproduce_signing_key_root.sh. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "params.h"
#include "sign.h"
#include "packing.h"
#include "polyvec.h"
#include "poly.h"
#include "symmetric.h"

#include "drng.h"

DRNG_ctx drng_algorithm;

static int try_key(const uint8_t *pk, const uint8_t *m, size_t mlen,
                   const uint8_t *sig, const uint8_t key[SEEDBYTES],
                   polyvecl *cs1_out, int *nonce_out)
{
  uint8_t tr[TRBYTES], mu[CRHBYTES], rhoprime[CRHBYTES], rnd[RNDBYTES] = {0};
  uint8_t rho[SEEDBYTES], c_sig[CTILDEBYTES], ctil[CTILDEBYTES];
  uint8_t w1buf[K * POLYW1_PACKEDBYTES];
  uint8_t pre[2] = {0, 0};
  polyveck t1, w, w1, w0;
  polyvecl mat[K], z, y, yh;
  keccak_state st;

  unpack_pk(rho, &t1, pk);
  unpack_sig(c_sig, &z, sig);
  shake256(tr, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  shake256_init(&st); shake256_absorb(&st, tr, TRBYTES);
  shake256_absorb(&st, pre, 2); shake256_absorb(&st, m, mlen);
  shake256_finalize(&st); shake256_squeeze(mu, CRHBYTES, &st);
  shake256_init(&st); shake256_absorb(&st, key, SEEDBYTES);
  shake256_absorb(&st, rnd, RNDBYTES); shake256_absorb(&st, mu, CRHBYTES);
  shake256_finalize(&st); shake256_squeeze(rhoprime, CRHBYTES, &st);
  polyvec_matrix_expand(mat, rho);
  for (int nonce = 0; nonce < 64; nonce++) {
    polyvecl_uniform_gamma1(&y, rhoprime, (uint16_t)nonce);
    yh = y; polyvecl_ntt(&yh);
    polyvec_matrix_pointwise_montgomery(&w, mat, &yh);
    polyveck_reduce(&w); polyveck_invntt_tomont(&w); polyveck_caddq(&w);
    polyveck_decompose(&w1, &w0, &w);
    polyveck_pack_w1(w1buf, &w1);
    shake256_init(&st); shake256_absorb(&st, mu, CRHBYTES);
    shake256_absorb(&st, w1buf, sizeof w1buf); shake256_finalize(&st);
    shake256_squeeze(ctil, CTILDEBYTES, &st);
    if (memcmp(ctil, c_sig, CTILDEBYTES) == 0) {
      for (int i = 0; i < L; i++) for (int j = 0; j < N; j++)
        cs1_out->vec[i].coeffs[j] = z.vec[i].coeffs[j] - y.vec[i].coeffs[j]; /* = c*s1 */
      *nonce_out = nonce;
      return 1;
    }
  }
  return 0;
}

int main(void)
{
  static uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], sig[CRYPTO_BYTES];
  uint8_t seed[48], msg[33] = "grpE COMPASS-SIG signing witness";
  uint8_t rho[SEEDBYTES], tr[TRBYTES], key[SEEDBYTES], ctil[CTILDEBYTES];
  polyvecl s1, cs1, cs1_true, zz; polyveck s2, t0; poly cp;
  size_t siglen; int nonce = -1, ok = 1;

  for (int i = 0; i < 48; i++) seed[i] = (uint8_t)i;
  init_random_number(&drng_algorithm, seed, 48);
  crypto_sign_keypair(pk, sk);
  crypto_sign_signature(sig, &siglen, msg, sizeof msg, NULL, 0, sk);
  printf("mode %d: SEEDBYTES=%d (K field), RNDBYTES=%d, CRHBYTES=%d, siglen=%zu\n",
         COMPASS_SIG_MODE, SEEDBYTES, RNDBYTES, CRHBYTES, siglen);

  /* reseed DRBG differently and sign again: deterministic? */
  { static uint8_t sig2[CRYPTO_BYTES]; size_t l2; uint8_t s2b[48];
    for (int i = 0; i < 48; i++) s2b[i] = (uint8_t)(0xA5 ^ i);
    init_random_number(&drng_algorithm, s2b, 48);
    crypto_sign_signature(sig2, &l2, msg, sizeof msg, NULL, 0, sk);
    printf("re-sign after DRBG reseed identical: %s\n",
           (l2 == siglen && memcmp(sig, sig2, siglen) == 0) ? "yes" : "no"); }

  unpack_sk(rho, tr, key, &s1, &s2, &t0, sk);

  /* attacker step with the correct 256-bit K */
  if (try_key(pk, msg, sizeof msg, sig, key, &cs1, &nonce)) {
    unpack_sig(ctil, &zz, sig);
    poly_challenge(&cp, ctil); poly_ntt(&cp);
    cs1_true = s1; polyvecl_ntt(&cs1_true);
    polyvecl_pointwise_poly_montgomery(&cs1_true, &cp, &cs1_true);
    polyvecl_invntt_tomont(&cs1_true); polyvecl_reduce(&cs1_true);
    for (int i = 0; i < L; i++) for (int j = 0; j < N; j++) {
      int32_t a = cs1.vec[i].coeffs[j] % Q, b = cs1_true.vec[i].coeffs[j] % Q;
      if (((a - b) % Q + Q) % Q != 0) ok = 0;
    }
    printf("correct K: challenge matched at nonce %d; z - y == c*s1: %s\n",
           nonce, ok ? "yes" : "no");
  } else printf("correct K: NO MATCH\n");

  key[0] ^= 1;
  printf("K with one bit flipped: %s\n",
         try_key(pk, msg, sizeof msg, sig, key, &cs1, &nonce) ? "MATCH (unexpected)" : "no match (control ok)");
  return 0;
}
