/* Rhyme-specific Hash/XOF wrappers — SM3 backend (thin veneer over sm3_xof).
 *
 * Internally these call sm3_xof256_* / sm3_xof128_* which in turn call
 * API_PKC auxfunc's pseudoXOF() (SM3-KDF). */
#include "rhyme_xof.h"

/* --- incremental XOF-256 (SM3-based) --- */
void rhyme_shake256_init(keccak_state *state)       { sm3_xof256_init(state); }
void rhyme_shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen) { sm3_xof256_absorb(state, in, inlen); }
void rhyme_shake256_finalize(keccak_state *state)   { sm3_xof256_finalize(state); }
void rhyme_shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state) { sm3_xof256_squeeze(out, outlen, state); }
void rhyme_shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state) { sm3_xof256_squeeze(out, nblocks * SM3_XOF256_RATE, state); }

/* --- one-shot XOF-256 (SM3-based) --- */
void rhyme_shake256_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) { sm3_xof256(out, outlen, in, inlen); }

/* --- incremental XOF-128 (SM3-based) --- */
void rhyme_shake128_init(keccak_state *state)       { sm3_xof128_init(state); }
void rhyme_shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen) { sm3_xof128_absorb(state, in, inlen); }
void rhyme_shake128_finalize(keccak_state *state)   { sm3_xof128_finalize(state); }
void rhyme_shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state) { sm3_xof128_squeezeblocks(out, nblocks, state); }
