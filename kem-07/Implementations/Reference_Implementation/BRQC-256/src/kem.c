/**
 * \file kem.c
 * \brief Implementation of the BRQC.KEM key encapsulation mechanism
 *
 * Based on the salted Fujisaki-Okamoto (FO) transform, converting BRQC.PKE
 * into an IND-CCA2 secure KEM. See algorithm specification Section 3.4, Figure 3.
 *
 * KEM.KGen:  (pk', sk') <- PKE.KGen(par); sample sigma <- F^k_{q^m}; pk = pk', sk = (sk', sigma)
 * KEM.Encaps: sample m, salt; theta = G(pk, m, salt); ct' = PKE.Enc(pk, m; theta); ct = (ct', salt); K = H(pk, m, ct, salt)
 * KEM.Decaps: m = PKE.Dec(sk', ct'); theta = G(pk, m, salt); re-encrypt and verify; use sigma instead of m on failure
 */

#include "brqc.h"
#include "parameters.h"
#include "parsing.h"

#include "rbc_qre.h"
#include "auxfunc.h"
#include "drng.h"
extern DRNG_ctx drng_algorithm;
#include "string.h"
#include <stdint.h>

/**
 * \fn int brqc_keygen(uint8_t* pk, uint8_t* sk)
 * \brief BRQC.KEM key generation (KEM.KGen)
 *
 * Corresponds to Figure 3, lines 03-06:
 *   1. Call PKE.KGen to generate (pk', sk')
 *   2. Independently sample sigma <-$ F^k_{q^m}
 *   3. Output pk = pk', sk = (sk', sigma)
 *
 * Secret key format: sk = sk_seed(SEEDEXPANDER_SEED_BYTES) || sigma(VEC_K_BYTES) || pk
 *
 * \param[out] pk Public key byte string
 * \param[out] sk Secret key byte string
 * \return 0 on success
 */
int brqc_keygen(uint8_t *pk, uint8_t *sk) {

#ifdef VERBOSE
  printf("\n\n\n### KEYGEN ###");
#endif

  rbc_vec sigma;
  rbc_field_init();
  rbc_qre_init_modulus(BRQC_PARAM_N);
  rbc_vec_init(&sigma, BRQC_PARAM_K);

  /* Step 1: Call PKE.KGen to generate (pk', sk')
   * Note: brqc_pke_keygen internally calls brqc_secret_key_to_string with a
   * temporary sigma, but we will re-serialize sk with the correct sigma below.
   * Let pke_keygen generate pk and a temporary sk (to obtain sk_seed). */
  brqc_pke_keygen(pk, sk);

  /* Step 2: Independently sample sigma in F^k_{q^m} (using PRNG, independent of sk_seed) */
  random_source prng;
  random_source_init(&prng, RANDOM_SOURCE_PRNG);
  rbc_vec_set_random(&prng, sigma, BRQC_PARAM_K);

  /* Step 3: Re-serialize secret key sk = sk_seed || sigma || pk
   * sk_seed was written to the first SEEDEXPANDER_SEED_BYTES bytes of sk by pke_keygen, reuse it */
  uint8_t sk_seed[SEEDEXPANDER_SEED_BYTES];
  memcpy(sk_seed, sk, SEEDEXPANDER_SEED_BYTES);
  brqc_secret_key_to_string(sk, sk_seed, sigma, pk);

  rbc_vec_clear(sigma);
  rbc_qre_clear_modulus();

  return 0;
}

/**
 * \fn int brqc_encaps(uint8_t* ct, uint8_t* ss, const uint8_t* pk)
 * \brief BRQC.KEM encapsulation (KEM.Encaps)
 *
 * Steps (corresponding to Figure 3, lines 07-13):
 *   1. Randomly sample plaintext m in F^k_{q^m} and salt in {0,1}^512
 *   2. Compute theta = G(pk, m, salt)
 *   3. ct' = PKE.Enc(pk, m; theta)
 *   4. ct = (ct', salt) = (u || v || salt)
 *   5. K = H(pk, m, ct, salt)
 *
 * \param[out] ct Ciphertext byte string
 * \param[out] ss Shared secret (64 bytes)
 * \param[in]  pk Public key byte string
 * \return 0 on success
 */
int brqc_encaps(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {

#ifdef VERBOSE
  printf("\n\n\n\n### ENCAPS ###");
#endif

  uint8_t theta[SHA512_BYTES] = {0};

  /* G hash input buffer: G_in = pk || m || salt */
  uint8_t G_in[BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + BRQC_SALT_BYTES] = {0};
  uint8_t *m_str = &G_in[BRQC_PUBLIC_KEY_BYTES];
  uint8_t *salt = &G_in[BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES];

  /* H hash input buffer: H_in = pk || m || u || v || salt */
  uint8_t H_in[BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + 2 * BRQC_VEC_N_BYTES + BRQC_SALT_BYTES] = {0};

  rbc_vec m;
  rbc_qre u, v;

  rbc_field_init();
  rbc_qre_init_modulus(BRQC_PARAM_N);
  rbc_vec_init(&m, BRQC_PARAM_K);
  rbc_qre_init(&u);
  rbc_qre_init(&v);

  random_source prng;
  random_source_init(&prng, RANDOM_SOURCE_PRNG);

  /* Step 1: Randomly sample plaintext m in F^k_{q^m} */
  rbc_vec_set_random(&prng, m, BRQC_PARAM_K);

  /* Step 1: Randomly sample salt in {0,1}^512 */
  get_random_number(&drng_algorithm, salt, BRQC_SALT_BYTES * 8);

  /* Step 2: Compute theta = G(pk, m, salt) */
  memcpy(G_in, pk, BRQC_PUBLIC_KEY_BYTES);
  rbc_vec_to_string(m_str, m, BRQC_PARAM_K);
  pseudohash(512, G_in, (BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + BRQC_SALT_BYTES) * 8, theta);

  /* Step 3: ct' = PKE.Enc(pk, m; theta), encrypt to obtain (u, v) */
  brqc_pke_encrypt(u, v, m, theta, pk);

  /* Step 4: Serialize ciphertext ct = u || v || salt */
  brqc_kem_ciphertext_to_string(ct, u, v, salt);

  /* Step 5: Compute shared secret K = H(pk, m, ct, salt)
   *   H_in = pk || m_str || u_bytes || v_bytes || salt */
  memcpy(H_in, pk, BRQC_PUBLIC_KEY_BYTES);
  memcpy(H_in + BRQC_PUBLIC_KEY_BYTES, m_str, BRQC_VEC_K_BYTES);
  rbc_qre_to_string(H_in + BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES, u);
  rbc_qre_to_string(H_in + BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + BRQC_VEC_N_BYTES, v);
  memcpy(H_in + BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + 2 * BRQC_VEC_N_BYTES, salt, BRQC_SALT_BYTES);
  pseudohash(512, H_in, (BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + 2 * BRQC_VEC_N_BYTES + BRQC_SALT_BYTES) * 8, ss);

#ifdef VERBOSE
  printf("\n\nm: "); rbc_vec_print(m, BRQC_PARAM_K);
  printf("\n\nsalt: "); for(int i = 0 ; i < BRQC_SALT_BYTES ; ++i) printf("%02x", salt[i]);
  printf("\n\ntheta: "); for(int i = 0 ; i < SHA512_BYTES ; ++i) printf("%02x", theta[i]);
  printf("\n\nciphertext: "); for(int i = 0 ; i < BRQC_CIPHERTEXT_BYTES ; ++i) printf("%02x", ct[i]);
  printf("\n\nsecret 1: "); for(int i = 0 ; i < BRQC_SHARED_SECRET_BYTES ; ++i) printf("%02x", ss[i]);
#endif

  rbc_vec_clear(m);
  rbc_qre_clear(u);
  rbc_qre_clear(v);
  rbc_qre_clear_modulus();

  return 0;
}

/**
 * \fn int brqc_decaps(uint8_t* ss, const uint8_t* ct, const uint8_t* sk)
 * \brief BRQC.KEM decapsulation (KEM.Decaps)
 *
 * Steps (corresponding to Figure 3, lines 14-18):
 *   1. Parse sk = (sk', sigma), ct = (ct', salt) = (u, v, salt)
 *   2. m = PKE.Dec(sk', ct')
 *   3. Extract sigma from the secret key
 *   4. theta = G(pk, m, salt)
 *   5. (u', v') = PKE.Enc(pk, m; theta), re-encryption verification
 *   6. If m = bot or (u', v') != (u, v): K = H(pk, sigma, ct, salt) (implicit rejection)
 *      Otherwise: K = H(pk, m, ct, salt)
 *
 * \param[out] ss Shared secret (64 bytes)
 * \param[in]  ct Ciphertext byte string
 * \param[in]  sk Secret key byte string
 * \return 0 on success
 */
int brqc_decaps(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {

#ifdef VERBOSE
  printf("\n\n\n\n### DECAPS ###");
#endif

  uint8_t pk[BRQC_PUBLIC_KEY_BYTES] = {0};
  uint8_t theta[SHA512_BYTES] = {0};

  /* G hash input buffer */
  uint8_t G_in[BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + BRQC_SALT_BYTES] = {0};
  uint8_t *m_str = &G_in[BRQC_PUBLIC_KEY_BYTES];
  uint8_t *salt = &G_in[BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES];

  /* H hash input buffer */
  uint8_t H_in[BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + 2 * BRQC_VEC_N_BYTES + BRQC_SALT_BYTES] = {0};

  rbc_vec m, sigma;
  rbc_qre u, v, u2, v2;

  rbc_field_init();
  rbc_qre_init_modulus(BRQC_PARAM_N);

  rbc_vec_init(&m, BRQC_PARAM_K);
  rbc_vec_init(&sigma, BRQC_PARAM_K);
  rbc_qre_init(&u);
  rbc_qre_init(&v);
  rbc_qre_init(&u2);
  rbc_qre_init(&v2);

  /* Step 1: Parse u, v, salt from ciphertext */
  brqc_kem_ciphertext_from_string(u, v, salt, ct);

  /* Step 1: Extract public key and sigma (implicit rejection value) from secret key */
  memcpy(pk, sk + SEEDEXPANDER_SEED_BYTES + BRQC_VEC_K_BYTES, BRQC_PUBLIC_KEY_BYTES);

  /* Step 2: PKE decryption to recover plaintext m = PKE.Dec(sk', ct') */
  brqc_pke_decrypt(m, u, v, sk);

  /* Step 3: Deserialize sigma from secret key (for implicit rejection) */
  rbc_vec_from_string(sigma, BRQC_PARAM_K, sk + SEEDEXPANDER_SEED_BYTES);

  /* Step 4: Compute theta = G(pk, m, salt) */
  memcpy(G_in, pk, BRQC_PUBLIC_KEY_BYTES);
  rbc_vec_to_string(m_str, m, BRQC_PARAM_K);
  pseudohash(512, G_in, (BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + BRQC_SALT_BYTES) * 8, theta);

  /* Step 5: Re-encrypt (u', v') = PKE.Enc(pk, m; theta) */
  brqc_pke_encrypt(u2, v2, m, theta, pk);

  /* Step 6: Constant-time FO verification.
   *
   * Serialize (u, v) and (u', v') to byte strings and compare them with an
   * OR-accumulator that does not short-circuit. The resulting mask is then
   * used to select between m and sigma for m_str in a branchless, bytewise
   * fashion. This prevents any control-flow or early-exit leak that could
   * reveal whether decryption succeeded (and thus leak information about the
   * secret key or the rejection value sigma). */
  {
    uint8_t u_bytes[BRQC_VEC_N_BYTES];
    uint8_t v_bytes[BRQC_VEC_N_BYTES];
    uint8_t u2_bytes[BRQC_VEC_N_BYTES];
    uint8_t v2_bytes[BRQC_VEC_N_BYTES];
    uint8_t sigma_bytes[BRQC_VEC_K_BYTES];
    rbc_qre_to_string(u_bytes, u);
    rbc_qre_to_string(v_bytes, v);
    rbc_qre_to_string(u2_bytes, u2);
    rbc_qre_to_string(v2_bytes, v2);
    rbc_vec_to_string(sigma_bytes, sigma, BRQC_PARAM_K);

    /* OR-accumulate every byte difference. Seed 0x0100 guarantees that
     * (r - 1) >> 8 is 0 only when every XOR contribution was zero. */
    uint16_t r = 0x0100;
    for(size_t i = 0; i < BRQC_VEC_N_BYTES; i++) r |= (uint16_t)(u_bytes[i] ^ u2_bytes[i]);
    for(size_t i = 0; i < BRQC_VEC_N_BYTES; i++) r |= (uint16_t)(v_bytes[i] ^ v2_bytes[i]);
    uint8_t mismatch_mask = (uint8_t)(-((r - 1) >> 8)); /* 0x00 match, 0xFF mismatch */

    /* m_str currently holds the bytes of m (written earlier while computing
     * theta = G(pk, m, salt)). Conditionally overwrite with sigma using XOR
     * selection, which has no branch dependent on mismatch_mask. */
    for(size_t i = 0; i < BRQC_VEC_K_BYTES; i++) {
      m_str[i] ^= mismatch_mask & (m_str[i] ^ sigma_bytes[i]);
    }
  }

  /* Compute shared secret K = H(pk, m/sigma, ct, salt) */
  memcpy(H_in, pk, BRQC_PUBLIC_KEY_BYTES);
  memcpy(H_in + BRQC_PUBLIC_KEY_BYTES, m_str, BRQC_VEC_K_BYTES);
  rbc_qre_to_string(H_in + BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES, u);
  rbc_qre_to_string(H_in + BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + BRQC_VEC_N_BYTES, v);
  memcpy(H_in + BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + 2 * BRQC_VEC_N_BYTES, salt, BRQC_SALT_BYTES);
  pseudohash(512, H_in, (BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + 2 * BRQC_VEC_N_BYTES + BRQC_SALT_BYTES) * 8, ss);

#ifdef VERBOSE
  printf("\n\npk: "); for(int i = 0 ; i < BRQC_PUBLIC_KEY_BYTES ; ++i) printf("%02x", pk[i]);
  printf("\n\nsk: "); for(int i = 0 ; i < BRQC_SECRET_KEY_BYTES ; ++i) printf("%02x", sk[i]);
  printf("\n\nciphertext: "); for(int i = 0 ; i < BRQC_CIPHERTEXT_BYTES ; ++i) printf("%02x", ct[i]);
  printf("\n\nm: "); rbc_vec_print(m, BRQC_PARAM_K);
  printf("\n\ntheta: "); for(int i = 0 ; i < SHA512_BYTES ; ++i) printf("%02x", theta[i]);
  printf("\n\n\n# Checking Ciphertext- Begin #");
  printf("\n\nu2: "); rbc_qre_print(u2);
  printf("\n\nv2: "); rbc_qre_print(v2);
  printf("\n\n# Checking Ciphertext - End #\n");
#endif

  rbc_vec_clear(m);
  rbc_vec_clear(sigma);
  rbc_qre_clear(u);
  rbc_qre_clear(v);
  rbc_qre_clear(u2);
  rbc_qre_clear(v2);
  rbc_qre_clear_modulus();

  return 0;
}
