/*
 * symmetric.h -- thin XOF wrapper declarations + the 16-stream design
 * helper.
 *
 * The four SCALAR primitives (xof128/256_init/squeeze) are declared in
 * xof.h and DEFINED in symmetric.c (external linkage, so the non-static
 * prototypes in xof.h match; this also keeps ref/ warning-clean -- a
 * `static inline` body in a header would clash with xof.h's non-static
 * declaration and trip -Wpedantic on unused-static in TUs that include the
 * header but call only some of them).
 *
 * This header additionally declares:
 *   - the avx2/avx512 lane-batched variant prototypes (bodies live in the
 *     backend TUs symmetric_avx2.c / symmetric_avx512.c; the prototypes
 * are pinned here so ExpandA/ExpandS/SampleY can be
 * written against a stable contract);
 *   - the 16-stream partition helper (shuttle_xof_passes), the documented,
 *     backend-invariant mapping of 16 logical streams onto physical lanes.
 *
 * MODE bindings (see xof.h for the switch):
 *
 *   NGCC_MODE: xof_ctx == DRNG_ctx (SM3 Hash-DRBG, drng.{c,h}).
 *       xof256_init   := init_random_number(ctx, seed, seed_len)   [BYTES]
 *       xof256_squeeze:= get_random_number (ctx, out,  out_len*8)  [BITS!]
 *     The *8 is the BYTES->BITS shim: get_random_number takes a BIT length
 *     (drng.h), our API takes BYTES.  We only ever request whole bytes, so
 * the MSB-first partial-byte masking inside get_random_number NEVER fires
 * on our plumbing path (sub-byte reads remain the
 *     consumers' concern -- e.g. SamplerU).
 *       xof128_* alias xof256_* verbatim (the 128/256 collapse).
 *
 *   SHA3_MODE: xof_ctx == keccak_state (fips202.{c,h}).
 *       xof128_* = SHAKE128 (rate 168), xof256_* = SHAKE256 (rate 136),
 * each as init + absorb_once (absorb the whole pre-concatenated seed
 * buffer) then incremental, rate-buffered squeeze.  Dilithium's
 * shake*_squeeze already tracks the leftover-rate cursor, so consecutive
 * xofK_squeeze calls chain correctly without us re-implementing the
 * squeeze position.
 */
#ifndef SHUTTLE_SYMMETRIC_H
#define SHUTTLE_SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>

#include "xof.h" /* xof_ctx, the four scalar prototypes, lane-count macros */

/* ===================================================================== *
 *  16-stream partition helper -- the heart of cross-backend KAT          *
 *  stability.  PURE arithmetic; no I/O, no PRNG state; safe in any TU. *
 * ===================================================================== */

/*
 * THE FIXED 16-STREAM FLOW.  Read this carefully -- it is the single
 * invariant that lets SM3-AVX512 (16 lanes), SM3-AVX2 (8 lanes),
 * Keccak-AVX512 (8 lanes), Keccak-AVX2 (4 lanes) and the scalar reference
 * (1 lane) all consume the IDENTICAL pseudorandom bytes in the IDENTICAL
 * order.
 *
 * A producer (ExpandA over the A-matrix sub-streams, ExpandS over the
 * secret components, SampleY over n/lane, ...) does
 * three things:
 *
 *  (1) PARTITION work into exactly XOF_STREAMS (=16) logical streams, each
 *      seeded by  tag || seed || IntegerToBytes(stream_idx, 2)  (the exact
 *      nonce encoding is the producer's; the 16-way partition count is
 * mandated here).
 *
 *  (2) FILL physical lanes by mapping logical streams onto lanes across
 *      XOF_NUM_PASSES(LANES) passes.  Pass p handles logical streams
 *      [p*LANES, p*LANES + LANES).  The producer loops
 *
 *          for (pass = 0; pass < XOF_NUM_PASSES(LANES); pass++) { ... }
 *
 *      and NEVER branches on LANES for the byte CONTENT -- only the loop
 * bound and the per-lane pointer marshaling depend on LANES.  The ref
 * scalar path is the degenerate LANES=1, 16-pass fold (one logical
 * stream/pass), which is exactly what test_xof verifies against.
 *
 *  (3) SPLIT the contiguous per-stream output across lanes by POINTER
 *      ARITHMETIC only (zero-cost): out_lane_ptrs[k] = base +
 * (stream_of(k))* bytes_per_stream.  The byte content of logical stream i
 * is independent of how many physical lanes ran.
 *
 *  CRITICAL (the KAT-killer): the per-lane cursor MUST advance by
 * the WHOLE squeeze granularity (XOF_SQUEEZE_GRANULARITY_BYTES) per
 * refill, INDEPENDENT of any early break in rejection sampling.  I.e.
 * consume the full mini-batch tail up-front so that ref (1 lane) and AVX
 * (N lanes) request the same NUMBER of bytes from each stream even when
 * one lane accepts early.  The discipline is the standard chunked
 * uniform/reject-block pattern: refill a whole block, then reject within
 * it; never short-squeeze.
 *
 * shuttle_xof_passes(lanes) returns XOF_NUM_PASSES(lanes); use it for the
 * loop bound so the "16 / LANES passes" rule is centralized and auditable.
 */
static inline unsigned shuttle_xof_passes(unsigned lanes)
{
    /* ceil(XOF_STREAMS / lanes); lanes is a public, compile-time-ish
     * constant (4/8/16 or 1 for the scalar fold) -- no secret-dependent
     * branch. */
    return (unsigned)((XOF_STREAMS + lanes - 1u) / lanes);
}

/* Logical-stream index handled by physical lane `lane` on pass `pass` at a
 * given physical lane width `lanes`.  This is the canonical mapping used
 * by the pointer-only output split in step (3) above:
 *
 *     stream_idx = pass * lanes + lane,   0 <= lane < lanes.
 *
 * For the scalar fold (lanes == 1) this collapses to stream_idx == pass,
 * i.e. the 16 passes enumerate streams 0..15 in order -- the KAT ground
 * truth. */
static inline unsigned shuttle_xof_stream_of(unsigned pass, unsigned lane,
                                             unsigned lanes)
{
    return pass * lanes + lane;
}

/* ===================================================================== *
 *  AVX2 / AVX512 lane-batched variant prototypes.                        *
 *                                                                        *
 *  Same xof_ctx mapping per MODE but a LANE-BATCHED ctx.  The signature *
 *  shape mirrors the existing N-way SM3 contract EXACTLY: lengths are *
 *  SHARED across lanes, the pointer arrays are PER-LANE, nothing is *
 *  validated.  The bytes->bits *8 shim applies identically in the N-way *
 *  NGCC squeeze.  Bodies live in the backend TUs (symmetric_avx2.c / *
 *  symmetric_avx512.c) which are compiled only in the avx2/avx512 builds.
 * *
 * ===================================================================== */
#if defined(SHUTTLE_XOF_DECLARE_AVX2)
#    if defined(SHA3_MODE)
#        include "fips202x4.h"
typedef keccakx4_state xof_ctx_avx2; /* 4-lane (Keccak) */
#    else
#        include "drng_avx2.h"
typedef DRNG_ctx_avx2 xof_ctx_avx2;     /* 8-lane (SM3) */
#    endif

void xof256_avx2_init(xof_ctx_avx2 *ctx,
                      const uint8_t *const seed[XOF_LANES_AVX2],
                      size_t seed_len);
void xof256_avx2_squeeze(xof_ctx_avx2 *ctx,
                         uint8_t *const out[XOF_LANES_AVX2],
                         size_t out_len);
/* xof128_avx2_* alias xof256_avx2_* under NGCC_MODE; distinct under
 * SHA3_MODE */
void xof128_avx2_init(xof_ctx_avx2 *ctx,
                      const uint8_t *const seed[XOF_LANES_AVX2],
                      size_t seed_len);
void xof128_avx2_squeeze(xof_ctx_avx2 *ctx,
                         uint8_t *const out[XOF_LANES_AVX2],
                         size_t out_len);
#endif /* SHUTTLE_XOF_DECLARE_AVX2 */

#if defined(SHUTTLE_XOF_DECLARE_AVX512)
#    if defined(SHA3_MODE)
#        include "fips202x8.h"
typedef keccakx8_state xof_ctx_avx512; /* 8-lane (Keccak) */
#    else
#        include "drng_avx512.h"
typedef DRNG_ctx_avx512 xof_ctx_avx512; /* 16-lane (SM3) */
#    endif

void xof256_avx512_init(xof_ctx_avx512 *ctx,
                        const uint8_t *const seed[XOF_LANES_AVX512],
                        size_t seed_len);
void xof256_avx512_squeeze(xof_ctx_avx512 *ctx,
                           uint8_t *const out[XOF_LANES_AVX512],
                           size_t out_len);
void xof128_avx512_init(xof_ctx_avx512 *ctx,
                        const uint8_t *const seed[XOF_LANES_AVX512],
                        size_t seed_len);
void xof128_avx512_squeeze(xof_ctx_avx512 *ctx,
                           uint8_t *const out[XOF_LANES_AVX512],
                           size_t out_len);
#endif /* SHUTTLE_XOF_DECLARE_AVX512 */

#endif /* SHUTTLE_SYMMETRIC_H */
