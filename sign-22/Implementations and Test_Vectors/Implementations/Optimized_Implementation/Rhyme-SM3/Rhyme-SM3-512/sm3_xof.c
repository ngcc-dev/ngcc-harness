/* SM3-based eXtensible Output Function state machine (INCREMENTAL).
 *
 * Implements GB/T 32918.4-2016 Section 5.4.3 (KDF-SM3 in counter mode)
 * directly via the public sm3hash() API from auxfunc.h, so that each
 * 32-byte output block is computed exactly once — O(n) SM3 calls rather
 * than the O(n^2) produced by repeated pseudoXOF(total) calls.
 *
 * Bit-identical to the Reference implementation that routes through
 * pseudoXOF().  No API_PKC no-modify files are modified.
 *
 * Two modes:
 *   XOF128  (mode=0) — for Agen expansion / sampling streams
 *   XOF256  (mode=1) — for hashing mu, rhoprime, c_tilde, keygen seed */

#include "sm3_xof.h"
#include "auxfunc.h"

#ifdef USE_AVX2_SM3
#include "simd/sm3_avx2.h"
#endif
#include <stdlib.h>
#include <string.h>

enum { TAG_XOF128 = 0x01, TAG_XOF256 = 0x02 };

/* ------------------------------------------------------------------ helpers */

/* Replicate auxfunc.c:normalize() — zero partial-byte bits at the tail. */
static void xof_normalize(uint8_t *input, uint64_t total_bits)
{
    uint64_t full_bytes = total_bits / 8;
    unsigned  rem       = (unsigned)(total_bits % 8);
    if (rem > 0)
        input[full_bytes] &= (uint8_t)(~((1U << (8 - rem)) - 1U));
}

/* Reset state, zeroing everything (cache=NULL, xof_ctr=0). */
static void state_reset(sm3_xof_state *s, uint8_t mode)
{
    memset(s, 0, sizeof(*s));
    s->mode = mode;
}

static void state_absorb(sm3_xof_state *s, const uint8_t *in, size_t inlen)
{
    if (s->overflow || s->finalized || inlen == 0)
        return;
    if (s->inlen + inlen > sizeof(s->input)) {
        s->overflow = 1;
        return;
    }
    memcpy(s->input + s->inlen, in, inlen);
    s->inlen += inlen;
}

/* ------------------------------------------------------------------ */
/* Incremental SM3-KDF block generator (replaces pseudoXOF).
 *
 * Builds counter blocks:  [tag_byte] [absorbed_data] [ct_be32]
 * and calls sm3hash() once per 32-byte output block.
 * Blocks are appended to s->cache (realloc'd as needed).
 * Each block is computed exactly once; subsequent squeezes
 * within already-cached range are pure memcpy. */
/* ------------------------------------------------------------------ */

static int xof_generate(sm3_xof_state *s, size_t need_bytes)
{
    size_t  need_blocks = (need_bytes + 31) / 32;
    size_t  alloc_bytes = need_blocks * 32;
    uint8_t tag         = (s->mode == 0) ? TAG_XOF128 : TAG_XOF256;
    size_t  msg_bytes   = s->inlen + 1;

    if (alloc_bytes > s->cache_len) {
        uint8_t *bigger = (uint8_t *)realloc(s->cache, alloc_bytes);
        if (!bigger) return -1;
        s->cache = bigger;
    }

    uint8_t prefix[SM3_XOF_CASCADE_MAX];
    prefix[0] = tag;
    if (s->inlen > 0)
        memcpy(prefix + 1, s->input, s->inlen);

#ifdef USE_AVX2_SM3
    while (s->xof_ctr < need_blocks) {
        unsigned char lanes[8][32];
        size_t remaining = need_blocks - s->xof_ctr;
        unsigned int take = remaining < 8 ? (unsigned int)remaining : 8;

        rhyme_sm3_xof_batch8(lanes, prefix, msg_bytes,
                              (unsigned int)(s->xof_ctr + 1));

        for (unsigned int lane = 0; lane < take; lane++) {
            memcpy(s->cache + (s->xof_ctr + lane) * 32,
                   lanes[lane], 32);
        }

        s->xof_ctr += take;
    }
#else
    {
        const uint64_t cascade_bits = (uint64_t)(msg_bytes * 8 + 32);

        while (s->xof_ctr < need_blocks) {
            uint8_t cascade[SM3_XOF_CASCADE_MAX];
            uint32_t ct = s->xof_ctr + 1;

            memcpy(cascade, prefix, msg_bytes);
            cascade[msg_bytes]     = (uint8_t)(ct >> 24);
            cascade[msg_bytes + 1] = (uint8_t)(ct >> 16);
            cascade[msg_bytes + 2] = (uint8_t)(ct >> 8);
            cascade[msg_bytes + 3] = (uint8_t)ct;

            (void)sm3hash(256, cascade, cascade_bits,
                          s->cache + s->xof_ctr * 32);

            s->xof_ctr++;
        }
    }
#endif

    s->cache_len = alloc_bytes;
    return 0;
}

/* Squeeze outlen bytes from the XOF stream.  Calls xof_generate() only
 * when more bytes are needed than currently cached.
 *
 * Normalisation (zeroing partial-byte bits in the last output byte) is
 * applied to the *output* buffer after copying, NOT to the cache —
 * matching pseudoXOF which normalises the output copy, not its internal
 * K buffer.  This is critical for incremental correctness: a byte that is
 * only partially consumed by the current squeeze may be fully consumed
 * later, and the cache must retain the original SM3 bits. */
static void state_squeeze(uint8_t *out, size_t outlen, sm3_xof_state *s)
{
    if (outlen == 0) return;
    if (s->overflow) { memset(out, 0, outlen); return; }

    size_t need = s->outpos + outlen;

    if (need > s->cache_len) {
        if (xof_generate(s, need) != 0) {
            memset(out, 0, outlen);
            return;
        }
    }

    memcpy(out, s->cache + s->outpos, outlen);

    /* Normalise output (not cache) per GB/T 32918.4 §5.4.3 step 5 */
    if ((need * 8) % 256 != 0)
        xof_normalize(out + outlen - 1, (uint64_t)need * 8);

    s->outpos += outlen;
}

/* ------------------------------------------------------------------ public */

void sm3_xof_clear(sm3_xof_state *s)
{
    free(s->cache);
    s->cache    = NULL;
    s->cache_len = 0;
    s->xof_ctr  = 0;
}

/* --- XOF128: one-shot absorb + squeezeblocks --- */

void sm3_xof128_init(sm3_xof_state *s)    { state_reset(s, 0); }
void sm3_xof128_absorb(sm3_xof_state *s, const uint8_t *in, size_t inlen) { state_absorb(s, in, inlen); }
void sm3_xof128_finalize(sm3_xof_state *s) { s->finalized = 1; }
void sm3_xof128_absorb_once(sm3_xof_state *s, const uint8_t *in, size_t inlen)
{
    sm3_xof128_init(s);
    sm3_xof128_absorb(s, in, inlen);
    sm3_xof128_finalize(s);
}
void sm3_xof128_squeezeblocks(uint8_t *out, size_t nblocks, sm3_xof_state *s)
{
    state_squeeze(out, nblocks * SM3_XOF128_RATE, s);
}

/* --- XOF256: one-shot (keeps pseudoXOF — it's a single call, no O(n^2)) --- */

void sm3_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    /* One-shot: use a temporary state for consistency with the incremental
     * path, ensuring bit-identical output. */
    sm3_xof_state tmp;
    sm3_xof256_init(&tmp);
    sm3_xof256_absorb(&tmp, in, inlen);
    sm3_xof256_finalize(&tmp);
    state_squeeze(out, outlen, &tmp);
    sm3_xof_clear(&tmp);
}

/* --- XOF256 incremental API --- */

void sm3_xof256_init(sm3_xof_state *s)    { state_reset(s, 1); }
void sm3_xof256_absorb(sm3_xof_state *s, const uint8_t *in, size_t inlen) { state_absorb(s, in, inlen); }
void sm3_xof256_finalize(sm3_xof_state *s) { s->finalized = 1; }
void sm3_xof256_squeeze(uint8_t *out, size_t outlen, sm3_xof_state *s)
{
    state_squeeze(out, outlen, s);
}
