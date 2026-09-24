// SPDX-License-Identifier: Apache-2.0
//
// API_PKC / NGCC submission adapter.
//
// This file REPLACES the original Keccak-based FIPS-202 implementation of
// SQIsignTriangle.  The submission requirements of the Institute of Commercial
// Cryptography Standards (ICCS) mandate (see API_PKC README, Note 5, and
// 公钥密码算法提交要求 §3.4(5)) that the cryptographic hash and eXtendable-Output
// Function (XOF) used by an implementation be provided by the ICCS auxiliary
// functions (auxfunc.c):
//
//     int sm3hash   (int len_bits, const uint8_t *msg, uint64_t msg_bits, uint8_t *out);
//     int pseudohash(int len_bits, const uint8_t *msg, uint64_t msg_bits, uint8_t *out);
//     int pseudoXOF (uint64_t out_bits, const uint8_t *msg, uint64_t msg_bits, uint8_t *out);
//
// We keep the exact FIPS-202 function signatures declared in <fips202.h> so that
// NO calling code in the scheme has to change; only the underlying primitive is
// swapped from SHAKE/SHA3 (Keccak) to the SM3-based auxiliary functions.
//
// Why this is a faithful, deterministic drop-in
// ----------------------------------------------
// pseudoXOF is the GB/T 32918.4 (SM2) key-derivation function in counter mode:
//
//     stream = SM3(msg ‖ 0x00000001) ‖ SM3(msg ‖ 0x00000002) ‖ ...
//
// truncated to the requested length.  The i-th 32-byte block depends only on
// (msg, i); it is independent of the total requested length.  Therefore the
// stream is PREFIX-CONSISTENT at byte granularity:
//
//     pseudoXOF(N bytes, msg) == the first N bytes of pseudoXOF(M bytes, msg)   (N <= M)
//
// This lets us emulate the SHAKE sponge (absorb-everything, then squeeze an
// unbounded byte stream — possibly across several squeeze calls) exactly:
// absorb buffers the input, and each squeeze regenerates the prefix it needs
// and returns the next slice.  The result is fully deterministic, so the
// known-answer test (KAT) vectors are reproducible.
//
// The scheme (src/verification/ref/lvlx/common.c, hash_to_challenge_*) uses ONLY
// the SHAKE256 incremental API.  The remaining SHAKE128 / SHA3-256/384/512
// entry points are unused by the scheme; they are provided here for ABI/link
// completeness and are mapped onto the same auxiliary functions.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fips202.h"
#include "auxfunc.h"

/* ------------------------------------------------------------------------- */
/* Sponge emulation state                                                    */
/*                                                                           */
/* The opaque FIPS-202 context types in <fips202.h> are uint64_t arrays.  We */
/* store a single heap pointer to our real state in the first machine word   */
/* of that storage (read/written with memcpy to stay strict-aliasing-safe).  */
/* ------------------------------------------------------------------------- */

typedef struct {
    unsigned char *buf; /* absorbed input                              */
    size_t len;         /* number of absorbed bytes                    */
    size_t cap;         /* allocated capacity of buf                   */
    size_t squeezed;    /* number of output bytes already produced     */
} xof_state;

static xof_state *xs_get(const void *ctx)
{
    xof_state *p;
    memcpy(&p, ctx, sizeof(p));
    return p;
}

static void xs_set(void *ctx, xof_state *p)
{
    memcpy(ctx, &p, sizeof(p));
}

static xof_state *xs_new(void)
{
    xof_state *p = (xof_state *)calloc(1, sizeof(xof_state));
    return p; /* buf == NULL, len == cap == squeezed == 0 */
}

static void xs_absorb(xof_state *p, const uint8_t *in, size_t inlen)
{
    if (p == NULL || inlen == 0)
        return;
    if (p->len + inlen > p->cap) {
        size_t ncap = (p->cap == 0) ? 256 : p->cap;
        while (ncap < p->len + inlen)
            ncap *= 2;
        p->buf = (unsigned char *)realloc(p->buf, ncap);
        p->cap = ncap;
    }
    memcpy(p->buf + p->len, in, inlen);
    p->len += inlen;
}

/* Produce the next `outlen` bytes of the (prefix-consistent) output stream. */
static void xs_squeeze(xof_state *p, uint8_t *out, size_t outlen)
{
    size_t total;
    unsigned char *tmp;

    if (p == NULL || outlen == 0)
        return;

    total = p->squeezed + outlen;
    tmp = (unsigned char *)malloc(total);
    if (tmp == NULL)
        return;

    /* Regenerate the whole prefix [0, total) and hand back [squeezed, total). */
    pseudoXOF((unsigned long long)total * 8,
              p->buf, (unsigned long long)p->len * 8, tmp);
    memcpy(out, tmp + p->squeezed, outlen);
    p->squeezed = total;

    free(tmp);
}

static void xs_clone(void *dst, const void *src)
{
    xof_state *s = xs_get(src);
    xof_state *d = xs_new();
    if (d != NULL && s != NULL) {
        d->len = s->len;
        d->squeezed = s->squeezed;
        if (s->len > 0) {
            d->buf = (unsigned char *)malloc(s->len);
            d->cap = s->len;
            memcpy(d->buf, s->buf, s->len);
        }
    }
    xs_set(dst, d);
}

static void xs_release(void *ctx)
{
    xof_state *p = xs_get(ctx);
    if (p != NULL) {
        free(p->buf);
        free(p);
    }
    xs_set(ctx, NULL);
}

/* ------------------------------------------------------------------------- */
/* One-shot XOFs                                                             */
/* ------------------------------------------------------------------------- */

void
shake256(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen)
{
    pseudoXOF((unsigned long long)outlen * 8,
              input, (unsigned long long)inlen * 8, output);
}

void
shake128(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen)
{
    /* The scheme does not use SHAKE128; map to the same XOF for completeness. */
    pseudoXOF((unsigned long long)outlen * 8,
              input, (unsigned long long)inlen * 8, output);
}

/* ------------------------------------------------------------------------- */
/* SHAKE256 incremental API  (the only hash path used by the scheme)         */
/* ------------------------------------------------------------------------- */

void
shake256_inc_init(shake256incctx *state)
{
    xs_set(state, xs_new());
}

void
shake256_inc_absorb(shake256incctx *state, const uint8_t *input, size_t inlen)
{
    xs_absorb(xs_get(state), input, inlen);
}

void
shake256_inc_finalize(shake256incctx *state)
{
    (void)state; /* nothing to do: input is buffered, output is on demand */
}

void
shake256_inc_squeeze(uint8_t *output, size_t outlen, shake256incctx *state)
{
    xs_squeeze(xs_get(state), output, outlen);
}

void
shake256_inc_ctx_clone(shake256incctx *dest, const shake256incctx *src)
{
    xs_clone(dest, src);
}

void
shake256_inc_ctx_release(shake256incctx *state)
{
    xs_release(state);
}

/* ------------------------------------------------------------------------- */
/* SHAKE256 one-shot-absorb / squeezeblocks API                              */
/* ------------------------------------------------------------------------- */

void
shake256_absorb(shake256ctx *state, const uint8_t *input, size_t inlen)
{
    xof_state *p = xs_new();
    xs_set(state, p);
    xs_absorb(p, input, inlen);
}

void
shake256_squeezeblocks(uint8_t *output, size_t nblocks, shake256ctx *state)
{
    xs_squeeze(xs_get(state), output, nblocks * SHAKE256_RATE);
}

void
shake256_ctx_clone(shake256ctx *dest, const shake256ctx *src)
{
    xs_clone(dest, src);
}

void
shake256_ctx_release(shake256ctx *state)
{
    xs_release(state);
}

/* ------------------------------------------------------------------------- */
/* SHAKE128 API (unused by the scheme; provided for completeness)            */
/* ------------------------------------------------------------------------- */

void
shake128_inc_init(shake128incctx *state)
{
    xs_set(state, xs_new());
}

void
shake128_inc_absorb(shake128incctx *state, const uint8_t *input, size_t inlen)
{
    xs_absorb(xs_get(state), input, inlen);
}

void
shake128_inc_finalize(shake128incctx *state)
{
    (void)state;
}

void
shake128_inc_squeeze(uint8_t *output, size_t outlen, shake128incctx *state)
{
    xs_squeeze(xs_get(state), output, outlen);
}

void
shake128_inc_ctx_clone(shake128incctx *dest, const shake128incctx *src)
{
    xs_clone(dest, src);
}

void
shake128_inc_ctx_release(shake128incctx *state)
{
    xs_release(state);
}

void
shake128_absorb(shake128ctx *state, const uint8_t *input, size_t inlen)
{
    xof_state *p = xs_new();
    xs_set(state, p);
    xs_absorb(p, input, inlen);
}

void
shake128_squeezeblocks(uint8_t *output, size_t nblocks, shake128ctx *state)
{
    xs_squeeze(xs_get(state), output, nblocks * SHAKE128_RATE);
}

void
shake128_ctx_clone(shake128ctx *dest, const shake128ctx *src)
{
    xs_clone(dest, src);
}

void
shake128_ctx_release(shake128ctx *state)
{
    xs_release(state);
}

/* ------------------------------------------------------------------------- */
/* SHA3 fixed-output functions (unused by the scheme; provided for ABI)      */
/*                                                                           */
/*   SHA3-256 -> SM3 (256-bit)                                               */
/*   SHA3-512 -> pseudohash (512-bit)                                        */
/*   SHA3-384 -> first 384 bits of the SM2-KDF XOF                           */
/* ------------------------------------------------------------------------- */

void
sha3_256(uint8_t *output, const uint8_t *input, size_t inlen)
{
    sm3hash(256, input, (unsigned long long)inlen * 8, output);
}

void
sha3_384(uint8_t *output, const uint8_t *input, size_t inlen)
{
    pseudoXOF(384, input, (unsigned long long)inlen * 8, output);
}

void
sha3_512(uint8_t *output, const uint8_t *input, size_t inlen)
{
    pseudohash(512, input, (unsigned long long)inlen * 8, output);
}

/* Incremental SHA3: buffer, then apply the one-shot primitive on finalize. */

void
sha3_256_inc_init(sha3_256incctx *state)
{
    xs_set(state, xs_new());
}

void
sha3_256_inc_absorb(sha3_256incctx *state, const uint8_t *input, size_t inlen)
{
    xs_absorb(xs_get(state), input, inlen);
}

void
sha3_256_inc_finalize(uint8_t *output, sha3_256incctx *state)
{
    xof_state *p = xs_get(state);
    sm3hash(256, p ? p->buf : NULL,
            (unsigned long long)(p ? p->len : 0) * 8, output);
    xs_release(state);
}

void
sha3_256_inc_ctx_clone(sha3_256incctx *dest, const sha3_256incctx *src)
{
    xs_clone(dest, src);
}

void
sha3_256_inc_ctx_release(sha3_256incctx *state)
{
    xs_release(state);
}

void
sha3_384_inc_init(sha3_384incctx *state)
{
    xs_set(state, xs_new());
}

void
sha3_384_inc_absorb(sha3_384incctx *state, const uint8_t *input, size_t inlen)
{
    xs_absorb(xs_get(state), input, inlen);
}

void
sha3_384_inc_finalize(uint8_t *output, sha3_384incctx *state)
{
    xof_state *p = xs_get(state);
    pseudoXOF(384, p ? p->buf : NULL,
              (unsigned long long)(p ? p->len : 0) * 8, output);
    xs_release(state);
}

void
sha3_384_inc_ctx_clone(sha3_384incctx *dest, const sha3_384incctx *src)
{
    xs_clone(dest, src);
}

void
sha3_384_inc_ctx_release(sha3_384incctx *state)
{
    xs_release(state);
}

void
sha3_512_inc_init(sha3_512incctx *state)
{
    xs_set(state, xs_new());
}

void
sha3_512_inc_absorb(sha3_512incctx *state, const uint8_t *input, size_t inlen)
{
    xs_absorb(xs_get(state), input, inlen);
}

void
sha3_512_inc_finalize(uint8_t *output, sha3_512incctx *state)
{
    xof_state *p = xs_get(state);
    pseudohash(512, p ? p->buf : NULL,
               (unsigned long long)(p ? p->len : 0) * 8, output);
    xs_release(state);
}

void
sha3_512_inc_ctx_clone(sha3_512incctx *dest, const sha3_512incctx *src)
{
    xs_clone(dest, src);
}

void
sha3_512_inc_ctx_release(sha3_512incctx *state)
{
    xs_release(state);
}
