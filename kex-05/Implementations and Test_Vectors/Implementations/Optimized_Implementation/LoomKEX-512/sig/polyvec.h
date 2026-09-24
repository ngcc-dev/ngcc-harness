/*
 * polyvec.h -- vector-level XOF-driven sampling surface for SHUTTLE.
 *
 * This header owns the deterministic seed expanders and the 16-lane
 * matrix/vector samplers that sit between the unified XOF layer and
 * the top-level KeyGen/Sign orchestration:
 *
 *     expand_seeds          (DS 0x00, xof256, single one-shot)
 *     expand_signing_seeds  (DS 0x01, xof256, single one-shot)
 *     expand_a              (DS 0x02, xof128, 16 lanes; A direct in NTT)
 *     expand_s              (DS 0x03, xof256, 16 lanes; BaseSampler+fold)
 *     sample_c              (DS 0x07, xof256, single ctx; Fisher-Yates)
 *     sample_y              (DS 0x08, xof256, 16 lanes; wide Gaussian r)
 *
 * It pins the exact per-attempt PRNG byte schedule, the 16-lane stream
 * partition, the OUTPUT-indexed sign stream, the cursor-advance
 * determinism rule, and the batch-orchestration plumbing
 * (gauss_stream) so that ref == avx2 == avx512 is byte-exact within each
 * MODE.  It is the KAT-fragile heart of the scheme.
 *
 * ===================================================================== *
 *  THE NGCC DRBG NO-RATE-CURSOR RULE (binding) -- read this.         *
 * ===================================================================== *
 * Under NGCC_MODE the SM3 Hash-DRBG behind xof256_squeeze has NO sub-call
 * rate cursor: every squeeze is a FRESH `SM3_DRNG_Generate` that produces
 * bytes from the current internal state V and then advances V by exactly
 * ONE step, regardless of how many bytes were requested.  Two consecutive
 * xof256_squeeze calls therefore DO NOT concatenate like SHAKE's
 * rate-buffered squeeze: the first byte of the second squeeze restarts
 * from a fresh generate, not from where the first left off.  (Verified
 * empirically: a 64-byte squeeze's prefix equals a 32-byte squeeze, but
 * its tail differs from a following 32-byte squeeze.)
 *
 * CONSEQUENCE: each logical stream is opened with ONE xof_init and then
 * advanced purely by repeated FIXED-SIZE xof_squeeze calls off the SAME
 * persisted ctx.  Under NGCC each squeeze is one Generate that advances V
 * by exactly one step (independent of length), so a chain of fixed-size
 * squeezes is a proper per-lane DRBG stream; under SHA3 the squeezes are
 * the rate-buffered continuation of one stream.  Because the per-squeeze
 * block size is byte-determining under NGCC, it is PINNED identically
 * across ref/avx2/avx512 (the gauss_block_bytes / UNIFORM_DRAW /
 * SAMPLEC_DRAW units, all multiples of XOF_SQUEEZE_GRANULARITY_BYTES). The
 * two MODEs are two DISTINCT KAT sets; within each MODE, ref/avx2/avx512
 * must be byte-exact.
 *
 * The absorbed nonce layout for the 16-lane / single-ctx producers is thus
 *
 *     tag || seed || IntegerToBytes(stream_idx, 2)
 *
 * (no refill counter -- the persisted-ctx chain supplies every block); the
 * single-context one-shots (ExpandSeeds/ExpandSigningSeeds) need no stream
 * index because their output length is fixed and small.  SampleC is
 * single-stream, so it uses stream_idx = 0.
 */
#ifndef SHUTTLE_POLYVEC_H
#define SHUTTLE_POLYVEC_H

#include <stddef.h>
#include <stdint.h>

#include "params.h"
/* Disable with -DDISABLE_NAMESPACE=1. */
#include "namespace.h"
#include "poly.h"
#include "sampler.h"
#include "symmetric.h"

/* RNDBYTES: the per-signature randomness length fed to ExpandSigningSeeds.
 * For deterministic/KAT signing this is a fixed value owned by Sign;
 * this header only fixes the byte LENGTH (= SEEDBYTES, the Dilithium rnd
 * width).
 */
#ifndef RNDBYTES
#    define RNDBYTES SEEDBYTES
#endif

/* ===================================================================== *
 *  Deterministic seed expanders (single-context one-shot, xof256)       *
 * ===================================================================== */

/* T (len = 5*LAMBDA/8) = ExpandSeeds(0x00 || xi || IntToBytes(EM,2) ||
 *                                    IntToBytes(kappa,4)).
 * Slice T into seedA(SEEDBYTES) | seedsk(CHALLENGESEEDBYTES) |
 * masterSeed K(CHALLENGESEEDBYTES).  KeyGen passes kappa>=1. */
#define EXPAND_SEEDS_BYTES (5 * LAMBDA / 8) /* 80 / 160 / 320 */
void expand_seeds(uint8_t T[EXPAND_SEEDS_BYTES],
                  const uint8_t xi[SEEDBYTES], uint32_t kappa);

/* seedY (len = SEEDBYTES) = ExpandSigningSeeds(0x01 || K || rnd || mu ||
 *                                              IntToBytes(kappa,4)).
 * K = masterSeed (CHALLENGESEEDBYTES), rnd (RNDBYTES), mu
 * (CHALLENGESEEDBYTES).  Sign passes the CURRENT kappa (first iter 0). */
void expand_signing_seeds(uint8_t seedY[SEEDBYTES],
                          const uint8_t K[CHALLENGESEEDBYTES],
                          const uint8_t rnd[RNDBYTES],
                          const uint8_t mu[CHALLENGESEEDBYTES],
                          uint32_t kappa);

/* ===================================================================== *
 *  ExpandA (xof128, tag 0x02, 16 lanes; A_gen direct in NTT domain)     *
 * ===================================================================== */
/* agen : coeff-domain, EM polys, every coeff in [0,q).
 * hAgen: NTT-domain, EM*ELL polys, every coeff in [0,q).  hAgen is written
 * in canonical (ref ascending-index == bit-reversed) NTT order; the
 * scalar ref needs no nttunpack (the AVX forks import via
 * poly_ntt_import).
 */
void expand_a(poly16 agen[EM], poly16 hAgen[EM * ELL],
              const uint8_t seedA[SEEDBYTES]);

/* ===================================================================== *
 *  ExpandS (xof256, tag 0x03, 16 lanes; BaseSampler + zero-fold + sign) *
 * ===================================================================== */
/* s1s2: ONE contiguous poly[ELL+EM]; the caller aliases
 *   poly *s = s1s2;  poly *e = s1s2 + ELL;
 * |s|<=L_sigma1, |e|<=L_sigma2.  Coefficient domain (signed int32). */
void expand_s(poly s1s2[ELL + EM],
              const uint8_t seedsk[CHALLENGESEEDBYTES]);

/* ===================================================================== *
 *  SampleC (xof256, tag 0x07, single ctx; partial Fisher-Yates)         *
 * ===================================================================== */
/* Binary challenge c in {0,1}^n, Hamming weight TAU, all set bits +1. */
void sample_c(poly *c, const uint8_t seedC[CHALLENGESEEDBYTES]);

/* ===================================================================== *
 *  SampleY / SampleDGauss (xof256, tag 0x08, 16 lanes; D_{Z,r=825})     *
 * ===================================================================== */
/* y: KVEC (= 1+ELL+EM) polys, flattened across 16 lanes; each coeff is a
 * wide discrete-Gaussian sample (scheme/coefficient domain, signed int32).
 */
void sample_y(poly y[KVEC], const uint8_t seedY[SEEDBYTES]);

/* ===================================================================== *
 *  gauss_stream -- persistent per-lane XOF buffer for the wide sampler   *
 *                                                                       *
 *  The scalar reference services one lane at a time (the degenerate      *
 *  LANES=1 fold of the fixed 16-stream flow).  gauss_stream wraps the    *
 *  SINGLE-INIT-THEN-SQUEEZE discipline: gauss_stream_init opens the lane *
 *  ctx ONCE (nonce = tag||seed||LE16(lane), no refill); gs_fill draws    *
 *  the next gauss_block_bytes block in a single xof256 squeeze off that  *
 *  persisted ctx; gs_ensure advances the chain (one more squeeze) when   *
 *  the cursor would run past `avail`.  The whole mini-batch tail is      *
 *  requested at once so the cursor advances identically across backends.
 */
/* The wide-sampler lane chunk is KVEC*n/16 coeffs; its OUTPUT-indexed sign
 * stream is therefore (KVEC*n/16 + 7)/8 bytes (+ AVX512 LE64 read pad). */
#define SIGN_BYTES_PER_CHUNK (((KVEC * N / XOF_STREAMS) + 7) / 8)

/* ---- Right-sized per-squeeze block (no over-squeezed tail) ------------
 * * Each gauss_stream block is one xof256 squeeze of
 * gauss_block_bytes(tag) bytes off the persisted per-lane ctx; consumption
 * reads contiguous regions of that one squeeze.  The block is the EXACT
 * minimum that holds one logical unit (one mini-batch, plus the sign
 * stream on the first block), with no magic slack:
 *
 *   SampleY (tag 0x08): one GAUSS_BATCH mini-batch is
 * MINIBATCH_RAND_BYTES. The FIRST block ALSO holds the up-front
 * OUTPUT-indexed sign stream (SIGN_BYTES_PER_CHUNK bytes, + the AVX512
 * LE64 over-read pad) ahead of the first mini-batch, so the block must
 * cover SIGN_BYTES_PER_CHUNK + SIGN_PAD_AVX512 + MINIBATCH_RAND_BYTES;
 * that is also enough for every later block (which holds exactly one
 * mini-batch after the small leftover is memmoved to the front -- never
 * enough to skip a squeeze, so one consume per block). ExpandS (tag 0x03):
 * one noise mini-batch is NOISE_MINIBATCH_RAND_BYTES, consumed whole per
 * block -- the block is exactly that.
 *
 * The per-squeeze block size is byte-determining under NGCC, so it is the
 * SAME fixed size on every squeeze of the chain and is PINNED identically
 * across ref/avx2/avx512.  GAUSS_STREAM_BLOCK is the larger of the two
 * (the buffer/fill unit shared by ref and the N-way SIMD fill);
 * gauss_block_bytes() picks the tag-tuned size at fill time.  Buffer =
 * 2*block: one freshly drawn block plus the memmoved leftover of the
 * previous one. */
#define GAUSS_BLOCK_Y \
    ((size_t)SIGN_BYTES_PER_CHUNK + SIGN_PAD_AVX512 + MINIBATCH_RAND_BYTES)
#define GAUSS_BLOCK_S ((size_t)NOISE_MINIBATCH_RAND_BYTES)
#define GAUSS_STREAM_BLOCK \
    (GAUSS_BLOCK_Y > GAUSS_BLOCK_S ? GAUSS_BLOCK_Y : GAUSS_BLOCK_S)
typedef struct {
    xof_ctx ctx;         /* persisted per-lane xof256 state            */
    uint8_t tag;         /* domain-separation tag (DS_SAMPLE_Y)        */
    const uint8_t *seed; /* lane seed (SEEDBYTES)                      */
    uint16_t lane;       /* logical stream index 0..15                 */
    size_t pos, avail;   /* cursor into buf                            */
    /* AVX N-way bulk source (ref leaves these 0 -> gs_fill always XOF).
     * When the AVX forks pre-fill K blocks N-at-a-time, gs_fill draws each
     * block from `bulk_ptr` (bulk_left counts the remaining pre-filled
     * blocks) instead of squeezing; once exhausted it falls back to a
     * scalar squeeze off ctx (already advanced to V_K -- copied out of the
     * N-way ctx under NGCC, or re-derived by ff_blocks fast-forward under
     * SHA3). */
    const uint8_t *bulk_ptr; /* next pre-filled block, or NULL          */
    size_t bulk_left;        /* pre-filled blocks remaining             */
    size_t ff_blocks;        /* SHA3 fallback: blocks to fast-forward    */
    uint8_t
        buf[2 * GAUSS_STREAM_BLOCK]; /* one block + memmoved leftover */
} gauss_stream;

/* Cost-optimal N-way bulk preset K (blocks pre-filled per lane by the AVX
 * forks before per-lane scalar consume).  Measured empirically (320k J
 * samples/cell via the MEASURE_JK hook) and pinned to K* = the smallest K
 * with per-lane overflow P(J>K) <= 1/16 -- the zero-crossing of the cost
 * model E[cost](K) = K + 16*sum_{j>K}(j-K)P(J=j) (one extra bulk block
 * costs one N-way squeeze and saves the ~1 of 16 lanes still active
 * there). The fallback path is mandatory anyway (a fixed K can be exceeded
 * by an unlucky seed) -- K only trades bulk-waste against fallback-cost,
 * it is NEVER a correctness constant (KAT is identical for any K).
 * Reproduced (and CI-checkable via --check) by tools/gen_bulk_k.sh; per
 * (sampler,mode), 320k samples/cell, with the measured per-lane overflow
 * P(J>K*): ExpandS  jmax 7 / 10 / 18,  K* = 6 / 9 / 16,  P(J>K*)
 * 2e-4/6e-4/0.030 SampleY  jmax 5 /  8 / 14,  K* = 5 / 7 / 13,  P(J>K*) 0
 * /0.051/0.014 */
#if SHUTTLE_MODE == 128
#    define BULK_K_EXPAND_S 6
#    define BULK_K_SAMPLE_Y 5
#elif SHUTTLE_MODE == 256
#    define BULK_K_EXPAND_S 9
#    define BULK_K_SAMPLE_Y 7
#elif SHUTTLE_MODE == 512
#    define BULK_K_EXPAND_S 16
#    define BULK_K_SAMPLE_Y 13
#endif
static inline size_t gauss_bulk_k(uint8_t tag)
{
    return (tag == DS_SAMPLE_Y) ? (size_t)BULK_K_SAMPLE_Y
                                : (size_t)BULK_K_EXPAND_S;
}

/* Tag-tuned per-squeeze block size (bytes drawn in one squeeze). */
static inline size_t gauss_block_bytes(uint8_t tag)
{
    return (tag == DS_SAMPLE_Y) ? (size_t)GAUSS_BLOCK_Y
                                : (size_t)GAUSS_BLOCK_S;
}

void gauss_stream_init(gauss_stream *gs, uint8_t tag, const uint8_t *seed,
                       unsigned lane);
/* Ensure at least `need` contiguous bytes are available at
 * gs->buf+gs->pos; memmoves the leftover to the front and draws a fresh
 * one-shot block. */
void gs_ensure(gauss_stream *gs, size_t need);
/* Produce a flat run of `count` wide-Gaussian samples into dst[0..count).
 * The OUTPUT-indexed sign stream covers exactly these `count` outputs and
 * is squeezed up front; the wide mini-batches then fill them. `count`
 * is the lane chunk width KVEC*n/16 (<= GAUSS_STREAM_BLOCK-worth of
 * signs).
 */
void gauss_stream_chunk(gauss_stream *gs, int32_t *dst, size_t count);

#endif /* SHUTTLE_POLYVEC_H */
