#ifndef RHYME_SYMMETRIC_H
#define RHYME_SYMMETRIC_H

#include <stdint.h>
#include "params.h"
#include "rhyme_xof.h"

/* hashing helpers (SM3-based via auxfunc wrappers) */
#define rhyme_shake256_absorb_twice RHYME_NAMESPACE(shake256_absorb_twice)
void rhyme_shake256_absorb_twice(keccak_state *state, const uint8_t *in1,
                                 size_t in1_len, const uint8_t *in2, size_t in2_len);

#define xof256_absorbe_once(STATE, IN, IN_LEN) \
    rhyme_shake256_absorb(STATE, IN, IN_LEN)
#define xof256_absorbe_twice(STATE, IN, IN_LEN, IN2, IN2_LEN) \
    rhyme_shake256_absorb_twice(STATE, IN, IN_LEN, IN2, IN2_LEN)
#define xof256_squeeze(OUT, OUT_LEN, STATE) \
    rhyme_shake256_squeeze(OUT, OUT_LEN, STATE)

#ifdef RHYME_USE_AES
/* ---------------------------------------------------------------
 * Hybrid mode (default on x86-64): AES-256-CTR (AES-NI) replaces the
 * SHAKE XOF for Agen expansion (stream128) and all sampling streams
 * (stream256), mirroring the previous scheme's hybrid design.  SHAKE
 * remains the hash for mu, c_tilde, the w-commitment and the challenge.
 * Sampling seeds are 64-byte CRH outputs; AES keys on the first 32 bytes.
 * --------------------------------------------------------------- */
#include "aes256ctr.h"

typedef aes256ctr_ctx stream128_state;
typedef aes256ctr_ctx stream256_state;
#define STREAM128_BLOCKBYTES AES256CTR_BLOCKBYTES
#define STREAM256_BLOCKBYTES AES256CTR_BLOCKBYTES

#define stream128_init(STATE, SEED, NONCE) aes256ctr_init(STATE, SEED, NONCE)
#define stream128_free(STATE)
#define stream128_squeezeblocks(OUT, OUTBLOCKS, STATE) \
    aes256ctr_squeezeblocks(OUT, OUTBLOCKS, STATE)

#define stream256_init(STATE, SEED, NONCE) aes256ctr_init(STATE, SEED, NONCE)
#define stream256_free(STATE)
#define stream256_squeezeblocks(OUT, OUTBLOCKS, STATE) \
    aes256ctr_squeezeblocks(OUT, OUTBLOCKS, STATE)

#else
/* ---------------------------------------------------------------
 * Pure-SM3 mode (portable): SM3-XOF128 for Agen, SM3-XOF256 for sampling.
 * --------------------------------------------------------------- */
typedef keccak_state stream128_state;
typedef keccak_state stream256_state;
#define STREAM128_BLOCKBYTES SM3_XOF128_RATE
#define STREAM256_BLOCKBYTES SM3_XOF256_RATE

#define rhyme_shake128_stream_init RHYME_NAMESPACE(shake128_stream_init)
void rhyme_shake128_stream_init(keccak_state *state,
                                const uint8_t seed[SEEDBYTES], uint16_t nonce);
#define rhyme_shake256_stream_init RHYME_NAMESPACE(shake256_stream_init)
void rhyme_shake256_stream_init(keccak_state *state,
                                const uint8_t seed[CRHBYTES], uint16_t nonce);

#define stream128_init(STATE, SEED, NONCE) \
    rhyme_shake128_stream_init(STATE, SEED, NONCE)
#define stream128_free(STATE)
#define stream128_squeezeblocks(OUT, OUTBLOCKS, STATE) \
    rhyme_shake128_squeezeblocks(OUT, OUTBLOCKS, STATE)

#define stream256_init(STATE, SEED, NONCE) \
    rhyme_shake256_stream_init(STATE, SEED, NONCE)
#define stream256_free(STATE)
#define stream256_squeezeblocks(OUT, OUTBLOCKS, STATE) \
    rhyme_shake256_squeezeblocks(OUT, OUTBLOCKS, STATE)

#endif /* RHYME_USE_AES */
#endif /* RHYME_SYMMETRIC_H */
