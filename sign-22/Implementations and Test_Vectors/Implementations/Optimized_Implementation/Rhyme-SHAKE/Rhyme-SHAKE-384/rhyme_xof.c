/* Rhyme-specific Hash/XOF wrappers — SHAKE backend (thin veneer over fips202). */
#include "rhyme_xof.h"

/* --- incremental XOF-256 (SHAKE-256) --- */
void rhyme_shake256_init(keccak_state *state) { shake256_init(state); }
void rhyme_shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen) { shake256_absorb(state, in, inlen); }
void rhyme_shake256_finalize(keccak_state *state) { shake256_finalize(state); }
void rhyme_shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state) { shake256_squeeze(out, outlen, state); }
void rhyme_shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state) { shake256_squeezeblocks(out, nblocks, state); }

/* --- one-shot XOF-256 --- */
void rhyme_shake256_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) { shake256(out, outlen, in, inlen); }

/* --- incremental XOF-128 (SHAKE-128) --- */
void rhyme_shake128_init(keccak_state *state) { shake128_init(state); }
void rhyme_shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen) { shake128_absorb(state, in, inlen); }
void rhyme_shake128_finalize(keccak_state *state) { shake128_finalize(state); }
void rhyme_shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state) { shake128_squeezeblocks(out, nblocks, state); }
