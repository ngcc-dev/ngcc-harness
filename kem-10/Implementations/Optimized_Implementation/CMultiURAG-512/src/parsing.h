/** 
 * \file parsing.h
 * \brief Key and ciphertext serialization/deserialization function declarations for the CMultiURAG scheme
 *
 * Provides conversion between byte strings and internal data structures for
 * public keys, secret keys, and ciphertexts.
 * The PKE secret key prefix is serialized as sk = sk_seed || pk.
 * The KEM layer extends it to sk = sk_seed || pk || sigma.
 * Public key pk = S || pk_seed (matrix S + public key seed).
 * Ciphertext ct = U || V || salt.
 */

 #ifndef CMULTIURAG_PARSING_H
 #define CMULTIURAG_PARSING_H
 
 #include "rbc_mat.h"
 
 
 void cmultiurag_secret_key_to_string(uint8_t* sk, const uint8_t* seed, const uint8_t* pk);
 void cmultiurag_secret_key_from_string(rbc_mat X, rbc_mat Y, uint8_t* pk, const uint8_t* sk);
 
 void cmultiurag_public_key_to_string(uint8_t* pk, const rbc_mat S, const uint8_t* seed);
 void cmultiurag_public_key_from_string(rbc_vec g, rbc_mat H, rbc_mat S, const uint8_t* pk);
 
 void cmultiurag_kem_ciphertext_to_string(uint8_t* ct, const rbc_mat U, const rbc_mat V, const uint8_t* salt);
 void cmultiurag_kem_ciphertext_from_string(rbc_mat U, rbc_mat V, uint8_t* salt, const uint8_t* ct);
 #endif
 
 
