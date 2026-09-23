/**
 * \file brqc.c
 * \brief Implementation of the BRQC.PKE public key encryption scheme
 *
 * The BRQC.PKE scheme is constructed from the Ideal Blockwise Rank Decoding
 * (IBRD) problem. Main components:
 *   - Gabidulin code C_{gab}(g, k): an [n, k] Gabidulin code determined by generator g
 *   - Ideal code C_{id}(h): an ideal code determined by h
 *
 * See algorithm specification Section 3.3, Figure 2.
 */

#include "brqc.h"
#include "parameters.h"
#include "rbc_vspace.h"
#include "rbc_qre.h"
#include "gabidulin.h"
#include "parsing.h"

/**
 * \fn void brqc_pke_keygen(uint8_t* pk, uint8_t* sk)
 * \brief PKE key generation (PKE.KGen)
 *
 * Corresponds to Figure 2, lines 03-06:
 *   1. Generate secret key vectors (x, y) with Supp(x) cap Supp(y) = {0} (direct sum)
 *      wt_R(x) = w_x, wt_R(y) = w_y
 *   2. Generate public key vectors g (full rank) and h (random)
 *   3. Compute syndrome s = x + h*y mod P(X)
 *   4. pk = (s, pk_seed), sk = (sk_seed, pk)
 *
 * \param[out] pk Public key byte string
 * \param[out] sk Secret key byte string
 */
void brqc_pke_keygen(uint8_t* pk, uint8_t* sk) {
  random_source sk_seedexpander;
  random_source pk_seedexpander;
  random_source_init(&sk_seedexpander, RANDOM_SOURCE_SEEDEXP);
  random_source_init(&pk_seedexpander, RANDOM_SOURCE_SEEDEXP);
  uint8_t sk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
  uint8_t pk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
  rbc_vspace support_x, support_y, support_xy;
  rbc_qre x, y;
  rbc_qre g, h, s;

  rbc_field_init();
  rbc_qre_init_modulus(BRQC_PARAM_N);

  rbc_vspace_init(&support_x, BRQC_PARAM_W_x);
  rbc_vspace_init(&support_y, BRQC_PARAM_W_y);

  rbc_qre_init(&x);
  rbc_qre_init(&y);
  rbc_qre_init(&g);
  rbc_qre_init(&h);
  rbc_qre_init(&s);

  random_source prng;
  random_source_init(&prng, RANDOM_SOURCE_PRNG);

  /* Generate seeds for secret key and public key */
  random_source_get_bytes(&prng, sk_seed, SEEDEXPANDER_SEED_BYTES);
  random_source_get_bytes(&prng, pk_seed, SEEDEXPANDER_SEED_BYTES);
  random_source_seed_with_len(&sk_seedexpander, sk_seed, SEEDEXPANDER_SEED_BYTES);
  random_source_seed_with_len(&pk_seedexpander, pk_seed, SEEDEXPANDER_SEED_BYTES);

  /* Generate secret key (x, y)
   * To ensure Supp(x) + Supp(y) = direct sum, first generate a full-rank
   * support set support_xy of size w_x + w_y, then split it into support_x
   * and support_y. This guarantees Supp(x) cap Supp(y) = {0}. */
  rbc_vspace_init(&support_xy, BRQC_PARAM_W_x + BRQC_PARAM_W_y);
  rbc_vspace_set_random_full_rank(&sk_seedexpander, support_xy, BRQC_PARAM_W_x + BRQC_PARAM_W_y);

  for(size_t i = 0; i < BRQC_PARAM_W_x; i++) {
    rbc_elt_set(support_x[i], support_xy[i]);
  }
  for(size_t i = 0; i < BRQC_PARAM_W_y; i++) {
    rbc_elt_set(support_y[i], support_xy[BRQC_PARAM_W_x + i]);
  }

  rbc_qre_set_random_from_support(&sk_seedexpander, x, support_x, BRQC_PARAM_W_x, 1);
  rbc_qre_set_random_from_support(&sk_seedexpander, y, support_y, BRQC_PARAM_W_y, 1);

  /* Generate public key vectors g (full rank) and h (random) */
  rbc_qre_set_random_full_rank(&pk_seedexpander, g);
  rbc_qre_set_random(&pk_seedexpander, h);

  /* Compute syndrome s = x + h*y mod P(X) */
  rbc_qre_mul(s, h, y);
  rbc_qre_add(s, s, x);

  /* Serialize keys (PKE layer passes a zero temporary sigma; KEM layer will
   * re-serialize sk with the correct sigma) */
  rbc_vec sigma_tmp;
  rbc_vec_init(&sigma_tmp, BRQC_PARAM_K);
  brqc_public_key_to_string(pk, s, pk_seed);
  brqc_secret_key_to_string(sk, sk_seed, sigma_tmp, pk);
  rbc_vec_clear(sigma_tmp);

#ifdef VERBOSE
  printf("\n\nsk_seed: "); for(int i = 0 ; i < SEEDEXPANDER_SEED_BYTES ; ++i) printf("%02x", sk_seed[i]);
  printf("\n\npk_seed: "); for(int i = 0 ; i < SEEDEXPANDER_SEED_BYTES ; ++i) printf("%02x", pk_seed[i]);
  printf("\n\nsupport_x: "); rbc_vspace_print(support_x, BRQC_PARAM_W_x);
  printf("\n\nsupport_y: "); rbc_vspace_print(support_y, BRQC_PARAM_W_y);
  printf("\n\nx: "); rbc_qre_print(x);
  printf("\n\ny: "); rbc_qre_print(y);
  printf("\n\ng: "); rbc_qre_print(g);
  printf("\n\nh: "); rbc_qre_print(h);
  printf("\n\ns: "); rbc_qre_print(s);
  printf("\n\nsk: "); for(int i = 0 ; i < BRQC_SECRET_KEY_BYTES ; ++i) printf("%02x", sk[i]);
  printf("\n\npk: "); for(int i = 0 ; i < BRQC_PUBLIC_KEY_BYTES ; ++i) printf("%02x", pk[i]);
#endif

  rbc_vspace_clear(support_x);
  rbc_vspace_clear(support_y);
  rbc_vspace_clear(support_xy);
  rbc_qre_clear(x);
  rbc_qre_clear(y);
  rbc_qre_clear(g);
  rbc_qre_clear(h);
  rbc_qre_clear(s);
  rbc_qre_clear_modulus();
  random_source_clear(&sk_seedexpander);
  random_source_clear(&pk_seedexpander);
}


/**
 * \fn void brqc_pke_encrypt(rbc_qre u, rbc_qre v, const rbc_vec m, uint8_t* theta, const uint8_t* pk)
 * \brief PKE encryption (PKE.Enc)
 *
 * Corresponds to Figure 2, lines 07-10:
 *   1. Deterministically generate random vectors (r1, r2, e) using theta
 *      Supp(r1) + Supp(r2) + Supp(e) = direct sum
 *      wt_R(r1) = w_{r1}, wt_R(r2) = w_{r2}, wt_R(e) = w_e
 *   2. u = r1 + h*r2 mod P(X)
 *   3. v = Encode(m) + s*r2 + e mod P(X)
 *      Encode uses Gabidulin code C_{gab}(g, k)
 *
 * \param[out] u First ciphertext component u in R_{q^m}
 * \param[out] v Second ciphertext component v in R_{q^m}
 * \param[in]  m Plaintext vector m in F^k_{q^m}
 * \param[in]  theta Seed for deterministic random generation (theta)
 * \param[in]  pk Public key byte string
 */
void brqc_pke_encrypt(rbc_qre u, rbc_qre v, const rbc_vec m, uint8_t* theta, const uint8_t* pk) {
  random_source seedexpander;
  random_source_init(&seedexpander, RANDOM_SOURCE_SEEDEXP);
  rbc_qre g, h, s;
  rbc_vspace support_r1, support_r2, support_e, support_re;
  rbc_qre r1, r2, e;
  rbc_qre tmp;
  rbc_gabidulin code;

  rbc_field_init();
  rbc_qre_init_modulus(BRQC_PARAM_N);

  rbc_qre_init(&g);
  rbc_qre_init(&h);
  rbc_qre_init(&s);

  rbc_vspace_init(&support_r1, BRQC_PARAM_W_r1);
  rbc_vspace_init(&support_r2, BRQC_PARAM_W_r2);
  rbc_vspace_init(&support_e, BRQC_PARAM_W_e);

  rbc_qre_init(&r1);
  rbc_qre_init(&r2);
  rbc_qre_init(&e);
  rbc_qre_init(&tmp);

  /* Initialize seed expander with theta (deterministic random) */
  random_source_seed_with_len(&seedexpander, theta, SHA512_BYTES);

  /* Recover g, h, s from public key byte string */
  brqc_public_key_from_string(g, h, s, pk);

  /* Generate (r1, r2, e) with direct sum support
   * First generate a full-rank support of size w_{r1} + w_{r2} + w_e,
   * then split into three parts */
  rbc_vspace_init(&support_re, BRQC_PARAM_W_r1 + BRQC_PARAM_W_r2 + BRQC_PARAM_W_e);
  rbc_vspace_set_random_full_rank(&seedexpander, support_re, BRQC_PARAM_W_r1 + BRQC_PARAM_W_r2 + BRQC_PARAM_W_e);

  for(size_t i = 0; i < BRQC_PARAM_W_r1; i++) {
    rbc_elt_set(support_r1[i], support_re[i]);
  }
  for(size_t i = 0; i < BRQC_PARAM_W_r2; i++) {
    rbc_elt_set(support_r2[i], support_re[BRQC_PARAM_W_r1 + i]);
  }
  for(size_t i = 0; i < BRQC_PARAM_W_e; i++) {
    rbc_elt_set(support_e[i], support_re[BRQC_PARAM_W_r1 + BRQC_PARAM_W_r2 + i]);
  }

  rbc_qre_set_random_from_support(&seedexpander, r1, support_r1, BRQC_PARAM_W_r1, 1);
  rbc_qre_set_random_from_support(&seedexpander, r2, support_r2, BRQC_PARAM_W_r2, 1);
  rbc_qre_set_random_from_support(&seedexpander, e, support_e, BRQC_PARAM_W_e, 1);

  /* Compute u = r1 + h*r2 mod P(X) */
  rbc_qre_mul(u, h, r2);
  rbc_qre_add(u, u, r1);

  /* Compute v = Encode(m) + s*r2 + e mod P(X)
   * First encode plaintext using Gabidulin code: v = m*G */
  rbc_gabidulin_init(&code, g, BRQC_PARAM_K, BRQC_PARAM_N);
  rbc_gabidulin_encode(v, code, m);

  /* Then add noise: v = m*G + s*r2 + e */
  rbc_qre_mul(tmp, s, r2);
  rbc_qre_add(tmp, tmp, e);
  rbc_qre_add(v, v, tmp);

#ifdef VERBOSE
  printf("\n\ng: "); rbc_qre_print(g);
  printf("\n\nh: "); rbc_qre_print(h);
  printf("\n\ns: "); rbc_qre_print(s);
  printf("\n\nsupport_r1: "); rbc_vspace_print(support_r1, BRQC_PARAM_W_r1);
  printf("\n\nsupport_r2: "); rbc_vspace_print(support_r2, BRQC_PARAM_W_r2);
  printf("\n\nsupport_e: "); rbc_vspace_print(support_e, BRQC_PARAM_W_e);
  printf("\n\nr1: "); rbc_qre_print(r1);
  printf("\n\nr2: "); rbc_qre_print(r2);
  printf("\n\ne: "); rbc_qre_print(e);
  printf("\n\nu: "); rbc_qre_print(u);
  printf("\n\nv: "); rbc_qre_print(v);
#endif

  rbc_qre_clear(g);
  rbc_qre_clear(h);
  rbc_qre_clear(s);
  rbc_vspace_clear(support_r1);
  rbc_vspace_clear(support_r2);
  rbc_vspace_clear(support_e);
  rbc_vspace_clear(support_re);
  rbc_qre_clear(r1);
  rbc_qre_clear(r2);
  rbc_qre_clear(e);
  rbc_qre_clear(tmp);
  rbc_qre_clear_modulus();
  random_source_clear(&seedexpander);
}


/**
 * \fn void brqc_pke_decrypt(rbc_vec m, const rbc_qre u, const rbc_qre v, const uint8_t* sk)
 * \brief PKE decryption (PKE.Dec)
 *
 * Corresponds to Figure 2, lines 11-12:
 *   1. Compute v - u*y
 *      v - u*y = m*G + s*r2 + e - (r1 + h*r2)*y
 *             = m*G + (x + h*y)*r2 + e - r1*y - h*r2*y
 *             = m*G + x*r2 + e - r1*y
 *      Since wt_R(x*r2 + e - r1*y) is small, the Gabidulin code can correct the error
 *   2. m = Decode(v - u*y), recover plaintext using Gabidulin decoding
 *
 * \param[out] m Recovered plaintext vector m in F^k_{q^m}
 * \param[in]  u First ciphertext component
 * \param[in]  v Second ciphertext component
 * \param[in]  sk Secret key byte string
 */
void brqc_pke_decrypt(rbc_vec m, const rbc_qre u, const rbc_qre v, const uint8_t* sk) {
  uint8_t pk[BRQC_PUBLIC_KEY_BYTES] = {0};
  rbc_qre x, y, g, h, s;
  rbc_qre tmp;
  rbc_gabidulin code;

  rbc_field_init();
  rbc_qre_init_modulus(BRQC_PARAM_N);

  rbc_qre_init(&x);
  rbc_qre_init(&y);
  rbc_qre_init(&g);
  rbc_qre_init(&h);
  rbc_qre_init(&s);
  rbc_qre_init(&tmp);

  /* Recover x, y and public key from secret key (PKE layer does not need sigma) */
  rbc_vec sigma_tmp;
  rbc_vec_init(&sigma_tmp, BRQC_PARAM_K);
  brqc_secret_key_from_string(x, y, sigma_tmp, pk, sk);
  rbc_vec_clear(sigma_tmp);

  /* Recover g, h, s from public key */
  brqc_public_key_from_string(g, h, s, pk);

  /* Compute v - u*y (cancels h*r2*y term, leaving correctable error) */
  rbc_qre_mul(tmp, u, y);
  rbc_qre_add(tmp, v, tmp);

  /* Gabidulin decoding to recover plaintext m */
  rbc_gabidulin_init(&code, g, BRQC_PARAM_K, BRQC_PARAM_N);
  rbc_gabidulin_decode(m, code, tmp);

#ifdef VERBOSE
  printf("\n\nu: "); rbc_qre_print(u);
  printf("\n\nv: "); rbc_qre_print(v);
  printf("\n\ny: "); rbc_qre_print(y);
  printf("\n\nv - u.y: "); rbc_qre_print(tmp);
#endif

  rbc_qre_clear(x);
  rbc_qre_clear(y);
  rbc_qre_clear(g);
  rbc_qre_clear(h);
  rbc_qre_clear(s);
  rbc_qre_clear(tmp);
  rbc_qre_clear_modulus();
}
