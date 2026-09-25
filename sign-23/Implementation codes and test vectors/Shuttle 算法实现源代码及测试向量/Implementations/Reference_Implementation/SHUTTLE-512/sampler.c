/*
 * sampler.c -- 96-bit reverse-CDT (RCDT) discrete-Gaussian base sampler
 *              (PURE SCALAR REFERENCE; the KAT oracle).
 *
 * =========================================================================
 *  What this samples
 * =========================================================================
 * A half-discrete-Gaussian magnitude centred at 0 via reverse-CDT
 * sampling. Each public threshold Z[i] is round(Pr[X >= i+1] * 2^96), a
 * 96-bit integer stored as 3x32-bit little-endian limbs (value = Z[i][0] +
 * Z[i][1]*2^32 + Z[i][2]*2^64).  For a 96-bit uniform draw v, the sample
 * is the count
 *
 *     z = #{ i : v <_u Z[i] }.
 *
 * Because the thresholds are monotone-decreasing, "v is below the first z
 * thresholds and not below the rest" has probability Pr[X = z]; so z
 * follows the half-Gaussian magnitude law.  The loop ALWAYS scans all
 * `entries` rows (no z-dependent early exit), which is the source of the
 * constant-time behaviour: the timing depends only on the PUBLIC table
 * length, never on the secret draw v or the secret output z.
 *
 * Two table families (tools/rcdt_tables.h, emitted by tools/gen_rcdt.py):
 *   - RCDT_Z      (sigma_s = 825/256 = 3.22265625, 36 rows): the wide
 * masking / BLISS base sampler (sampler_sigma2).  MODE-INDEPENDENT.
 *   - RCDT_NOISE_* (sigma in {0.85, 0.9, 1.0}, 9/10/11 rows): keygen
 * secret noise magnitude (noise_magnitude_batch).  Per-set selection in
 * sampler.h.
 *
 * =========================================================================
 *  The 96-bit borrow chain (the table invariant) -- why the borrow-FOLD
 * form, why INV-NOMAX
 * =========================================================================
 * A 96-bit unsigned compare v <_u Z is a borrow chain low->high limb.  The
 * textbook "less-or-(equal-and-borrow)" definition is
 *
 *     b_i = [v_i <_u Z_i]  OR  ([v_i ==_u Z_i] AND b_{i-1}),   b_{-1} = 0,
 *
 * with b_2 = [v <_u Z].  That needs an extra equality compare per limb.
 * The single-compare borrow-FOLD form folds the incoming borrow into the
 * threshold:
 *
 *     b_0 = [v_0 <_u Z_0]
 *     b_1 = [v_1 <_u Z_1 + b_0]               (*)
 *     b_2 = [v_2 <_u Z_2 + b_1]
 *
 * Equivalence: when b_{i-1} = 1, "v_i <_u Z_i + 1" is exactly "v_i <=_u
 * Z_i" (covers the "less-or-equal" branch); when b_{i-1} = 0 it
 * degenerates to "v_i <_u Z_i".  The ONLY trap is Z_i + b_{i-1} wrapping
 * the 32-bit limb: if Z_i = 0xFFFFFFFF and b_{i-1} = 1 then Z_i+1 = 0,
 * "v_i <_u 0" is always false, but the correct answer is "always true".
 * This is exactly why the 93-bit baseline's two 31-bit shortcuts -- (a)
 * signed vpcmpgtd == unsigned compare, and (b) borrow == bit 31 of (v - Z
 * - cc) -- break at full 32 bits.
 *
 * INV-NOMAX removes the trap: for EVERY row the MID and HIGH limbs satisfy
 * Z[i][1] != 0xFFFFFFFF and Z[i][2] != 0xFFFFFFFF (the LOW limb never
 * receives an incoming borrow, so it is unconstrained).  Then Z_i +
 * b_{i-1} <= 0xFFFFFFFE + 1 = 0xFFFFFFFF, never wraps, and (*) is
 * bit-exact to the textbook compare.  This constrains the PUBLIC table
 * only; it is asserted offline in tools/gen_rcdt.py (and re-asserted at
 * runtime by the test), so it is invisible to constant-time.  The hit
 * probability per limb is ~2^-32, whole table ~2^-27 -- effectively never;
 * the high limb is mass-bounded (RCDT_Z's first high limb ~0.78*2^32) so
 * it is automatically far from 0xFFFFFFFF.  If a future regeneration ever
 * hits it, the fix is a +/-1 ulp nudge (~2^-96 probability change,
 * negligible for Renyi) followed by re-running the assert.
 *
 * The scalar reference uses (*) with ct_lt_u32 (a 64-bit subtract, exact
 * at full 32 bits), NOT the 93-bit (v_i - Z_i - cc) >> 31 shift form (that
 * shift reads a garbage data bit at 32-bit width -- the true borrow is in
 * bit 32).
 *
 * =========================================================================
 *  Constant-time
 * =========================================================================
 *   - Loop count = public `entries` (table length).  No early-out on v or
 * z.
 *   - The body is pure data-independent arithmetic (subtract,
 * shift-by-const, and).  No branch, no v-indexed memory access, no
 * v-dependent shift count, no division/modulo.  ct_lt_u32 compiles to a
 * 64-bit sub + shift + and (CMOV-free, branch-free).  The accumulation `z
 * += b` is an unconditional add of a {0,1} value.
 *   - INV-NOMAX is checked offline against the public table --
 * runtime-invisible.
 *
 * GOTCHA (do NOT do this): never fold the borrow into v ("v minus borrow")
 * to save a compare -- v_i - b_{i-1} underflows to 0xFFFFFFFF when v_i =
 * 0, b_{i-1} = 1, inverting the borrow semantics.  The borrow MUST fold
 * into the THRESHOLD side (protected by INV-NOMAX); v is secret with
 * unconstrained values.
 *
 * =========================================================================
 *  SIMD design sketch (NOT built here -- this pass is scalar ref only)
 * =========================================================================
 * The avx2/avx512 forks (BaseSampler.tex sec5/sec6, demo_basesampler.c)
 * keep
 * (*) bit-identical to this scalar oracle:
 *   - AVX512: native unsigned vpcmpltud; borrow folded as a masked +1 on
 * the threshold (_mm512_mask_add_epi32(Z, m, Z, one)).  The ONLY change
 * from the 93-bit cdt_scan32 is dropping the 31-bit load mask; stores raw
 * Z. 32 lanes = two independent 16-lane streams a/b.
 *   - AVX2: no unsigned compare, so flip to signed with K = CDT96_FLIP =
 *     0x80000000.  The flip-commute lemma  (Z + b) ^ K == (Z ^ K) + b (mod
 *     2^32)  [proof: x ^ K = x + 2^31 (mod 2^32), both sides = Z + b +
 * 2^31] lets the table store the PRE-FLIPPED Zf = Z ^ K once; v is XOR'd
 * once on load; the borrow folds by _mm256_sub_epi32(Zf, b_vec) where
 * b_vec is the {0,-1} vpcmpgtd mask, and the compare is signed vpcmpgtd.
 * This deletes the 3 port-0 vpsrld $31 of the 93-bit path (~21% faster on
 * i7-11700K). 16 lanes = two independent 8-lane streams a/b. Both read the
 * SAME grouped byte layout this file reads, so ref==avx2==avx512 is
 * byte-exact over a mini-batch (lane-equivalence).
 */
#include "sampler.h"

#include "rcdt_tables.h" /* SHUTTLE_RCDT_Z, SHUTTLE_RCDT_NOISE_* (static const) */

/*
 * ===================== CT-1 HARDENING (gather defense) =================
 *
 * The scans below (cdt_scan96 / noise_magnitude_batch / gauss_finalize)
 * are data-independent linear sweeps over the PUBLIC RCDT table length and
 * the SEQUENTIAL PRNG byte buffer (the loop index s is a public sample
 * counter; rand+base is a public sequential offset, never a secret-derived
 * index). They are source-level constant-time.  But the constant-time
 * policy is explicit: "do NOT rely on the compiler to preserve
 * constant-time."  Some compilers AUTO-VECTORIZE the per-sample LE32 limb
 * reads into a SIMD GATHER -- e.g.
 *
 *     clang -Os -march=skylake (and newer clang baselines under -mavx2)
 *
 * re-emits `vpgatherqd` over the rand buffer with a vectorized PUBLIC
 * sequential offset (verified: the gather index is
 * group*96+lane*4+{0,32,64} built from the public loop counter; the secret
 * limb VALUES are the loaded data, the ADDRESS is public).  So this is a
 * BENIGN public-index vectorization, NOT a secret-index leak.  But the
 * machine-code CT scanner (tools/ct_scan.py) cannot prove the index is
 * public, so it conservatively (and correctly, per the constant-time
 * policy) FLAGS any gather/scatter in a secret-handling object as a
 * VIOLATION.
 *
 * Fix: the gather is the OUTER per-sample loop being SLP-vectorized to
 * pack 8 independent samples and gather their rand reads.  We force the
 * scan to stay scalar by carrying the per-sample accumulator in a
 * `volatile` int32_t: C99 6.7.3 makes each `z += b` a real side-effecting
 * memory op the optimizer may NOT remove, reorder, or pack across lanes,
 * so the samples are no longer provably independent and NO compiler
 * (gcc/clang x -O0..-O3,-Os) emits a gather/scatter for the scan.  (A
 * volatile view of the rand *bytes* is NOT sufficient -- clang -Os
 * re-gathers the volatile bytes; the loop-carried volatile dependency is
 * what defeats it.)  This is C99 + -Wpedantic clean -- NO GNU inline
 * __asm__ (which warns under -Wpedantic and would break the Reference
 * -Werror gate); `volatile` is ISO C.  The arithmetic is unchanged -- z
 * accumulates the SAME {0,1} adds in the SAME order -- so the result is
 * bit-exact (KAT-neutral); only the instruction selection (scalar loads
 * instead of a gather) changes.  On the mandated gcc -O2/-O3 production
 * build the scan was already scalar, so the barrier costs ~0; under clang
 * -Os it is FASTER (it avoids the slow gather).
 */

/*
 * cdt_scan96 -- scalar borrow-FOLD RCDT scan over an arbitrary public
 * table.
 *
 * Reads the grouped 12-byte-per-sample layout: for sample s, group = s>>3,
 * lane = s&7, and the three 32-bit limbs are at byte offsets
 * group*96 + lane*4 + {0, 32, 64}.  Each group of 8 samples occupies 96
 * bytes (8 lanes x 3 limbs x 4 bytes).  Writes `batch` int32 magnitudes to
 * `out` in sample order.  This is the exact byte schedule the SIMD forks
 * consume (byte-exact), so a scalar mini-batch is byte-identical to the
 * SIMD over the same buffer.
 */
void cdt_scan96(int32_t *out, const uint8_t *rand, const uint32_t Z[][3],
                int entries, int batch)
{
    int s;
    for (s = 0; s < batch; s++) {
        int group = s >> 3;
        int lane = s & 7;
        int base = group * 96 + lane * 4;
        /* full 32-bit limb loads -- NO 31-bit mask (the 96-bit upgrade).
         */
        uint32_t v0 = load_le32(rand + base + 0);
        uint32_t v1 = load_le32(rand + base + 32);
        uint32_t v2 = load_le32(rand + base + 64);
        /* volatile accumulator = gather barrier (CT-1; see the
         * gather-defense block at the top of this file). */
        volatile int32_t z = 0;
        int i;
        for (i = 0; i < entries; i++) {
            /* borrow-FOLD: b folds into the THRESHOLD; exact under
             * INV-NOMAX. */
            uint32_t b = ct_lt_u32(v0, Z[i][0]); /* b0 = [v0 <_u Z0]     */
            b = ct_lt_u32(v1, Z[i][1] + b);      /* b1 = [v1 <_u Z1+b0]  */
            b = ct_lt_u32(v2, Z[i][2] + b);      /* b2 = [v2 <_u Z2+b1]  */
            z = z + (int32_t)b;                  /* unconditional += b   */
        }
        out[s] = z;
    }
}

/* Wide masking / BLISS base sampler (RCDT_Z, sigma_s = 825/256),
 * GAUSS_BATCH samples.  The uniform-y / ApproxExp accept / sign live in
 * the wide-sampler caller. */
void sampler_sigma2(int32_t *z_out, const uint8_t *rand)
{
    cdt_scan96(z_out, rand, SHUTTLE_RCDT_Z, RCDT_Z_ENTRIES, GAUSS_BATCH);
}

/* Keygen secret-noise magnitude scan (RCDT_NOISE_S / RCDT_NOISE_E),
 * NOISE_BATCH samples.  Returns RAW unsigned magnitudes; the caller
 * applies the sign and the 1/2 zero-fold rejection from the tail bytes.
 *
 * KEYGEN-NOISE PIPELINE CONTRACT (the caller owns the loop; this file owns
 * the magnitude + this contract).  Per candidate j, the caller reads a
 * 2-bit tail field  f = (tailp[j>>2] >> (2*(j&3))) & 3  (bit0 = sign, bit1
 * = zero-fold, LSB-first; NOISE_TAIL_BYTES = NOISE_BATCH/4 bytes consumed
 * up front, independent of the cnt==N break, so all backends advance
 * identically), then: reject   = ct_is_zero_u32(m[j]) & (f >> 1); //
 * reject z==0 w.p. 1/2 r[cnt]   = ct_sel_i32(f & 1, -m[j], m[j]);     //
 * apply sign cnt     += 1 ^ reject; The induced post-zero-fold SIGNED
 * stddev (the binding SK metric) is pinned per sigma in
 * tools/log/rcdt_tables.txt (0.85 -> -2^-15.98, the largest gap). */
void noise_magnitude_batch(int32_t *m_out, const uint8_t *rand,
                           const uint32_t Z[][3], int entries)
{
    cdt_scan96(m_out, rand, Z, entries, NOISE_BATCH);
}

/* ===================================================================== *
 *  Wide-Gaussian per-candidate finalize (SampleDGauss step 5-8)         *
 * ===================================================================== *
 * See the contract in sampler.h.  Constant-time decomposition:
 *
 *   cand   = WIDE_K*x + y               (= 256x + y, the candidate |z|)
 *   u      = LE64(tail)                 (full 64-bit Bernoulli draw)
 *   accept = ct_lt_u64(u, p_hat)        (spec step 6: keep iff u < p_hat)
 *   z0     = ct_is_zero(cand)           (the z==0 fold branch)
 *
 * Zero-fold (spec step 7): a z==0 candidate value 0 is identical for both
 * signs, so emitting it for BOTH would double its mass.  We keep it only
 * when the OUTPUT sign bit is 0 (probability 1/2), which is mass-correct
 * and needs NO extra byte.  For z != 0 the sign bit just chooses +/-.
 *
 *   keep   = accept & ~(z0 & sign_bit)
 *   *out   = ct_sel_i32(sign_bit, -cand, cand)
 *
 * All operations are data-independent (the only data-dependent quantity
 * that escapes is the {0,1} accept count -- the standard, whitelisted
 * BLISS/Gaussian rejection-timing leak; the value, sign and fold are
 * branchless).  No table, no division, no float.
 */
static uint64_t ct_lt_u64(uint64_t a, uint64_t b)
{
    /* Branchless unsigned less-than, exact for all 64-bit a,b with no
     * 128-bit type.  Standard Hacker's-Delight identity:
     *   a <_u b  iff  bit 63 of  ( a ^ ((a ^ b) | ((a - b) ^ b)) )  is 1.
     * Returns 0 or 1. */
    uint64_t t = a ^ ((a ^ b) | ((a - b) ^ b));
    return (t >> 63) & 1u;
}

static uint64_t load_le64(const uint8_t *p)
{
    uint64_t x = 0;
    int i;
    for (i = 0; i < 8; i++)
        x |= (uint64_t)p[i] << (8 * i);
    return x;
}

int gauss_finalize(int32_t *out, int32_t x, int32_t y, uint64_t p_hat,
                   const uint8_t tail[8], uint32_t sign_bit)
{
    int32_t cand = WIDE_K * x + y; /* 256x + y */
    uint64_t u = load_le64(tail);  /* full 64-bit Bernoulli draw */
    uint32_t accept = (uint32_t)ct_lt_u64(u, p_hat); /* u < p_hat */
    uint32_t z0 = ct_is_zero_u32((uint32_t)cand);    /* cand == 0 */
    /* zero-fold: drop a z==0 candidate when sign_bit == 1 (half it). */
    uint32_t keep = accept & (1u ^ (z0 & (sign_bit & 1u)));
    *out = ct_sel_i32(sign_bit & 1u, -cand, cand);
    return (int)keep;
}
