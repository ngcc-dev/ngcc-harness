/*
 * xof.h -- the unified XOF compatibility layer for SHUTTLE.
 *
 * Every keygen/sign/verify randomness draw flows through exactly four
 * scalar primitives (plus their _avx2/_avx512 lane-batched variants):
 *
 *     xof128_init / xof128_squeeze   -- PUBLIC material only (ExpandA, DS
 * 0x02) xof256_init / xof256_squeeze   -- everything else (H, ExpandS,
 * SampleY,...)
 *
 * The interface is parameter-set-agnostic; it is switchable at MAKE TIME
 * between two backends with one macro:
 *
 *   NGCC_MODE (default, no extra -D): SM3 Hash-DRBG via the UNMODIFIABLE
 *       drng.{c,h}.  xof_ctx == DRNG_ctx.  Both xof128_* and xof256_*
 * collapse onto the SAME SM3 DRBG (SM3 cannot reach 256-bit security; the
 * 128/256 split is a placeholder for a future NGCC hash).
 *
 *   SHA3_MODE (-DSHA3_MODE): SHAKE128 (public) / SHAKE256 (secret) via the
 *       vendored fips202.{c,h}.  xof_ctx == keccak_state.  These are TWO
 *       genuinely distinct primitives (real 128/256-bit separation).
 *
 * The two MODEs are TWO DISTINCT KAT sets; within each MODE,
 * ref/avx2/avx512 must be byte-exact.
 *
 * Prototypes (BINDING):
 *     void xofK_init   (xof_ctx *ctx, const uint8_t *seed, size_t
 * seed_len); void xofK_squeeze(xof_ctx *ctx, uint8_t *out,        size_t
 * out_len);
 *   - the caller PRE-CONCATENATES  tag || seed || nonce  into one `seed`
 *     buffer (one-byte DS tag prepended; see DS_* below);
 *   - `seed_len`  is in BYTES;
 *   - `out_len`   is in BYTES  (the NGCC get_random_number takes BITS; the
 *     NGCC_MODE wrapper does the bytes->bits *8 shim -- see symmetric.h).
 *
 * The actual function BODIES live in symmetric.h (thin static-inline
 * wrappers).  This header owns only the macro
 * switch, the xof_ctx typedef, the rate macros, the lane-count macros and
 * the four prototypes (so callers can include xof.h alone for the
 * declarations).
 *
 * ------------------------------------------------------------------------
 * Domain-separation tags: DEFINED IN params.h (NOT here), one byte,
 * prepended to the seed buffer by the CALLER before xofK_init.  Reproduced
 * for reference: DS_EXPAND_SEEDS  = 0x00   DS_HASH_PK   = 0x05
 *   DS_EXPAND_SIGNING= 0x01   DS_HASH_MSG  = 0x06
 *   DS_EXPAND_A      = 0x02 (xof128, public)
 *   DS_EXPAND_S      = 0x03   DS_SAMPLE_C  = 0x07
 *   DS_HASH_CH       = 0x04   DS_SAMPLE_Y  = 0x08
 *                             DS_IRS       = 0x09
 * DS_SAMPLE_Y (0x08) and DS_IRS (0x09) both derive from seed_y; IRS opens
 * a FRESH ctx absorbing 0x09||seed_y and does NOT continue SampleY's ctx.
 * The XOF layer makes xof256_init cheap and stateless precisely to
 * enable this.
 * ------------------------------------------------------------------------
 */
#ifndef SHUTTLE_XOF_H
#define SHUTTLE_XOF_H

#include <stddef.h>
#include <stdint.h>

#include "params.h" /* DS_* tags (defined here), via config.h -> namespace.h */

#if defined(SHA3_MODE)

#    include "fips202.h"
typedef keccak_state xof_ctx;
#    define XOF128_RATE SHAKE128_RATE /* 168 */
#    define XOF256_RATE SHAKE256_RATE /* 136 */

#else /* NGCC_MODE (default) */

#    include "drng.h"
typedef DRNG_ctx xof_ctx;
/* OUTLEN: one SM3 compression emits 32 bytes; a request of L bytes costs
 * ceil(L/4) SM3 calls.  Both rates are 32 because the two primitives
 * collapse onto the same SM3 DRBG under NGCC_MODE. */
#    define XOF128_RATE 32
#    define XOF256_RATE 32

#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the four scalar primitives (bodies in symmetric.h) ---- */
void xof128_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len);
void xof128_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len);
void xof256_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len);
void xof256_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len);

#ifdef __cplusplus
}
#endif

/* ===================================================================== *
 *  Lane-count macros: resolve the SM3 (8/16) vs Keccak (4/8) divergence  *
 *  and the FIXED 16-stream algorithm flow.                               *
 * ===================================================================== */
#if defined(SHA3_MODE)
#    define XOF_LANES_AVX2 4
#    define XOF_LANES_AVX512 8
#else
#    define XOF_LANES_AVX2 8
#    define XOF_LANES_AVX512 16
#endif

/* The algorithm flow is FIXED at 16 independent XOF streams regardless of
 * backend.  This is the invariant that keeps every physical lane width
 * byte-compatible:
 *   SM3-AVX512   16 lanes -> 1 pass    SM3-AVX2     8 lanes -> 2 passes
 *   Keccak-AVX512 8 lanes -> 2 passes  Keccak-AVX2  4 lanes -> 4 passes
 *   ref scalar    1 lane  -> 16 passes (the degenerate fold)
 * Producers iterate `for (pass = 0; pass < XOF_STREAMS / LANES; pass++)`
 * and NEVER branch on lane count for the byte CONTENT; splitting
 * contiguous per-stream output across lanes is pointer arithmetic only.
 * See the shuttle_xof_passes() helper below and the 16-stream note. */
#define XOF_STREAMS 16

/* Number of lane-batched passes needed to cover all XOF_STREAMS at a given
 * physical lane width.  Ceil-divide so an odd stream count would still be
 * covered (here 16 is a multiple of every lane width we use). */
#define XOF_NUM_PASSES(lanes) ((XOF_STREAMS + (lanes)-1) / (lanes))

/* Pinned per-MODE squeeze granularity (BYTES per lane per refill).  The
 * 16-stream producers (ExpandA/ExpandS/SampleY) refill in whole units
 * of this size so ref (1 lane) and AVX (N lanes) consume the SAME amount
 * even when rejection sampling stops early (full-mini-batch-tail
 * cursor advance).
 *
 *   NGCC : 32 B/SM3-compression * 4 = 128 B/lane  (4 SM3 calls per refill)
 *   SHA3 : 168 B/lane = SHAKE128_RATE (one squeezeblocks block)
 *
 * MUST be identical across ref/avx2/avx512 within a MODE; asserted by the
 * diff-fuzz / KAT gate, NOT by a generator (these are rate-fixed, not
 * derived). */
#if defined(SHA3_MODE)
#    define XOF_SQUEEZE_GRANULARITY_BYTES SHAKE128_RATE /* 168 */
#else
#    define XOF_SQUEEZE_GRANULARITY_BYTES (32 * 4) /* 128 */
#endif

#endif /* SHUTTLE_XOF_H */
