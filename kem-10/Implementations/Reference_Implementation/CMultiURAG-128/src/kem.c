/** 
 * \file kem.c
 * \brief Implementation of the CMultiURAG.KEM key encapsulation mechanism
 * 
 * Based on the salted Fujisaki-Okamoto (FO) transform, converting CMultiURAG.PKE
 * into an IND-CCA2 secure KEM. See algorithm specification Section 3.4, Figure 3.
 *
 * KEM.KGen:  (pk', sk') <- PKE.KGen(par); sample sigma <- F^k_{q^m}; pk = pk', sk = (sk', sigma)
 * KEM.Encaps: sample m, salt; theta = G(pk, m, salt); ct' = PKE.Enc(pk, m; theta); ct = (ct', salt); K = H(pk, m, ct', salt)
 * KEM.Decaps: m = PKE.Dec(sk', ct'); theta = G(pk, m, salt); re-encrypt and verify; use sigma instead of m on failure
 */

#include "cmultiurag.h"
#include "parameters.h"
#include "parsing.h"

#include "rbc_mat.h"
#include "auxfunc.h"
#include "drng.h"
extern DRNG_ctx drng_algorithm;
#include "string.h"
#include <stdint.h>

 /** 
  * \fn int cmultiurag_keygen(uint8_t* pk, uint8_t* sk)
  * \brief CMultiURAG.KEM key generation (KEM.KGen)
  *
  * Corresponds to Figure 3, lines 03-06:
 *   1. Call PKE.KGen to generate (pk', sk')
 *   2. Independently sample sigma <-$ F^k_{q^m}
 *   3. Output pk = pk', sk = (sk', sigma) 
 * 
 * Secret key format: sk = sk_seed(SEEDEXPANDER_SEED_BYTES) || pk(PUBLIC_KEY_BYTES) || sigma(VEC_K_BYTES)
 * 
 * \param[out] pk Public key byte string
 * \param[out] sk Secret key byte string
 * \return 0 on success
 */
 int cmultiurag_keygen(uint8_t *pk, uint8_t *sk) {

   #ifdef VERBOSE
     printf("\n\n\n### KEYGEN ###");
   #endif

   rbc_vec sigma;
   rbc_vec_init(&sigma, CMULTIURAG_PARAM_K);

   /* Step 1: Call PKE.KGen to generate (pk', sk').
    * The PKE layer serializes sk_seed || pk, and we append sigma below to form
    * the KEM secret key. */
  cmultiurag_pke_keygen(pk, sk);


   /* Step 2: Independently sample sigma in F^k_{q^m} (using PRNG, independent of sk_seed) */
   random_source prng;
   random_source_init(&prng, RANDOM_SOURCE_PRNG);
   rbc_vec_set_random(&prng, sigma, CMULTIURAG_PARAM_K);

   /* Step 3: Complete the KEM secret key as sk = sk_seed || pk || sigma.
    * The prefix sk_seed || pk was written by pke_keygen; append sigma in the
    * last field using the standard vector serialization format. */
   rbc_vec_to_string(sk + SEEDEXPANDER_SEED_BYTES + CMULTIURAG_PUBLIC_KEY_BYTES, sigma, CMULTIURAG_PARAM_K);

   rbc_vec_clear(sigma);
   random_source_clear(&prng);

   return 0;
 }
 
 
 
 /** 
  * \fn int cmultiurag_encaps(uint8_t* ct, uint8_t* ss, const uint8_t* pk)
  * \brief CMultiURAG.KEM encapsulation (KEM.Encaps)
  * 
  * Steps (corresponding to Figure 3, lines 07-13):
  *   1. Randomly sample plaintext m in F^k_{q^m} and salt in {0,1}^512
  *   2. Compute theta = G(pk, m, salt)
  *   3. ct' = PKE.Enc(pk, m; theta)
  *   4. ct = (ct', salt) = (U || V || salt)
  *   5. K = H(pk, m, ct, salt)
  *
  * \param[out] ct Ciphertext byte string
  * \param[out] ss Shared secret (64 bytes)
  * \param[in]  pk Public key byte string
  * \return 0 on success
  */
int cmultiurag_encaps(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {

   #ifdef VERBOSE
     printf("\n\n\n\n### ENCAPS ###");
   #endif
 
   uint8_t theta[SHA512_BYTES] = {0};

   /* G hash input buffer: G_in = pk || m || salt */
   uint8_t G_in[CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_SALT_BYTES] = {0};
   uint8_t *m_str = &G_in[CMULTIURAG_PUBLIC_KEY_BYTES];
   uint8_t *salt = &G_in[CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES];

   /* H hash input buffer: H_in = pk || m || u || v || salt */
   uint8_t H_in[CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES + CMULTIURAG_SALT_BYTES] = {0};

   rbc_vec m;
   rbc_mat U, V;
 
   rbc_vec_init(&m, CMULTIURAG_PARAM_K);
   rbc_mat_init(&U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
   rbc_mat_init(&V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
 
   random_source prng;
   random_source_init(&prng, RANDOM_SOURCE_PRNG);
 
   /* Step 1: Randomly sample plaintext m in F^k_{q^m} */
   rbc_vec_set_random(&prng, m, CMULTIURAG_PARAM_K);

   /* Step 1: Randomly sample salt in {0,1}^512 */
   get_random_number(&drng_algorithm, salt, CMULTIURAG_SALT_BYTES * 8);
 
   /* Step 2: Compute theta = G(pk, m, salt) */
   memcpy(G_in, pk, CMULTIURAG_PUBLIC_KEY_BYTES);
   rbc_vec_to_string(m_str, m, CMULTIURAG_PARAM_K);
   pseudohash(512, G_in, (CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_SALT_BYTES) * 8, theta);
 
   /* Step 3: ct' = PKE.Enc(pk, m; theta), encrypt to obtain (U, V) */
   cmultiurag_pke_encrypt(U, V, m, theta, pk);
 
   /* Step 4: Serialize ciphertext ct = U || V || salt */
   cmultiurag_kem_ciphertext_to_string(ct, U, V, salt);

   /* Step 5: Compute shared secret K = H(pk, m, ct, salt) */
   memcpy(H_in, pk, CMULTIURAG_PUBLIC_KEY_BYTES);
   memcpy(H_in + CMULTIURAG_PUBLIC_KEY_BYTES, m_str, CMULTIURAG_VEC_K_BYTES);
   rbc_mat_to_string(H_in + CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES, U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
   rbc_mat_to_string(H_in + CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES, V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
   memcpy(H_in + CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES, salt, CMULTIURAG_SALT_BYTES);
   pseudohash(512, H_in, (CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES + CMULTIURAG_SALT_BYTES) * 8, ss);
 
   #ifdef VERBOSE
     printf("\n\nm: "); rbc_vec_print(m, CMULTIURAG_PARAM_K);
     printf("\n\nsalt: "); for(int i = 0 ; i < CMULTIURAG_SALT_BYTES ; ++i) printf("%02x", salt[i]);
     printf("\n\ntheta: "); for(int i = 0 ; i < SHA512_BYTES ; ++i) printf("%02x", theta[i]);
     printf("\n\nciphertext: "); for(int i = 0 ; i < CMULTIURAG_CIPHERTEXT_BYTES ; ++i) printf("%02x", ct[i]);
     printf("\n\nsecret 1: "); for(int i = 0 ; i < CMULTIURAG_SHARED_SECRET_BYTES ; ++i) printf("%02x", ss[i]);
   #endif
   
   rbc_vec_clear(m);
   rbc_mat_clear(U);
   rbc_mat_clear(V);
   random_source_clear(&prng);
 
   return 0;
 }
 
 
 
 /** 
 * \fn int cmultiurag_decaps(uint8_t* ss, const uint8_t* ct, const uint8_t* sk)
 * \brief CMultiURAG.KEM decapsulation (KEM.Decaps)
 *
 * Steps (corresponding to Figure 3, lines 14-18):
 *   1. Parse sk = (sk', sigma), ct = (ct', salt) = (U, V, salt)
 *   2. m = PKE.Dec(sk', ct')
 *   3. Extract sigma from the secret key
 *   4. theta = G(pk, m, salt)
 *   5. (U', V') = PKE.Enc(pk, m; theta), re-encryption verification
 *   6. If (U', V') != (U, V): K = H(pk, sigma, ct, salt) (implicit rejection)
 *      Otherwise: K = H(pk, m, ct, salt)
 *
 * \param[out] ss Shared secret (64 bytes)
 * \param[in]  ct Ciphertext byte string
 * \param[in]  sk Secret key byte string
 * \return 0 on success
  */
int cmultiurag_decaps(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {

   #ifdef VERBOSE
     printf("\n\n\n\n### DECAPS ###");
   #endif
 
   uint8_t pk[CMULTIURAG_PUBLIC_KEY_BYTES] = {0};
   uint8_t theta[SHA512_BYTES] = {0};

   /* G hash input buffer */
   uint8_t G_in[CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_SALT_BYTES] = {0};
   uint8_t *m_str = &G_in[CMULTIURAG_PUBLIC_KEY_BYTES];
   uint8_t *salt = &G_in[CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES];

   /* H hash input buffer */
   uint8_t H_in[CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES + CMULTIURAG_SALT_BYTES] = {0};
   
   rbc_vec m, sigma;
   rbc_mat U, V, U2, V2;

   rbc_vec_init(&m, CMULTIURAG_PARAM_K);
   rbc_vec_init(&sigma, CMULTIURAG_PARAM_K); 
   rbc_mat_init(&U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
   rbc_mat_init(&V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
   rbc_mat_init(&U2, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
   rbc_mat_init(&V2, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);

   /* Step 1: Parse U, V, salt from ciphertext */
   cmultiurag_kem_ciphertext_from_string(U, V, salt, ct);

   /* Step 2: Extract public key and deserialize sigma (implicit rejection value)
    * from the secret key layout sk = sk_seed || pk || sigma. */
   memcpy(pk, sk + SEEDEXPANDER_SEED_BYTES, CMULTIURAG_PUBLIC_KEY_BYTES);
   rbc_vec_from_string(sigma, CMULTIURAG_PARAM_K, sk + SEEDEXPANDER_SEED_BYTES + CMULTIURAG_PUBLIC_KEY_BYTES);

   /* Step 3: PKE decryption to recover plaintext m = PKE.Dec(sk', ct') */
   cmultiurag_pke_decrypt(m, U, V, sk);

   /* Step 4: Compute theta = G(pk, m, salt) */
   memcpy(G_in, pk, CMULTIURAG_PUBLIC_KEY_BYTES);
   rbc_vec_to_string(m_str, m, CMULTIURAG_PARAM_K);
   pseudohash(512, G_in, (CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_SALT_BYTES) * 8, theta);

   /* Step 5: Re-encrypt (u', v') = PKE.Enc(pk, m; theta) */
   cmultiurag_pke_encrypt(U2, V2, m, theta, pk);

   /* Step 6: Constant-time FO verification.
    *
    * Serialize (U, V) and (U', V') to byte strings and compare them with an
    * OR-accumulator that does not short-circuit. The resulting mask is then
    * used to select between m and sigma for m_str in a branchless, bytewise
    * fashion. This prevents any control-flow or early-exit leak that could
    * reveal whether decryption succeeded (and thus leak information about the
    * secret key or the rejection value sigma). */
   {
     uint8_t U_bytes[CMULTIURAG_MAT_NN2_BYTES];
     uint8_t V_bytes[CMULTIURAG_MAT_N1N2_BYTES];
     uint8_t U2_bytes[CMULTIURAG_MAT_NN2_BYTES];
     uint8_t V2_bytes[CMULTIURAG_MAT_N1N2_BYTES];
     uint8_t sigma_bytes[CMULTIURAG_VEC_K_BYTES];
     rbc_mat_to_string(U_bytes,  U,  CMULTIURAG_PARAM_N,  CMULTIURAG_PARAM_N2);
     rbc_mat_to_string(V_bytes,  V,  CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
     rbc_mat_to_string(U2_bytes, U2, CMULTIURAG_PARAM_N,  CMULTIURAG_PARAM_N2);
     rbc_mat_to_string(V2_bytes, V2, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
     rbc_vec_to_string(sigma_bytes, sigma, CMULTIURAG_PARAM_K);

     /* OR-accumulate every byte difference. Seed 0x0100 guarantees that
      * (r - 1) >> 8 is 0 only when every XOR contribution was zero. */
     uint16_t r = 0x0100;
     for(size_t i = 0; i < CMULTIURAG_MAT_NN2_BYTES;  i++) r |= (uint16_t)(U_bytes[i] ^ U2_bytes[i]);
     for(size_t i = 0; i < CMULTIURAG_MAT_N1N2_BYTES; i++) r |= (uint16_t)(V_bytes[i] ^ V2_bytes[i]);
     uint8_t mismatch_mask = (uint8_t)(-((r - 1) >> 8)); /* 0x00 match, 0xFF mismatch */

     /* m_str currently holds the bytes of m (written earlier while computing
      * theta = G(pk, m, salt)). Conditionally overwrite with sigma using XOR
      * selection, which has no branch dependent on mismatch_mask. */
     for(size_t i = 0; i < CMULTIURAG_VEC_K_BYTES; i++) {
       m_str[i] ^= mismatch_mask & (m_str[i] ^ sigma_bytes[i]);
     }
   }

   /* Compute shared secret K = H(pk, m/sigma, ct, salt) */
   memcpy(H_in, pk, CMULTIURAG_PUBLIC_KEY_BYTES);
   memcpy(H_in + CMULTIURAG_PUBLIC_KEY_BYTES, m_str, CMULTIURAG_VEC_K_BYTES); 
   rbc_mat_to_string(H_in + CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES, U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
   rbc_mat_to_string(H_in + CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES, V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
   memcpy(H_in + CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES, salt, CMULTIURAG_SALT_BYTES);
   pseudohash(512, H_in, (CMULTIURAG_PUBLIC_KEY_BYTES + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES + CMULTIURAG_SALT_BYTES) * 8, ss);
 
   #ifdef VERBOSE
     printf("\n\npk: "); for(int i = 0 ; i < CMULTIURAG_PUBLIC_KEY_BYTES ; ++i) printf("%02x", pk[i]);
     printf("\n\nsk: "); for(int i = 0 ; i < CMULTIURAG_SECRET_KEY_BYTES ; ++i) printf("%02x", sk[i]);
     printf("\n\nciphertext: "); for(int i = 0 ; i < CMULTIURAG_CIPHERTEXT_BYTES ; ++i) printf("%02x", ct[i]);
     printf("\n\nm: "); rbc_vec_print(m, CMULTIURAG_PARAM_K);
     printf("\n\ntheta: "); for(int i = 0 ; i < SHA512_BYTES ; ++i) printf("%02x", theta[i]);
     printf("\n\n\n# Checking Ciphertext- Begin #");
     printf("\n\nU2: "); rbc_mat_print(U2, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
     printf("\n\nV2: "); rbc_mat_print(V2, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
     printf("\n\n# Checking Ciphertext - End #\n");
   #endif
 
   rbc_vec_clear(m);
   rbc_vec_clear(sigma);
   rbc_mat_clear(U);
   rbc_mat_clear(V);
   rbc_mat_clear(U2);
   rbc_mat_clear(V2);
 
   return 0;
 }
