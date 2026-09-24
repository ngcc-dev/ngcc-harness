/**
 * @file polarkem_core.h
 * @brief Core functional PolarKEM-512 operations used by the official API.
 */
#ifndef POLARKEM_CORE_H
#define POLARKEM_CORE_H

#include "drng.h"
#include "polarkem_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Validate the full deterministic serialization of a public key.
 *
 * @param[in] pk Exactly POLARKEM_PK_BYTES public-key bytes.
 * @return 0 if valid, -3 for malformed input, or -4 for helper failure.
 */
int polarkem_validate_public_key(
    const unsigned char pk[POLARKEM_PK_BYTES]);

/**
 * Validate the full deterministic serialization of a secret key and embedded pk.
 *
 * @param[in] sk Exactly POLARKEM_SK_BYTES secret-key bytes.
 * @return 0 if valid, -3 for malformed input, or -4 for helper failure.
 */
int polarkem_validate_secret_key(
    const unsigned char sk[POLARKEM_SK_BYTES]);

/**
 * Generate one public/secret-key pair using the official algorithm DRNG.
 *
 * Randomness is consumed as two sequential 256-bit calls: transform seed then
 * rejection seed.  Output lengths are fixed by polarkem_params.h.
 *
 * @param[out] pk Exactly POLARKEM_PK_BYTES public-key bytes.
 * @param[out] sk Exactly POLARKEM_SK_BYTES secret-key bytes.
 * @param[in,out] drng Initialized official algorithm DRNG context.
 * @return 0 on success or -4 on official DRNG/hash/XOF failure.
 */
int polarkem_keygen(unsigned char pk[POLARKEM_PK_BYTES],
                    unsigned char sk[POLARKEM_SK_BYTES],
                    DRNG_ctx *drng);

/**
 * Encapsulate to a validated public key using the official algorithm DRNG.
 *
 * @param[in] pk Exactly POLARKEM_PK_BYTES validated public-key bytes.
 * @param[out] ss Exactly POLARKEM_SS_BYTES shared-secret bytes.
 * @param[out] ct Exactly POLARKEM_CT_BYTES ciphertext bytes.
 * @param[in,out] drng Initialized official algorithm DRNG context.
 * @return 0 on success or -4 on official DRNG/hash/XOF failure.
 */
int polarkem_enc(const unsigned char pk[POLARKEM_PK_BYTES],
                 unsigned char ss[POLARKEM_SS_BYTES],
                 unsigned char ct[POLARKEM_CT_BYTES],
                 DRNG_ctx *drng);

/**
 * Decapsulate with full deterministic re-encryption and implicit rejection.
 *
 * @param[in] sk Exactly POLARKEM_SK_BYTES validated secret-key bytes.
 * @param[in] ct Exactly POLARKEM_CT_BYTES ciphertext bytes.
 * @param[out] ss Exactly POLARKEM_SS_BYTES valid or rejection-secret bytes.
 * @return 0 for a valid ciphertext, -1 for implicit rejection, or -4 on an
 *         official hash/XOF helper failure.  A -1 result still writes full ss.
 */
int polarkem_dec(const unsigned char sk[POLARKEM_SK_BYTES],
                 const unsigned char ct[POLARKEM_CT_BYTES],
                 unsigned char ss[POLARKEM_SS_BYTES]);

#ifdef __cplusplus
}
#endif

#endif /* POLARKEM_CORE_H */

