/** @file
 *
 * @brief The verification protocol
 */

#ifndef VERIFICATION_H
#define VERIFICATION_H

#include <sqisign_namespace.h>
#include <ec.h>
#include <intbig.h>

/** @defgroup verification SQIsignHD verification protocol
 * @{
 */

/** @defgroup verification_t Types for SQIsignHD verification protocol
 * @{
 */

typedef digit_t scalar_t[NWORDS_ORDER];

/** @brief Type for the new signature
 *
 * @typedef new_signature_t
 *
 * @struct new_signature
 *
 */
typedef struct new_signature
{
    fp2_t E_aux_A; // the Montgomery A-coefficient for the auxiliary curve
    ec_basis_t B_aux;
    scalar_t q;
} new_signature_t;

/** @brief Type for the public keys
 *
 * @typedef public_key_t
 *
 * @struct public_key
 *
 */
typedef struct public_key
{
    ec_curve_t curve; // the normalized A-coefficient of the Montgomery curve
    uint8_t hint_pk;
} public_key_t;

/** @}
 */

/*************************** Functions *****************************/

void public_key_init(public_key_t *pk);
void public_key_finalize(public_key_t *pk);

void hash_to_challenge_prime_with_pk_j(ibz_t *c1,
                                       ibz_t *c2,
                                       const fp2_t *pk_j_inv,
                                       const ec_curve_t *com_curve,
                                       const unsigned char *message,
                                       size_t length);

/**
 * @brief SQIsignTriangle verification
 *
 * @param sig signature
 * @param pk public key
 * @param m message
 * @param l size
 * @returns 1 if the signature verifies, 0 otherwise
 */
int new_protocols_verify(new_signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l);

/*************************** Encoding *****************************/

/** @defgroup encoding Encoding and decoding functions
 * @{
 */

/**
 * @brief Encodes a SQIsignTriangle signature as a byte array
 *
 * @param enc : Byte array to encode the signature in
 * @param sig : SQIsignTriangle signature to encode
 */
void new_signature_to_bytes(unsigned char *enc, const new_signature_t *sig);

/**
 * @brief Decodes a SQIsignTriangle signature from a byte array
 *
 * @param sig : Structure to decode the SQIsignTriangle signature in
 * @param enc : Byte array to decode
 */
void new_signature_from_bytes(new_signature_t *sig, const unsigned char *enc);

/**
 * @brief Encodes a public key as a byte array
 *
 * @param enc : Byte array to encode the public key in
 * @param pk : Public key to encode
 */
unsigned char *public_key_to_bytes(unsigned char *enc, const public_key_t *pk);

/**
 * @brief Decodes a public key from a byte array
 *
 * @param pk : Structure to decode the public key in
 * @param enc : Byte array to decode
 */
const unsigned char *public_key_from_bytes(public_key_t *pk, const unsigned char *enc);

/** @}
 */

/** @}
 */

#endif
