#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "auxfunc.h"
#include "params.h"
#include "sign.h"
#include "packing.h"
#include "polyvec.h"
#include "poly.h"
#include "drng.h"
#include "mult.h"

extern DRNG_ctx drng_algorithm;

int randombytes(unsigned char *buf, unsigned long long len)
{
    return get_random_number(&drng_algorithm, buf, len * 8ULL);
}


/* Byte-oriented wrapper around the ICCS auxiliary XOF. */
static void crh(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
  pseudohash((unsigned long long)outlen * 8ULL, in, (unsigned long long)inlen * 8ULL, out);
}

/* Generate a public/secret key pair for the signature scheme. */
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
  uint8_t rho[SEEDBYTES];
  uint8_t rhoprime[CRHBYTES];
  uint8_t tr[CRHBYTES];
  polyvecl mat[K];
  polyvecl s1, s1hat;
  polyveck s2, t1, t0;

  randombytes(rho, SEEDBYTES);
  randombytes(rhoprime, CRHBYTES);

  polyvec_matrix_expand(mat, rho);

  polyvecl_uniform_eta(&s1, rhoprime, 0);
  polyveck_uniform_eta(&s2, rhoprime, L);

  s1hat = s1;
  polyvecl_ntt(&s1hat);
  polyvec_matrix_pointwise_montgomery(&t1, mat, &s1hat);
  polyveck_reduce(&t1);
  polyveck_invntt_tomont(&t1);

  polyveck_add(&t1, &t1, &s2);

  polyveck_caddq(&t1);
  polyveck_power2round(&t1, &t0, &t1);
  pack_pk(pk, rho, &t1);

  crh(tr, CRHBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  pack_sk(sk, rho, tr, &t0, &s1, &s2);

  return 0;
}

/*
 * Internal signing routine — smallpoly + earlycheck path.
 *
 * Replaces the three NTT-based small-poly multiplications:
 *   c*s2  →  evaluate_cs1_cs2_early_check_ops128 (fused with c*s1)
 *   c*s1  →  (fused above)
 *   c*t0  →  evaluate_ct0_ops128
 *
 * The packing tables (s1_table, s2_table, t0_table) are built once from
 * the time-domain secret key material before the rej loop, since s1/s2/t0
 * do not change across rejection iterations.
 *
 * poly_challenge produces time-domain c (coeffs in {-1,0,+1}); the smallpoly
 * functions consume it directly without requiring poly_ntt(&cp).
 */
static int crypto_sign_signature_internal(uint8_t *sig,
                                          size_t *siglen,
                                          const uint8_t *m,
                                          size_t mlen,
                                          const uint8_t *pre,
                                          size_t prelen,
                                          const uint8_t *sk)
{
  unsigned int n;
  uint8_t rho[SEEDBYTES];
  uint8_t tr[CRHBYTES];
  uint8_t mu[CRHBYTES];
  uint8_t rhoprime[CRHBYTES];
  uint16_t nonce = 0;
  uint8_t ctmp[CRHBYTES + K*POLYW1_PACKEDBYTES];
  polyvecl mat[K], s1, y, z;
  polyveck t0, s2, w1, w0, w0prime, h;
  poly cp;

  /* Tables for the smallpoly path — declared here, prepared before rej loop */
  uint32_t s1_table[2*N];
  uint32_t s2_table[2*N];
  uint64_t t0_table[2*N];

  unpack_sk(rho, tr, &t0, &s1, &s2, sk);

  unsigned long long inlen = CRHBYTES + prelen + mlen;
  uint8_t *tmp = (uint8_t *)malloc(inlen);
  if(tmp == NULL) return -1;
  unsigned long long off = 0;
  memcpy(tmp + off, tr, CRHBYTES); off += CRHBYTES;
  memcpy(tmp + off, pre, prelen); off += prelen;
  memcpy(tmp + off, m, mlen);
  crh(mu, CRHBYTES, tmp, inlen);
  free(tmp);

  randombytes(rhoprime, CRHBYTES);

  polyvec_matrix_expand(mat, rho);

  /* Build packing tables once from time-domain s1/s2/t0 (before rej loop).
   * These remain constant across all rejection iterations. */
  prepare_s1_table_ops128(s1_table, &s1);
  prepare_s2_table_ops128(s2_table, &s2);
  prepare_t0_table_ops128(t0_table, &t0);

rej:
  polyvecl_uniform_gamma1(&y, rhoprime, nonce++);

  /* Compute w = A*y via NTT (z is a scratch copy of y here) */
  z = y;
  polyvecl_ntt(&z);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &z);
  polyveck_reduce(&w1);
  polyveck_invntt_tomont(&w1);

  polyveck_caddq(&w1);
  polyveck_decompose(&w1, &w0, &w1);
  polyveck_pack_w1(sig, &w1);

  /* Compute challenge seed c_tilde and challenge polynomial cp */
  memcpy(ctmp, mu, CRHBYTES);
  memcpy(ctmp + CRHBYTES, sig, K*POLYW1_PACKEDBYTES);
  sm3hash((unsigned long long)CTILDEBYTES * 8ULL, ctmp,
          (unsigned long long)sizeof(ctmp) * 8ULL, sig);

  poly_challenge(&cp, sig);
  /* No poly_ntt(&cp): smallpoly functions consume time-domain cp directly */

  /* Fused cs1+cs2: recover z=y+cs1 and w0prime=w0-cs2, early-check both */
  if(evaluate_cs1_cs2_early_check_ops128(&z, &w0prime, &cp,
                                          s1_table, s2_table,
                                          &y, &w0,
                                          GAMMA1 - BETA, GAMMA2 - BETA))
    goto rej;

  /* ct0 smallpoly: compute c*t0 into h, then norm-check */
  evaluate_ct0_ops128(&h, &cp, t0_table);
  if(polyveck_chknorm(&h, GAMMA2))
    goto rej;

  /* w0'' = w0' + ct0 */
  polyveck_add(&w0prime, &w0prime, &h);
  n = polyveck_make_hint(&h, &w0prime, &w1);
  if(n > OMEGA)
    goto rej;

  pack_sig(sig, sig, &z, &h);
  *siglen = CRYPTO_BYTES;
  return 0;
}

/* External signing API that prepares the context prefix and deterministic randomness. */
int crypto_sign_signature(uint8_t *sig,
                          size_t *siglen,
                          const uint8_t *m,
                          size_t mlen,
                          const uint8_t *ctx,
                          size_t ctxlen,
                          const uint8_t *sk)
{
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  pre[0] = 0;
  pre[1] = (uint8_t)ctxlen;
  for(size_t i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];

  crypto_sign_signature_internal(sig, siglen, m, mlen, pre, 2 + ctxlen, sk);
  return 0;
}

/* Internal verification routine that mirrors the signing transcript reconstruction. */
static int crypto_sign_verify_internal(uint8_t *sig,
                                       size_t siglen,
                                       const uint8_t *m,
                                       size_t mlen,
                                       const uint8_t *pre,
                                       size_t prelen,
                                       const uint8_t *pk)
{
  uint8_t buf[K*POLYW1_PACKEDBYTES];
  uint8_t rho[SEEDBYTES];
  uint8_t mu[CRHBYTES];
  uint8_t tr[CRHBYTES];
  uint8_t c[CTILDEBYTES];
  uint8_t c2[CTILDEBYTES];
  uint8_t ctmp[CRHBYTES + K*POLYW1_PACKEDBYTES];
  poly cp;
  polyvecl mat[K], z;
  polyveck t1, w1, h;

  if(siglen != CRYPTO_BYTES)
    return -1;

  unpack_pk(rho, &t1, pk);
  if(unpack_sig(c, &z, &h, sig))
    return -1;
  if(polyvecl_chknorm(&z, GAMMA1 - BETA))
    return -1;


  crh(tr, CRHBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  unsigned long long inlen = CRHBYTES + prelen + mlen;
  uint8_t *tmp = (uint8_t *)malloc(inlen);
  if(tmp == NULL) return -1;
  unsigned long long off = 0;
  memcpy(tmp + off, tr, CRHBYTES); off += CRHBYTES;
  memcpy(tmp + off, pre, prelen); off += prelen;
  memcpy(tmp + off, m, mlen);
  crh(mu, CRHBYTES, tmp, inlen);
  free(tmp);


  poly_challenge(&cp, c);
  polyvec_matrix_expand(mat, rho);

  polyvecl_ntt(&z);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &z);

  poly_ntt(&cp);
  polyveck_shiftl(&t1);
  polyveck_ntt(&t1);
  polyveck_pointwise_poly_montgomery(&t1, &cp, &t1);

  polyveck_sub(&w1, &w1, &t1);
  polyveck_reduce(&w1);
  polyveck_invntt_tomont(&w1);

  polyveck_caddq(&w1);
  polyveck_use_hint(&w1, &w1, &h);
  polyveck_pack_w1(buf, &w1);


  memcpy(ctmp, mu, CRHBYTES);
  memcpy(ctmp + CRHBYTES, buf, K*POLYW1_PACKEDBYTES);
  sm3hash((unsigned long long)CTILDEBYTES * 8ULL, ctmp, (unsigned long long)sizeof(ctmp) * 8ULL, c2);


  for(unsigned i = 0; i < CTILDEBYTES; ++i)
    if(c[i] != c2[i])
      return -1;

  return 0;
}

/* External verification API that prepares the context prefix. */
int crypto_sign_verify(uint8_t *sig,
                       size_t siglen,
                       const uint8_t *m,
                       size_t mlen,
                       const uint8_t *ctx,
                       size_t ctxlen,
                       const uint8_t *pk)
{
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  pre[0] = 0;
  pre[1] = (uint8_t)ctxlen;
  for(size_t i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];

  return crypto_sign_verify_internal(sig, siglen, m, mlen, pre, 2 + ctxlen, pk);
}
