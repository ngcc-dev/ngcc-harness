/*
 * polyvec.c -- vector-level XOF-driven sampling surface (AVX-512 FORK).
 *
 * =========================================================================
 *  What this is
 * =========================================================================
 * This is the AVX-512 fork of ref/polyvec.c.  It is BYTE-EXACT to the
 * scalar reference: every Expand / Sample routine consumes the IDENTICAL
 * PRNG stream (same nonces, same order, same amount) and produces the
 * IDENTICAL sampled coefficients, so make check-kat reproduces the EXACT
 * ref NGCC/SHA3 hashes.  Only the INSTRUCTION SELECTION of two hot inner
 * loops changes:
 *
 *   1. ExpandA uniform-[0,q) rejection scan (uniform_reject_chunk):
 *      vectorized to a 32-wide AVX-512 reject + vpcompressw compaction,
 *      consuming the SAME contiguous 2-byte candidates in the SAME
 * ascending order with the SAME mask + unsigned-q test as the scalar
 *      us_next_candidate -> dst stream.  (ExpandA is ~25% of Sign and
 *      ~83% of Verify -- the headline lever.)  This is the AVX-512 analog
 * of the avx2 BMI2 pdep/pext path: a native unsigned 16-bit compare
 * (_mm512_cmplt_epu16_mask) yields a 32-bit accept mask, and
 *      _mm512_mask_compressstoreu_epi16 (vpcompressw, requires VBMI2)
 * gathers the surviving uint16 to the front in ascending lane order.
 *
 *   2. ExpandS / SampleY BaseSampler magnitude scan goes through the
 * AVX-512 cdt_scan96 (the sampler.c fork), which is bit-identical to
 * the scalar borrow chain.  The wide-Gaussian finalize, the
 * zero-fold/sign logic and the ApproxExp Bernoulli
 * (approx_exp_accept_q64_x4) are kept scalar (cheap per candidate, already
 * bit-exact, low register pressure)
 *      -- they are called from this file gauss_stream_chunk /
 * noise_minibatch exactly as the scalar reference does.
 *
 * Everything else (ExpandSeeds/ExpandSigningSeeds/SampleC, the
 * gauss_stream orchestration, the per-attempt byte schedule, the
 * OUTPUT-indexed sign stream) is copied VERBATIM from ref/polyvec.c.  When
 * -DUSE_AVX512_SAMPLER is NOT defined the file is bit-for-bit the scalar
 * reference.
 *
 * Read polyvec.h FIRST for the binding NGCC-DRBG no-rate-cursor rule and
 * the 16-stream nonce layout.  The PINNED PRNG byte schedules are
 * documented in ref/polyvec.c and are UNCHANGED here.
 */
#include "polyvec.h"

#include <stdlib.h> /* malloc/free for the N-way batched-fill scratch */
#include <string.h>

#include "approx_exp.h" /* approx_exp_accept_q64_x4, approx_exp_accept_q64 */
#include "rcdt_tables.h" /* SHUTTLE_RCDT_Z, SHUTTLE_RCDT_NOISE_* */
#include "test/prof.h" /* PT_G_SHAKE/BASESAMP/APPROXEXP/FINAL -- ((void)0) unless PROF_TIME */

#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__)
#    include <immintrin.h>
#endif

/* ===================================================================== *
 *  N-way batched XOF refill (USE_AVX512_XOF_NWAY)                        *
 * ===================================================================== *
 *
 *  This mirrors the avx2 fork's xof_nway_fill16 / gs_batch_first_fill
 *  design at the AVX-512 lane width.  ExpandA / ExpandS / SampleY each
 *  partition their work into the fixed XOF_STREAMS (=16) logical streams.
 *  The scalar reference fills the 16 lanes one at a time with 16
 *  SEQUENTIAL single-stream xof128/256 squeezes -- and profiling shows
 * that squeeze (the SM3/SHAKE XOF) is the dominant cost of ExpandA (~83%
 * of Verify, ~25% of Sign) and a large part of SampleY.
 *
 *  This fork keeps the *bytes* identical but computes the 16 streams'
 *  initial blocks N-AT-A-TIME with the already-built, lane-equivalent
 * N-way AVX-512 primitives (16-way SM3 under NGCC_MODE, 8-way SHAKE under
 *  SHA3_MODE), filling all 16 lane buffers in XOF_STREAMS/XOF_LANES_AVX512
 *  batched passes (a SINGLE pass of 16-way SM3, or 2 passes of 8-way
 * SHAKE) instead of 16 sequential single-stream squeezes.
 *
 *  WHY THIS IS BYTE-EXACT (lane-equivalence, test-verified):
 *  lane k of an N-way init+squeeze over the per-lane nonce
 *      tag || seed || LE16(stream_idx) || LE16(refill)
 *  produces the IDENTICAL byte stream as the scalar xof128/256 over the
 *  SAME absorbed nonce.  The 16-way SM3 / 8-way SHAKE kernels were proven
 *  byte-for-byte equal to the scalar reference (test_xof_nway, sm3-test,
 * the existing Keccak gate).  We do NOT change which bytes a stream gets,
 * only compute them N-at-a-time -- so the sampled coefficients and the KAT
 * are unchanged.  The SM3 DRBG has no sub-call rate cursor (each lane
 * draws its WHOLE block in one squeeze), which is exactly the
 * one-squeeze-per-fill / no-rate-cursor discipline the scalar streams
 * already use (see polyvec.h), so the 16-lane split is the natural
 * batching.
 *
 *  Only the INITIAL block (refill==0) of every lane is batched here --
 * that is the block that is ALWAYS drawn (16 of them per Expand/Sample
 * call). The statistically rare continuation refills (rc>=1, almost never
 * hit; see UNIFORM_BLOCK / GAUSS_STREAM_BLOCK sizing) fall back to the
 * scalar per-lane us_fill / gs_fill, byte-identical to ref.  This keeps
 * the refill nonce/rc bookkeeping in exactly one place and matters not at
 * all for throughput.
 */
#if defined(USE_AVX512_XOF_NWAY) && defined(__AVX512F__)
#    define SHUTTLE_XOF_DECLARE_AVX512 1
#    include "symmetric.h" /* xof_ctx_avx512, xof128/256_avx512_* */

/* At most XOF_STREAMS/XOF_LANES_AVX512 N-way passes cover the 16 logical
 * streams (1 for 16-way SM3, 2 for 8-way SHAKE).  XOF_STREAMS is a
 * multiple of XOF_LANES_AVX512 for both MODEs (16 % 16 == 0, 16 % 8 == 0),
 * so the passes tile the streams exactly with no partial trailing pass; a
 * caller that needs only nfill streams drives ceil(nfill / lanes) of them.
 */
_Static_assert(
    (XOF_STREAMS % XOF_LANES_AVX512) == 0,
    "16 logical streams must tile the N-way lane count exactly");

/* Batch-fill the refill==0 block of the leading `nfill` lanes (slots
 * 0..nfill-1) with the N-way XOF.  `nonces[t]` is the fully-built absorbed
 * nonce for stream t (tag||seed||LE16(t)||LE16(0)); `nonce_len` is shared
 * (the producer builds them all the same length).  `dst[t]` receives
 * `block_len` bytes for stream t.  `use_xof128` selects the 128 (public,
 * ExpandA) vs 256 family; under NGCC_MODE the two collapse to the same SM3
 * DRBG (the 128/256 XOF collapse).
 *
 * Callers compact their active lanes into the leading `nfill` slots, so we
 * issue only ceil(nfill / lanes) passes: a pass whose lanes are all >=
 * nfill would fill nothing the caller reads.  When the lane width covers
 * all 16 streams in one pass (16-way SM3) this is always a single pass;
 * the win is in the 8-way SHAKE path, where a caller needing <= 8 lanes
 * skips the second pass entirely.
 *
 * Lane k of pass p serves logical stream (p*XOF_LANES_AVX512 + k),
 * matching shuttle_xof_stream_of() -- so dst[stream_idx] gets lane k's
 * bytes, which the N-way primitive guarantees equals the scalar xof over
 * nonces[stream]. */
static void xof_nway_fill16(uint8_t *const dst[XOF_STREAMS],
                            const uint8_t *const nonces[XOF_STREAMS],
                            size_t nonce_len, size_t block_len,
                            int use_xof128, unsigned nfill)
{
    const unsigned passes = (nfill + (unsigned)XOF_LANES_AVX512 - 1) /
                            (unsigned)XOF_LANES_AVX512;
    unsigned pass, k;
    for (pass = 0; pass < passes; pass++) {
        const uint8_t *seedp[XOF_LANES_AVX512];
        uint8_t *outp[XOF_LANES_AVX512];
        xof_ctx_avx512 ctx;
        for (k = 0; k < (unsigned)XOF_LANES_AVX512; k++) {
            unsigned s = pass * (unsigned)XOF_LANES_AVX512 + k;
            seedp[k] = nonces[s];
            outp[k] = dst[s];
        }
        if (use_xof128) {
            xof128_avx512_init(&ctx, seedp, nonce_len);
            xof128_avx512_squeeze(&ctx, outp, block_len);
        } else {
            xof256_avx512_init(&ctx, seedp, nonce_len);
            xof256_avx512_squeeze(&ctx, outp, block_len);
        }
    }
}
#endif /* USE_AVX512_XOF_NWAY && __AVX512F__ */

/* ===================================================================== *
 *  Local little-endian byte helpers (data-independent schedule)         *
 * ===================================================================== */
static void put_le16(uint8_t *p, uint16_t x)
{
    p[0] = (uint8_t)(x & 0xFFu);
    p[1] = (uint8_t)((x >> 8) & 0xFFu);
}
static void put_le32(uint8_t *p, uint32_t x)
{
    p[0] = (uint8_t)(x & 0xFFu);
    p[1] = (uint8_t)((x >> 8) & 0xFFu);
    p[2] = (uint8_t)((x >> 16) & 0xFFu);
    p[3] = (uint8_t)((x >> 24) & 0xFFu);
}
static uint32_t get_le_masked(const uint8_t *p, unsigned nbytes,
                              uint32_t mask)
{
    uint32_t x = 0;
    unsigned i;
    for (i = 0; i < nbytes; i++)
        x |= (uint32_t)p[i] << (8 * i);
    return x & mask;
}

/* ===================================================================== *
 *  ExpandSeeds (DS 0x00, xof256, single one-shot)                       *
 * ===================================================================== */
void expand_seeds(uint8_t T[EXPAND_SEEDS_BYTES],
                  const uint8_t xi[SEEDBYTES], uint32_t kappa)
{
    /* absorb 0x00 || xi || IntToBytes(EM,2) || IntToBytes(kappa,4) */
    uint8_t in[1 + SEEDBYTES + 2 + 4];
    xof_ctx ctx;
    in[0] = DS_EXPAND_SEEDS;
    memcpy(in + 1, xi, SEEDBYTES);
    put_le16(in + 1 + SEEDBYTES, (uint16_t)EM);
    put_le32(in + 1 + SEEDBYTES + 2, kappa);
    xof256_init(&ctx, in, sizeof(in));
    xof256_squeeze(&ctx, T, EXPAND_SEEDS_BYTES);
}

/* ===================================================================== *
 *  ExpandSigningSeeds (DS 0x01, xof256, single one-shot, Sign-only)     *
 * ===================================================================== */
void expand_signing_seeds(uint8_t seedY[SEEDBYTES],
                          const uint8_t K[CHALLENGESEEDBYTES],
                          const uint8_t rnd[RNDBYTES],
                          const uint8_t mu[CHALLENGESEEDBYTES],
                          uint32_t kappa)
{
    /* absorb 0x01 || K || rnd || mu || IntToBytes(kappa,4) */
    uint8_t in[1 + CHALLENGESEEDBYTES + RNDBYTES + CHALLENGESEEDBYTES + 4];
    size_t off = 0;
    xof_ctx ctx;
    in[off++] = DS_EXPAND_SIGNING;
    memcpy(in + off, K, CHALLENGESEEDBYTES);
    off += CHALLENGESEEDBYTES;
    memcpy(in + off, rnd, RNDBYTES);
    off += RNDBYTES;
    memcpy(in + off, mu, CHALLENGESEEDBYTES);
    off += CHALLENGESEEDBYTES;
    put_le32(in + off, kappa);
    off += 4;
    xof256_init(&ctx, in, off);
    xof256_squeeze(&ctx, seedY, SEEDBYTES);
}

/* ===================================================================== *
 *  16-lane uniform-reject stream (ExpandA)                              *
 *                                                                       *
 *  Each lane opens ONE XOF instance (nonce = tag||seedA||LE16(lane)) and *
 *  is advanced purely by repeated fixed-size UNIFORM_DRAW squeezes off   *
 *  that persisted ctx.  Under NGCC each squeeze is one SM3 DRBG Generate *
 *  (V advances one step); the fixed UNIFORM_DRAW block size is what keeps
 * * ref and this N-way fork byte-exact.  Only the unused tail is never *
 *  squeezed. */

/* Right-sized slice: ceil-to-granularity of ~2x the mean per-lane byte
 * need (per-lane coeffs * BQ * 2, mean acceptance ~q/2^DQ).  Covers the
 * largest mode in one slice with margin; lazy continuation almost never
 * runs.  It is the FIXED per-squeeze block size ref and this fork agree
 * on. */
#define UNIFORM_LANE_COEFFS ((size_t)EM * N * (1u + ELL) / XOF_STREAMS)
#define UNIFORM_DRAW_RAW (UNIFORM_LANE_COEFFS * (size_t)BQ * 2u)
#define UNIFORM_DRAW                                                    \
    (((UNIFORM_DRAW_RAW + (size_t)XOF_SQUEEZE_GRANULARITY_BYTES - 1u) / \
      (size_t)XOF_SQUEEZE_GRANULARITY_BYTES) *                          \
     (size_t)XOF_SQUEEZE_GRANULARITY_BYTES)

/* Right-sized first/continuation slice for SampleC's partial Fisher-Yates.
 * The mean candidate need is sum_{i=n-tau}^{n-1} 2^DN/(i+1) BN-byte draws
 * (well under 256 B for every mode); TAU*BN*2 ceil-to-granularity gives a
 * comfortable margin so the first slice covers the whole challenge in
 * essentially every call.  SampleC opens ONE ctx and is advanced by
 * repeated fixed-size SAMPLEC_DRAW squeezes off it. */
#define SAMPLEC_DRAW_RAW ((size_t)TAU * (size_t)BN * 2u)
#define SAMPLEC_DRAW                                                    \
    (((SAMPLEC_DRAW_RAW + (size_t)XOF_SQUEEZE_GRANULARITY_BYTES - 1u) / \
      (size_t)XOF_SQUEEZE_GRANULARITY_BYTES) *                          \
     (size_t)XOF_SQUEEZE_GRANULARITY_BYTES)

typedef struct {
    uint8_t nonce[1 + SEEDBYTES + 2]; /* tag||seed||LE16(lane) */
    size_t nonce_len;
    uint16_t lane;
    size_t pos, avail; /* cursor / valid bytes within buf            */
    size_t drawn;      /* bytes squeezed from the persisted XOF chain  */
    int ctx_ready;     /* persisted ctx initialised                    */
    xof_ctx ctx;       /* live XOF instance: single init, then squeeze  */
    uint8_t buf[UNIFORM_DRAW];
} uniform_stream;

/* Squeeze the next UNIFORM_DRAW slice off the persisted XOF chain into
 * buf. If the persisted ctx is not yet live (the initial slice was filled
 * by the N-way batched path), init it from the nonce and fast-forward past
 * the `drawn` bytes already produced -- replaying the chain in
 * UNIFORM_DRAW units gives the identical V evolution, so the bytes match
 * either way. */
static void us_draw_slice(uniform_stream *us)
{
    if (!us->ctx_ready) {
        put_le16(us->nonce + 1 + SEEDBYTES, us->lane);
        xof128_init(&us->ctx, us->nonce, us->nonce_len);
        if (us->drawn) {
            uint8_t skip[UNIFORM_DRAW];
            size_t left = us->drawn;
            while (left) {
                size_t s = left < UNIFORM_DRAW ? left : UNIFORM_DRAW;
                xof128_squeeze(&us->ctx, skip, s);
                left -= s;
            }
        }
        us->ctx_ready = 1;
    }
    xof128_squeeze(&us->ctx, us->buf, UNIFORM_DRAW);
    us->pos = 0;
    us->avail = UNIFORM_DRAW;
    us->drawn += UNIFORM_DRAW;
}

static void us_fill(uniform_stream *us)
{
    us->ctx_ready = 0;
    us->drawn = 0;
    us_draw_slice(us);
}

/* us_setup: build the per-lane nonce + state WITHOUT drawing the first
 * slice (so the 16 lanes' initial fills can be batched N-way).  The
 * nonce's LE16(lane) field is written here so the batched N-way fill can
 * absorb us->nonce directly. */
static void us_setup(uniform_stream *us, const uint8_t *seedA,
                     unsigned lane)
{
    us->nonce[0] = DS_EXPAND_A;
    memcpy(us->nonce + 1, seedA, SEEDBYTES);
    us->nonce_len = 1 + SEEDBYTES + 2;
    us->lane = (uint16_t)lane;
    put_le16(us->nonce + 1 + SEEDBYTES, us->lane); /* LE16(lane) */
    us->pos = 0;
    us->avail = 0; /* not yet filled */
    us->drawn = 0;
    us->ctx_ready = 0;
}

static void us_init(uniform_stream *us, const uint8_t *seedA,
                    unsigned lane)
{
    us_setup(us, seedA, lane);
    us_fill(us); /* scalar single-stream fill */
}

/* Pull BQ bytes (a uniform-Z_q candidate).  When the current slice runs
 * out, squeeze the next UNIFORM_DRAW block off the SAME persisted ctx (the
 * V chain just advances one Generate -- no re-init, no refill counter). */
static uint16_t us_next_candidate(uniform_stream *us)
{
    uint32_t mask = (1u << DQ_BITS) - 1u;
    uint16_t a;
    if (us->pos + BQ > us->avail)
        us_draw_slice(us);
    a = (uint16_t)get_le_masked(us->buf + us->pos, BQ, mask);
    us->pos += BQ;
    return a;
}

#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__)
/* uniform_reject_block_avx512 -- 32-wide AVX-512 rejection over a
 * contiguous run of 2-byte candidates already resident in the stream
 * buffer, byte-identical to a run of scalar us_next_candidate() calls.
 *
 * Consumes whole 32-candidate (64-byte) chunks from `src` while >= 32
 * input candidates and >= 32 output slots remain, masking `& (2^DQ-1)` and
 * accepting iff `< q` via the NATIVE unsigned 16-bit compare
 * _mm512_cmplt_epu16_mask (the AVX-512 analog of the avx2
 * min_epu16==self trick).  The 32-bit accept mask drives
 * _mm512_mask_compressstoreu_epi16 (vpcompressw, VBMI2): the surviving
 * uint16 are written to dst[*cnt..) in ascending input order (vpcompressw
 * preserves lane order), exactly like the scalar accept stream.  Returns
 * the number of INPUT candidates consumed (always a multiple of 32); the
 * caller advances the byte cursor by 2x that and finishes the sub-32
 * remainder / refill on the scalar path so EVERY byte matches the scalar
 * schedule.  `incap` = candidates available, `want` = total output count
 * for the chunk. */
static size_t uniform_reject_block_avx512(uint16_t *dst, size_t *cnt,
                                          size_t want, const uint8_t *src,
                                          size_t incap)
{
    const uint16_t qmask = (uint16_t)((1u << DQ_BITS) - 1u);
    const __m512i maskv = _mm512_set1_epi16((short)qmask);
    const __m512i qv = _mm512_set1_epi16((short)(uint16_t)Q);
    size_t in = 0;
    while (in + 32 <= incap && *cnt + 32 <= want) {
        __m512i f = _mm512_and_si512(
            _mm512_loadu_si512((const __m512i *)(src + 2 * in)), maskv);
        /* native unsigned (f < Q) -> 32-bit accept mask */
        __mmask32 acc = _mm512_cmplt_epu16_mask(f, qv);
        /* compact surviving uint16 to the front of dst (ascending order)
         */
        _mm512_mask_compressstoreu_epi16(dst + *cnt, acc, f);
        *cnt += (size_t)_mm_popcnt_u32((unsigned)acc);
        in += 32;
    }
    return in;
}
#endif /* USE_AVX512_SAMPLER && __AVX512F__ */

/* Sample `count` uniform coeffs in [0,q) (ascending index) into `dst`.
 *
 * Byte-exact to the scalar reference (a run of `count` accepted
 * us_next_candidate() draws).  Under AVX-512 the bulk of the in-buffer
 * candidates is processed 32-at-a-time (uniform_reject_block_avx512); the
 * sub-32 remainder inside the current buffer and any refill boundary fall
 * back to the scalar us_next_candidate path -- so the consumed bytes, the
 * candidate order, the mask and the unsigned-q test are identical, and the
 * accepted stream matches the scalar oracle exactly (constant-time). */
static void uniform_reject_chunk(uniform_stream *us, uint16_t *dst,
                                 size_t count)
{
    size_t cnt = 0;
#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__)
    while (cnt < count) {
        /* whole candidates currently resident in the buffer */
        size_t incap = (us->avail - us->pos) / (size_t)BQ;
        if (incap >= 32 && cnt + 32 <= count) {
            /* SIMD bulk: consume whole 32-candidate chunks (>=32 in, >=32
             * out slots).  Returns 0 only when no full chunk fit, in which
             * case the scalar step below makes progress. */
            size_t consumed = uniform_reject_block_avx512(
                dst, &cnt, count, us->buf + us->pos, incap);
            us->pos += consumed * (size_t)BQ;
            if (consumed != 0)
                continue; /* re-evaluate buffer/output capacity */
        }
        /* scalar step: handles the sub-32 input remainder, the sub-32
         * output remainder, and the refill boundary -- all byte-identical
         * to ref. */
        {
            uint16_t a;
            do {
                a = us_next_candidate(us);
            } while (a >= (uint16_t)Q);
            dst[cnt++] = a;
        }
    }
#else
    size_t k;
    for (k = 0; k < count; k++) {
        uint16_t a;
        do {
            a = us_next_candidate(us); /* REPEAT { 2B; mask } UNTIL a<q */
        } while (a >= (uint16_t)Q);
        dst[k] = a;
    }
    cnt = count;
#endif
    (void)cnt;
}

/* ===================================================================== *
 *  ExpandA (DS 0x02, xof128, 16 lanes; A_gen direct in NTT domain)     *
 * ===================================================================== */
void expand_a(poly16 agen[EM], poly16 hAgen[EM * ELL],
              const uint8_t seedA[SEEDBYTES])
{
    /* Flat views over the two output arrays. */
    uint16_t *abar = (uint16_t *)agen;  /* EM*n coeffs   */
    uint16_t *hbar = (uint16_t *)hAgen; /* EM*ELL*n coeffs */
    const size_t na = (size_t)EM * N;
    const size_t nh = (size_t)EM * ELL * N;
    const size_t wa = na / XOF_STREAMS; /* agen chunk width  */
    const size_t wh = nh / XOF_STREAMS; /* hAgen chunk width */
    unsigned t;

    _Static_assert(((EM * N) % XOF_STREAMS) == 0,
                   "EM*n must split into 16 lanes");
    _Static_assert(((EM * ELL * N) % XOF_STREAMS) == 0,
                   "EM*ELL*n must split into 16 lanes");
    _Static_assert(sizeof(poly16) == 2 * N, "poly16 flat layout");

#if defined(USE_AVX512_XOF_NWAY) && defined(__AVX512F__)
    /* N-WAY BATCHED INITIAL FILL: draw the refill==0 FIRST SLICE
     * (UNIFORM_DRAW bytes) of all XOF_STREAMS lanes N-at-a-time (16-way
     * SM3 = 1 pass / 8-way SHAKE = 2 passes), byte-exact to 16 sequential
     * scalar us_fill() slices (lane-equivalence).  ExpandA is the headline
     * lever: this replaces 16 sequential xof128 squeezes -- the dominant
     * cost of Verify (~83%) and a big chunk of Sign -- with
     * XOF_STREAMS/XOF_LANES_AVX512 batched squeezes, now right-sized so
     * the wasted tail is never squeezed.  Then the (already vectorized)
     * per-lane uniform_reject_chunk runs on each pre-filled buffer,
     * drawing any further slice on the scalar path.  UNIFORM_DRAW is a
     * whole multiple of the per-MODE squeeze granularity, so the N-way
     * squeeze is rate-aligned and byte-exact. */
    {
        uniform_stream *us = (uniform_stream *)malloc(
            (size_t)XOF_STREAMS * sizeof(uniform_stream));
        const uint8_t *nonces[XOF_STREAMS];
        uint8_t *dst[XOF_STREAMS];
        if (us) {
            for (t = 0; t < XOF_STREAMS; t++) {
                us_setup(&us[t], seedA, t);
                nonces[t] = us[t].nonce;
                dst[t] = us[t].buf;
            }
            /* ExpandA uses xof128 (public material, tag 0x02). */
            xof_nway_fill16(dst, nonces, us[0].nonce_len, UNIFORM_DRAW,
                            /*use_xof128=*/1, /*nfill=*/XOF_STREAMS);
            for (t = 0; t < XOF_STREAMS; t++) {
                /* First slice resident; ctx not yet live -- a continuation
                 * slice (rare) lazily inits the scalar ctx and skips
                 * drawn. */
                us[t].pos = 0;
                us[t].avail = UNIFORM_DRAW;
                us[t].drawn = UNIFORM_DRAW;
                us[t].ctx_ready = 0;
                uniform_reject_chunk(&us[t], abar + (size_t)t * wa, wa);
                uniform_reject_chunk(&us[t], hbar + (size_t)t * wh, wh);
            }
            free(us);
            return;
        }
        /* malloc failure: fall through to the scalar per-lane path below.
         */
    }
#endif

    for (t = 0; t < XOF_STREAMS; t++) {
        uniform_stream us;
        us_init(&us, seedA, t);
        uniform_reject_chunk(&us, abar + (size_t)t * wa, wa);
        uniform_reject_chunk(&us, hbar + (size_t)t * wh, wh);
    }
}

/* ===================================================================== *
 *  16-lane noise/gauss stream (ExpandS, SampleY) -- one-squeeze-per-fill *
 * ===================================================================== */
/* Build the absorbed nonce (tag || seed || LE16(lane), no refill) into
 * `nonce`; returns the nonce length.  Shared by gauss_stream_init and the
 * gs_fill SHA3 fallback so the byte layout is defined in one place. */
static size_t gs_build_nonce(const gauss_stream *gs, uint8_t *nonce)
{
    size_t seedlen =
        (gs->tag == DS_SAMPLE_Y) ? SEEDBYTES : CHALLENGESEEDBYTES;
    nonce[0] = gs->tag;
    memcpy(nonce + 1, gs->seed, seedlen);
    put_le16(nonce + 1 + seedlen, gs->lane);
    return 1 + seedlen + 2;
}

void gauss_stream_init(gauss_stream *gs, uint8_t tag, const uint8_t *seed,
                       unsigned lane)
{
    uint8_t nonce[1 + CHALLENGESEEDBYTES + 2];
    size_t nlen;
    gs->tag = tag;
    gs->seed = seed;
    gs->lane = (uint16_t)lane;
    gs->pos = 0;
    gs->avail = 0;
    gs->bulk_ptr = NULL; /* scalar path: always XOF off gs->ctx */
    gs->bulk_left = 0;
    gs->ff_blocks = 0;
    nlen = gs_build_nonce(gs, nonce);
    xof256_init(&gs->ctx, nonce, nlen);
}

/* Draw one gauss_block_bytes block into gs->buf+gs->avail.  Source order:
 *  (1) if N-way pre-filled blocks remain, memcpy the next one (the AVX
 * bulk path);  (2) else squeeze off the persisted per-lane ctx -- which is
 *      already at V_K (the post-bulk state, copied out of the N-way ctx
 *      under NGCC) or, under SHA3, lazily re-derived by fast-forwarding
 *      ff_blocks blocks on the first fallback. */
static void gs_fill(gauss_stream *gs)
{
    size_t blk = gauss_block_bytes(gs->tag);
    if (gs->bulk_left) {
        memcpy(gs->buf + gs->avail, gs->bulk_ptr, blk);
        gs->bulk_ptr += blk;
        gs->bulk_left--;
        gs->avail += blk;
        return;
    }
    if (gs->ff_blocks) {
        uint8_t nonce[1 + CHALLENGESEEDBYTES + 2];
        size_t nlen = gs_build_nonce(gs, nonce);
        size_t f;
        xof256_init(&gs->ctx, nonce, nlen);
        for (f = 0; f < gs->ff_blocks; f++) {
            uint8_t skip[GAUSS_STREAM_BLOCK];
            xof256_squeeze(&gs->ctx, skip, blk);
        }
        gs->ff_blocks = 0;
    }
    xof256_squeeze(&gs->ctx, gs->buf + gs->avail, blk);
    gs->avail += blk;
}

void gs_ensure(gauss_stream *gs, size_t need)
{
    if (gs->avail - gs->pos >= need)
        return;
    {
        size_t left = gs->avail - gs->pos;
        if (left)
            memmove(gs->buf, gs->buf + gs->pos, left);
        gs->pos = 0;
        gs->avail = left;
        gs_fill(gs);
    }
}

#if defined(USE_AVX512_XOF_NWAY) && defined(__AVX512F__)
/* gauss_bulk_setup: N-way pre-fill of the gauss lane buffers (xof256:
 * 16-way SM3 = 1 pass / 8-way SHAKE = 2 passes), preserving each lane's
 * single-init model.
 *
 * Kyber gen_matrix-style **bulk-preset K + scalar fallback**: one
 * persisted N-way XOF ctx per pass squeezes K fixed-size blocks for all
 * XOF_STREAMS lanes in lockstep (uniform, no skip problem since every lane
 * draws exactly K blocks), then each lane consumes scalar from its K*blk
 * private region. A lane that needs a (K+1)-th block hits gs_fill's
 * fallback: under NGCC the lane's evolved DRBG state V_K is copied
 * straight out of the post-bulk N-way ctx (drng_copy_lane, one SoA
 * memcpy), so the lane's gs->ctx is already at V_K and continuation
 * squeezes are byte-identical to ref; under SHA3 the lane re-inits and
 * fast-forwards K blocks (ff_blocks) on its first fallback.  K is tuned
 * (BULK_K_* in polyvec.h) so P(lane exceeds K) <= 1/16
 * -- the marginal N-way block pays for itself exactly when >=1/16 of the
 * 16 lanes still need it -- so almost every lane finishes inside the bulk
 * and the fallback waste is negligible (see the polyvec.h cost-model
 * comment). */

#    ifndef SHA3_MODE
/* Copy lane `lane`'s evolved SM3-DRBG state out of a (post-bulk) N-way ctx
 * into a scalar ctx, so scalar squeezes continue the exact same V-chain.
 */
static void drng_copy_lane(xof_ctx *dst, const xof_ctx_avx512 *src,
                           unsigned lane)
{
    memcpy(dst->V, src->V[lane], SEEDLEN);
    memcpy(dst->C, src->C[lane], SEEDLEN);
    memcpy(dst->reseed_counter, src->reseed_counter[lane], SEEDLEN);
}
#    endif

/* Bulk-preset K blocks for all XOF_STREAMS gauss lanes and arm each gss[t]
 * to consume from its private region (with the V_K fallback ctx prepared).
 * Returns the malloc'd bulk buffer (caller frees) or NULL on OOM. */
static uint8_t *gauss_bulk_setup(gauss_stream gss[XOF_STREAMS],
                                 uint8_t tag, const uint8_t *seed,
                                 size_t K)
{
    const size_t lanes = (size_t)XOF_LANES_AVX512;
    const size_t passes = (size_t)XOF_STREAMS / lanes;
    size_t blk = gauss_block_bytes(tag);
    size_t seedlen = (tag == DS_SAMPLE_Y) ? SEEDBYTES : CHALLENGESEEDBYTES;
    size_t nlen = 1 + seedlen + 2;
    uint8_t nonces[XOF_STREAMS][1 + CHALLENGESEEDBYTES + 2];
    xof_ctx_avx512 nctx[XOF_STREAMS / XOF_LANES_AVX512];
    uint8_t *bulk;
    size_t i, p, k, t;

    bulk = (uint8_t *)malloc((size_t)XOF_STREAMS * K * blk);
    if (!bulk)
        return NULL;

    for (t = 0; t < XOF_STREAMS; t++) {
        nonces[t][0] = tag;
        memcpy(nonces[t] + 1, seed, seedlen);
        put_le16(nonces[t] + 1 + seedlen, (uint16_t)t);
    }
    for (p = 0; p < passes; p++) {
        const uint8_t *seedp[XOF_LANES_AVX512];
        for (k = 0; k < lanes; k++)
            seedp[k] = nonces[p * lanes + k];
        xof256_avx512_init(&nctx[p], seedp, nlen);
    }
#    ifdef SHA3_MODE
    /* SHA3: the scalar xof256_squeeze is rate-buffered/continuous (it
     * tracks a leftover-rate cursor), so ref's K consecutive blk-squeezes
     * == one continuous (K*blk)-squeeze.  The x8 squeeze, by contrast, is
     * block-granular and DISCARDS the sub-rate tail of each call (blk is
     * not a multiple of the SHAKE rate).  So issue ONE N-way squeeze of
     * K*blk per lane (lane t's K blocks are contiguous in `bulk`): the
     * only discarded tail is past the K*blk boundary, which the fallback
     * re-derives via ff_blocks.  A per-block loop would wrongly drop every
     * block's tail. */
    for (p = 0; p < passes; p++) {
        uint8_t *outp[XOF_LANES_AVX512];
        for (k = 0; k < lanes; k++)
            outp[k] = bulk + (p * lanes + k) * K * blk;
        xof256_avx512_squeeze(&nctx[p], outp, K * blk);
    }
    (void)i;
#    else
    /* NGCC: each squeeze is one independent SM3 Generate advancing V
     * exactly one step, matching ref's K separate gs_fill squeezes -- so
     * issue K separate N-way squeezes (one big K*blk call would evolve V
     * only once and diverge).  Squeeze i fills block i of every lane. */
    for (i = 0; i < K; i++)
        for (p = 0; p < passes; p++) {
            uint8_t *outp[XOF_LANES_AVX512];
            for (k = 0; k < lanes; k++) {
                t = p * lanes + k;
                outp[k] = bulk + (t * K + i) * blk;
            }
            xof256_avx512_squeeze(&nctx[p], outp, blk);
        }
#    endif
    for (t = 0; t < XOF_STREAMS; t++) {
        gss[t].tag = tag;
        gss[t].seed = seed;
        gss[t].lane = (uint16_t)t;
        gss[t].pos = 0;
        gss[t].avail = 0;
        gss[t].bulk_ptr = bulk + t * K * blk;
        gss[t].bulk_left = K;
#    ifdef SHA3_MODE
        gss[t].ff_blocks = K; /* lazy re-init+fast-forward on overflow */
#    else
        drng_copy_lane(&gss[t].ctx, &nctx[t / lanes],
                       (unsigned)(t % lanes));
        gss[t].ff_blocks = 0;
#    endif
    }
    return bulk;
}
#endif /* USE_AVX512_XOF_NWAY && __AVX512F__ */

/* ===================================================================== *
 *  ExpandS (DS 0x03, xof256, 16 lanes; BaseSampler + zero-fold + sign)  *
 * ===================================================================== */
/* noise_consume_minibatch: process ONE NOISE_BATCH mini-batch from the
 * lane buffer (cdt_scan96 over `Z`/`entries`, NOISE_BATCH=32; the AVX-512
 * cdt_scan96 from the sampler.c fork, bit-identical to scalar), then a
 * 2-bit-per-candidate tail (bit0 sign, bit1 zero-fold), 4 cand/byte.  PURE
 * buffer operation -- the caller MUST have ensured >=
 * NOISE_MINIBATCH_RAND_BYTES are resident at gs->buf+gs->pos.  The WHOLE
 * NOISE_MINIBATCH_RAND_BYTES (392) is consumed up front so the cursor
 * advances independently of the *cnt==want early break.  Shared by the
 * scalar (A) and N-way round-robin (B) ExpandS paths so they cannot drift.
 */
static void noise_consume_minibatch(gauss_stream *gs, int32_t *dst,
                                    size_t *cnt, size_t want,
                                    const uint32_t Z[][3], int entries)
{
    int32_t mag[NOISE_BATCH];
    const uint8_t *tailp;
    int j;
    noise_magnitude_batch(mag, gs->buf + gs->pos, Z, entries);
    tailp = gs->buf + gs->pos + NOISE_CDT_BYTES; /* 2-bit fields */
    for (j = 0; j < NOISE_BATCH; j++) {
        uint32_t f = (uint32_t)(tailp[j >> 2] >> (2 * (j & 3))) & 3u;
        uint32_t reject = ct_is_zero_u32((uint32_t)mag[j]) & (f >> 1);
        int32_t r = ct_sel_i32(f & 1u, -mag[j], mag[j]);
        if (*cnt < want &&
            !reject) /* placement only; cursor already advanced */
            dst[(*cnt)++] = r;
    }
    gs->pos += NOISE_MINIBATCH_RAND_BYTES; /* WHOLE tail consumed */
}

/* noise_minibatch: scalar single-lane mini-batch (A) -- ensure one
 * mini-batch is resident (one scalar xof256 squeeze on refill) then
 * consume it.  Byte-identical to the shared pure-buffer body above; kept
 * for the non-USE_AVX512_XOF_NWAY build and the malloc-failure fallback.
 */
static void noise_minibatch(gauss_stream *gs, int32_t *dst, size_t *cnt,
                            size_t want, const uint32_t Z[][3],
                            int entries)
{
    gs_ensure(gs, NOISE_MINIBATCH_RAND_BYTES);
    noise_consume_minibatch(gs, dst, cnt, want, Z, entries);
}

void expand_s(poly s1s2[ELL + EM],
              const uint8_t seedsk[CHALLENGESEEDBYTES])
{
    int32_t *sbar = s1s2[0].coeffs;   /* ELL*n contiguous */
    int32_t *ebar = s1s2[ELL].coeffs; /* EM*n contiguous  */
    const size_t ns = (size_t)ELL * N;
    const size_t ne = (size_t)EM * N;
    const size_t ws = ns / XOF_STREAMS; /* s chunk width  */
    const size_t we = ne / XOF_STREAMS; /* e chunk width  */
    unsigned t;

    _Static_assert(((ELL * N) % XOF_STREAMS) == 0,
                   "ELL*n must split into 16 lanes");
    _Static_assert(((EM * N) % XOF_STREAMS) == 0,
                   "EM*n must split into 16 lanes");
    _Static_assert(sizeof(poly) == 4 * N, "poly flat layout");
    _Static_assert(ELL + EM >= 2,
                   "s1s2 holds ELL s-polys then EM e-polys");

#if defined(USE_AVX512_XOF_NWAY) && defined(__AVX512F__)
    /* N-WAY BULK PRESET: keep ALL of ExpandS's gauss XOF on the N-way path
     * via gauss_bulk_setup -- pre-fill BULK_K_EXPAND_S blocks for all 16
     * lanes N-at-a-time, then consume each lane scalar from its private
     * region (the rare (K+1)-th block falls back to a scalar squeeze off
     * the lane's V_K ctx).  Byte-exact to ref. */
    {
        gauss_stream *gss = (gauss_stream *)malloc((size_t)XOF_STREAMS *
                                                   sizeof(gauss_stream));
        if (gss) {
            uint8_t *bulk = gauss_bulk_setup(gss, DS_EXPAND_S, seedsk,
                                             BULK_K_EXPAND_S);
            if (bulk) {
                for (t = 0; t < XOF_STREAMS; t++) {
                    size_t cnt = 0;
                    while (cnt < ws)
                        noise_minibatch(&gss[t], sbar + (size_t)t * ws,
                                        &cnt, ws, RCDT_NOISE_S,
                                        RCDT_NOISE_S_ENTRIES);
                    cnt = 0;
                    while (cnt < we)
                        noise_minibatch(&gss[t], ebar + (size_t)t * we,
                                        &cnt, we, RCDT_NOISE_E,
                                        RCDT_NOISE_E_ENTRIES);
                }
                free(bulk);
                free(gss);
                return;
            }
            free(gss);
        }
        /* malloc failure: fall through to the scalar per-lane path. */
    }
#endif

    for (t = 0; t < XOF_STREAMS; t++) {
        gauss_stream gs;
        size_t cnt;
        gauss_stream_init(&gs, DS_EXPAND_S, seedsk, t);
        cnt = 0;
        while (cnt < ws)
            noise_minibatch(&gs, sbar + (size_t)t * ws, &cnt, ws,
                            RCDT_NOISE_S, RCDT_NOISE_S_ENTRIES);
        cnt = 0;
        while (cnt < we)
            noise_minibatch(&gs, ebar + (size_t)t * we, &cnt, we,
                            RCDT_NOISE_E, RCDT_NOISE_E_ENTRIES);
    }
}

/* ===================================================================== *
 *  SampleC (DS 0x07, xof256, single ctx; partial Fisher-Yates)          *
 * ===================================================================== */
void sample_c(poly *c, const uint8_t seedC[CHALLENGESEEDBYTES])
{
    /* Single-stream rejection sampler over the public seedC.  ONE XOF.Init
     * (tag||seedC||LE16(0)), then the stream is advanced purely by
     * repeated fixed-size SAMPLEC_DRAW squeezes off the SAME persisted ctx
     * (under NGCC each squeeze is one SM3 Generate advancing V one step;
     * the fixed block size is what keeps ref and this fork byte-exact).
     * Variable-time loop is fine here (seedC is public). */
    uint8_t nonce[1 + CHALLENGESEEDBYTES + 2];
    uint8_t block[SAMPLEC_DRAW];
    /* cursor / valid bytes within block */
    size_t pos = 0, avail = 0;
    uint32_t mask = (1u << DN_BITS) - 1u;
    int i;
    xof_ctx ctx;

    nonce[0] = DS_SAMPLE_C;
    memcpy(nonce + 1, seedC, CHALLENGESEEDBYTES);
    put_le16(nonce + 1 + CHALLENGESEEDBYTES, 0u); /* single stream idx 0 */
    xof256_init(&ctx, nonce, sizeof(nonce));      /* single init */

    for (i = 0; i < N; i++)
        c->coeffs[i] = 0;

    for (i = N - TAU; i < N; i++) {
        uint32_t j;
        do {
            if (pos + BN > avail) {
                xof256_squeeze(&ctx, block, SAMPLEC_DRAW); /* next block */
                pos = 0;
                avail = SAMPLEC_DRAW;
            }
            j = get_le_masked(block + pos, BN, mask);
            pos += BN;
        } while (j > (uint32_t)i);
        c->coeffs[i] = c->coeffs[j];
        c->coeffs[j] = 1;
    }
}

/* ===================================================================== *
 *  SampleDGauss / SampleY (DS 0x08, xof256, 16 lanes; wide Gaussian r)  *
 * ===================================================================== *
 *
 *  Two byte-IDENTICAL code paths share the SAME per-buffer consumption
 *  kernels (gs_load_sign_stream / gs_consume_minibatch) so they cannot
 *  drift:
 *
 *   (A) scalar per-lane: gauss_stream_chunk() drives ONE lane to
 * completion, refilling its own buffer with the scalar single-stream
 * gs_ensure -> gs_fill (one xof256 squeeze per refill).  This is the
 * scalar reference path and the malloc-failure fallback.
 *
 *   (B) N-way round-robin (the headline lever): sample_y() drives ALL
 * 16 lanes in LOCKSTEP -- every round, the lanes that still need bytes are
 *       refilled N-AT-A-TIME (16-way SM3 = 1 pass / 8-way SHAKE = 2
 * passes) via xof_nway_fill16, then each not-yet-done lane consumes from
 * its freshly topped-up buffer.  Lanes finish at different rounds
 * (per-lane rejection makes R_k vary); finished lanes simply stop
 * consuming.
 *
 *  WHY (B) IS BYTE-EXACT TO (A) (consumed-bytes invariance):
 *  lane k's gauss_stream is the deterministic byte schedule
 *      xof256(0x08 || seedY || LE16(k) || LE16(refill)),  refill = 1,2,...
 *  consumed in a fixed sign-stream prefix + GAUSS_BATCH mini-batches with
 * a fully-consumed-up-front tail (the cursor advances regardless of the
 * early coefcnt==count break).  Lane k consumes refills 1..R_k.  Both
 * paths use the IDENTICAL nonce schedule (gs_build_nonce) and the
 * IDENTICAL per-buffer consumption kernels; only WHERE the bytes are
 * computed moves -- (A) makes them one scalar squeeze at a time, (B) makes
 * the not-done lanes' next blocks N-at-a-time.  The N-way SM3/SHAKE
 * kernels were proven byte-for-byte equal to the scalar xof256 over the
 * same nonce (lane-equivalence, test_xof_nway).  So the bytes lane k
 * CONSUMES -- and therefore every sampled coefficient and the KAT hash --
 * are unchanged.  Extra N-way bytes produced for already-finished lanes in
 * a round are simply UNUSED (harmless: the KAT depends only on the bytes
 * each lane CONSUMES). */

/* gs_load_sign_stream: consume the up-front OUTPUT-indexed sign stream
 * from the lane buffer into `signs` (signbytes = (count+7)/8).  PURE
 * buffer operation -- the caller MUST have ensured >= signbytes are
 * resident.  The AVX512 +SIGN_PAD_AVX512 over-read window stays zero
 * (caller zero-inits signs). */
static void gs_load_sign_stream(gauss_stream *gs, uint8_t *signs,
                                size_t signbytes)
{
    memcpy(signs, gs->buf + gs->pos, signbytes);
    gs->pos += signbytes;
}

/* gs_consume_minibatch: process ONE GAUSS_BATCH mini-batch from the lane
 * buffer, appending accepted coeffs to dst[*coefcnt..count).  PURE buffer
 * operation -- the caller MUST have ensured >= MINIBATCH_RAND_BYTES are
 * resident.  The whole MINIBATCH_RAND_BYTES is consumed up front (cursor
 * advances independent of the coefcnt==count early break).  This is
 * the EXACT scalar body extracted verbatim from gauss_stream_chunk,
 * shared by the scalar (A) and N-way (B) paths so they cannot diverge. */
static void gs_consume_minibatch(gauss_stream *gs, int32_t *dst,
                                 size_t *coefcnt, size_t count,
                                 const uint8_t *signs)
{
    int32_t x[GAUSS_BATCH];
    int32_t yv[GAUSS_BATCH];
    uint64_t phat[GAUSS_BATCH];
    const uint8_t *yp, *tailp;
    int j;

    {
        PROF_START(t_bs);
        sampler_sigma2(x, gs->buf + gs->pos);
        PROF_STOP(PT_G_BASESAMP, t_bs);
    }
    yp = gs->buf + gs->pos + SIGMA_S_RAND_BYTES;
    for (j = 0; j < GAUSS_BATCH; j++)
        yv[j] = (int32_t)yp[j]; /* Y_BITS=8: plain byte copy */
    tailp = gs->buf + gs->pos + SIGMA_S_RAND_BYTES + Y_RAND_BYTES;

    /* Two-path body.  When more than a full GAUSS_BATCH accepts are still
     * outstanding (count - *coefcnt > GAUSS_BATCH) every one of the 32
     * candidates is needed -- coefcnt cannot reach count this mini-batch
     * -- so we run the full-width SIMD path, byte-identical to before and
     * with the common-case throughput unchanged.  Otherwise this MIGHT be
     * the last mini-batch: we fuse approx_exp + finalize candidate-major
     * in SIMD groups of 8 (the AVX-512 gauss_finalize_batch width) and
     * BREAK the moment `count` accepts have landed, eliding the approx_exp
     * / finalize for the unused tail of the GAUSS_BATCH window.  Both
     * paths leave any candidate past `count` DISCARDED (the dst write was
     * already
     * `*coefcnt < count`-gated), and the byte cursor always advances the
     * WHOLE MINIBATCH_RAND_BYTES, so the per-lane byte schedule is byte-
     * identical to processing all 32.  The break is on the PUBLIC accept-
     * count (same data-dependence as the mini-batch count). */
    if (count - *coefcnt > (size_t)GAUSS_BATCH) {
        /* ----- common path: every candidate needed, full-width SIMD -----
         */
        {
            PROF_START(t_ae);
            for (j = 0; j < GAUSS_BATCH; j += 4) {
                int xi[4], yi[4];
                uint64_t po[4];
                int g;
                for (g = 0; g < 4; g++) {
                    xi[g] = (int)x[j + g];
                    yi[g] = (int)yv[j + g];
                }
                approx_exp_accept_q64_x4(xi, yi, po);
                for (g = 0; g < 4; g++)
                    phat[j + g] = po[g];
            }
            PROF_STOP(PT_G_APPROXEXP, t_ae);
        }
        {
            PROF_START(t_fin);
#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__) && \
    !defined(GAUSS_FINALIZE_SCALAR)
            int32_t cand[GAUSS_BATCH], negcand[GAUSS_BATCH];
            int32_t accept[GAUSS_BATCH], z0[GAUSS_BATCH];
            gauss_finalize_batch(cand, negcand, accept, z0, x, yv, phat,
                                 tailp, GAUSS_BATCH);
            for (j = 0; j < GAUSS_BATCH; j++) {
                size_t idx =
                    *coefcnt; /* OUTPUT index of the NEXT accept */
                uint32_t sgn =
                    (uint32_t)(signs[idx >> 3] >> (idx & 7)) & 1u;
                uint32_t keep =
                    (uint32_t)accept[j] & (1u ^ ((uint32_t)z0[j] & sgn));
                if (keep)
                    dst[(*coefcnt)++] = sgn ? negcand[j] : cand[j];
            }
#else
            for (j = 0; j < GAUSS_BATCH; j++) {
                int32_t r;
                uint32_t sgn;
                size_t idx =
                    *coefcnt; /* OUTPUT index of the NEXT accept */
                sgn = (uint32_t)(signs[idx >> 3] >> (idx & 7)) & 1u;
                if (gauss_finalize(&r, x[j], yv[j], phat[j],
                                   tailp + (size_t)j * GAUSS_RAND_BYTES,
                                   sgn))
                    dst[(*coefcnt)++] = r;
            }
#endif
            PROF_STOP(PT_G_FINAL, t_fin);
        }
    } else {
        /* ----- last-batch path: cover the candidate window [base,32) in
         * at most two WIDE SIMD passes, breaking the compaction once
         * `count` accepts land.  The first pass spans the candidates we
         * expect to need (remaining accepts rounded up to the 8-candidate
         * SIMD group); the second pass (taken only if a rare in-window
         * rejection left us short) covers the rest, so we NEVER
         * under-process and force an extra mini-batch.  Eliding the tail
         * of approx_exp / finalize past the window is what saves work;
         * keeping ONE wide finalize_batch per pass keeps the per-candidate
         * SIMD efficiency of the common path. Byte-exact: candidates are
         * processed in order with the same per- candidate math and the
         * same coefcnt progression as the full path. ----- */
        size_t remain = count - *coefcnt;     /* >0, <= GAUSS_BATCH */
        int win = (int)((remain + 7u) & ~7u); /* round up to SIMD group */
        int base = 0;
        if (win > GAUSS_BATCH)
            win = GAUSS_BATCH;
        while (base < GAUSS_BATCH && *coefcnt < count) {
            int n = win - base;
            int e = win;
            {
                PROF_START(t_ae);
                for (j = base; j < e; j += 4) {
                    int xi[4], yi[4];
                    uint64_t po[4];
                    int g;
                    for (g = 0; g < 4; g++) {
                        xi[g] = (int)x[j + g];
                        yi[g] = (int)yv[j + g];
                    }
                    approx_exp_accept_q64_x4(xi, yi, po);
                    for (g = 0; g < 4; g++)
                        phat[j + g] = po[g];
                }
                PROF_STOP(PT_G_APPROXEXP, t_ae);
            }
            {
                PROF_START(t_fin);
#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__) && \
    !defined(GAUSS_FINALIZE_SCALAR)
                int32_t cand[GAUSS_BATCH], negcand[GAUSS_BATCH];
                int32_t accept[GAUSS_BATCH], z0[GAUSS_BATCH];
                gauss_finalize_batch(
                    cand + base, negcand + base, accept + base, z0 + base,
                    x + base, yv + base, phat + base,
                    tailp + (size_t)base * GAUSS_RAND_BYTES, n);
                for (j = base; j < e && *coefcnt < count; j++) {
                    size_t idx =
                        *coefcnt; /* OUTPUT index of NEXT accept */
                    uint32_t sgn =
                        (uint32_t)(signs[idx >> 3] >> (idx & 7)) & 1u;
                    uint32_t keep = (uint32_t)accept[j] &
                                    (1u ^ ((uint32_t)z0[j] & sgn));
                    if (keep)
                        dst[(*coefcnt)++] = sgn ? negcand[j] : cand[j];
                }
#else
                for (j = base; j < e && *coefcnt < count; j++) {
                    int32_t r;
                    uint32_t sgn;
                    size_t idx =
                        *coefcnt; /* OUTPUT index of NEXT accept */
                    sgn = (uint32_t)(signs[idx >> 3] >> (idx & 7)) & 1u;
                    if (gauss_finalize(
                            &r, x[j], yv[j], phat[j],
                            tailp + (size_t)j * GAUSS_RAND_BYTES, sgn))
                        dst[(*coefcnt)++] = r;
                }
#endif
                PROF_STOP(PT_G_FINAL, t_fin);
            }
            base = win;
            win =
                GAUSS_BATCH; /* second pass (if needed) covers the rest */
        }
    }
    gs->pos += MINIBATCH_RAND_BYTES; /* WHOLE tail consumed */
}

/* gauss_stream_chunk: scalar per-lane path (A) -- produce a flat run of
 * `count` wide-Gaussian samples, refilling this lane's buffer one scalar
 * xof256 squeeze at a time.  Byte-identical to the scalar reference;
 * kept for the non-USE_AVX512_XOF_NWAY build and the malloc-failure
 * fallback. */
void gauss_stream_chunk(gauss_stream *gs, int32_t *dst, size_t count)
{
    uint8_t signs[SIGN_BYTES_PER_CHUNK + SIGN_PAD_AVX512];
    size_t signbytes = (count + 7) / 8;
    size_t coefcnt = 0;

    memset(signs, 0,
           sizeof(signs)); /* pad zero for the AVX512 LE64 window */
    {
        PROF_START(t_sh0);
        gs_ensure(gs, signbytes);
        PROF_STOP(PT_G_SHAKE, t_sh0);
    }
    gs_load_sign_stream(gs, signs, signbytes);

    while (coefcnt < count) {
        {
            PROF_START(t_sh);
            gs_ensure(gs, MINIBATCH_RAND_BYTES);
            PROF_STOP(PT_G_SHAKE, t_sh);
        }
        gs_consume_minibatch(gs, dst, &coefcnt, count, signs);
    }
}

void sample_y(poly y[KVEC], const uint8_t seedY[SEEDBYTES])
{
    int32_t *ybar = y[0].coeffs; /* KVEC*n contiguous */
    const size_t ntot = (size_t)KVEC * N;
    const size_t wy = ntot / XOF_STREAMS; /* lane chunk width (coeffs) */
    unsigned t;

    _Static_assert(((KVEC * N) % XOF_STREAMS) == 0,
                   "KVEC*n must split into 16 lanes");
    _Static_assert(sizeof(poly) == 4 * N, "poly flat layout");

#if defined(USE_AVX512_XOF_NWAY) && defined(__AVX512F__)
    /* N-WAY BULK PRESET: keep the BULK ~74 KB/sig of SampleY's gauss XOF
     * (PT_G_SHAKE, ~93% of SampleY) on the N-way path via gauss_bulk_setup
     * -- pre-fill BULK_K_SAMPLE_Y blocks for all 16 lanes N-at-a-time,
     * then consume each lane scalar from its private region
     * (gauss_stream_chunk), with the rare (K+1)-th block falling back to a
     * scalar squeeze off the lane's V_K ctx.  Byte-exact to ref. */
    {
        gauss_stream *gss = (gauss_stream *)malloc((size_t)XOF_STREAMS *
                                                   sizeof(gauss_stream));
        if (gss) {
            uint8_t *bulk =
                gauss_bulk_setup(gss, DS_SAMPLE_Y, seedY, BULK_K_SAMPLE_Y);
            if (bulk) {
                for (t = 0; t < XOF_STREAMS; t++)
                    gauss_stream_chunk(&gss[t], ybar + (size_t)t * wy, wy);
                free(bulk);
                free(gss);
                return;
            }
            free(gss);
        }
        /* malloc failure: fall through to the scalar per-lane path. */
    }
#endif

    for (t = 0; t < XOF_STREAMS; t++) {
        gauss_stream gs;
        gauss_stream_init(&gs, DS_SAMPLE_Y, seedY, t);
        gauss_stream_chunk(&gs, ybar + (size_t)t * wy, wy);
    }
}
