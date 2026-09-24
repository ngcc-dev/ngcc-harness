/**
 * \file parsing.c
 * \brief Key and ciphertext serialization/deserialization implementation for the BRA scheme
 *
 * Data formats:
 *   Secret key sk = sk_seed(SEEDEXPANDER_SEED_BYTES) || sigma(VEC_K_BYTES) || pk
 *   Public key pk = s_bytes(VEC_N_BYTES) || pk_seed(SEEDEXPANDER_SEED_BYTES)
 *   Ciphertext ct = u_bytes(VEC_N_BYTES) || v_bytes(VEC_N_BYTES) || salt(SALT_BYTES)
 */

#include "string.h"
#include "rbc_vspace.h"
#include "rbc_qre.h"
#include "parameters.h"
#include "parsing.h"
#include "rbc_elt.h"
#include "rbc_vec.h"


/**
 * \fn void bra_secret_key_to_string(uint8_t* sk, const uint8_t* seed, const rbc_vec sigma, const uint8_t* pk)
 * \brief Serialize the secret key to a byte string
 *
 * sk = sk_seed || sigma_bytes || pk
 * The secret key contains sigma (FO transform implicit rejection value)
 * and the public key (needed during decapsulation).
 *
 * \param[out] sk Secret key byte string
 * \param[in]  seed Seed for generating secret key vectors (x, y)
 * \param[in]  sigma FO transform implicit rejection value sigma in F^k_{q^m}
 * \param[in]  pk Public key byte string
 */
void bra_secret_key_to_string(uint8_t* sk, const uint8_t* seed, const rbc_vec sigma, const uint8_t* pk) {
  memcpy(sk, seed, SEEDEXPANDER_SEED_BYTES);
  rbc_vec_to_string(sk + SEEDEXPANDER_SEED_BYTES, sigma, BRA_PARAM_K);
  memcpy(sk + SEEDEXPANDER_SEED_BYTES + BRA_VEC_K_BYTES, pk, BRA_PUBLIC_KEY_BYTES);
}


/**
 * \fn void bra_secret_key_from_string(rbc_qre x, rbc_qre y, rbc_vec sigma, uint8_t* pk, const uint8_t* sk)
 * \brief Recover secret key vectors (x, y), implicit rejection value sigma, and public key from byte string
 *
 * Deterministically regenerates (x, y) from sk_seed to match keygen.
 * Deserializes sigma (FO transform implicit rejection value) from the sigma field.
 * Support set splitting matches keygen, ensuring Supp(x) + Supp(y) = direct sum.
 *
 * \param[out] x Secret key vector x in R_{q^m}
 * \param[out] y Secret key vector y in R_{q^m}
 * \param[out] sigma FO transform implicit rejection value sigma in F^k_{q^m}
 * \param[out] pk Recovered public key byte string
 * \param[in]  sk Secret key byte string
 */
void bra_secret_key_from_string(rbc_qre x, rbc_qre y, rbc_vec sigma, uint8_t* pk, const uint8_t* sk) {
  uint8_t sk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
  random_source sk_seedexpander;
  random_source_init(&sk_seedexpander, RANDOM_SOURCE_SEEDEXP);

  rbc_vspace support_x, support_y, support_xy;
  rbc_vspace_init(&support_x, BRA_PARAM_W_x);
  rbc_vspace_init(&support_y, BRA_PARAM_W_y);

  memcpy(sk_seed, sk, SEEDEXPANDER_SEED_BYTES);
  random_source_seed_with_len(&sk_seedexpander, sk_seed, SEEDEXPANDER_SEED_BYTES);

  /* Regenerate support sets and split (deterministic process matching keygen) */
  rbc_vspace_init(&support_xy, BRA_PARAM_W_x + BRA_PARAM_W_y);
  rbc_vspace_set_random_full_rank(&sk_seedexpander, support_xy, BRA_PARAM_W_x + BRA_PARAM_W_y);

  for(size_t i = 0; i < BRA_PARAM_W_x; i++) {
    rbc_elt_set(support_x[i], support_xy[i]);
  }
  for(size_t i = 0; i < BRA_PARAM_W_y; i++) {
    rbc_elt_set(support_y[i], support_xy[BRA_PARAM_W_x + i]);
  }

  rbc_qre_set_random_from_support(&sk_seedexpander, x, support_x, BRA_PARAM_W_x, 1);
  rbc_qre_set_random_from_support(&sk_seedexpander, y, support_y, BRA_PARAM_W_y, 1);

  /* Extract sigma (implicit rejection value) from sk */
  rbc_vec_from_string(sigma, BRA_PARAM_K, sk + SEEDEXPANDER_SEED_BYTES);

  /* Extract public key from sk */
  memcpy(pk, sk + SEEDEXPANDER_SEED_BYTES + BRA_VEC_K_BYTES, BRA_PUBLIC_KEY_BYTES);

  rbc_vspace_clear(support_x);
  rbc_vspace_clear(support_y);
  rbc_vspace_clear(support_xy);
  random_source_clear(&sk_seedexpander);
}


/**
 * \fn void bra_public_key_to_string(uint8_t* pk, const rbc_qre s, const uint8_t* seed)
 * \brief Serialize the public key to a byte string
 *
 * pk = s_bytes || pk_seed
 * The syndrome s is directly serialized; pk_seed is used to regenerate g and h during parsing.
 *
 * \param[out] pk Public key byte string
 * \param[in]  s  Syndrome s = x + h*y in R_{q^m}
 * \param[in]  seed Seed for generating (g, h)
 */
void bra_public_key_to_string(uint8_t* pk, const rbc_qre s, const uint8_t* seed) {
  rbc_qre_to_string(pk, s);
  memcpy(pk + BRA_VEC_N_BYTES, seed, SEEDEXPANDER_SEED_BYTES);
}


/**
 * \fn void bra_public_key_from_string(rbc_qre g, rbc_qre h, rbc_qre s, const uint8_t* pk)
 * \brief Recover vectors g, h, s from the public key byte string
 *
 * Deterministically regenerates g (full rank) and h (random) from pk_seed;
 * directly deserializes s from pk.
 *
 * \param[out] g Extended Gabidulin code generator g in R_{q^m} (full rank)
 * \param[out] h Ideal code multiplier h in R_{q^m}
 * \param[out] s Syndrome s in R_{q^m}
 * \param[in]  pk Public key byte string
 */
void bra_public_key_from_string(rbc_qre g, rbc_qre h, rbc_qre s, const uint8_t* pk) {
  uint8_t pk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
  random_source pk_seedexpander;
  random_source_init(&pk_seedexpander, RANDOM_SOURCE_SEEDEXP);

  rbc_qre_from_string(s, pk);

  memcpy(pk_seed, pk + BRA_VEC_N_BYTES, SEEDEXPANDER_SEED_BYTES);
  random_source_seed_with_len(&pk_seedexpander, pk_seed, SEEDEXPANDER_SEED_BYTES);

  rbc_qre_set_random_full_rank(&pk_seedexpander, g);
  rbc_qre_set_random(&pk_seedexpander, h);

  random_source_clear(&pk_seedexpander);
}


/**
 * \fn void bra_kem_ciphertext_to_string(uint8_t* ct, const rbc_qre u, const rbc_qre v, const uint8_t* salt)
 * \brief Serialize the ciphertext to a byte string
 *
 * ct = u_bytes || v_bytes || salt
 *
 * \param[out] ct   Ciphertext byte string
 * \param[in]  u    First ciphertext component u in R_{q^m}
 * \param[in]  v    Second ciphertext component v in R_{q^m}
 * \param[in]  salt Salt value (512 bits)
 */
void bra_kem_ciphertext_to_string(uint8_t* ct, const rbc_qre u, const rbc_qre v, const uint8_t* salt) {
  rbc_qre_to_string(ct, u);
  rbc_qre_to_string(ct + BRA_VEC_N_BYTES, v);
  memcpy(ct + 2 * BRA_VEC_N_BYTES, salt, BRA_SALT_BYTES);
}


/**
 * \fn void bra_kem_ciphertext_from_string(rbc_qre u, rbc_qre v, uint8_t* salt, const uint8_t* ct)
 * \brief Recover u, v and salt from the ciphertext byte string
 *
 * \param[out] u    First ciphertext component u in R_{q^m}
 * \param[out] v    Second ciphertext component v in R_{q^m}
 * \param[out] salt Salt value (512 bits)
 * \param[in]  ct   Ciphertext byte string
 */
void bra_kem_ciphertext_from_string(rbc_qre u, rbc_qre v, uint8_t* salt, const uint8_t* ct) {
  rbc_qre_from_string(u, ct);
  rbc_qre_from_string(v, ct + BRA_VEC_N_BYTES);
  memcpy(salt, ct + 2 * BRA_VEC_N_BYTES, BRA_SALT_BYTES);
}
