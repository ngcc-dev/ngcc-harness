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
#include "sign_test.h"
#define RRLWR_HASH_DOMAIN_SIGN
#include "hash_domain.h"
#include "test/cpucycles.h"

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

uint64_t sign_test_cycles[SIGN_TEST_STAGE_COUNT];
unsigned int sign_test_rejection_rounds;
int sign_test_measure_awin_base;

static uint64_t sign_test_overhead;

void sign_test_add_cycles(unsigned int stage, uint64_t start)
{
  uint64_t elapsed = cpucycles() - start;

  if(elapsed > sign_test_overhead) {
    sign_test_cycles[stage] += elapsed - sign_test_overhead;
  }
}

void sign_test_reset_cycles(void)
{
  if(sign_test_overhead == 0) {
    sign_test_overhead = cpucycles_overhead();
  }

  memset(sign_test_cycles, 0, sizeof(sign_test_cycles));
  sign_test_rejection_rounds = 0;
  sign_test_measure_awin_base = 0;
}

const char *sign_test_stage_name(unsigned int stage)
{
  static const char *names[SIGN_TEST_STAGE_COUNT] = {
    "keygen.random_hash",
    "keygen.A_uniform_Awinx2",
    "keygen.Awin_base",
    "keygen.Awin_base.shake128x4",
    "keygen.Awin_base.unpack",
    "keygen.Awin_prepare_p1",
    "keygen.Awin_prepare_p2",
    "keygen.s_uniform",
    "keygen.A_times_s1",
    "keygen.A_times_s1.ring_ntt32x2",
    "keygen.A_times_s1.ntt_p1",
    "keygen.A_times_s1.ntt_p2",
    "keygen.A_times_s1.mul_invntt_x2",
    "keygen.A_times_s1.mul_invntt_p1",
    "keygen.A_times_s1.mul_invntt_p2",
    "keygen.A_times_s1.crt",
    "keygen.pack_s1",
    "keygen.round_s2",
    "keygen.pack_b",
    "keygen.hash_store",
    "sign.sk_decode",
    "sign.prepare_A",
    "sign.unpack_ntt_secret",
    "sign.hash_mu_rhopp",
    "sign.sample_y",
    "sign.ntt_y_x2",
    "sign.A_times_y",
    "sign.w1_hash_ctilde",
    "sign.sample_c_ntt",
    "sign.c_s2_check_w",
    "sign.c_s1_check_y",
    "sign.c_b0_hint",
    "sign.pack_signature",
    "verify.unpack_check",
    "verify.prepare_A",
    "verify.A_times_z",
    "verify.sample_c_ntt",
    "verify.unpack_hint",
    "verify.c_b1_w1",
    "verify.hash_compare"
  };

  return stage < SIGN_TEST_STAGE_COUNT ? names[stage] : "unknown";
}

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

int sig_keygen_test(
  unsigned char *pk, unsigned long long *pk_len_bytes,
  unsigned char *sk, unsigned long long *sk_len_bytes)
{
  uint64_t ts;
  unsigned char xi[RRLWR_SIGN_XI_LEN];
  unsigned char rho_rhoprime_K[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_RHOPRIME_LEN + RRLWR_SIGN_K_LEN]; // (rho, rho', K)
  unsigned char *rho = rho_rhoprime_K;
  unsigned char *rhoprime = rho_rhoprime_K + RRLWR_SIGN_RHO_LEN;
  unsigned char *K = rhoprime + RRLWR_SIGN_RHOPRIME_LEN;
  unsigned char tr[RRLWR_SIGN_TR_LEN];
  ring_element s1, t, b;
  ring_elementx2 sx2, tx2;
  ring_element_Awinx2 ax2;
  ring_element *s2 = &s1;

  sign_test_reset_cycles();

  // Generate (rho, rho', K)
  ts = cpucycles();
  GENERATE_RANDOM_BYTES(xi, RRLWR_SIGN_XI_LEN, &drng_algorithm);
  RRLWR_SIGN_HASH_KDF(rho_rhoprime_K, xi);
  sign_test_add_cycles(SIGN_TEST_KEYGEN_RANDOM_HASH, ts);

  // Generate A in Awin form with coefficients in [-q/2+1, q/2]
  ts = cpucycles();
  {
    uint64_t ta = cpucycles();
    sign_test_measure_awin_base = 1;
    ring_uniform_Awin_base(&ax2.xp1, RRLWR_SIGN_LOGQ, rho, RRLWR_SIGN_RHO_LEN);
    sign_test_measure_awin_base = 0;
    ax2.xp2 = ax2.xp1;
    sign_test_add_cycles(SIGN_TEST_KEYGEN_A_UNIFORM_BASE, ta);

    ta = cpucycles();
    ring_Awin_ntt_prepare_ncoeffs(&ax2.xp1, RRLWR_K,
                                  RRLWR_SIGN_PRIME1,
                                  RRLWR_SIGN_PRIME1INV,
                                  RRLWR_SIGN_RMODPRIME1,
                                  RRLWR_SIGN_2RMODPRIME1,
                                  rrlwr_sign_zetas1);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_A_PREPARE_P1, ta);

    ta = cpucycles();
    ring_Awin_ntt_prepare_ncoeffs(&ax2.xp2, RRLWR_K,
                                  RRLWR_SIGN_PRIME2,
                                  RRLWR_SIGN_PRIME2INV,
                                  RRLWR_SIGN_RMODPRIME2,
                                  RRLWR_SIGN_2RMODPRIME2,
                                  rrlwr_sign_zetas2);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_A_PREPARE_P2, ta);
  }
  sign_test_add_cycles(SIGN_TEST_KEYGEN_A_UNIFORM, ts);

  // Generate s with coefficients in [-2, 1]
  ts = cpucycles();
  ring_uniform_secret(&s1, RRLWR_SIGN_LOG_ETA+1, rhoprime, RRLWR_SIGN_RHOPRIME_LEN);
  sign_test_add_cycles(SIGN_TEST_KEYGEN_S_UNIFORM, ts);

  // Compute A*s1
  ts = cpucycles();
  {
    uint64_t ta = cpucycles();
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        sx2.xp1.x[j].coeffs[k] = s1.x[j].coeffs[k];
        sx2.xp2.x[j].coeffs[k] = s1.x[j].coeffs[k];
      }
    }

    uint64_t tb = cpucycles();
    ring_ntt32(&sx2.xp1, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1_NTT_P1, tb);

    tb = cpucycles();
    ring_ntt32(&sx2.xp2, RRLWR_SIGN_PRIME2, RRLWR_SIGN_PRIME2INV, rrlwr_sign_zetas2);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1_NTT_P2, tb);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1_NTT_X2, ta);

    ta = cpucycles();
    tb = cpucycles();
    ring_mul_Awin_invntt32(tx2.xp1.x,
                           &ax2.xp1,
                           &sx2.xp1,
                           RRLWR_K,
                           RRLWR_SIGN_PRIME1,
                           RRLWR_SIGN_PRIME1INV,
                           RRLWR_SIGN_NTTINV_FINALCONST1,
                           rrlwr_sign_zetas1);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_P1, tb);

    tb = cpucycles();
    ring_mul_Awin_invntt32(tx2.xp2.x,
                           &ax2.xp2,
                           &sx2.xp2,
                           RRLWR_K,
                           RRLWR_SIGN_PRIME2,
                           RRLWR_SIGN_PRIME2INV,
                           RRLWR_SIGN_NTTINV_FINALCONST2,
                           rrlwr_sign_zetas2);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_P2, tb);

    tb = cpucycles();
    ring_crt(&t, &tx2.xp1, &tx2.xp2);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1_CRT, tb);
    sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_X2, ta);
  }
  sign_test_add_cycles(SIGN_TEST_KEYGEN_MUL_AS1, ts);

  // Store s1 to the secret key
  ts = cpucycles();
  ring_pack(sk + RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN, &s1, RRLWR_SIGN_LOG_ETA+1);
  sign_test_add_cycles(SIGN_TEST_KEYGEN_PACK_S1, ts);

  // Compute b = round(p/q*A*s) computed as (A*s + q/(2*p)) >> (eq - ep)
  ts = cpucycles();
  ring_round_xtoy(&b, &t, RRLWR_SIGN_LOGQ, RRLWR_SIGN_LOGP);

  // Compute s2 = t - q/p*b and store to secret key
  ring_shift_and_sub(s2, &t, &b, RRLWR_SIGN_LOGQ - RRLWR_SIGN_LOGP);

  // Store s2 to the secret key
  ring_pack(sk + RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + RRLWR_SIGN_S_LEN, s2, RRLWR_SIGN_LOG_ETA+1);
  sign_test_add_cycles(SIGN_TEST_KEYGEN_ROUND_S2, ts);

  // Round and pack b into b0 and b1
  ts = cpucycles();
  ring_power2round_and_pack(sk + RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + 2*RRLWR_SIGN_S_LEN, pk + RRLWR_SIGN_RHO_LEN, &b);
  sign_test_add_cycles(SIGN_TEST_KEYGEN_PACK_B, ts);

  // Store rho to secret and public key
  ts = cpucycles();
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
  sign_test_add_cycles(SIGN_TEST_KEYGEN_HASH_STORE, ts);

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

int sig_sign_test(
  unsigned char *sk, unsigned long long sk_len_bytes,
  unsigned char *m, unsigned long long m_len_bytes,
  unsigned char *sn, unsigned long long *sn_len_bytes)
{
  uint64_t ts;
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
  ring_element *h = &w;
  ring_elementx2 yx2;
  ring_element_Awinx2 ax2;

  sign_test_reset_cycles();

  // Sanity check on the secret key length
  if(sk_len_bytes != RRLWR_SIGN_SK_LEN) {
    return -1;
  }

  // Deserialize the private key
  ts = cpucycles();
  skDecode(rho, K, tr, sk);
  sign_test_add_cycles(SIGN_TEST_SIGN_SKDECODE, ts);

  // Retrieve secret and public values and convert to NTT domains
  ts = cpucycles();
  ring_uniform_Awinx2(&ax2, RRLWR_SIGN_LOGQ, rho, RRLWR_SIGN_RHO_LEN);
  sign_test_add_cycles(SIGN_TEST_SIGN_PREPARE_A, ts);

  ts = cpucycles();
  ring_unpack(&s1, &sk[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN], RRLWR_SIGN_LOG_ETA+1); // s1
  ring_ntt32(&s1, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  ring_unpack(&s2, &sk[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + RRLWR_SIGN_S_LEN], RRLWR_SIGN_LOG_ETA+1); // s2
  ring_ntt32(&s2, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  ring_unpack(&b0, &sk[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN + 2*RRLWR_SIGN_S_LEN], RRLWR_SIGN_D); // b0
  ring_ntt32(&b0, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  sign_test_add_cycles(SIGN_TEST_SIGN_UNPACK_NTT_SECRET, ts);

  // Create the digest mu = H(tr || M)
  ts = cpucycles();
  if(RRLWR_SIGN_HASH_MU(mu, tr, m, m_len_bytes)) {
    return -1;
  }
  ch_w1_pos = RRLWR_SIGN_HASH_CH_INIT(ch_input, mu);

  // Create the random seed rho''
  GENERATE_RANDOM_BYTES(rnd, RRLWR_SIGN_RND_LEN, &drng_algorithm);
  RRLWR_SIGN_HASH_RHOPP(rhopp, K, rnd, mu);
  sign_test_add_cycles(SIGN_TEST_SIGN_HASH_MU_RHOPP, ts);

  // Start the rejection loop
  kappa = 0;

reject:

  #ifdef MEASURE_HEURISTICS
    num_rounds++;
  #endif
  sign_test_rejection_rounds++;

  // Sample y and transform to NTT domain
  ts = cpucycles();
  rrlwr_store32_le(rhopp + RRLWR_SIGN_RHOPRIMEPRIME_LEN, kappa);
  ring_uniform_secret_x4(&y, RRLWR_SIGN_LOG_GAMMA1+1, rhopp_kappa, RRLWR_SIGN_RHOPRIMEPRIME_LEN+4);
  kappa += RRLWR_K;
  sign_test_add_cycles(SIGN_TEST_SIGN_SAMPLE_Y, ts);

  ts = cpucycles();
  ring_ntt32x2(&yx2, &y);
  sign_test_add_cycles(SIGN_TEST_SIGN_NTT_Y, ts);

  // Compute w = A*y assuming A and y already in NTT domain
  ts = cpucycles();
  ring_mul_Awin_invntt32x2(&w, &ax2, &yx2);
  sign_test_add_cycles(SIGN_TEST_SIGN_MUL_AY, ts);

  // Compute the commitment w1 = w/(2*gamma_2)
  ts = cpucycles();
  ring_w1(w1, &w);

  // Compute the challenge seed ctilde
  memcpy(ch_input + ch_w1_pos, w1, RRLWR_SIGN_PACKED_W1_LEN);
  RRLWR_SIGN_HASH_CH_FINAL(ctilde, ch_input);
  sign_test_add_cycles(SIGN_TEST_SIGN_W1_HASH_C, ts);

  // Expand the challenge from ctilde, rejecting if insufficient pseudo-randomness is available
  ts = cpucycles();
  if(poly_sampleInBall(&c, ctilde)) {
    sign_test_add_cycles(SIGN_TEST_SIGN_SAMPLE_C_NTT, ts);
    goto reject;
  }

  // Convert the challenge to NTT domain
  poly_ntt32(&c, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  sign_test_add_cycles(SIGN_TEST_SIGN_SAMPLE_C_NTT, ts);

  // Compute w+c*s2 mod (2*gamma2)
  ts = cpucycles();
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
      sign_test_add_cycles(SIGN_TEST_SIGN_CS2_CHECK, ts);
      goto reject;
    }
  }
  sign_test_add_cycles(SIGN_TEST_SIGN_CS2_CHECK, ts);

  // Compute y+c*s1
  ts = cpucycles();
  for(i = 0; i < RRLWR_K; i++) {
    poly_basemul32(&t0, &c, &s1.x[i], RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV);
    poly_invntt32(&t0, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1, rrlwr_sign_zetas1);
    poly_conditional_final_reduce32(&t0, RRLWR_SIGN_PRIME1); // Reduce to unique representation

    poly_add(&y.x[i], &y.x[i], &t0);
    if(poly_check_norm(&y.x[i], RRLWR_SIGN_GAMMA1-RRLWR_SIGN_BETA)) {
      #ifdef MEASURE_HEURISTICS
        num_rejects_y++;
      #endif
      sign_test_add_cycles(SIGN_TEST_SIGN_CS1_CHECK, ts);
      goto reject;
    }
  }
  sign_test_add_cycles(SIGN_TEST_SIGN_CS1_CHECK, ts);

  // Compute c*q/p*b0 and compute hint vector
  ts = cpucycles();
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
      sign_test_add_cycles(SIGN_TEST_SIGN_CB0_HINT, ts);
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
    sign_test_add_cycles(SIGN_TEST_SIGN_CB0_HINT, ts);
    goto reject;
  }
  sign_test_add_cycles(SIGN_TEST_SIGN_CB0_HINT, ts);

  // Pack the signature
  ts = cpucycles();
  for(i = 0; i < RRLWR_SIGN_CTILDE_LEN; i++) {
    sn[i] = ctilde[i];
  }
  ring_pack(sn + RRLWR_SIGN_CTILDE_LEN, &y, RRLWR_SIGN_LOG_GAMMA1+1);
  ring_pack_hint(sn + RRLWR_SIGN_CTILDE_LEN + RRLWR_SIGN_PACKED_Z_LEN, h);
  sign_test_add_cycles(SIGN_TEST_SIGN_PACK, ts);

  // Return signature length
  *sn_len_bytes = RRLWR_SIGN_SIG_LEN;

  return 0;
}

int sig_verify_test(
  unsigned char *pk, unsigned long long pk_len_bytes,
  unsigned char *sn, unsigned long long sn_len_bytes,
  unsigned char *m, unsigned long long m_len_bytes)
{
  uint64_t ts;
  unsigned int i;
  unsigned char rho[RRLWR_SIGN_RHO_LEN];
  unsigned char tr[RRLWR_SIGN_TR_LEN];
  unsigned char mu_w1[RRLWR_SIGN_MU_LEN + RRLWR_SIGN_PACKED_W1_LEN];
  unsigned char *mu = mu_w1;
  unsigned char *w1 = mu + RRLWR_SIGN_MU_LEN;
  unsigned char ctilde[RRLWR_SIGN_CTILDE_LEN];
  unsigned char ctildep[RRLWR_SIGN_CTILDE_LEN];
  ring_element z, w, b1, h;
  ring_element_Awinx2 ax2;
  poly c, t0, t1;

  sign_test_reset_cycles();

  // Sanity check on the public key length
  if(pk_len_bytes != RRLWR_SIGN_PK_LEN) {
    return -1;
  }

  // Sanity check on the signature length
  if(sn_len_bytes != RRLWR_SIGN_SIG_LEN) {
    return -1;
  }

  // Unpack the public key
  ts = cpucycles();
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
      sign_test_add_cycles(SIGN_TEST_VERIFY_UNPACK_CHECK, ts);
      return -1;
    }
  }
  sign_test_add_cycles(SIGN_TEST_VERIFY_UNPACK_CHECK, ts);

  // Generate the matrix A in Awin form
  ts = cpucycles();
  ring_uniform_Awinx2(&ax2, RRLWR_SIGN_LOGQ, rho, RRLWR_SIGN_RHO_LEN);
  sign_test_add_cycles(SIGN_TEST_VERIFY_PREPARE_A, ts);

  // Compute a*z
  ts = cpucycles();
  ring_mul_Awin32x2(&w, &ax2, &z);
  sign_test_add_cycles(SIGN_TEST_VERIFY_MUL_AZ, ts);

  // Expand the challenge from ctilde, rejecting if insufficient pseudo-randomness is available
  ts = cpucycles();
  if(poly_sampleInBall(&c, ctilde)) {
    sign_test_add_cycles(SIGN_TEST_VERIFY_SAMPLE_C_NTT, ts);
    return -1;
  }

  // Convert the challenge to NTT domain
  poly_ntt32(&c, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  sign_test_add_cycles(SIGN_TEST_VERIFY_SAMPLE_C_NTT, ts);

  // Unpack the hint
  ts = cpucycles();
  if(ring_unpack_hint(&h, sn + RRLWR_SIGN_CTILDE_LEN + RRLWR_SIGN_PACKED_Z_LEN)) {
    sign_test_add_cycles(SIGN_TEST_VERIFY_UNPACK_HINT, ts);
    return -1;
  }
  sign_test_add_cycles(SIGN_TEST_VERIFY_UNPACK_HINT, ts);

  // Compute c*b1, multiply with (q/p)*2^d and add to a*z
  ts = cpucycles();
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
  sign_test_add_cycles(SIGN_TEST_VERIFY_CB1_W1, ts);

  // Create the digest mu = H(tr || M)
  ts = cpucycles();
  RRLWR_SIGN_HASH_TR(tr, pk);
  if(RRLWR_SIGN_HASH_MU(mu, tr, m, m_len_bytes)) {
    return -1;
  }

  // Re-compute the challenge seed ctilde
  RRLWR_SIGN_HASH_CH(ctildep, mu, w1);

  // Check that the challenge matches
  for(i = 0; i < RRLWR_SIGN_CTILDE_LEN; i++) {
    if(ctildep[i] != ctilde[i]) {
      sign_test_add_cycles(SIGN_TEST_VERIFY_HASH_COMPARE, ts);
      return -1;
    }
  }
  sign_test_add_cycles(SIGN_TEST_VERIFY_HASH_COMPARE, ts);

  return 0;
}
