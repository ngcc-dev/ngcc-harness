/** 
 * \file parsing.c
 * \brief Key and ciphertext serialization/deserialization implementation for the CMultiURAG scheme
 *
 * Data formats:
 *   Secret key sk = sk_seed(SEEDEXPANDER_SEED_BYTES) || pk || sigma(VEC_K_BYTES)
 *   Public key pk = S_bytes(MAT_NN1_BYTES) || pk_seed(SEEDEXPANDER_SEED_BYTES)
 *   Ciphertext ct = U_bytes(MAT_NN2_BYTES) || V_bytes(MAT_N1N2_BYTES) || salt(SALT_BYTES)
 */

 #include "string.h"
 #include "rbc_vspace.h"
 #include "rbc_mat.h"
 #include "parameters.h"
 #include "parsing.h"
  
 
 /** 
 * \fn void cmultiurag_secret_key_to_string(uint8_t* sk, const uint8_t* seed, const uint8_t* pk)
 * \brief Serialize the secret key to a byte string
 *
 * The PKE secret material is serialized as sk_seed || pk. The KEM layer appends
 * sigma after this prefix to obtain sk = sk_seed || pk || sigma.
 *
 * \param[out] sk Secret key byte string
 * \param[in] seed Seed for generating secret key matrices (X, Y)
 * \param[in] pk Public key byte string
  */
void cmultiurag_secret_key_to_string(uint8_t* sk, const uint8_t* seed, const uint8_t* pk) {
   memcpy(sk, seed, SEEDEXPANDER_SEED_BYTES);
   memcpy(sk + SEEDEXPANDER_SEED_BYTES, pk, CMULTIURAG_PUBLIC_KEY_BYTES);
 }
 
 
 
 /** 
 * \fn void cmultiurag_secret_key_from_string(rbc_mat X, rbc_mat Y, uint8_t* pk, const uint8_t* sk)
 * \brief Recover secret key matrices (X, Y) and public key from byte string
 *
 * Deterministically regenerates (X, Y) from sk_seed to match keygen.
 * The KEM sigma field, if present, lives after pk and is ignored here.
 * 
 * \param[out] X Secret key matrix X in support w1
 * \param[out] Y Secret key matrix Y in support w1
 * \param[out] pk Recovered public key byte string
 * \param[in] sk Secret key byte string
 */
 void cmultiurag_secret_key_from_string(rbc_mat X, rbc_mat Y, uint8_t* pk, const uint8_t* sk) {
   uint8_t sk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
   random_source sk_seedexpander;
   random_source_init(&sk_seedexpander, RANDOM_SOURCE_SEEDEXP);

   rbc_vspace support_1;
   rbc_vspace_init(&support_1, CMULTIURAG_PARAM_W1);
 
   memcpy(sk_seed, sk, SEEDEXPANDER_SEED_BYTES);
   random_source_seed_with_len(&sk_seedexpander, sk_seed, SEEDEXPANDER_SEED_BYTES);

   rbc_vspace_set_random_full_rank_with_one(&sk_seedexpander, support_1, CMULTIURAG_PARAM_W1);
   rbc_mat_set_random_from_support(&sk_seedexpander, X, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1, support_1, CMULTIURAG_PARAM_W1, 1);
   rbc_mat_set_random_from_support(&sk_seedexpander, Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1, support_1, CMULTIURAG_PARAM_W1, 1);
 
   memcpy(pk, sk + SEEDEXPANDER_SEED_BYTES, CMULTIURAG_PUBLIC_KEY_BYTES);
 
   rbc_vspace_clear(support_1);
   random_source_clear(&sk_seedexpander);
 }
 
 
 
 /** 
  * \fn void cmultiurag_public_key_to_string(uint8_t* pk, const rbc_mat s, const uint8_t* seed)
  * \brief Serialize the public key to a byte string
  *
  * The public key is composed of the matrix <b>S</b> as well as the seed used to generate vector <b>g</b> and matrix <b>H</b>.
  *
  * \param[out] pk Public key byte string
  * \param[in] S matrix S = X + HY
  * \param[in] seed Seed vector <b>g</b> and matrix <b>H</b>
  */
 void cmultiurag_public_key_to_string(uint8_t* pk, const rbc_mat S, const uint8_t* seed) {
   rbc_mat_to_string(pk, S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
   memcpy(pk + CMULTIURAG_MAT_NN1_BYTES, seed, SEEDEXPANDER_SEED_BYTES);
 }
 
 
 
 /** 
  * \fn void cmultiurag_public_key_from_string(rbc_vec g, rbc_mat H, rbc_mat S, const uint8_t* pk)
 * \brief Recover vector g and matrices H, S from the public key byte string
  *
  * Deterministically regenerates g (EG code generator, full rank) and H (random) from pk_seed;
  * directly deserializes S from pk.
  *
  * \param[out] g EG code generator vector g(full rank, N1 × N2)
  * \param[out] H Random matrix H (N x N)
  * \param[out] S Public matrix S (N x N1)
  * \param[in] pk Public key byte string
  */
 void cmultiurag_public_key_from_string(rbc_vec g, rbc_mat H, rbc_mat S, const uint8_t* pk) {
   uint8_t pk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
   random_source pk_seedexpander;
   random_source_init(&pk_seedexpander, RANDOM_SOURCE_SEEDEXP);

   rbc_mat_from_string(S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1, pk);

   memcpy(pk_seed, pk + CMULTIURAG_MAT_NN1_BYTES, SEEDEXPANDER_SEED_BYTES);
   random_source_seed_with_len(&pk_seedexpander, pk_seed, SEEDEXPANDER_SEED_BYTES);

   rbc_vec_set_random_full_rank(&pk_seedexpander, g, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
   rbc_mat_set_random(&pk_seedexpander, H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N); 
 
   random_source_clear(&pk_seedexpander);
 }
 
 
 
 /** 
  * \fn void cmultiurag_kem_ciphertext_to_string(uint8_t* ct, const rbc_mat U, const rbc_mat V, const uint8_t* salt) {
  * \brief Serialize the ciphertext to a byte string
  *
  * ct = U_bytes || V_bytes || salt
  *
  * \param[out] ct   Ciphertext byte string
  * \param[in]  U    First ciphertext component matrix U 
  * \param[in]  V    Second ciphertext component matrix V
  * \param[in]  salt Salt value (512 bits)
  */
 void cmultiurag_kem_ciphertext_to_string(uint8_t* ct, const rbc_mat U, const rbc_mat V, const uint8_t* salt) {
   rbc_mat_to_string(ct, U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
   rbc_mat_to_string(ct + CMULTIURAG_MAT_NN2_BYTES, V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
   memcpy(ct + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES, salt, CMULTIURAG_SALT_BYTES);
 }
 
 
 /** 
  * \fn void cmultiurag_kem_ciphertext_from_string(rbc_mat U, rbc_mat V, uint8_t* salt, const uint8_t* ct)
  * \brief Recover U, V and salt from the ciphertext byte string
  *
  * \param[out] U    First ciphertext component matrix U 
  * \param[out] V    Second ciphertext component matrix V
  * \param[out] salt Salt value (512 bits)
  * \param[in]  ct   Ciphertext byte string
  */
 void cmultiurag_kem_ciphertext_from_string(rbc_mat U, rbc_mat V, uint8_t* salt, const uint8_t* ct) {
   rbc_mat_from_string(U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2, ct);
   rbc_mat_from_string(V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2, ct + CMULTIURAG_MAT_NN2_BYTES);
   memcpy(salt, ct + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES, CMULTIURAG_SALT_BYTES);
 }
 
 
