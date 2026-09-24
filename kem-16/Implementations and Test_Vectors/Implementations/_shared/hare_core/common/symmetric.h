/**
 * @file symmetric.h
 * @brief API_PKC-backed symmetric primitive seam for the HARE submission code.
 *
 * The HARE core uses this small call surface for PRNG, XOF and hash operations.
 * In this clean submission package the implementation is bound to the ICCS
 * API_PKC auxiliary functions. Those auxiliary functions are intended for KAT
 * generation and preliminary self-evaluation, matching the NGCC API_PKC usage
 * requirement for submitted public-key implementations.
 */

#ifndef HARE_SYMMETRIC_H
#define HARE_SYMMETRIC_H

#include <stdint.h>
#include "data_structures.h"
#include "parameters.h"

/** Incremental XOF context represented by a seed and consumed-byte counter. */
typedef struct {
    uint8_t seed[SEED_BYTES];
    uint64_t consumed_bytes;
} shake256_xof_ctx;

/** Placeholder SHA3-512 context type retained for the stable HARE call surface. */
typedef struct {
    uint8_t opaque_state[1];
} sha3_512_ctx;

/** Placeholder SHA3-256 context type retained for the stable HARE call surface. */
typedef struct {
    uint8_t opaque_state[1];
} sha3_256_ctx;

/** Domain separator for the PRNG call surface. */
#define HARE_PRNG_DOMAIN 0

/** Domain separator for the XOF call surface. */
#define HARE_XOF_DOMAIN 1

/** Domain separator for the KEM G function. */
#define HARE_G_FCT_DOMAIN 0

/** Domain separator for the public-key hash H function. */
#define HARE_H_FCT_DOMAIN 1

/** Domain separator for the PKE seed-derivation hash I function. */
#define HARE_I_FCT_DOMAIN 2

/** Domain separator for the implicit-rejection hash J function. */
#define HARE_J_FCT_DOMAIN 3

/** Initialize the deterministic PRNG used by tests, KAT generation and benchmarks. */
void prng_init(uint8_t *entropy_input, uint8_t *personalization_string, uint32_t enlen, uint32_t perlen);

/** Fill an output buffer with bytes from the deterministic PRNG. */
void prng_get_bytes(uint8_t *output, uint32_t outlen);

/** Initialize an XOF context from a seed. */
void xof_init(shake256_xof_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size);

/** Squeeze bytes from an initialized XOF context. */
void xof_get_bytes(shake256_xof_ctx *xof_ctx, uint8_t *output, uint32_t output_size);

/** Compute G(H(ek) || m || salt), returning K || theta. */
void hash_g(uint8_t *output, const uint8_t h_ek[SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
            const uint8_t salt[SALT_BYTES]);

/** Compute H(ek), the hash of an encapsulation key. */
void hash_h(uint8_t *output, const uint8_t pk[PUBLIC_KEY_BYTES]);

/** Compute I(seed), the seed splitter used by HARE-PKE key generation. */
void hash_i(uint8_t *output, const uint8_t *seed);

/** Compute J(H(ek) || sigma || c), the implicit-rejection fallback key. */
void hash_j(uint8_t *output, const uint8_t h_ek[SEED_BYTES], const uint8_t sigma[PARAM_SECURITY_BYTES],
            const uint8_t *c_kem);

#endif /* HARE_SYMMETRIC_H */
