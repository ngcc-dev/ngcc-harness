/**
 * @file polarkem_core.c
 * @brief Key serialization, FO transform, and KDFs for PolarKEM-512.
 */
#include "polarkem_core.h"

#include <stddef.h>
#include <string.h>

#include "auxfunc.h"
#include "polarkem_ct.h"

static const unsigned char domain_pkpad[] = "PolarKEM-PKPAD-v1";
static const unsigned char domain_skpad[] = "PolarKEM-SKPAD-v1";
static const unsigned char domain_ss[] = "PolarKEM-SS-v1";
static const unsigned char domain_reject[] = "PolarKEM-REJ-v1";

/**
 * Expand the deterministic public-key padding from a transform seed.
 *
 * @param[out] padding Exactly POLARKEM_PK_PAD_BYTES output bytes.
 * @param[in] seed Exactly POLARKEM_SEED_BYTES transform-seed bytes.
 * @return 0 on success or -4 when the official XOF fails.
 */
static int expand_public_key_padding(
    unsigned char padding[POLARKEM_PK_PAD_BYTES],
    const unsigned char seed[POLARKEM_SEED_BYTES])
{
    unsigned char input[(sizeof(domain_pkpad) - 1U) + POLARKEM_SEED_BYTES];

    memcpy(input, domain_pkpad, sizeof(domain_pkpad) - 1U);
    memcpy(input + sizeof(domain_pkpad) - 1U, seed, POLARKEM_SEED_BYTES);
    if (pseudoXOF((unsigned long long)POLARKEM_PK_PAD_BYTES * 8ULL,
                  input, (unsigned long long)sizeof(input) * 8ULL,
                  padding) != 0) {
        memset(input, 0, sizeof(input));
        return -4;
    }
    memset(input, 0, sizeof(input));
    return 0;
}

/**
 * Expand deterministic secret-key padding from z and SM3(pk).
 *
 * @param[out] padding Exactly POLARKEM_SK_PAD_BYTES output bytes.
 * @param[in] z Exactly POLARKEM_Z_BYTES rejection-seed bytes.
 * @param[in] pk_hash Exactly POLARKEM_HASH_BYTES digest bytes.
 * @return 0 on success or -4 when the official XOF fails.
 */
static int expand_secret_key_padding(
    unsigned char padding[POLARKEM_SK_PAD_BYTES],
    const unsigned char z[POLARKEM_Z_BYTES],
    const unsigned char pk_hash[POLARKEM_HASH_BYTES])
{
    unsigned char input[(sizeof(domain_skpad) - 1U) + POLARKEM_Z_BYTES
                        + POLARKEM_HASH_BYTES];
    size_t offset = 0U;

    memcpy(input + offset, domain_skpad, sizeof(domain_skpad) - 1U);
    offset += sizeof(domain_skpad) - 1U;
    memcpy(input + offset, z, POLARKEM_Z_BYTES);
    offset += POLARKEM_Z_BYTES;
    memcpy(input + offset, pk_hash, POLARKEM_HASH_BYTES);
    if (pseudoXOF((unsigned long long)POLARKEM_SK_PAD_BYTES * 8ULL,
                  input, (unsigned long long)sizeof(input) * 8ULL,
                  padding) != 0) {
        memset(input, 0, sizeof(input));
        return -4;
    }
    memset(input, 0, sizeof(input));
    return 0;
}

/**
 * Hash an exact serialized ciphertext with the official SM3 helper.
 *
 * @param[out] digest Exactly POLARKEM_HASH_BYTES digest bytes.
 * @param[in] ct Exactly POLARKEM_CT_BYTES serialized ciphertext bytes.
 * @return 0 on success or -4 when the official SM3 helper fails.
 */
static int hash_ciphertext(
    unsigned char digest[POLARKEM_HASH_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES])
{
    if (sm3hash(256, ct, (unsigned long long)POLARKEM_CT_BYTES * 8ULL,
                digest) != 0) {
        return -4;
    }
    return 0;
}

/**
 * Derive the valid shared secret from mu and an already-computed SM3(ct).
 *
 * @param[out] ss Exactly POLARKEM_SS_BYTES shared-secret bytes.
 * @param[in] mu Exactly POLARKEM_MU_BYTES message bytes.
 * @param[in] ct_hash Exactly POLARKEM_HASH_BYTES digest bytes.
 * @return 0 on success or -4 when the official XOF fails.
 */
static int derive_valid_secret(
    unsigned char ss[POLARKEM_SS_BYTES],
    const unsigned char mu[POLARKEM_MU_BYTES],
    const unsigned char ct_hash[POLARKEM_HASH_BYTES])
{
    unsigned char input[(sizeof(domain_ss) - 1U) + POLARKEM_MU_BYTES
                        + POLARKEM_HASH_BYTES];
    size_t offset = 0U;

    memcpy(input + offset, domain_ss, sizeof(domain_ss) - 1U);
    offset += sizeof(domain_ss) - 1U;
    memcpy(input + offset, mu, POLARKEM_MU_BYTES);
    offset += POLARKEM_MU_BYTES;
    memcpy(input + offset, ct_hash, POLARKEM_HASH_BYTES);
    if (pseudoXOF((unsigned long long)POLARKEM_SS_BYTES * 8ULL,
                  input, (unsigned long long)sizeof(input) * 8ULL, ss) != 0) {
        memset(input, 0, sizeof(input));
        return -4;
    }
    memset(input, 0, sizeof(input));
    return 0;
}

/**
 * Derive the implicit-rejection secret from z and SM3(received ciphertext).
 *
 * @param[out] ss Exactly POLARKEM_SS_BYTES rejection-secret bytes.
 * @param[in] z Exactly POLARKEM_Z_BYTES rejection-seed bytes.
 * @param[in] ct_hash Exactly POLARKEM_HASH_BYTES digest bytes.
 * @return 0 on success or -4 when the official XOF fails.
 */
static int derive_rejection_secret(
    unsigned char ss[POLARKEM_SS_BYTES],
    const unsigned char z[POLARKEM_Z_BYTES],
    const unsigned char ct_hash[POLARKEM_HASH_BYTES])
{
    unsigned char input[(sizeof(domain_reject) - 1U) + POLARKEM_Z_BYTES
                        + POLARKEM_HASH_BYTES];
    size_t offset = 0U;

    memcpy(input + offset, domain_reject, sizeof(domain_reject) - 1U);
    offset += sizeof(domain_reject) - 1U;
    memcpy(input + offset, z, POLARKEM_Z_BYTES);
    offset += POLARKEM_Z_BYTES;
    memcpy(input + offset, ct_hash, POLARKEM_HASH_BYTES);
    if (pseudoXOF((unsigned long long)POLARKEM_SS_BYTES * 8ULL,
                  input, (unsigned long long)sizeof(input) * 8ULL, ss) != 0) {
        memset(input, 0, sizeof(input));
        return -4;
    }
    memset(input, 0, sizeof(input));
    return 0;
}

int polarkem_validate_public_key(
    const unsigned char pk[POLARKEM_PK_BYTES])
{
    unsigned char expected[POLARKEM_PK_PAD_BYTES];
    int result;

    if (!polarkem_header_is_valid(pk)) {
        return -3;
    }
    result = expand_public_key_padding(expected,
                                       pk + POLARKEM_PK_SEED_OFFSET);
    if (result != 0) {
        return result;
    }
    result = polarkem_constant_time_equal(
        expected, pk + POLARKEM_PK_PAD_OFFSET,
        POLARKEM_PK_PAD_BYTES) ? 0 : -3;
    memset(expected, 0, sizeof(expected));
    return result;
}

int polarkem_validate_secret_key(
    const unsigned char sk[POLARKEM_SK_BYTES])
{
    const unsigned char *pk = sk + POLARKEM_SK_PK_OFFSET;
    unsigned char expected[POLARKEM_SK_PAD_BYTES];
    unsigned char pk_hash[POLARKEM_HASH_BYTES];
    int result;

    if (!polarkem_header_is_valid(sk)) {
        return -3;
    }
    result = polarkem_validate_public_key(pk);
    if (result != 0) {
        return result;
    }
    result = polarkem_hash_public_key(pk_hash, pk);
    if (result != 0) {
        return result;
    }
    result = expand_secret_key_padding(expected, sk + POLARKEM_SK_Z_OFFSET,
                                       pk_hash);
    if (result != 0) {
        memset(pk_hash, 0, sizeof(pk_hash));
        return result;
    }
    result = polarkem_constant_time_equal(
        expected, sk + POLARKEM_SK_PAD_OFFSET,
        POLARKEM_SK_PAD_BYTES) ? 0 : -3;
    memset(expected, 0, sizeof(expected));
    memset(pk_hash, 0, sizeof(pk_hash));
    return result;
}

int polarkem_keygen(unsigned char pk[POLARKEM_PK_BYTES],
                    unsigned char sk[POLARKEM_SK_BYTES],
                    DRNG_ctx *drng)
{
    unsigned char seed[POLARKEM_SEED_BYTES];
    unsigned char z[POLARKEM_Z_BYTES];
    unsigned char pk_hash[POLARKEM_HASH_BYTES];
    int result = -4;

    memset(pk, 0, POLARKEM_PK_BYTES);
    memset(sk, 0, POLARKEM_SK_BYTES);
    if (get_random_number(drng, seed,
                          (unsigned long long)POLARKEM_SEED_BYTES * 8ULL) != 0) {
        goto cleanup;
    }
    if (get_random_number(drng, z,
                          (unsigned long long)POLARKEM_Z_BYTES * 8ULL) != 0) {
        goto cleanup;
    }

    polarkem_write_header(pk);
    memcpy(pk + POLARKEM_PK_SEED_OFFSET, seed, POLARKEM_SEED_BYTES);
    if (expand_public_key_padding(pk + POLARKEM_PK_PAD_OFFSET, seed) != 0) {
        goto cleanup;
    }
    if (polarkem_hash_public_key(pk_hash, pk) != 0) {
        goto cleanup;
    }

    polarkem_write_header(sk);
    memcpy(sk + POLARKEM_SK_Z_OFFSET, z, POLARKEM_Z_BYTES);
    memcpy(sk + POLARKEM_SK_PK_OFFSET, pk, POLARKEM_PK_BYTES);
    if (expand_secret_key_padding(sk + POLARKEM_SK_PAD_OFFSET, z,
                                  pk_hash) != 0) {
        goto cleanup;
    }
    result = 0;

cleanup:
    if (result != 0) {
        memset(pk, 0, POLARKEM_PK_BYTES);
        memset(sk, 0, POLARKEM_SK_BYTES);
    }
    memset(seed, 0, sizeof(seed));
    memset(z, 0, sizeof(z));
    memset(pk_hash, 0, sizeof(pk_hash));
    return result;
}

int polarkem_enc(const unsigned char pk[POLARKEM_PK_BYTES],
                 unsigned char ss[POLARKEM_SS_BYTES],
                 unsigned char ct[POLARKEM_CT_BYTES],
                 DRNG_ctx *drng)
{
    unsigned char mu[POLARKEM_MU_BYTES];
    unsigned char ct_hash[POLARKEM_HASH_BYTES];
    int result = -4;

    memset(ss, 0, POLARKEM_SS_BYTES);
    memset(ct, 0, POLARKEM_CT_BYTES);
    if (get_random_number(drng, mu,
                          (unsigned long long)POLARKEM_MU_BYTES * 8ULL) != 0) {
        goto cleanup;
    }
    if (polarkem_build_ciphertext(ct, pk, mu) != 0) {
        goto cleanup;
    }
    if (hash_ciphertext(ct_hash, ct) != 0) {
        goto cleanup;
    }
    if (derive_valid_secret(ss, mu, ct_hash) != 0) {
        goto cleanup;
    }
    result = 0;

cleanup:
    if (result != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        memset(ct, 0, POLARKEM_CT_BYTES);
    }
    memset(mu, 0, sizeof(mu));
    memset(ct_hash, 0, sizeof(ct_hash));
    return result;
}

int polarkem_dec(const unsigned char sk[POLARKEM_SK_BYTES],
                 const unsigned char ct[POLARKEM_CT_BYTES],
                 unsigned char ss[POLARKEM_SS_BYTES])
{
    const unsigned char *z = sk + POLARKEM_SK_Z_OFFSET;
    const unsigned char *pk = sk + POLARKEM_SK_PK_OFFSET;
    unsigned char mu[POLARKEM_MU_BYTES];
    unsigned char recomputed[POLARKEM_CT_BYTES];
    unsigned char valid_secret[POLARKEM_SS_BYTES];
    unsigned char rejection_secret[POLARKEM_SS_BYTES];
    unsigned char ct_hash[POLARKEM_HASH_BYTES];
    unsigned char selection_mask;
    size_t i;
    int valid;

    memset(ss, 0, POLARKEM_SS_BYTES);
    if (polarkem_recover_and_reencrypt(mu, recomputed, pk, ct) != 0) {
        goto helper_failure;
    }
    if (hash_ciphertext(ct_hash, ct) != 0) {
        goto helper_failure;
    }
    if (derive_valid_secret(valid_secret, mu, ct_hash) != 0) {
        goto helper_failure;
    }
    if (derive_rejection_secret(rejection_secret, z, ct_hash) != 0) {
        goto helper_failure;
    }

    valid = polarkem_header_is_valid(ct)
            & polarkem_constant_time_equal(ct, recomputed, POLARKEM_CT_BYTES);
    selection_mask = (unsigned char)(0U - (unsigned int)(valid != 0));
    for (i = 0U; i < POLARKEM_SS_BYTES; ++i) {
        ss[i] = (unsigned char)((valid_secret[i] & selection_mask)
                               | (rejection_secret[i]
                                  & (unsigned char)~selection_mask));
    }

    memset(mu, 0, sizeof(mu));
    memset(recomputed, 0, sizeof(recomputed));
    memset(valid_secret, 0, sizeof(valid_secret));
    memset(rejection_secret, 0, sizeof(rejection_secret));
    memset(ct_hash, 0, sizeof(ct_hash));
    return valid != 0 ? 0 : -1;

helper_failure:
    memset(ss, 0, POLARKEM_SS_BYTES);
    memset(mu, 0, sizeof(mu));
    memset(recomputed, 0, sizeof(recomputed));
    memset(valid_secret, 0, sizeof(valid_secret));
    memset(rejection_secret, 0, sizeof(rejection_secret));
    memset(ct_hash, 0, sizeof(ct_hash));
    return -4;
}

