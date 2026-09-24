#ifndef RHYME_SM3_XOF_H
#define RHYME_SM3_XOF_H

#include <stddef.h>
#include <stdint.h>

/* Block rates for SM3-based XOF: 168 bytes for XOF128, 136 bytes for XOF256. */
#define SM3_XOF128_RATE 168
#define SM3_XOF256_RATE 136

/* Backward-compatible aliases for core files that reference SHAKE rate constants. */
#define SHAKE128_RATE SM3_XOF128_RATE
#define SHAKE256_RATE SM3_XOF256_RATE

/* Maximum buffered input for incremental absorb (generous: 8 KiB). */
#define SM3_XOF_INPUT_MAX 8192

/* Maximum cascade buffer: input + 1 tag byte + 4 counter bytes */
#define SM3_XOF_CASCADE_MAX (SM3_XOF_INPUT_MAX + 1 + 4)

typedef struct {
    uint8_t  input[SM3_XOF_INPUT_MAX];
    size_t   inlen;       /* bytes absorbed so far */
    size_t   outpos;      /* output cursor (bytes squeezed so far) */
    uint8_t  *cache;      /* incremental output cache (malloc'd via realloc) */
    size_t   cache_len;   /* bytes currently in cache */
    uint32_t xof_ctr;     /* current SM3-KDF counter (0 = not started, 1-based) */
    uint8_t  mode;        /* 0 = XOF128, 1 = XOF256 */
    uint8_t  finalized;   /* set after finalize / absorb_once */
    uint8_t  overflow;    /* set if input exceeds buffer */
} sm3_xof_state;

/* Free the internal squeeze cache (idempotent). */
void sm3_xof_clear(sm3_xof_state *state);

/* One-shot absorb (typical for stream init: seed + nonce). */
void sm3_xof128_absorb_once(sm3_xof_state *state, const uint8_t *in, size_t inlen);

/* Squeeze nblocks of SM3_XOF128_RATE bytes each. */
void sm3_xof128_squeezeblocks(uint8_t *out, size_t nblocks, sm3_xof_state *state);

/* One-shot XOF256 (for PRF / single-call hash). */
void sm3_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/* Incremental API: init / absorb / finalize / squeeze (for multi-call hashing). */
void sm3_xof256_init(sm3_xof_state *state);
void sm3_xof256_absorb(sm3_xof_state *state, const uint8_t *in, size_t inlen);
void sm3_xof256_finalize(sm3_xof_state *state);
void sm3_xof256_squeeze(uint8_t *out, size_t outlen, sm3_xof_state *state);

/* Streaming absorb (no one-shot: absorb then squeezeblocks). */
void sm3_xof128_init(sm3_xof_state *state);
void sm3_xof128_absorb(sm3_xof_state *state, const uint8_t *in, size_t inlen);
void sm3_xof128_finalize(sm3_xof_state *state);

#endif /* RHYME_SM3_XOF_H */
