#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "symmetric.h"
#include "auxfunc.h"

/* ------------------------------------------------------------------ */
/* Internal: call pseudoXOF to fill the squeeze buffer                 */
/* ------------------------------------------------------------------ */
static int fill_squeeze(keccak_state *state, size_t min_bytes)
{
  /* If buffer is already big enough, just reuse */
  if (state->squeeze_buf && state->squeeze_len >= min_bytes)
    return 0;

  /* Round up to next multiple of 256 bits (32 bytes) */
  size_t want = (min_bytes + 31) & ~31UL;
  if (want < 128) want = 128;                     /* floor */

  uint8_t *nb = realloc(state->squeeze_buf, want);
  if (!nb) return -1;
  state->squeeze_buf = nb;
  state->squeeze_len  = want;

  pseudoXOF(want * 8,
            state->absorb_buf, state->absorb_len * 8,
            state->squeeze_buf);
  state->squeeze_pos = 0;
  return 0;
}

/* ------------------------------------------------------------------ */
/* Internal: free dynamically-allocated state members                  */
/* ------------------------------------------------------------------ */
static void state_cleanup(keccak_state *state)
{
  if (state->squeeze_buf) { free(state->squeeze_buf); state->squeeze_buf = NULL; }
  if (state->stream_extra) { free(state->stream_extra); state->stream_extra = NULL; }
}

/* ------------------------------------------------------------------ */
/* SHAKE128                                                            */
/* ------------------------------------------------------------------ */
void shake128_init(keccak_state *state)
{
  state->absorb_len = 0;
  state->finalized  = 0;
  state->squeeze_buf = NULL;
  state->squeeze_len = 0;
  state->squeeze_pos = 0;
  state->stream_extra = NULL;
  state->stream_extra_len = 0;
  state->stream_extra_pos = 0;
}

void shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen)
{
  while (inlen) {
    size_t room = sizeof(state->absorb_buf) - state->absorb_len;
    size_t take = inlen < room ? inlen : room;
    memcpy(state->absorb_buf + state->absorb_len, in, take);
    state->absorb_len += take;
    in    += take;
    inlen -= take;
    if (room == 0) {
      /* absorb buffer full — call pseudoXOF to compress, then reset */
      uint8_t tmp[32];
      pseudoXOF(256, state->absorb_buf, state->absorb_len * 8, tmp);
      memcpy(state->absorb_buf, tmp, 32);
      state->absorb_len = 32;
    }
  }
}

void shake128_finalize(keccak_state *state)
{
  state->finalized = 1;
}

void shake128_squeeze(uint8_t *out, size_t outlen, keccak_state *state)
{
  if (state->stream_extra) {
    /* Reading from pre-generated stream buffer */
    while (outlen) {
      size_t avail = state->stream_extra_len - state->stream_extra_pos;
      if (avail == 0) {
        /* Need more — regenerate with offset */
        uint8_t ctr[4];
        uint32_t ct = (uint32_t)(state->stream_extra_len / 32) + 1;
        ctr[0] = ct >> 24; ctr[1] = ct >> 16;
        ctr[2] = ct >> 8;  ctr[3] = ct & 0xff;

        /* Build new input: absorb_buf || ctr */
        size_t alen = state->absorb_len;
        uint8_t *new_in = malloc(alen + 4);
        memcpy(new_in, state->absorb_buf, alen);
        memcpy(new_in + alen, ctr, 4);

        size_t extra = 256; /* 32 bytes */
        uint8_t *nb = realloc(state->stream_extra,
                               state->stream_extra_len + 32);
        state->stream_extra = nb;
        pseudoXOF(256, new_in, (alen + 4) * 8,
                   state->stream_extra + state->stream_extra_len);
        state->stream_extra_len += 32;
        free(new_in);
        avail = 32;
      }
      size_t take = outlen < avail ? outlen : avail;
      memcpy(out, state->stream_extra + state->stream_extra_pos, take);
      out    += take;
      outlen -= take;
      state->stream_extra_pos += take;
    }
    return;
  }

  fill_squeeze(state, state->squeeze_pos + outlen);
  memcpy(out, state->squeeze_buf + state->squeeze_pos, outlen);
  state->squeeze_pos += outlen;
}

void shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state)
{
  shake128_squeeze(out, nblocks * SHAKE128_RATE, state);
}

void shake128_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen)
{
  shake128_init(state);
  shake128_absorb(state, in, inlen);
  shake128_finalize(state);
}

/* ------------------------------------------------------------------ */
/* SHAKE256                                                            */
/* ------------------------------------------------------------------ */
void shake256_init(keccak_state *state)
{
  state->absorb_len = 0;
  state->finalized  = 0;
  state->squeeze_buf = NULL;
  state->squeeze_len = 0;
  state->squeeze_pos = 0;
  state->stream_extra = NULL;
  state->stream_extra_len = 0;
  state->stream_extra_pos = 0;
}

void shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen)
{
  while (inlen) {
    size_t room = sizeof(state->absorb_buf) - state->absorb_len;
    size_t take = inlen < room ? inlen : room;
    memcpy(state->absorb_buf + state->absorb_len, in, take);
    state->absorb_len += take;
    in    += take;
    inlen -= take;
    if (room == 0) {
      uint8_t tmp[32];
      pseudoXOF(256, state->absorb_buf, state->absorb_len * 8, tmp);
      memcpy(state->absorb_buf, tmp, 32);
      state->absorb_len = 32;
    }
  }
}

void shake256_finalize(keccak_state *state)
{
  state->finalized = 1;
}

void shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state)
{
  if (state->stream_extra) {
    while (outlen) {
      size_t avail = state->stream_extra_len - state->stream_extra_pos;
      if (avail == 0) {
        uint8_t ctr[4];
        uint32_t ct = (uint32_t)(state->stream_extra_len / 32) + 1;
        ctr[0] = ct >> 24; ctr[1] = ct >> 16;
        ctr[2] = ct >> 8;  ctr[3] = ct & 0xff;
        size_t alen = state->absorb_len;
        uint8_t *new_in = malloc(alen + 4);
        memcpy(new_in, state->absorb_buf, alen);
        memcpy(new_in + alen, ctr, 4);
        uint8_t *nb = realloc(state->stream_extra,
                               state->stream_extra_len + 32);
        state->stream_extra = nb;
        pseudoXOF(256, new_in, (alen + 4) * 8,
                   state->stream_extra + state->stream_extra_len);
        state->stream_extra_len += 32;
        free(new_in);
        avail = 32;
      }
      size_t take = outlen < avail ? outlen : avail;
      memcpy(out, state->stream_extra + state->stream_extra_pos, take);
      out    += take;
      outlen -= take;
      state->stream_extra_pos += take;
    }
    return;
  }

  fill_squeeze(state, state->squeeze_pos + outlen);
  memcpy(out, state->squeeze_buf + state->squeeze_pos, outlen);
  state->squeeze_pos += outlen;
}

void shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state)
{
  shake256_squeeze(out, nblocks * SHAKE256_RATE, state);
}

void shake256_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen)
{
  shake256_init(state);
  shake256_absorb(state, in, inlen);
  shake256_finalize(state);
}

/* ------------------------------------------------------------------ */
/* One-shot API                                                       */
/* ------------------------------------------------------------------ */
void shake128(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
  keccak_state st;
  shake128_absorb_once(&st, in, inlen);
  shake128_squeeze(out, outlen, &st);
  state_cleanup(&st);
}

void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
  keccak_state st;
  shake256_absorb_once(&st, in, inlen);
  shake256_squeeze(out, outlen, &st);
  state_cleanup(&st);
}

/* ------------------------------------------------------------------ */
/* Stream wrappers — absorb seed||nonce, then pre-gen output           */
/* ------------------------------------------------------------------ */
void COMPASS_SIG_shake128_stream_init(keccak_state *state,
                                      const uint8_t seed[SEEDBYTES],
                                      uint16_t nonce)
{
  uint8_t t[2];
  t[0] = nonce;
  t[1] = nonce >> 8;

  shake128_init(state);
  shake128_absorb(state, seed, SEEDBYTES);
  shake128_absorb(state, t, 2);
  shake128_finalize(state);

  /* Pre-generate 2048 bytes (more than enough for all N=256 use cases) */
  state->stream_extra_len = 2048;
  state->stream_extra = malloc(state->stream_extra_len);
  state->stream_extra_pos = 0;
  pseudoXOF(state->stream_extra_len * 8,
            state->absorb_buf, state->absorb_len * 8,
            state->stream_extra);
}

void COMPASS_SIG_shake256_stream_init(keccak_state *state,
                                      const uint8_t seed[CRHBYTES],
                                      uint16_t nonce)
{
  uint8_t t[2];
  t[0] = nonce;
  t[1] = nonce >> 8;

  shake256_init(state);
  shake256_absorb(state, seed, CRHBYTES);
  shake256_absorb(state, t, 2);
  shake256_finalize(state);

  /* Pre-generate 1024 bytes */
  state->stream_extra_len = 1024;
  state->stream_extra = malloc(state->stream_extra_len);
  state->stream_extra_pos = 0;
  pseudoXOF(state->stream_extra_len * 8,
            state->absorb_buf, state->absorb_len * 8,
            state->stream_extra);
}
