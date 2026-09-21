/*
 * sampler.h -- 96-bit reverse-CDT (RCDT) discrete-Gaussian BASE sampler
 *              (scalar reference).
 *
 * This header owns the RCDT *kernel* contract: the constant-time 96-bit
 * borrow-FOLD compare `cdt_scan96`, the two table-parameterized entry
 * points
 * (`sampler_sigma2` for the wide masking / BLISS base sampler, and
 * `noise_magnitude_batch` for the keygen secret-noise magnitude), the
 * grouped 12-byte-per-sample uniform-draw layout, the batch-size /
 * byte-budget macros, the branchless scalar helpers, and the per-set
 * selection of WHICH physical noise table feeds s (sigma_1) and e
 * (sigma_2).
 *
 * It does NOT own: the wide-sampler uniform-y / ApproxExp Bernoulli
 * accept, the keygen vector loop that consumes the magnitudes, nor the
 * sign / zero-fold fold.  This file exposes only the RAW unsigned
 * magnitude scan; the caller does the +/- sign and the 1/2 zero-fold
 * rejection from a separate tail-byte field (see the contract in
 * sampler.c).
 *
 * The threshold tables themselves live in tools/rcdt_tables.h (the
 * @@AUTOGEN:rcdt@@ region emitted by tools/gen_rcdt.py) as `static const`
 * arrays; they are MODE-INDEPENDENT for the wide table (sigma_s = 825/256)
 * and per-sigma for the three noise tables (sigma in {0.85, 0.9, 1.0}).
 *
 *   ===================  INV-NOMAX (table invariant)
 * =================== Every threshold row's MID and HIGH 32-bit limb is !=
 * 0xFFFFFFFF (the LOW limb is unconstrained -- it never receives an
 * incoming borrow).  This is what makes the single-compare borrow-FOLD
 * form `b_i = [v_i <_u Z_i + b_{i-1}]` bit-exact to the textbook
 * "less-or-(equal-and-borrow)" 96-bit compare: under INV-NOMAX, Z_i +
 * b_{i-1} <= 0xFFFFFFFE + 1 = 0xFFFFFFFF, so the threshold-side add never
 * wraps.  The invariant constrains only the PUBLIC table and is asserted
 * offline at table-gen (tools/gen_rcdt.py) and re-asserted at runtime by
 * the test; it is therefore invisible to the constant-time argument.  See
 * sampler.c for the full borrow-chain proof.
 *   =========================================================================
 */
#ifndef SHUTTLE_SAMPLER_H
#define SHUTTLE_SAMPLER_H

#include <stddef.h>
#include <stdint.h>

#include "params.h" /* SHUTTLE_MODE, THETA, WIDE_RCDT_LEN */
/* Disable with -DDISABLE_NAMESPACE=1. */
#include "namespace.h"
/* ---- batch sizes (carried over from the prior 93-bit baseline; both
 * must be whole 16-sample units so the AVX2 16-lane / AVX512 32-lane SIMD
 * forks tile the mini-batch exactly -- byte-budget / lane-equivalence).
 * ----
 */
#define GAUSS_BATCH 32
#define NOISE_BATCH 32
_Static_assert(GAUSS_BATCH % 16 == 0,
               "GAUSS_BATCH must be a whole number of 16-sample units");
_Static_assert(NOISE_BATCH % 16 == 0,
               "NOISE_BATCH must be a whole number of 16-sample units");

/* ---- per-sample / per-mini-batch random-byte budgets.  Each sample
 * is THETA/8 = 12 bytes (3 x 32-bit limbs); the prior 93-bit baseline
 * masked off 3 bits, the 96-bit version uses the full 12 bytes (same
 * budget). ---- */
#define CDT96_SAMPLE_BYTES \
    (THETA / 8) /* 12 bytes / sample (3x32-bit limbs) */
_Static_assert(CDT96_SAMPLE_BYTES == 12,
               "THETA must be 96 (3x32-bit limbs)");

#define SIGMA2_RAND_BYTES (GAUSS_BATCH * CDT96_SAMPLE_BYTES) /* 384 */
#define NOISE_CDT_BYTES (NOISE_BATCH * CDT96_SAMPLE_BYTES)   /* 384 */
/* 4 candidates share one tail byte: bit0 = sign, bit1 = zero-fold
 * (LSB-first); the caller consumes the whole tail block up front for
 * cross-backend determinism. */
#define NOISE_TAIL_BYTES (NOISE_BATCH / 4) /* 8   */
#define NOISE_MINIBATCH_RAND_BYTES \
    (NOISE_CDT_BYTES + NOISE_TAIL_BYTES) /* 392 */

/* ---- AVX2 sign-flip / borrow mask K (see sampler.c flip-commute lemma).
 * The scalar reference does not use it; it is the canonical name shared
 * with the SIMD forks (sketched in sampler.c comments). ---- */
#define CDT96_FLIP 0x80000000U

/* ===================================================================== *
 *  Wide-Gaussian (SampleDGauss / SampleY) mini-batch sizing macros       *
 *                                                                        *
 *  These are PURE DERIVATIONS (closed-form from GAUSS_BATCH and N) -- no *
 *  magic numbers.  They live here (next to GAUSS_BATCH) rather than in   *
 *  params.h so the @@AUTOGEN@@ / check-consts regions are untouched;     *
 *  the divisibility invariants are re-asserted below and in polyvec.c.   *
 *                                                                        *
 *  The wide sampler draws candidates in GAUSS_BATCH-sized mini-batches:  *
 *    - SIGMA_S_RAND_BYTES : 12-byte rho_u per candidate (BaseSampler     *
 *      magnitude, grouped 96B/8-sample limb layout; == SIGMA2_RAND_BYTES)*
 *    - Y_RAND_BYTES       : one full byte y per candidate (k=256,        *
 *      Y_BITS=8, so a plain byte copy, unlike a 7-bit pack)              *
 *    - GAUSS_RAND_BYTES   : the 8-byte (full 64-bit) Bernoulli tail u    *
 *  The whole mini-batch tail is consumed up front so ref==avx2==avx512   *
 *  advance the cursor identically regardless of any early coefcnt==N     *
 *  break (the consumed-bytes determinism rule).                          *
 * ===================================================================== */
#define SIGMA_S_RAND_BYTES (GAUSS_BATCH * CDT96_SAMPLE_BYTES) /* 384 */
#define Y_RAND_BYTES (GAUSS_BATCH)                            /* 32  */
#define GAUSS_RAND_BYTES 8                                    /* u   */
#define MINIBATCH_RAND_BYTES             \
    (SIGMA_S_RAND_BYTES + Y_RAND_BYTES + \
     GAUSS_BATCH * GAUSS_RAND_BYTES) /* 672 */
_Static_assert(SIGMA_S_RAND_BYTES == SIGMA2_RAND_BYTES,
               "SIGMA_S_RAND_BYTES must equal SIGMA2_RAND_BYTES (same "
               "12-byte rho_u limb layout as the noise sampler)");
_Static_assert(Y_RAND_BYTES == GAUSS_BATCH,
               "k=256 => one full byte of y per candidate (Y_BITS=8)");

/* One OUTPUT-indexed sign bit per emitted coefficient, squeezed up front
 * (the output-indexed sign convention).  The magnitude sign of the k-th
 * accepted output is
 * signs[(coefcnt+k)>>3] >> ((coefcnt+k)&7) & 1.  AVX512 over-reads an
 * 8-byte LE window in gauss_finalize_batch, hence SIGN_PAD_AVX512. */
#define SIGN_BYTES_PER_POLY ((N + 7) / 8)
#define SIGN_PAD_AVX512 8

/* ===================================================================== *
 *  Per-set RCDT table selection (which physical noise table per sigma)  *
 * ===================================================================== *
 * The wide table (RCDT_Z) is shared across all three sets (sigma_s =
 * 825/256). The keygen noise sampler uses sigma_1 for s and sigma_2 for
 * e': SHUTTLE-128: (sigma_1, sigma_2) = (0.85, 0.85)  -> both -> 0.85
 * table (9) SHUTTLE-256: (sigma_1, sigma_2) = (0.90, 1.00)  -> 0.90 (10)
 * / 1.00 (11) SHUTTLE-512: (sigma_1, sigma_2) = (0.90, 0.90)  -> both ->
 * 0.90 table (10) (the per-set sigma_1/sigma_2 mapping; RCDT
 * lengths L_{0.85}=9, L_{0.90}=10, L_{1.0}=11.)
 *
 * RCDT_NOISE_S / RCDT_NOISE_E expand to the table symbol names defined in
 * tools/rcdt_tables.h, so a TU that uses them must include rcdt_tables.h.
 * The
 * `*_ENTRIES` macros give the row counts (used to drive the scan and to
 * size caller buffers).  RCDT_Z_ENTRIES mirrors WIDE_RCDT_LEN from
 * params.h. */
#define RCDT_Z_ENTRIES WIDE_RCDT_LEN /* 36 */
_Static_assert(RCDT_Z_ENTRIES == 36,
               "wide RCDT table is 36 rows (sigma_s=825/256)");

#if SHUTTLE_MODE == 128 /* (0.85, 0.85) */
#    define RCDT_NOISE_S SHUTTLE_RCDT_NOISE_0_85
#    define RCDT_NOISE_E SHUTTLE_RCDT_NOISE_0_85
#    define RCDT_NOISE_S_ENTRIES 9
#    define RCDT_NOISE_E_ENTRIES 9
#elif SHUTTLE_MODE == 256 /* (0.90, 1.00) */
#    define RCDT_NOISE_S SHUTTLE_RCDT_NOISE_0_90
#    define RCDT_NOISE_E SHUTTLE_RCDT_NOISE_1_00
#    define RCDT_NOISE_S_ENTRIES 10
#    define RCDT_NOISE_E_ENTRIES 11
#elif SHUTTLE_MODE == 512 /* (0.90, 0.90) */
#    define RCDT_NOISE_S SHUTTLE_RCDT_NOISE_0_90
#    define RCDT_NOISE_E SHUTTLE_RCDT_NOISE_0_90
#    define RCDT_NOISE_S_ENTRIES 10
#    define RCDT_NOISE_E_ENTRIES 10
#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

/* ===================================================================== *
 *  Branchless scalar helpers (all backends share the contract)         *
 * ===================================================================== */

/* little-endian 32-bit load: p[0] | p[1]<<8 | p[2]<<16 | p[3]<<24.  Reads
 * the full 32 bits (NO 31-bit mask -- that was the 93-bit shortcut). */
static inline uint32_t load_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* constant-time unsigned less-than at FULL 32-bit width via a 64-bit
 * subtract: the borrow-out of (a-b) lands in bit 32, so >>32 & 1 is
 * exactly [a <_u b]. Exact for all 32-bit a,b -- this is the scalar oracle
 * the SIMD must match. */
static inline uint32_t ct_lt_u32(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a - (uint64_t)b) >> 32) & 1u;
}

/* 1 iff x == 0, else 0 (constant-time).  (x-1) borrows out only when x==0.
 */
static inline uint32_t ct_is_zero_u32(uint32_t x)
{
    return (uint32_t)(((uint64_t)x - 1u) >> 32) & 1u;
}

/* constant-time select: bit ? a : b (bit must be 0 or 1). */
static inline int32_t ct_sel_i32(uint32_t bit, int32_t a, int32_t b)
{
    uint32_t m = (uint32_t)0u - (bit & 1u); /* 0xFFFFFFFF if bit, else 0 */
    return (int32_t)((m & (uint32_t)a) | (~m & (uint32_t)b));
}

/* ===================================================================== *
 *  Public API                                                          *
 * ===================================================================== */

/* Core RCDT scan over an arbitrary public table Z[entries][3] (satisfying
 * INV-NOMAX).  Reads the grouped 12-byte-per-sample layout
 * (group*96 + j*32 + lane*4, group = s>>3, lane = s&7) from `rand` and
 * writes `batch` int32 magnitudes to `out` in sample order. Constant-time:
 * the loop count is the PUBLIC table length, no secret-dependent
 * branch/index/shift. The scalar reference; the SIMD forks supply
 * cdt_scan96_avx2 / cdt_scan96_avx512 (bit-identical). */
void cdt_scan96(int32_t *out, const uint8_t *rand, const uint32_t Z[][3],
                int entries, int batch);

/* Wide masking / BLISS base sampler magnitude scan (RCDT_Z,
 * sigma_s=825/256), GAUSS_BATCH samples; reads SIGMA2_RAND_BYTES from
 * `rand`.  The uniform-y / ApproxExp accept / sign that turn this
 * magnitude into a signed y-coefficient live in the wide-sampler caller.
 */
void sampler_sigma2(int32_t *z_out, const uint8_t *rand);

/* Keygen secret-noise magnitude scan over a noise table (RCDT_NOISE_S /
 * RCDT_NOISE_E), NOISE_BATCH samples; reads NOISE_CDT_BYTES from `rand`.
 * Returns RAW unsigned magnitudes; the caller applies the sign and
 * the 1/2 zero-fold rejection from the tail bytes.  Generalized to take
 * the table
 * + entries because SHUTTLE-256 needs TWO distinct noise tables. */
void noise_magnitude_batch(int32_t *m_out, const uint8_t *rand,
                           const uint32_t Z[][3], int entries);

/* ===================================================================== *
 *  Wide-Gaussian per-candidate finalize (SampleDGauss step 5-8)         *
 *                                                                       *
 *  Combines the BaseSampler magnitude x, the uniform low byte y, the    *
 *  Q64 ApproxExp accept threshold p_hat, the 8-byte (full 64-bit)       *
 *  Bernoulli tail u, and the OUTPUT-indexed magnitude sign bit into the *
 *  signed wide-Gaussian coefficient (or a rejection).  Branchless apart *
 *  from the (PUBLIC, masking-sampler) accept count.  Returns the {0,1}  *
 *  accept flag; on accept *out holds (-1)^sign * (WIDE_K*x + y).        *
 *                                                                       *
 *  Byte schedule pinned: u is the FULL 64-bit little-endian             *
 *  value of the 8-byte tail; accept iff u < p_hat (spec step 5/6, scale *
 *  2^64).  The z==0 zero-fold (spec step 7) is realized WITHOUT an extra *
 *  byte: a z==0 candidate is kept only when its OUTPUT sign bit is 0     *
 *  (one of the two identical-value signs of 0), halving the 0 mass.     *
 *  Since z==0 forces p_hat ~= 2^64-1 (exp(0)), the Bernoulli accept is   *
 *  ~always true there, so the sign-bit fold is effectively independent  *
 *  of u -- mass-correct to the ApproxExp rel-err (~2^-54).  This keeps   *
 *  the per-candidate tail at exactly GAUSS_RAND_BYTES = 8 (fixed-size,   *
 *  the consumed-bytes determinism rule) and the magnitude sign           *
 *  OUTPUT-indexed. */
int gauss_finalize(int32_t *out, int32_t x, int32_t y, uint64_t p_hat,
                   const uint8_t tail[8], uint32_t sign_bit);

/* ===================================================================== *
 *  Vectorized SIGN-INDEPENDENT gauss finalize precompute (SIMD only)     *
 * ===================================================================== *
 *  Vectorizes the per-candidate part of gauss_finalize that does NOT     *
 *  depend on the OUTPUT-indexed sign bit (which is only known one accept *
 *  at a time): over a whole GAUSS_BATCH mini-batch it computes, for      *
 *  every candidate j in [0,GAUSS_BATCH),                                 *
 *                                                                        *
 *      cand[j]    = WIDE_K*x[j] + y[j]            (256x+y, int32, exact) *
 *      accept[j]  = ( LE64(tail+8j) <_u p_hat[j] )  (0/1, the 64-bit     *
 *                                                    Bernoulli compare) *
 *      z0[j]      = ( cand[j] == 0 )              (0/1, the zero-fold) *
 *      negcand[j] = -cand[j]                                             *
 *                                                                        *
 *  EVERY op is an EXACT integer op (no float, no rounding): the 64-bit   *
 *  UNSIGNED compare is bit-identical to the scalar ct_lt_u64 (avx2: the  *
 *  high-bit sign-flip + signed cmpgt; avx512: native cmplt_epu64_mask).  *
 *  So accept[]/z0[]/cand[]/negcand[] are BIT-IDENTICAL to GAUSS_BATCH    *
 *  scalar gauss_finalize calls; the caller finishes each candidate with  *
 *  the cheap OUTPUT-indexed-sign zero-fold + compaction (byte-exact      *
 *  schedule, KAT-preserving).  Constant-time: pure data-oblivious SIMD   *
 *  arithmetic, no branch / gather / v-dependent index (see sampler.c).   *
 *                                                                        *
 *  Defined ONLY in the SIMD forks (avx2/avx512 sampler.c) under          *
 *  USE_AVX{2,512}_SAMPLER; ref/sampler.c has no SIMD body so it is NOT   *
 *  declared there.  `accept` and `z0` are written as int32 0/1 flags. */
#if (defined(USE_AVX2_SAMPLER) && defined(__AVX2__)) || \
    (defined(USE_AVX512_SAMPLER) && defined(__AVX512F__))
void gauss_finalize_batch(int32_t *cand, int32_t *negcand, int32_t *accept,
                          int32_t *z0, const int32_t *x, const int32_t *y,
                          const uint64_t *p_hat, const uint8_t *tail,
                          int batch);
#endif

#endif /* SHUTTLE_SAMPLER_H */
