/**
 * @file polarkem_ct.h
 * @brief Deterministic PolarKEM-256 ciphertext construction and recovery.
 */
#ifndef POLARKEM_CT_H
#define POLARKEM_CT_H

#include <stddef.h>
#include <stdint.h>

#include "polarkem_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write the canonical 16-byte PolarKEM-256 object header.
 *
 * @param[out] object Writable object with at least POLARKEM_HEADER_BYTES bytes.
 * @return Nothing; all header bytes are initialized.
 */
void polarkem_write_header(unsigned char object[POLARKEM_HEADER_BYTES]);

/**
 * Validate a canonical PolarKEM-256 object header.
 *
 * @param[in] object Readable object with at least POLARKEM_HEADER_BYTES bytes.
 * @return 1 if magic, instance id, and reserved bytes match; otherwise 0.
 */
int polarkem_header_is_valid(
    const unsigned char object[POLARKEM_HEADER_BYTES]);

/**
 * Hash a serialized public key with the official SM3 helper.
 *
 * @param[out] digest Exactly POLARKEM_HASH_BYTES output bytes.
 * @param[in] pk Exactly POLARKEM_PK_BYTES serialized public-key bytes.
 * @return 0 on success or -4 if the official helper fails.
 */
int polarkem_hash_public_key(
    unsigned char digest[POLARKEM_HASH_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES]);

/**
 * Expand a transform seed into the canonical unbiased signed permutation.
 *
 * @param[out] permutation POLARKEM_N source indices, one for each destination.
 * @param[out] signs POLARKEM_N values, each exactly +1 or -1.
 * @param[in] seed Exactly POLARKEM_SEED_BYTES public transform-seed bytes.
 * @return 0 on success or -4 if the official XOF fails.
 */
int polarkem_signed_permutation(
    uint16_t permutation[POLARKEM_N],
    int8_t signs[POLARKEM_N],
    const unsigned char seed[POLARKEM_SEED_BYTES]);

/**
 * Deterministically construct a complete ciphertext from pk and mu.
 *
 * @param[out] ct Exactly POLARKEM_CT_BYTES initialized ciphertext bytes.
 * @param[in] pk Exactly POLARKEM_PK_BYTES serialized public-key bytes.
 * @param[in] mu Exactly POLARKEM_MU_BYTES encapsulated-message bytes.
 * @return 0 on success or -4 if an official hash/XOF helper fails.
 */
int polarkem_build_ciphertext(
    unsigned char ct[POLARKEM_CT_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char mu[POLARKEM_MU_BYTES]);

/**
 * Recover the candidate message from a ciphertext payload.
 *
 * Header/tag/padding validation is deliberately deferred to full FO
 * re-encryption and comparison in the core decapsulation function.
 *
 * @param[out] mu Exactly POLARKEM_MU_BYTES recovered candidate bytes.
 * @param[in] pk Exactly POLARKEM_PK_BYTES serialized public-key bytes.
 * @param[in] ct Exactly POLARKEM_CT_BYTES ciphertext bytes.
 * @return 0 on success or -4 if the official XOF fails.
 */
int polarkem_recover_message(
    unsigned char mu[POLARKEM_MU_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES]);

/**
 * Recover a candidate message and rebuild its ciphertext with one permutation.
 *
 * This decapsulation-specific combined path expands the public signed
 * permutation once, reuses it for inverse recovery and forward re-encryption,
 * and keeps all cached state local to the call.  It is thread-safe and exactly
 * byte-compatible with separate recover/build calls.
 *
 * @param[out] mu Exactly POLARKEM_MU_BYTES recovered candidate bytes.
 * @param[out] rebuilt Exactly POLARKEM_CT_BYTES deterministic ciphertext bytes.
 * @param[in] pk Exactly POLARKEM_PK_BYTES serialized public-key bytes.
 * @param[in] received Exactly POLARKEM_CT_BYTES received ciphertext bytes.
 * @return 0 on success or -4 if an official hash/XOF helper fails.
 */
int polarkem_recover_and_reencrypt(
    unsigned char mu[POLARKEM_MU_BYTES],
    unsigned char rebuilt[POLARKEM_CT_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char received[POLARKEM_CT_BYTES]);

/**
 * Compare two byte strings without data-dependent early exit.
 *
 * @param[in] a First readable byte string.
 * @param[in] b Second readable byte string.
 * @param length Number of bytes to compare.
 * @return 1 if all bytes match, otherwise 0.
 */
int polarkem_constant_time_equal(const unsigned char *a,
                                 const unsigned char *b,
                                 size_t length);

#ifdef __cplusplus
}
#endif

#endif /* POLARKEM_CT_H */
