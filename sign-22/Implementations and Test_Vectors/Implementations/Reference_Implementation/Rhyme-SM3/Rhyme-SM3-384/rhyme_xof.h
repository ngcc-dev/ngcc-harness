#ifndef RHYME_XOF_H
#define RHYME_XOF_H

#include <stddef.h>
#include <stdint.h>

/*
 * Rhyme-specific Hash/XOF wrappers — SM3 backend.
 *
 * These thin wrappers route all hash/XOF traffic through SM3-based XOF
 * (pseudoXOF from auxfunc, via sm3_xof) and exist solely so that core
 * algorithm files do not directly call fips202 or include fips202.h.
 *
 * This file is NOT part of the API_PKC no-modify set; it is a Rhyme-specific
 * addition to the auxiliary-function layer.
 */
#include "sm3_xof.h"

/* Backward compatibility: core files use keccak_state as the opaque
 * hash-state type.  Under SM3 we alias it to sm3_xof_state. */
typedef sm3_xof_state keccak_state;

/* Backward compatibility: SHAKE rate constants used by sampler.c. */
#define SHAKE128_RATE SM3_XOF128_RATE
#define SHAKE256_RATE SM3_XOF256_RATE

#ifdef __cplusplus
extern "C" {
#endif

/* --- incremental XOF-256 (SM3-based) --- */
void rhyme_shake256_init(keccak_state *state);
void rhyme_shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
void rhyme_shake256_finalize(keccak_state *state);
void rhyme_shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
void rhyme_shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state);

/* --- one-shot XOF-256 (SM3-based) --- */
void rhyme_shake256_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/* --- incremental XOF-128 (SM3-based, for stream init) --- */
void rhyme_shake128_init(keccak_state *state);
void rhyme_shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
void rhyme_shake128_finalize(keccak_state *state);
void rhyme_shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state);

#ifdef __cplusplus
}
#endif

#endif /* RHYME_XOF_H */
