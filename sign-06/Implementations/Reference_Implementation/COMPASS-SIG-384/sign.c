#include <stdint.h>
#include "params.h"
#include "sign.h"
#include "packing.h"
#include "polyvec.h"
#include "poly.h"
#include "drng.h"
#include "symmetric.h"

/* Global DRNG context — initialized by KAT program with seed.
   Used for deterministic key generation and randomized signing. */
extern DRNG_ctx drng_algorithm;

/*************************************************
* Name:        crypto_sign_keypair
*
* Description: Generates public and private key.
*              Uses the ICCS deterministic DRNG for
*              reproducible key generation.
*
* Arguments:   - uint8_t *pk: pointer to output public key
*              - uint8_t *sk: pointer to output private key
*
* Returns 0 (success)
**************************************************/
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
  uint8_t seed[SEEDBYTES];
  uint8_t seedbuf[2*SEEDBYTES + CRHBYTES];
  uint8_t tr[TRBYTES];
  const uint8_t *rho, *rhoprime, *key;
  polyvecl mat[K];
  polyvecl s1, s1hat;
  polyveck s2, t1, t0, t;

  /* Step 1: Generate seed via deterministic DRNG, then expand with XOF */
  get_random_number(&drng_algorithm, seed, SEEDBYTES * 8);
  shake256(seedbuf, 2*SEEDBYTES + CRHBYTES, seed, SEEDBYTES);

  rho = seedbuf;
  rhoprime = rho + SEEDBYTES;
  key = rhoprime + CRHBYTES;

  /* Step 2: Expand matrix A from seed rho */
  polyvec_matrix_expand(mat, rho);

  /* Step 3: Sample secret vectors s1 (length L) and s2 (length K) */
  polyvecl_uniform_eta_s(&s1, rhoprime, 0);
  polyveck_uniform_eta_e(&s2, rhoprime, L);

  /* Step 4: Compute t = A * s1 + s2 */
  s1hat = s1;
  polyvecl_ntt(&s1hat);
  polyvec_matrix_pointwise_montgomery(&t, mat, &s1hat);
  polyveck_reduce(&t);
  polyveck_invntt_tomont(&t);
  polyveck_add(&t, &t, &s2);
  polyveck_caddq(&t);

  /* Step 5: Decompose t into high bits t1 (public) and low bits t0 (secret) */
  polyveck_power2round(&t1, &t0, &t);

  /* Step 6: Pack public key and compute its hash */
  pack_pk(pk, rho, &t1);
  shake256(tr, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);

  /* Step 7: Pack secret key */
  pack_sk(sk, rho, tr, key, &s1, &s2, &t0);

  return 0;
}

/*************************************************
* Name:        crypto_sign_signature_internal
*
* Description: Computes signature (internal API with
*              explicit randomness input).
*
* Arguments:   - uint8_t *sig:   output signature buffer
*              - size_t *siglen: output signature length
*              - uint8_t *m:     message to sign
*              - size_t mlen:    message length
*              - uint8_t *pre:   prefix (ctx encoding)
*              - size_t prelen:  prefix length
*              - uint8_t rnd[]:  randomness (all-zero for deterministic)
*              - uint8_t *sk:    packed secret key
*
* Returns 0 (success)
**************************************************/
int crypto_sign_signature_internal(uint8_t *sig,
                                   size_t *siglen,
                                   const uint8_t *m,
                                   size_t mlen,
                                   const uint8_t *pre,
                                   size_t prelen,
                                   const uint8_t rnd[RNDBYTES],
                                   const uint8_t *sk)
{
  uint8_t seedbuf[2*SEEDBYTES + TRBYTES + 2*CRHBYTES];
  uint8_t *rho, *tr, *key, *mu, *rhoprime;
  uint16_t nonce = 0;
  polyvecl mat[K], s1, y, z;
  polyveck t0, s2, w1, w0, w;
  poly cp;
  keccak_state state;

  /* Unpack secret key into components */
  rho = seedbuf;
  tr = rho + SEEDBYTES;
  key = tr + TRBYTES;
  mu = key + SEEDBYTES;
  rhoprime = mu + CRHBYTES;
  unpack_sk(rho, tr, key, &s1, &s2, &t0, sk);

  /* Compute message representative mu = CRH(tr || pre || m) */
  shake256_init(&state);
  shake256_absorb(&state, tr, TRBYTES);
  shake256_absorb(&state, pre, prelen);
  shake256_absorb(&state, m, mlen);
  shake256_finalize(&state);
  shake256_squeeze(mu, CRHBYTES, &state);

  /* Compute randomness seed rhoprime = CRH(key || rnd || mu) */
  shake256_init(&state);
  shake256_absorb(&state, key, SEEDBYTES);
  shake256_absorb(&state, rnd, RNDBYTES);
  shake256_absorb(&state, mu, CRHBYTES);
  shake256_finalize(&state);
  shake256_squeeze(rhoprime, CRHBYTES, &state);

  /* Expand matrix A from rho */
  polyvec_matrix_expand(mat, rho);

  /* Convert secret vectors to NTT domain */
  polyvecl_ntt(&s1);
  polyveck_ntt(&s2);
  polyveck_ntt(&t0);

  /* Rejection sampling loop */
  while(1) {
    /* Sample masking vector y from XOF(rhoprime || nonce) */
    polyvecl_uniform_gamma1(&y, rhoprime, nonce++);
    z = y;

    /* Compute w = A * y */
    polyvecl_ntt(&z);
    polyvec_matrix_pointwise_montgomery(&w, mat, &z);
    polyveck_reduce(&w);
    polyveck_invntt_tomont(&w);
    polyveck_caddq(&w);

    /* Save original w for later hint consistency check */
    polyveck w_original = w;

    /* Decompose w into high bits w1 and low bits w0 */
    polyveck_decompose(&w1, &w0, &w);

    /* Compute challenge hash c = H(mu || w1) */
    polyveck_pack_w1(sig, &w1);
    shake256_init(&state);
    shake256_absorb(&state, mu, CRHBYTES);
    shake256_absorb(&state, sig, K*POLYW1_PACKEDBYTES);
    shake256_finalize(&state);
    shake256_squeeze(sig, CTILDEBYTES, &state);

    /* Generate challenge polynomial cp from hash */
    poly_challenge(&cp, sig);
    poly_ntt(&cp);

    /* Compute z = y + cp * s1 */
    polyvecl_pointwise_poly_montgomery(&z, &cp, &s1);
    polyvecl_invntt_tomont(&z);
    polyvecl_add(&z, &z, &y);
    polyvecl_reduce(&z);

    /* Check 1: z must have infinity norm < gamma1 - beta_z */
    if(polyvecl_chknorm(&z, GAMMA1 - BETA_Z))
      continue;

    /* Check 2: Verify hint consistency — ensure verifier can recover w1.
     * Compute w_approx = (w_original - cp*s2) + cp*t0 and check that
     * decompose(w_approx) yields the same w1. */
    polyveck cs2, ct0, u, u1, u0, w_approx, w1_prime, w0_prime;

    /* cs2 = cp * s2 (inverse NTT to normal domain) */
    polyveck_pointwise_poly_montgomery(&cs2, &cp, &s2);
    polyveck_invntt_tomont(&cs2);

    /* u = w_original - cp*s2 */
    polyveck_sub(&u, &w_original, &cs2);
    polyveck_reduce(&u);
    polyveck_caddq(&u);

    /* Check u0 infinity norm */
    polyveck_decompose(&u1, &u0, &u);
    if(polyveck_chknorm(&u0, GAMMA2 - BETA_W))
      continue;

    /* ct0 = cp * t0 */
    polyveck_pointwise_poly_montgomery(&ct0, &cp, &t0);
    polyveck_invntt_tomont(&ct0);

    /* w_approx = u + cp*t0 */
    polyveck_add(&w_approx, &u, &ct0);
    polyveck_reduce(&w_approx);
    polyveck_caddq(&w_approx);

    /* Decompose and verify w1' matches original w1 */
    polyveck_decompose(&w1_prime, &w0_prime, &w_approx);

    int w1_match = 1;
    for(int i = 0; i < K; ++i) {
      for(int j = 0; j < N; ++j) {
        if(w1_prime.vec[i].coeffs[j] != w1.vec[i].coeffs[j]) {
          w1_match = 0;
          break;
        }
      }
      if(!w1_match) break;
    }
    if(!w1_match)
      continue; /* w1 mismatch — reject and retry */

    /* Check 3: L2-norm bound for z and w0' */
    if(polyvec_check_L2_bound(&z, &w0_prime) == 1)
      continue;

    /* All checks passed — accept signature */
    break;
  }

  /* Pack signature: challenge hash || z vector */
  pack_sig(sig, sig, &z);
  *siglen = CRYPTO_BYTES;

  return 0;
}

/*************************************************
* Name:        crypto_sign_signature
*
* Description: Computes signature with context string.
*              Uses deterministic (all-zero) randomness
*              unless COMPASS_SIG_RANDOMIZED_SIGNING is defined.
*
* Arguments:   - uint8_t *sig:   output signature buffer
*              - size_t *siglen: output signature length
*              - uint8_t *m:     message to sign
*              - size_t mlen:    message length
*              - uint8_t *ctx:   context string (max 255 bytes)
*              - size_t ctxlen:  context string length
*              - uint8_t *sk:    packed secret key
*
* Returns 0 (success) or -1 (context string too long)
**************************************************/
int crypto_sign_signature(uint8_t *sig,
                          size_t *siglen,
                          const uint8_t *m,
                          size_t mlen,
                          const uint8_t *ctx,
                          size_t ctxlen,
                          const uint8_t *sk)
{
  size_t i;
  uint8_t pre[257];
  uint8_t rnd[RNDBYTES];

  if(ctxlen > 255)
    return -1;

  /* Encode context: pre = (0 || ctxlen || ctx) */
  pre[0] = 0;
  pre[1] = ctxlen;
  for(i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];

#ifdef COMPASS_SIG_RANDOMIZED_SIGNING
  get_random_number(&drng_algorithm, rnd, RNDBYTES * 8);
#else
  /* Deterministic mode: all-zero randomness */
  for(i = 0; i < RNDBYTES; i++)
    rnd[i] = 0;
#endif

  crypto_sign_signature_internal(sig, siglen, m, mlen, pre, 2 + ctxlen, rnd, sk);
  return 0;
}

/*************************************************
* Name:        crypto_sign
*
* Description: Compute signed message (signature || message).
*
* Arguments:   - uint8_t *sm:    output signed message buffer
*              - size_t *smlen:  output length of signed message
*              - uint8_t *m:     message to sign
*              - size_t mlen:    message length
*              - uint8_t *ctx:   context string
*              - size_t ctxlen:  context string length
*              - uint8_t *sk:    packed secret key
*
* Returns 0 (success) or -1
**************************************************/
int crypto_sign(uint8_t *sm,
                size_t *smlen,
                const uint8_t *m,
                size_t mlen,
                const uint8_t *ctx,
                size_t ctxlen,
                const uint8_t *sk)
{
  int ret;
  size_t i;

  /* Copy message to end of signed message buffer (backwards for in-place safety) */
  for(i = 0; i < mlen; ++i)
    sm[CRYPTO_BYTES + mlen - 1 - i] = m[mlen - 1 - i];
  ret = crypto_sign_signature(sm, smlen, sm + CRYPTO_BYTES, mlen, ctx, ctxlen, sk);
  *smlen += mlen;
  return ret;
}

/*************************************************
* Name:        crypto_sign_verify_internal
*
* Description: Verifies signature (internal API).
*
* Arguments:   - uint8_t *sig:   signature to verify
*              - size_t siglen:  signature length
*              - uint8_t *m:     message
*              - size_t mlen:    message length
*              - uint8_t *pre:   prefix (ctx encoding)
*              - size_t prelen:  prefix length
*              - uint8_t *pk:    packed public key
*
* Returns 0 (valid) or -1 (invalid)
**************************************************/
int crypto_sign_verify_internal(const uint8_t *sig,
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
  uint8_t c[CTILDEBYTES];
  uint8_t c2[CTILDEBYTES];
  poly cp;
  polyvecl mat[K], z, z_ntt;
  polyveck t1, w1, w0;
  keccak_state state;

  if(siglen != CRYPTO_BYTES)
    return -1;

  /* Unpack public key and signature */
  unpack_pk(rho, &t1, pk);
  if(unpack_sig(c, &z, sig))
    return -1;

  /* Check 1: z infinity norm < gamma1 - beta_z */
  if(polyvecl_chknorm(&z, GAMMA1 - BETA_Z))
    return -1;

  /* Compute message representative mu = CRH(CRH(pk) || pre || m) */
  shake256(mu, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  shake256_init(&state);
  shake256_absorb(&state, mu, TRBYTES);
  shake256_absorb(&state, pre, prelen);
  shake256_absorb(&state, m, mlen);
  shake256_finalize(&state);
  shake256_squeeze(mu, CRHBYTES, &state);

  /* Recover challenge polynomial cp from signature */
  poly_challenge(&cp, c);
  poly_ntt(&cp);

  /* Expand matrix A from rho */
  polyvec_matrix_expand(mat, rho);

  /* Compute A*z using NTT */
  z_ntt = z;
  polyvecl_ntt(&z_ntt);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &z_ntt);

  /* Compute cp * t1 * 2^d */
  polyveck_shiftl(&t1);
  polyveck_ntt(&t1);
  polyveck_pointwise_poly_montgomery(&t1, &cp, &t1);

  /* w_approx = A*z - cp*t1*2^d */
  polyveck_sub(&w1, &w1, &t1);
  polyveck_reduce(&w1);
  polyveck_invntt_tomont(&w1);
  polyveck_caddq(&w1);

  /* Decompose to recover high bits w1 and low bits w0 */
  polyveck_decompose(&w1, &w0, &w1);

  /* Compute challenge hash c' = H(mu || w1) and compare */
  polyveck_pack_w1(buf, &w1);
  shake256_init(&state);
  shake256_absorb(&state, mu, CRHBYTES);
  shake256_absorb(&state, buf, K*POLYW1_PACKEDBYTES);
  shake256_finalize(&state);
  shake256_squeeze(c2, CTILDEBYTES, &state);

  for(int i = 0; i < CTILDEBYTES; ++i) {
    if(c[i] != c2[i])
      return -1;
  }

  /* Check 2: L2-norm bound for z and w0 */
  if(polyvec_check_L2_bound(&z, &w0) == 1)
    return -1;

  return 0;
}

/*************************************************
* Name:        crypto_sign_verify
*
* Description: Verifies signature with context string.
*
* Arguments:   - uint8_t *sig:   signature
*              - size_t siglen:  signature length
*              - uint8_t *m:     message
*              - size_t mlen:    message length
*              - uint8_t *ctx:   context string
*              - size_t ctxlen:  context string length
*              - uint8_t *pk:    packed public key
*
* Returns 0 (valid) or -1 (invalid)
**************************************************/
int crypto_sign_verify(const uint8_t *sig,
                       size_t siglen,
                       const uint8_t *m,
                       size_t mlen,
                       const uint8_t *ctx,
                       size_t ctxlen,
                       const uint8_t *pk)
{
  size_t i;
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  pre[0] = 0;
  pre[1] = ctxlen;
  for(i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];

  return crypto_sign_verify_internal(sig, siglen, m, mlen, pre, 2 + ctxlen, pk);
}

/*************************************************
* Name:        crypto_sign_open
*
* Description: Verify signed message and recover message.
*
* Arguments:   - uint8_t *m:     output message buffer
*              - size_t *mlen:   output message length
*              - uint8_t *sm:    signed message
*              - size_t smlen:   signed message length
*              - uint8_t *ctx:   context string
*              - size_t ctxlen:  context string length
*              - uint8_t *pk:    packed public key
*
* Returns 0 (success) or -1 (invalid)
**************************************************/
int crypto_sign_open(uint8_t *m,
                     size_t *mlen,
                     const uint8_t *sm,
                     size_t smlen,
                     const uint8_t *ctx,
                     size_t ctxlen,
                     const uint8_t *pk)
{
  size_t i;

  if(smlen < CRYPTO_BYTES)
    goto badsig;

  *mlen = smlen - CRYPTO_BYTES;
  if(crypto_sign_verify(sm, CRYPTO_BYTES, sm + CRYPTO_BYTES, *mlen, ctx, ctxlen, pk))
    goto badsig;
  else {
    /* Valid signature — copy message to output */
    for(i = 0; i < *mlen; ++i)
      m[i] = sm[CRYPTO_BYTES + i];
    return 0;
  }

badsig:
  /* Signature verification failed — zeroize output */
  *mlen = 0;
  for(i = 0; i < smlen; ++i)
    m[i] = 0;
  return -1;
}
