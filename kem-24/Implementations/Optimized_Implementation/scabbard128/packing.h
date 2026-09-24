#ifndef PACKING_H
#define PACKING_H   

#include <stdint.h>
#include "params.h"
#include "polyvec.h"

/// @brief Serialize the public key
/// @param[out] pk Serialized public key
/// @param[in] pkpv Public key polynomials
/// @param[in] seed Public seed for generating the public matrix A
void pack_pk(
    uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    const polyvec *pkpv, 
    const uint8_t seed[SCABBARD_SYMBYTES]);

/// @brief De-serialize the public key
/// @param[out] pkpv Public key vector of polynomials
/// @param[out] seed Public seed for generating the public matrix A
/// @param[in] pk Serialized public key
void unpack_pk(
    polyvec *pkpv, 
    uint8_t seed[SCABBARD_SYMBYTES], 
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES]);

/// @brief Serialize the secret key
/// @param[out] sk Serialized secret key (SCABBARD_B bits per coefficient)
/// @param[in] skpv Vector of secret key polynomials
void pack_sk(
    uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES], 
    const polyvec *skpv);

/// @brief De-serialize the secret key
/// @param[out] skpv Vector of secret key polynomials
/// @param[in] sk Serialized secret key (SCABBARD_B bits per coefficient)
void unpack_sk(
    polyvec *skpv, 
    const uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES]);

/// @brief Serialize the ciphertext (u, v)
/// @param[out] ct Serialized ciphertext
/// @param[in] u Vector of polynomials u
/// @param[in] v Polynomial v
void pack_ciphertext(
    uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const polyvec *u, 
    const poly *v);

/// @brief De-serialize the ciphertext
/// @param[out] u Vector of polynomials u
/// @param[out] v Polynomial v
/// @param[in] ct Serialized ciphertext
void unpack_ciphertext(
    polyvec *u, 
    poly *v, 
    const uint8_t ct[SCABBARD_INDCPA_BYTES]);


#endif