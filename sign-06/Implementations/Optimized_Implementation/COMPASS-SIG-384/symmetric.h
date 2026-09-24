#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stdint.h>
#include <stddef.h>
#include "params.h"
#include "auxfunc.h"

#define SYMMETRIC_BUFSZ 512

typedef struct {
  uint8_t  absorb_buf[SYMMETRIC_BUFSZ];
  size_t   absorb_len;
  int      finalized;
  uint8_t *squeeze_buf;
  size_t   squeeze_len;
  size_t   squeeze_pos;
  /* For stream wrappers: pre-generated extra buf */
  uint8_t *stream_extra;
  size_t   stream_extra_len;
  size_t   stream_extra_pos;
} keccak_state;

typedef keccak_state stream128_state;
typedef keccak_state stream256_state;

#define SHAKE128_RATE 168
#define SHAKE256_RATE 136
#define STREAM128_BLOCKBYTES SHAKE128_RATE
#define STREAM256_BLOCKBYTES SHAKE256_RATE

/* ---- SHAKE API (implemented via pseudoXOF/pseudohash) ---- */
void shake128_init(keccak_state *state);
void shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
void shake128_finalize(keccak_state *state);
void shake128_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
void shake128_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen);
void shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state);

void shake256_init(keccak_state *state);
void shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
void shake256_finalize(keccak_state *state);
void shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
void shake256_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen);
void shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state);

void shake128(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);
void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/* Stream wrappers */
void COMPASS_SIG_shake128_stream_init(keccak_state *state,
                                      const uint8_t seed[SEEDBYTES],
                                      uint16_t nonce);
void COMPASS_SIG_shake256_stream_init(keccak_state *state,
                                      const uint8_t seed[CRHBYTES],
                                      uint16_t nonce);

#define stream128_init(STATE, SEED, NONCE) \
        COMPASS_SIG_shake128_stream_init(STATE, SEED, NONCE)
#define stream128_squeezeblocks(OUT, OUTBLOCKS, STATE) \
        shake128_squeezeblocks(OUT, OUTBLOCKS, STATE)
#define stream256_init(STATE, SEED, NONCE) \
        COMPASS_SIG_shake256_stream_init(STATE, SEED, NONCE)
#define stream256_squeezeblocks(OUT, OUTBLOCKS, STATE) \
        shake256_squeezeblocks(OUT, OUTBLOCKS, STATE)

#endif
