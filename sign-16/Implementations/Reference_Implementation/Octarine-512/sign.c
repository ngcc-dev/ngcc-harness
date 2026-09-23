/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

*/

#include <stdlib.h>
#include <string.h>
#include "sign.h"
#define RRLWR_HASH_DOMAIN_SIGN
#include "hash_domain.h"

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

#ifdef MEASURE_HEURISTICS
  extern int32_t max_hint_weight;
  extern int32_t num_rejects_ct0;
  extern int32_t num_rejects_w;
  extern int32_t num_rejects_y;
  extern int32_t num_rejects_hint;
  extern int32_t num_rounds;
#endif

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long sig_get_pk_len_bytes() 
{
  return RRLWR_SIGN_PK_LEN;
}

unsigned long long sig_get_sk_len_bytes() 
{
  return RRLWR_SIGN_SK_LEN;
}

unsigned long long sig_get_sn_len_bytes()
{
  return RRLWR_SIGN_SIG_LEN;
}

int sig_keygen(
  unsigned char *pk, unsigned long long *pk_len_bytes,
  unsigned char *sk, unsigned long long *sk_len_bytes)
{
  unsigned char xi[RRLWR_SIGN_XI_LEN];
  unsigned char rho_rhoprime_K[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_RHOPRIME_LEN + RRLWR_SIGN_K_LEN]; // (rho, rho', K)
  unsigned char *rho = rho_rhoprime_K;
  unsigned char *rhoprime = rho_rhoprime_K + RRLWR_SIGN_RHO_LEN;
  unsigned char *K = rhoprime + RRLWR_SIGN_RHOPRIME_LEN;
  unsigned char tr[RRLWR_SIGN_TR_LEN];
  ring_element a, s1, t;
  ring_element *s2 = &s1;
  ring_element *b = &a;

  // Generate (rho, rho', K)
  GENERATE_RANDOM_BYTES(xi, RRLWR_SIGN_XI_LEN, &drng_algorithm);
  RRLWR_SIGN_HASH_KDF(rho_rhoprime_K, xi);

  // Generate a with coefficients in [-q/2+1, q/2]
  ring_uniform_public_x4(&a, RRLWR_SIGN_LOGQ, rho, RRLWR_SIGN_RHO_LEN);

  // Generate s with coefficients in [-2, 1]
  ring_uniform_secret(&s1, RRLWR_SIGN_LOG_ETA+1, rhoprime, RRLWR_SIGN_RHOPRIME_LEN);

  // Compute A*s1
  ring_mul32x2(&t, &a, &s1);

  // Store s1 to the secret key
  ring_pack(sk + RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN, &s1, RRLWR_SIGN_LOG_ETA+1);

  // Compute b = round(p/q*A*s) computed as (A*s + q/(2*p)) >> (eq - ep)
  ring_round_xtoy(b, &t, RRLWR_SIGN_LOGQ, RRLWR_SIGN_LOGP);

  // Compute s2 = t - q/p*b and store to secret key
  ring_shift_and_sub(s2, &t, b, RRLWR_SIGN_LOGQ - RRLWR_SIGN_LOGP);

  // Store s2 to the secret key
  ring_pack(sk + RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + RRLWR_SIGN_S_LEN, s2, RRLWR_SIGN_LOG_ETA+1);

  // Round and pack b into b0 and b1
  ring_power2round_and_pack(sk + RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + 2*RRLWR_SIGN_S_LEN, pk + RRLWR_SIGN_RHO_LEN, b);

  // Store rho to secret and public key
  for(unsigned int i = 0; i < RRLWR_SIGN_RHO_LEN; i++) {
    sk[i] = rho[i];
    pk[i] = rho[i];
  }

  // Compute tr as the hash of the public key
  RRLWR_SIGN_HASH_TR(tr, pk);

  // Store K to secret key
  for(unsigned int i = 0; i < RRLWR_SIGN_K_LEN; i++) {
    sk[RRLWR_SIGN_RHO_LEN + i] = K[i];
  }

  // Store tr to secret key
  for(unsigned int i = 0; i < RRLWR_SIGN_TR_LEN; i++) {
    sk[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + i] = tr[i];
  }

  // Return key lengths
  *sk_len_bytes = RRLWR_SIGN_SK_LEN;
  *pk_len_bytes = RRLWR_SIGN_PK_LEN;

  return 0;
}

static void skDecode(unsigned char rho[RRLWR_SIGN_RHO_LEN], unsigned char K[RRLWR_SIGN_K_LEN], unsigned char tr[RRLWR_SIGN_TR_LEN], unsigned char *skp)
{
  unsigned int i;

  for(i = 0; i < RRLWR_SIGN_RHO_LEN; i++) {
    rho[i] = skp[i];
  }

  skp += RRLWR_SIGN_RHO_LEN;
  for(i = 0; i < RRLWR_SIGN_K_LEN; i++) {
    K[i] = skp[i];
  }

  skp += RRLWR_SIGN_K_LEN;
  for(i = 0; i < RRLWR_SIGN_TR_LEN; i++) {
    tr[i] = skp[i];
  }
}

int sig_sign(
  unsigned char *sk, unsigned long long sk_len_bytes,
  unsigned char *m, unsigned long long m_len_bytes,
  unsigned char *sn, unsigned long long *sn_len_bytes)
{
  unsigned int i, kappa;
  unsigned char rho[RRLWR_SIGN_RHO_LEN];
  unsigned char K_rnd_mu_w1[RRLWR_SIGN_K_LEN + RRLWR_SIGN_RND_LEN + RRLWR_SIGN_MU_LEN + RRLWR_SIGN_PACKED_W1_LEN];
  unsigned char *K = K_rnd_mu_w1;
  unsigned char *rnd = K + RRLWR_SIGN_K_LEN;
  unsigned char *mu = rnd + RRLWR_SIGN_RND_LEN;
  unsigned char *w1 = mu + RRLWR_SIGN_MU_LEN;
  unsigned char tr[RRLWR_SIGN_TR_LEN];
  unsigned char rhopp_kappa[RRLWR_SIGN_RHOPRIMEPRIME_LEN+4];
  unsigned char *rhopp = rhopp_kappa;
  unsigned char ctilde[RRLWR_SIGN_CTILDE_LEN];
  unsigned char ch_input[RRLWR_SIGN_HASH_CH_INPUT_LEN];
  size_t ch_w1_pos;
  poly c, t0;
  ring_element s1, s2, b0, y, w;
  ring_element *t = &s1;
  ring_element *h = &w;
  ring_elementx2 yx2, ax2, tx2;

  // Sanity check on the secret key length
  if(sk_len_bytes != RRLWR_SIGN_SK_LEN) {
    return -1;
  }

  // Deserialize the private key
  skDecode(rho, K, tr, sk);

  // Retrieve secret and public values and convert to NTT domains
  ring_uniform_public_x4(t, RRLWR_SIGN_LOGQ, rho, RRLWR_SIGN_RHO_LEN);
  ring_ntt32x2(&ax2, t);
  ring_unpack(&s1, &sk[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN], RRLWR_SIGN_LOG_ETA+1); // s1
  ring_ntt32(&s1, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  ring_unpack(&s2, &sk[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + RRLWR_SIGN_S_LEN], RRLWR_SIGN_LOG_ETA+1); // s2
  ring_ntt32(&s2, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  ring_unpack(&b0, &sk[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + 2*RRLWR_SIGN_S_LEN], RRLWR_SIGN_D); // b0
  ring_ntt32(&b0, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);

  // Create the digest mu = H(tr || M)
  if(RRLWR_SIGN_HASH_MU(mu, tr, m, m_len_bytes)) {
    return -1;
  }
  ch_w1_pos = RRLWR_SIGN_HASH_CH_INIT(ch_input, mu);

  // Create the random seed rho''
  GENERATE_RANDOM_BYTES(rnd, RRLWR_SIGN_RND_LEN, &drng_algorithm);
  RRLWR_SIGN_HASH_RHOPP(rhopp, K, rnd, mu);

  // Start the rejection loop
  kappa = 0;

reject:

  #ifdef MEASURE_HEURISTICS
    num_rounds++;
  #endif

  // Sample y and transform to NTT domain
  rrlwr_store32_le(rhopp + RRLWR_SIGN_RHOPRIMEPRIME_LEN, kappa);
  ring_uniform_secret_x4(&y, RRLWR_SIGN_LOG_GAMMA1+1, rhopp_kappa, RRLWR_SIGN_RHOPRIMEPRIME_LEN+4);
  kappa += RRLWR_K;
  ring_ntt32x2(&yx2, &y);

  // Compute w = A*y assuming A and y already in NTT domain
  tx2 = ax2; // to avoid being overwritten in-place due to multiplication with (y+2)
  ring_mul_invntt32x2(&w, &tx2, &yx2);

  // Compute the commitment w1 = w/(2*gamma_2)
  ring_w1(w1, &w);

  // Compute the challenge seed ctilde
  memcpy(ch_input + ch_w1_pos, w1, RRLWR_SIGN_PACKED_W1_LEN);
  RRLWR_SIGN_HASH_CH_FINAL(ctilde, ch_input);

  // Expand the challenge from ctilde, rejecting if insufficient pseudo-randomness is available
  if(poly_sampleInBall(&c, ctilde)) {
    goto reject;
  }

  // Convert the challenge to NTT domain
  poly_ntt32(&c, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);

  // Compute w+c*s2 mod (2*gamma2)
  for(i = 0; i < RRLWR_K; i++) {
    poly_basemul32(&t0, &c, &s2.x[i], RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV);
    poly_invntt32(&t0, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1, rrlwr_sign_zetas1);
    poly_conditional_final_reduce32(&t0, RRLWR_SIGN_PRIME1); // Reduce to unique representation

    poly_add(&w.x[i], &w.x[i], &t0);
    poly_reduce_pow2(&w.x[i], &w.x[i], RRLWR_SIGN_LOG_GAMMA2+1);
    if(poly_check_norm(&w.x[i], RRLWR_SIGN_GAMMA2-RRLWR_SIGN_BETA)) {
      #ifdef MEASURE_HEURISTICS
        num_rejects_w++;
      #endif
      goto reject;
    }
  }

  // Compute y+c*s1
  for(i = 0; i < RRLWR_K; i++) {
    poly_basemul32(&t0, &c, &s1.x[i], RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV);
    poly_invntt32(&t0, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1, rrlwr_sign_zetas1);
    poly_conditional_final_reduce32(&t0, RRLWR_SIGN_PRIME1); // Reduce to unique representation

    poly_add(&y.x[i], &y.x[i], &t0);
    if(poly_check_norm(&y.x[i], RRLWR_SIGN_GAMMA1-RRLWR_SIGN_BETA)) {
      #ifdef MEASURE_HEURISTICS
        num_rejects_y++;
      #endif
      goto reject;
    }
  }

  // Compute c*q/p*b0 and compute hint vector
  int hint_weight = 0;
  for(i = 0; i < RRLWR_K; i++) {
    poly_basemul32(&t0, &c, &b0.x[i], RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV);
    poly_invntt32(&t0, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1, rrlwr_sign_zetas1);
    poly_conditional_final_reduce32(&t0, RRLWR_SIGN_PRIME1);
    poly_shift(&t0, RRLWR_SIGN_LOGQ-RRLWR_SIGN_LOGP);

    // Check the norm of c*q/p*b0
    if(poly_check_norm(&t0, RRLWR_SIGN_GAMMA2)) {
      #ifdef MEASURE_HEURISTICS
        num_rejects_ct0++;
      #endif
      goto reject;
    }

    poly_add(&t0, &w.x[i], &t0);
    hint_weight += poly_make_hint(&h->x[i], &w.x[i], &t0);
  }

  #ifdef MEASURE_HEURISTICS
    if(hint_weight >= max_hint_weight) {
      max_hint_weight = hint_weight;
    }
  #endif

  // Reject if too many hints are required
  if(hint_weight > RRLWR_SIGN_OMEGA) {
    #ifdef MEASURE_HEURISTICS // Parametrization only
      num_rejects_hint++;
    #endif
    goto reject;
  }

  // Pack the signature
  for(i = 0; i < RRLWR_SIGN_CTILDE_LEN; i++) {
    sn[i] = ctilde[i];
  }
  ring_pack(sn + RRLWR_SIGN_CTILDE_LEN, &y, RRLWR_SIGN_LOG_GAMMA1+1);
  ring_pack_hint(sn + RRLWR_SIGN_CTILDE_LEN + RRLWR_SIGN_PACKED_Z_LEN, h);

  // Return signature length
  *sn_len_bytes = RRLWR_SIGN_SIG_LEN;

  return 0;
}

int sig_verify(
  unsigned char *pk, unsigned long long pk_len_bytes,
  unsigned char *sn, unsigned long long sn_len_bytes,
  unsigned char *m, unsigned long long m_len_bytes)
{
  unsigned int i;
  unsigned char rho[RRLWR_SIGN_RHO_LEN];
  unsigned char tr[RRLWR_SIGN_TR_LEN];
  unsigned char mu_w1[RRLWR_SIGN_MU_LEN + RRLWR_SIGN_PACKED_W1_LEN];
  unsigned char *mu = mu_w1;
  unsigned char *w1 = mu + RRLWR_SIGN_MU_LEN;
  unsigned char ctilde[RRLWR_SIGN_CTILDE_LEN];
  unsigned char ctildep[RRLWR_SIGN_CTILDE_LEN];
  ring_element a, z, w, b1, h;
  poly c, t0, t1;

  // Sanity check on the public key length
  if(pk_len_bytes != RRLWR_SIGN_PK_LEN) {
    return -1;
  }

  // Sanity check on the signature length
  if(sn_len_bytes != RRLWR_SIGN_SIG_LEN) {
    return -1;
  }

  // Unpack the public key
  for(i = 0; i < RRLWR_SIGN_RHO_LEN; i++) {
    rho[i] = pk[i];
  }
  ring_unpack(&b1, pk + RRLWR_SIGN_RHO_LEN, RRLWR_SIGN_LOGP-RRLWR_SIGN_D);

  // Unpack the signature
  for(i = 0; i < RRLWR_SIGN_CTILDE_LEN; i++) {
    ctilde[i] = sn[i];
  }
  ring_unpack(&z, sn + RRLWR_SIGN_CTILDE_LEN, RRLWR_SIGN_LOG_GAMMA1+1);

  // Check the norm of z
  for(i = 0; i < RRLWR_K; i++) {
    if(poly_check_norm(&z.x[i], RRLWR_SIGN_GAMMA1-RRLWR_SIGN_BETA)) {
      return -1;
    }
  }

  // Generate the matrix a
  ring_uniform_public_x4(&a, RRLWR_SIGN_LOGQ, rho, RRLWR_SIGN_RHO_LEN);

  // Compute a*z
  ring_mul32x2(&w, &a, &z);

  // Expand the challenge from ctilde, rejecting if insufficient pseudo-randomness is available
  if(poly_sampleInBall(&c, ctilde)) {
    return -1;
  }

  // Convert the challenge to NTT domain
  poly_ntt32(&c, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);

  // Unpack the hint
  if(ring_unpack_hint(&h, sn + RRLWR_SIGN_CTILDE_LEN + RRLWR_SIGN_PACKED_Z_LEN)) {
    return -1;
  }

  // Compute c*b1, multiply with (q/p)*2^d and add to a*z
  for(i = 0; i < RRLWR_K; i++) {
    poly_ntt32(&b1.x[i], RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
    poly_basemul32(&b1.x[i], &c, &b1.x[i], RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV);
    poly_invntt32(&b1.x[i], RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1, rrlwr_sign_zetas1); // Could be merged with InvNTT(w)
    poly_conditional_final_reduce32(&b1.x[i], RRLWR_SIGN_PRIME1); // Reduce to unique representation

    poly_shift(&b1.x[i], RRLWR_SIGN_LOGQ - RRLWR_SIGN_LOGP + RRLWR_SIGN_D);
    poly_sub(&w.x[i], &w.x[i], &b1.x[i]);
    poly_power2round(&t0, &t1, &w.x[i], RRLWR_SIGN_LOG_GAMMA2+1);
    poly_apply_hint(&t1, &t0, &h.x[i]);
    poly_pack(w1 + i*RRLWR_SIGN_PACKED_POLYW1_LEN, &t1, RRLWR_SIGN_LOGQ-(RRLWR_SIGN_LOG_GAMMA2+1));
  }

  // Create the digest mu = H(tr || M)
  RRLWR_SIGN_HASH_TR(tr, pk);
  if(RRLWR_SIGN_HASH_MU(mu, tr, m, m_len_bytes)) {
    return -1;
  }

  // Re-compute the challenge seed ctilde
  RRLWR_SIGN_HASH_CH(ctildep, mu, w1);

  // Check that the challenge matches
  for(i = 0; i < RRLWR_SIGN_CTILDE_LEN; i++) {
    if(ctildep[i] != ctilde[i]) {
      return -1;
    }
  }

  return 0;
}
