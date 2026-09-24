/*
 * DKE-specific SM3 wrappers.
 *
 * Provides mode-aware convenience functions that use the correct
 * SEEDBYTES / SSBYTES for each DKE parameter set (128, 256, 512).
 *
 * DKE-128/256: SEEDBYTES=32, SSBYTES=32
 * DKE-512:     SEEDBYTES=64, SSBYTES=64
 */
#ifndef DKE_SM3_WRAPPER_H
#define DKE_SM3_WRAPPER_H

#include "parameters.h"
#include "sm3.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Hash a DKE seed (SEEDBYTES long) to produce a 32-byte digest.
 */
static inline void dke_hash_seed(const uint8_t seed[DKE_SEEDBYTES],
                                 uint8_t digest[SM3_DIGEST_BYTES]) {
    dke_sm3_hash_bytes(seed, DKE_SEEDBYTES, digest);
}

/*
 * Hash a DKE public key (PKBYTES long) to produce a 32-byte digest.
 * NOTE: This is the SM3-specific inline version. The generic dke_hash_pk()
 * in dke_hash.h/c supersedes this for all DKE_HASH modes.
 */
static inline void dke_sm3_hash_pk(const uint8_t pk[DKE_PKBYTES],
                                    uint8_t digest[SM3_DIGEST_BYTES]) {
    dke_sm3_hash_bytes(pk, DKE_PKBYTES, digest);
}

/*
 * Hash seed || nonce (SEEDBYTES + 1 byte) for secret/error sampling.
 * Used by DKE_getsecretA/B, DKE_geterrorA/B.
 */
static inline void dke_hash_seed_nonce(const uint8_t seed[DKE_SEEDBYTES],
                                       uint8_t nonce,
                                       uint8_t digest[SM3_DIGEST_BYTES]) {
    uint8_t buf[DKE_SEEDBYTES + 1];
    memcpy(buf, seed, DKE_SEEDBYTES);
    buf[DKE_SEEDBYTES] = nonce;
    dke_sm3_hash_bytes(buf, DKE_SEEDBYTES + 1, digest);
}

/*
 * Hash extseed (SEEDBYTES + 2 bytes) for matrix generation XOF.
 * Used by dke_xof_squeezeblocks via pseudoXOF.
 */
static inline void dke_hash_extseed(const uint8_t extseed[DKE_SEEDBYTES + 2],
                                    uint8_t digest[SM3_DIGEST_BYTES]) {
    dke_sm3_hash_bytes(extseed, DKE_SEEDBYTES + 2, digest);
}

/*
 * HMAC-SM3 with the DKE fixed key.
 * The key is defined in auxfunc.c as SM3("3.14159265358979") || SM3("2.71828182845904").
 * This wrapper is for use in pseudohash_512/768/1024.
 */
int dke_hmac_with_fixed_key(const unsigned char *msg,
                            unsigned long long msg_len_bits,
                            unsigned char *mac);

#ifdef __cplusplus
}
#endif

#endif /* DKE_SM3_WRAPPER_H */
