/*
 * polyvec.c -- vector-level XOF-driven sampling surface (PURE SCALAR
 *              REFERENCE; the KAT oracle).
 *
 * Implements ExpandSeeds / ExpandSigningSeeds / ExpandA / ExpandS /
 * SampleC / SampleDGauss / SampleY.  The AVX2/AVX512 forks must be
 * byte-identical to this scalar oracle within each MODE.
 *
 * Read polyvec.h FIRST for the binding NGCC-DRBG no-rate-cursor rule and
 * the 16-stream nonce layout: each logical stream is opened with ONE
 * XOF.Init (nonce = tag||seed||LE16(lane), no refill counter) and is then
 * advanced purely by repeated fixed-size XOF.Squeeze calls off the SAME
 * persisted ctx.  Under NGCC each squeeze is one SM3 DRBG Generate that
 * evolves V by one step (independent of length), so a chain of fixed-size
 * squeezes is a proper per-lane DRBG stream; the per-squeeze block size is
 * pinned identically across ref/avx2/avx512.  This is what keeps
 * ref==avx2==avx512 byte-exact and what makes the cursor advance
 * independent of any early rejection break.
 *
 * ===================================================================== *
 *  PINNED PRNG byte schedules (KAT-NORMATIVE)                            *
 * ===================================================================== *
 * ExpandSeeds        : absorb 0x00 || xi || LE16(EM) || LE32(kappa);
 *                      squeeze 5*LAMBDA/8 bytes. * ExpandSigningSeeds :
 * absorb 0x01 || K || rnd || mu || LE32(kappa);     * squeeze SEEDBYTES
 * bytes.                           * ExpandA (per lane) : nonce 0x02 ||
 * seedA || LE16(t); single init    * per coeff REPEAT{ 2 bytes; a = LE
 * & (2^DQ-1) }     * UNTIL a < q.  agen chunk THEN hAgen chunk.         *
 * ExpandS (per lane) : nonce 0x03 || seedsk || LE16(t); single init      *
 *                      per coeff via a NOISE-style mini-batch: 384-byte *
 *                      grouped rho_u (cdt_scan96) + 8-byte 2-bit tail *
 *                      (bit0 sign, bit1 zero-fold), whole tail up front. *
 *                      s chunk (RCDT_NOISE_S) THEN e chunk
 * (RCDT_NOISE_E).* SampleC            : nonce 0x07 || seedC || LE16(0);
 * single init     * per draw REPEAT{ BN bytes; j = LE & (2^DN-1) }     *
 *                      UNTIL j <= i. * SampleY (per lane) : nonce 0x08 ||
 * seedY || LE16(t); single init    * per poly: SIGN_BYTES_PER_POLY
 * up-front sign bits,  * then mini-batches of GAUSS_BATCH candidates: *
 *                      384-byte rho_u (cdt_scan96 RCDT_Z) + 32-byte y + *
 *                      256-byte (32x8) Bernoulli tails, whole tail up *
 *                      front; accept via gauss_finalize. *
 */
#include "polyvec.h"

#include <string.h>

#include "approx_exp.h" /* approx_exp_accept_q64_x4, approx_exp_accept_q64 */
#include "rcdt_tables.h" /* SHUTTLE_RCDT_Z, SHUTTLE_RCDT_NOISE_* */
#include "test/prof.h" /* PT_G_SHAKE/BASESAMP/APPROXEXP/FINAL -- ((void)0) unless PROF_TIME */

#ifdef MEASURE_JK
/* OFF-PATH measurement hook (gated; zero effect on normal builds): count
 * the number of per-lane XOF blocks J each gauss_stream consumes, to pin
 * the N-way bulk-preset K for the AVX forks (K* = ceil(15/16-quantile of
 * J)). shuttle_jk_blocks bumps once per gs_fill (one block); begin/end
 * snapshot a lane's J into shuttle_jk_hist. Read by test/measure_jk.c. */
unsigned long shuttle_jk_blocks = 0;
unsigned long shuttle_jk_hist[8192] = {0};
static unsigned long shuttle_jk_snap = 0;
static void shuttle_jk_begin(void)
{
    shuttle_jk_snap = shuttle_jk_blocks;
}
static void shuttle_jk_end(void)
{
    unsigned long j = shuttle_jk_blocks - shuttle_jk_snap;
    shuttle_jk_hist[j < 8192 ? j : 8191]++;
}
#endif

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
 *  Each lane opens ONE XOF instance (nonce = tag||seedA||LE16(lane))    *
 *  and is advanced purely by repeated fixed-size UNIFORM_DRAW squeezes  *
 *  off that persisted ctx.                                              *
 * ===================================================================== */
/* The rejection sampler reads each lane's stream as BQ-byte candidates.
 * Each coeff accepts with prob ~q/2^DQ, so the per-lane chunk (agen then
 * hAgen, total EM*n*(1+ELL)/16 coeffs) needs only ~that many BQ-byte
 * candidates.  We draw the stream in UNIFORM_DRAW-byte slices from the
 * persisted XOF ctx: under NGCC each squeeze is one SM3 DRBG Generate that
 * evolves V by one step, so successive UNIFORM_DRAW squeezes form a proper
 * per-lane chain; under SHA3 they are the rate-buffered continuation of
 * one stream.  The fixed UNIFORM_DRAW block size is what keeps ref and the
 * N-way AVX forks byte-exact; only the unused tail is never squeezed. */

/* Right-sized continuation slice: ceil-to-granularity of ~2x the mean
 * per-lane byte need (per-lane coeffs * BQ * 2, mean acceptance ~q/2^DQ).
 * Covers the largest mode in one slice with margin, so the slow lazy
 * continuation almost never runs; it is the FIXED per-squeeze block size
 * that ref and the AVX forks must agree on byte-for-byte.
 */
#define UNIFORM_LANE_COEFFS ((size_t)EM * N * (1u + ELL) / XOF_STREAMS)
#define UNIFORM_DRAW_RAW (UNIFORM_LANE_COEFFS * (size_t)BQ * 2u)
#define UNIFORM_DRAW                                                    \
    (((UNIFORM_DRAW_RAW + (size_t)XOF_SQUEEZE_GRANULARITY_BYTES - 1u) / \
      (size_t)XOF_SQUEEZE_GRANULARITY_BYTES) *                          \
     (size_t)XOF_SQUEEZE_GRANULARITY_BYTES)

/* Right-sized continuation slice for SampleC's partial Fisher-Yates.
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
 * buf. If the persisted ctx is not yet live (e.g. the initial block was
 * filled by the N-way batched path), init it from the nonce and
 * fast-forward past the `drawn` bytes already produced -- replaying the
 * chain in UNIFORM_DRAW units yields the identical V evolution, so the
 * bytes match either way.
 */
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

static void us_init(uniform_stream *us, const uint8_t *seedA,
                    unsigned lane)
{
    us->nonce[0] = DS_EXPAND_A;
    memcpy(us->nonce + 1, seedA, SEEDBYTES);
    us->nonce_len = 1 + SEEDBYTES + 2;
    us->lane = (uint16_t)lane;
    us_fill(us);
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

/* Sample `count` uniform coeffs in [0,q) (ascending index) into `dst`. */
static void uniform_reject_chunk(uniform_stream *us, uint16_t *dst,
                                 size_t count)
{
    size_t k;
    for (k = 0; k < count; k++) {
        uint16_t a;
        do {
            a = us_next_candidate(us); /* REPEAT { 2B; mask } UNTIL a<q */
        } while (a >= (uint16_t)Q);
        dst[k] = a;
    }
}

/* ===================================================================== *
 *  ExpandA (DS 0x02, xof128, 16 lanes; A_gen direct in NTT domain)      *
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

    /* Each lane fills its agen chunk THEN its hAgen chunk, from the SAME
     * per-lane stream (the contiguous flattening per the spec).  hAgen is
     * written ascending-index = canonical NTT order (ref needs no
     * nttunpack; the AVX forks poly_ntt_import the canonical poly). */
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
/* gauss_stream services ExpandS (noise mini-batches) and SampleY (wide
 * mini-batches).  Both consume their per-lane stream as a sequence of
 * fixed-size mini-batches whose whole tail is drawn up front.  The
 * struct is declared in polyvec.h.  GAUSS_STREAM_BLOCK is sized to hold a
 * SampleY poly's sign stream + one wide mini-batch, the larger of the two
 * uses; ExpandS mini-batches (NOISE_MINIBATCH_RAND_BYTES = 392) fit too.
 */

void gauss_stream_init(gauss_stream *gs, uint8_t tag, const uint8_t *seed,
                       unsigned lane)
{
    /* Single XOF.Init per lane (nonce = tag||seed||LE16(lane), no refill);
     * the stream is then advanced purely by gs_fill squeezes off gs->ctx.
     * seed length differs by tag: SampleY uses SEEDBYTES, ExpandS uses
     * CHALLENGESEEDBYTES. */
    uint8_t nonce[1 + CHALLENGESEEDBYTES + 2];
    size_t seedlen = (tag == DS_SAMPLE_Y) ? SEEDBYTES : CHALLENGESEEDBYTES;
    gs->tag = tag;
    gs->seed = seed;
    gs->lane = (uint16_t)lane;
    gs->pos = 0;
    gs->avail = 0;
    gs->bulk_ptr = NULL; /* ref: always XOF, no N-way bulk source */
    gs->bulk_left = 0;
    gs->ff_blocks = 0;
    nonce[0] = tag;
    memcpy(nonce + 1, seed, seedlen);
    put_le16(nonce + 1 + seedlen, gs->lane);
    xof256_init(&gs->ctx, nonce, 1 + seedlen + 2);
}

/* Draw one gauss_block_bytes block in ONE xof256 squeeze off the persisted
 * gs->ctx, appending after any leftover already memmoved to the front of
 * buf.  Under NGCC each squeeze is one SM3 Generate (V advances one step);
 * the fixed block size is what keeps ref and the AVX forks byte-exact. */
static void gs_fill(gauss_stream *gs)
{
    size_t blk = gauss_block_bytes(gs->tag);
#ifdef MEASURE_JK
    shuttle_jk_blocks++;
#endif
    xof256_squeeze(&gs->ctx, gs->buf + gs->avail, blk);
    gs->avail += blk;
}

void gs_ensure(gauss_stream *gs, size_t need)
{
    if (gs->avail - gs->pos >= need)
        return;
    /* memmove leftover to the front, draw the next block off the chain. */
    {
        size_t left = gs->avail - gs->pos;
        if (left)
            memmove(gs->buf, gs->buf + gs->pos, left);
        gs->pos = 0;
        gs->avail = left;
        gs_fill(gs);
    }
}

/* ===================================================================== *
 *  ExpandS (DS 0x03, xof256, 16 lanes; BaseSampler + zero-fold + sign)  *
 * ===================================================================== */
/* One noise mini-batch: cdt_scan96 over `Z`/`entries` (NOISE_BATCH=32),
 * then a 2-bit-per-candidate tail (bit0 sign, bit1 zero-fold), 4
 * cand/byte; the WHOLE NOISE_MINIBATCH_RAND_BYTES (392) is consumed up
 * front so the cursor advances independently of the cnt==want early break.
 * Appends accepted signed coeffs to dst[*cnt..]; stops at `want`.
 */
static void noise_minibatch(gauss_stream *gs, int32_t *dst, size_t *cnt,
                            size_t want, const uint32_t Z[][3],
                            int entries)
{
    int32_t mag[NOISE_BATCH];
    const uint8_t *tailp;
    int j;
    gs_ensure(gs, NOISE_MINIBATCH_RAND_BYTES);
    noise_magnitude_batch(mag, gs->buf + gs->pos, Z, entries);
    tailp = gs->buf + gs->pos + NOISE_CDT_BYTES; /* 2-bit fields */
    for (j = 0; j < NOISE_BATCH; j++) {
        /* f = (tailp[j>>2] >> (2*(j&3))) & 3 : bit0 = sign, bit1 = fold */
        uint32_t f = (uint32_t)(tailp[j >> 2] >> (2 * (j & 3))) & 3u;
        uint32_t reject = ct_is_zero_u32((uint32_t)mag[j]) & (f >> 1);
        int32_t r = ct_sel_i32(f & 1u, -mag[j], mag[j]);
        if (*cnt < want && !reject) /* placement only; cursor already
                                       advanced */
            dst[(*cnt)++] = r;
    }
    gs->pos += NOISE_MINIBATCH_RAND_BYTES; /* WHOLE tail consumed */
}

void expand_s(poly s1s2[ELL + EM],
              const uint8_t seedsk[CHALLENGESEEDBYTES])
{
    /* Flat views over the s (ELL*n) and e (EM*n) blocks of s1s2. */
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

    for (t = 0; t < XOF_STREAMS; t++) {
        gauss_stream gs;
        size_t cnt;
        /* s chunk uses RCDT_NOISE_S; e chunk uses RCDT_NOISE_E.  Both ride
         * the SAME per-lane stream (s first, then e), so the byte cursor
         * threads s->e exactly like the spec's flattened lane chunk. */
#ifdef MEASURE_JK
        shuttle_jk_begin();
#endif
        gauss_stream_init(&gs, DS_EXPAND_S, seedsk, t);
        cnt = 0;
        while (cnt < ws)
            noise_minibatch(&gs, sbar + (size_t)t * ws, &cnt, ws,
                            RCDT_NOISE_S, RCDT_NOISE_S_ENTRIES);
        cnt = 0;
        while (cnt < we)
            noise_minibatch(&gs, ebar + (size_t)t * we, &cnt, we,
                            RCDT_NOISE_E, RCDT_NOISE_E_ENTRIES);
#ifdef MEASURE_JK
        shuttle_jk_end();
#endif
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
     * the fixed block size is what keeps ref and the AVX forks
     * byte-exact). Variable-time loop is fine here (seedC is public). */
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

    /* for i = n-tau .. n-1: REPEAT{ BN bytes; j = LE & (2^DN-1) } UNTIL
     * j<=i; then c[i]=c[j]; c[j]=1. */
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
 * ===================================================================== */
/* gauss_stream_chunk: produce a flat run of `count` wide-Gaussian samples
 * (the lane chunk KVEC*n/16).
 *   1. up-front OUTPUT-indexed sign stream: (count+7)/8 bytes -- ONE
 *      bit per output, indexed by the running accept counter.
 *   2. mini-batches of GAUSS_BATCH candidates until `count` accepted:
 *        - cdt_scan96(x[32], buf+pos) over RCDT_Z (advance 384)
 *        - y[32] = buf[pos..pos+32]   (advance 32; Y_BITS=8 byte copy)
 *        - p_hat[32] via approx_exp_accept_q64_x4 (8 groups of 4)
 *        - per-candidate gauss_finalize, tail at buf+pos+8*j (advance 256)
 *      The whole MINIBATCH_RAND_BYTES is consumed up front. */
void gauss_stream_chunk(gauss_stream *gs, int32_t *dst, size_t count)
{
    uint8_t signs[SIGN_BYTES_PER_CHUNK + SIGN_PAD_AVX512];
    size_t signbytes = (count + 7) / 8;
    size_t coefcnt = 0;

    /* (1) up-front sign bits (one per OUTPUT sample in this chunk). */
    memset(signs, 0,
           sizeof(signs)); /* pad zero for the AVX512 LE64 window */
    {
        PROF_START(t_sh0);
        gs_ensure(gs, signbytes);
        PROF_STOP(PT_G_SHAKE, t_sh0);
    }
    memcpy(signs, gs->buf + gs->pos, signbytes);
    gs->pos += signbytes;

    /* (2) wide mini-batches. */
    while (coefcnt < count) {
        int32_t x[GAUSS_BATCH];
        int32_t yv[GAUSS_BATCH];
        uint64_t phat[GAUSS_BATCH];
        const uint8_t *yp, *tailp;
        int j;

        {
            PROF_START(t_sh);
            gs_ensure(gs, MINIBATCH_RAND_BYTES);
            PROF_STOP(PT_G_SHAKE, t_sh);
        }
        /* magnitude x via RCDT_Z (grouped 96B/8-sample layout) */
        {
            PROF_START(t_bs);
            sampler_sigma2(x, gs->buf + gs->pos);
            PROF_STOP(PT_G_BASESAMP, t_bs);
        }
        yp = gs->buf + gs->pos + SIGMA_S_RAND_BYTES;
        for (j = 0; j < GAUSS_BATCH; j++)
            yv[j] = (int32_t)yp[j]; /* Y_BITS=8: plain byte copy */
        tailp = gs->buf + gs->pos + SIGMA_S_RAND_BYTES + Y_RAND_BYTES;
        /* Two-path body.  When more than a full GAUSS_BATCH accepts are
         * still outstanding (count-coefcnt > GAUSS_BATCH) every one of the
         * 32 candidates is needed, so we run the full window (approx_exp
         * then finalize) -- byte-identical to before.  Otherwise this
         * MIGHT be the last mini-batch: we fuse approx_exp +
         * gauss_finalize candidate- major (groups of 4 for the x4
         * approx_exp API) and BREAK the moment `count` accepts have
         * landed, eliding the approx_exp / finalize for the unused tail of
         * the GAUSS_BATCH window.  Both paths leave any candidate past
         * `count` DISCARDED (the dst write was already `coefcnt <
         * count`-gated), and the byte cursor always advances the WHOLE
         * MINIBATCH_RAND_BYTES, so the per-lane byte schedule is byte-
         * identical to processing all 32.  The break is on the PUBLIC
         * accept- count (same data-dependence as the mini-batch count). */
        if (count - coefcnt > (size_t)GAUSS_BATCH) {
            /* ----- common path: every candidate needed ----- */
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
                for (j = 0; j < GAUSS_BATCH; j++) {
                    int32_t r;
                    uint32_t sgn;
                    size_t idx = coefcnt; /* OUTPUT index of NEXT accept */
                    sgn = (uint32_t)(signs[idx >> 3] >> (idx & 7)) & 1u;
                    if (gauss_finalize(
                            &r, x[j], yv[j], phat[j],
                            tailp + (size_t)j * GAUSS_RAND_BYTES, sgn))
                        dst[coefcnt++] = r;
                }
                PROF_STOP(PT_G_FINAL, t_fin);
            }
        } else {
            /* ----- last-batch path: fuse + break when `count` accepts
             * land, group-of-4 granular (safe: keeps going across
             * in-window rejections until count is reached, up to the full
             * GAUSS_BATCH). ----- */
            for (j = 0; j < GAUSS_BATCH && coefcnt < count; j += 4) {
                int xi[4], yi[4];
                uint64_t po[4];
                int g;
                {
                    PROF_START(t_ae);
                    for (g = 0; g < 4; g++) {
                        xi[g] = (int)x[j + g];
                        yi[g] = (int)yv[j + g];
                    }
                    approx_exp_accept_q64_x4(xi, yi, po);
                    for (g = 0; g < 4; g++)
                        phat[j + g] = po[g];
                    PROF_STOP(PT_G_APPROXEXP, t_ae);
                }
                {
                    PROF_START(t_fin);
                    for (g = 0; g < 4; g++) {
                        int32_t r;
                        uint32_t sgn;
                        size_t idx =
                            coefcnt; /* OUTPUT idx of NEXT accept */
                        sgn =
                            (uint32_t)(signs[idx >> 3] >> (idx & 7)) & 1u;
                        if (gauss_finalize(
                                &r, x[j + g], yv[j + g], phat[j + g],
                                tailp + (size_t)(j + g) * GAUSS_RAND_BYTES,
                                sgn)) {
                            if (coefcnt < count)
                                dst[coefcnt++] = r;
                        }
                    }
                    PROF_STOP(PT_G_FINAL, t_fin);
                }
            }
        }
        gs->pos += MINIBATCH_RAND_BYTES; /* WHOLE tail consumed */
    }
}

void sample_y(poly y[KVEC], const uint8_t seedY[SEEDBYTES])
{
    /* y flattened to ybar[0..KVEC*n-1]; chunk width w_y = KVEC*n/16.  Lane
     * t fills the FLAT chunk ybar[t*w_y .. (t+1)*w_y).  The flattening is
     * by COEFFICIENT index (the spec's "for each flat index k in chunk t:
     * ybar[k] <- SampleDGauss"), so a lane chunk is a contiguous run that
     * may cross poly boundaries -- which is fine, the output poly array is
     * one contiguous int32 buffer. */
    int32_t *ybar = y[0].coeffs; /* KVEC*n contiguous */
    const size_t ntot = (size_t)KVEC * N;
    const size_t wy = ntot / XOF_STREAMS; /* lane chunk width (coeffs) */
    unsigned t;

    _Static_assert(((KVEC * N) % XOF_STREAMS) == 0,
                   "KVEC*n must split into 16 lanes");
    _Static_assert(sizeof(poly) == 4 * N, "poly flat layout");

    for (t = 0; t < XOF_STREAMS; t++) {
        gauss_stream gs;
#ifdef MEASURE_JK
        shuttle_jk_begin();
#endif
        gauss_stream_init(&gs, DS_SAMPLE_Y, seedY, t);
        gauss_stream_chunk(&gs, ybar + (size_t)t * wy, wy);
#ifdef MEASURE_JK
        shuttle_jk_end();
#endif
    }
}
