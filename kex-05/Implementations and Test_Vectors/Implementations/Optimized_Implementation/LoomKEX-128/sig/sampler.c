/*
 * sampler.c -- 96-bit reverse-CDT (RCDT) discrete-Gaussian base sampler
 *              (AVX2 FORK).
 *
 * =========================================================================
 *  What this is
 * =========================================================================
 * This is the AVX2 fork of ref/sampler.c.  It is BYTE-EXACT to the scalar
 * reference: the public `cdt_scan96` writes the IDENTICAL `batch` int32
 * magnitudes the scalar oracle writes for the same grouped PRNG buffer, so
 * `make check-kat` reproduces the EXACT ref NGCC/SHA3 hashes.  Only the
 * INSTRUCTION SELECTION inside the RCDT scan changes (an 8-way SIMD borrow
 * chain instead of a per-sample scalar loop); the consumed bytes, the
 * table, the borrow-fold arithmetic and the output values are all
 * unchanged.
 *
 * The scalar body below (`cdt_scan96_scalar`, `sampler_sigma2`,
 * `noise_magnitude_batch`, `gauss_finalize`) is copied VERBATIM from
 * ref/sampler.c (the KAT oracle).  Under -DUSE_AVX2_SAMPLER the public
 * entry point `cdt_scan96` dispatches the 16-sample-aligned bulk through
 * the AVX2 kernel `cdt_scan96_avx2` and finishes any `batch % 16`
 * remainder with the scalar loop (in practice GAUSS_BATCH == NOISE_BATCH
 * == 32, an exact 2x 16-sample tiling, so the scalar tail never runs for
 * the production mini-batches -- it exists only for
 * completeness/robustness).  When USE_AVX2_SAMPLER is NOT defined the file
 * is bit-for-bit the scalar ref.
 *
 * =========================================================================
 *  The AVX2 SIMD scan (cdt_scan96_avx2) -- why it is byte-exact
 * =========================================================================
 * The scan replaced is the per-sample scalar loop in cdt_scan96_scalar.
 * For each sample s the scalar code reads three 32-bit limbs at the
 * grouped byte offsets group*96 + lane*4 + {0,32,64} (group = s>>3, lane =
 * s&7) and counts z = #{ i : v <_u Z[i] } via the borrow-FOLD chain
 * b0=[v0<Z0], b1=[v1<Z1+b0], b2=[v2<Z2+b1].
 *
 * In the grouped layout the 8 lanes of one group hold their v0 limbs in
 * the contiguous 32 bytes at group*96+0, their v1 limbs at group*96+32 and
 * their v2 limbs at group*96+64.  So a single _mm256_loadu_si256 at
 * group*96+{0,32, 64} loads the 8 lanes' v0/v1/v2 in lane order 0..7 --
 * the SIMD lane l is exactly scalar sample group*8+l.  The kernel
 * processes TWO groups per call (stream a = group 2k, stream b = group
 * 2k+1 => 16 samples), running the two independent 8-lane borrow chains in
 * parallel to hide the per-entry latency.
 *
 * AVX2 has no unsigned 32-bit compare, so we flip to the signed domain
 * with K = CDT96_FLIP = 0x80000000 (x^K == x + 2^31 mod 2^32,
 * order-preserving): a <_u b  <=>  (a^K) <_s (b^K). To fold the incoming
 * borrow on a PRE-FLIPPED threshold we use the flip-commute lemma  (Z+b)^K
 * == (Z^K)+b (mod 2^32)  [both = Z+b+2^31], so the table is XOR-K'd once
 * (Zf below, on the stack per dispatch since cdt_scan96 takes an arbitrary
 * public table), v is XOR-K'd once on load, and the borrow folds into Zf
 * by _mm256_sub_epi32(Zf, b_vec) where b_vec is the {0,-1} vpcmpgtd mask.
 * This is byte-for-byte the validated kernel exercised by the
 * basesampler demo's avx2_count8 (4e6-case fuzz: 0 mismatches vs the
 * scalar eq|lt reference).
 * Under INV-NOMAX (mid/high limb != 0xFFFFFFFF) the threshold-side add
 * never wraps, so the fold is bit-exact to the textbook 96-bit compare and
 * hence to the scalar oracle (the table invariant).  The accumulate is
 * _mm256_sub_epi32(z, b_vec) (+= the {0,1} borrow), identical to the
 * scalar `z += b`.
 *
 * =========================================================================
 *  Constant-time
 * =========================================================================
 * The SIMD scan is data-oblivious by construction, exactly like the
 * scalar:
 *   - The outer loop count is batch/16 (public) and the inner loop count
 * is the PUBLIC table length `entries`; no early-out on v or z.
 *   - The body is pure SIMD arithmetic (vpxor / vpcmpgtd / vpsubd) on
 * whole registers -- no branch, no v-indexed memory access (the only loads
 * are the contiguous grouped rand buffer at a PUBLIC sequential offset and
 * the broadcast of the PUBLIC table limb), no v-dependent shift count, no
 *     gather/scatter, no division/modulo, no float.
 *   - The per-dispatch table flip writes a public table to a public stack
 *     buffer; INV-NOMAX is a public-table property checked offline.
 * The AVX2 path therefore has the SAME (data-independent) timing profile
 * as the scalar oracle; the only data-dependent timing that escapes is the
 * caller's BLISS/zero-fold accept COUNT (a public masking-sampler
 * property, whitelisted by the constant-time policy), which lives in
 * polyvec.c, not here.
 *
 * NB the scalar fallback path keeps the `volatile z` gather-barrier from
 * the reference (see the long comment below) so that even the `batch % 16`
 * tail (when present) is not auto-vectorized into a secret-index gather.
 */
#include "sampler.h"

#include "rcdt_tables.h" /* SHUTTLE_RCDT_Z, SHUTTLE_RCDT_NOISE_* (static const) */

#if defined(USE_AVX2_SAMPLER) && defined(__AVX2__)
#    include <immintrin.h>
#endif

/*
 * ===================== CT GATHER HARDENING (gather defense) =============
 *
 * (Verbatim from ref/sampler.c.)  The scalar scan below is a
 * data-independent linear sweep over the PUBLIC RCDT table length and the
 * SEQUENTIAL PRNG byte buffer.  It is source-level constant-time, but some
 * compilers AUTO-VECTORIZE the per-sample LE32 limb reads into a SIMD
 * GATHER over the (public, sequential) offsets.  The machine-code CT
 * scanner cannot prove the index is public and conservatively FLAGS any
 * gather/scatter in a secret-handling object.  We force the scalar scan to
 * stay scalar by carrying the per-sample accumulator in a `volatile`
 * int32_t (C99 6.7.3: each `z += b` is a real side-effecting memory op the
 * optimizer may not remove, reorder, or pack across lanes), so no compiler
 * emits a gather/scatter for it.  The arithmetic is unchanged, so the
 * result is bit-exact (KAT-neutral).  On the AVX2 path the explicit
 * intrinsics below already pin the instruction selection, so the volatile
 * barrier matters only for the (normally unused) scalar tail.
 */

/*
 * cdt_scan96_scalar -- scalar borrow-FOLD RCDT scan over a public table.
 * Reads the grouped 12-byte-per-sample layout (group*96 + lane*4 +
 * {0,32,64}; group = s>>3, lane = s&7) and writes `batch` int32 magnitudes
 * to `out` in sample order.  This is the BYTE-EXACT oracle the AVX2 kernel
 * reproduces. Only used on the non-AVX2 fallback build (the AVX2 dispatch
 * inlines its own scalar tail for the normally-absent batch % 16
 * remainder).
 */
#if !(defined(USE_AVX2_SAMPLER) && defined(__AVX2__))
static void cdt_scan96_scalar(int32_t *out, const uint8_t *rand,
                              const uint32_t Z[][3], int entries,
                              int batch)
{
    int s;
    for (s = 0; s < batch; s++) {
        int group = s >> 3;
        int lane = s & 7;
        int base = group * 96 + lane * 4;
        uint32_t v0 = load_le32(rand + base + 0);
        uint32_t v1 = load_le32(rand + base + 32);
        uint32_t v2 = load_le32(rand + base + 64);
        volatile int32_t z = 0; /* gather barrier */
        int i;
        for (i = 0; i < entries; i++) {
            uint32_t b = ct_lt_u32(v0, Z[i][0]); /* b0 = [v0 <_u Z0]     */
            b = ct_lt_u32(v1, Z[i][1] + b);      /* b1 = [v1 <_u Z1+b0]  */
            b = ct_lt_u32(v2, Z[i][2] + b);      /* b2 = [v2 <_u Z2+b1]  */
            z = z + (int32_t)b;                  /* unconditional += b   */
        }
        out[s] = z;
    }
}
#endif /* !(USE_AVX2_SAMPLER && __AVX2__) */

#if defined(USE_AVX2_SAMPLER) && defined(__AVX2__)
/*
 * cdt_scan96_avx2 -- the 16-sample (two 8-lane streams a/b) AVX2
 * borrow-fold scan over a PRE-FLIPPED table Zf (Zf[i][j] = Z[i][j] ^
 * CDT96_FLIP).
 *
 * `rand` points at the start of two consecutive 96-byte groups (192
 * bytes): stream a limbs: rand+{0,32,64}      (group g,   8 lanes ->
 * out[0..7]) stream b limbs: rand+{96,128,160}   (group g+1, 8 lanes ->
 * out[8..15]) Byte-identical to two scalar groups; bit-identical to the
 * scalar borrow chain under INV-NOMAX (see the file header and
 * demo_basesampler.c).
 */
static void cdt_scan96_avx2(int32_t *out, const uint8_t *rand,
                            const uint32_t (*Zf)[3], int entries)
{
    const __m256i kflip = _mm256_set1_epi32((int)CDT96_FLIP);
    /* flip v once on load (the SAME grouped bytes the scalar reads) */
    __m256i v0a = _mm256_xor_si256(
        _mm256_loadu_si256((const __m256i *)(rand + 0)), kflip);
    __m256i v1a = _mm256_xor_si256(
        _mm256_loadu_si256((const __m256i *)(rand + 32)), kflip);
    __m256i v2a = _mm256_xor_si256(
        _mm256_loadu_si256((const __m256i *)(rand + 64)), kflip);
    __m256i v0b = _mm256_xor_si256(
        _mm256_loadu_si256((const __m256i *)(rand + 96)), kflip);
    __m256i v1b = _mm256_xor_si256(
        _mm256_loadu_si256((const __m256i *)(rand + 128)), kflip);
    __m256i v2b = _mm256_xor_si256(
        _mm256_loadu_si256((const __m256i *)(rand + 160)), kflip);
    __m256i za = _mm256_setzero_si256();
    __m256i zb = _mm256_setzero_si256();
    int i;
    for (i = 0; i < entries; i++) {
        __m256i Z0 = _mm256_set1_epi32((int)Zf[i][0]);
        __m256i Z1 = _mm256_set1_epi32((int)Zf[i][1]);
        __m256i Z2 = _mm256_set1_epi32((int)Zf[i][2]);
        /* b = -borrow (the {0,-1} vpcmpgtd mask); fold by sub on Zf */
        __m256i ba = _mm256_cmpgt_epi32(Z0, v0a); /* -b0 */
        __m256i bb = _mm256_cmpgt_epi32(Z0, v0b);
        ba =
            _mm256_cmpgt_epi32(_mm256_sub_epi32(Z1, ba), v1a); /* Zf1+b0 */
        bb = _mm256_cmpgt_epi32(_mm256_sub_epi32(Z1, bb), v1b);
        ba =
            _mm256_cmpgt_epi32(_mm256_sub_epi32(Z2, ba), v2a); /* Zf2+b1 */
        bb = _mm256_cmpgt_epi32(_mm256_sub_epi32(Z2, bb), v2b);
        za = _mm256_sub_epi32(za, ba); /* += b2 */
        zb = _mm256_sub_epi32(zb, bb);
    }
    _mm256_storeu_si256((__m256i *)(out + 0), za);
    _mm256_storeu_si256((__m256i *)(out + 8), zb);
}

/* Max RCDT table length across the suite (RCDT_Z = 36 rows). */
#    define CDT96_MAX_ENTRIES 36
#endif /* USE_AVX2_SAMPLER && __AVX2__ */

/*
 * cdt_scan96 -- public dispatch.  Under -DUSE_AVX2_SAMPLER the 16-sample
 * aligned bulk runs the AVX2 kernel (with a per-dispatch pre-flipped copy
 * of the public table), and any `batch % 16` remainder finishes on the
 * scalar loop -- byte-exact to the scalar oracle for every byte consumed.
 * Without the macro this is the verbatim scalar reference.
 */
void cdt_scan96(int32_t *out, const uint8_t *rand, const uint32_t Z[][3],
                int entries, int batch)
{
#if defined(USE_AVX2_SAMPLER) && defined(__AVX2__)
    uint32_t Zf[CDT96_MAX_ENTRIES][3];
    int bulk = batch & ~15; /* 16-sample-aligned bulk */
    int blk;
    int i;
    /* Pre-flip the public table once (Zf = Z ^ CDT96_FLIP per limb). */
    for (i = 0; i < entries; i++) {
        Zf[i][0] = Z[i][0] ^ CDT96_FLIP;
        Zf[i][1] = Z[i][1] ^ CDT96_FLIP;
        Zf[i][2] = Z[i][2] ^ CDT96_FLIP;
    }
    for (blk = 0; blk < bulk; blk += 16) {
        /* two 96-byte groups -> 16 samples; rand stride = 2*96 = 192. */
        cdt_scan96_avx2(out + blk, rand + (size_t)(blk >> 4) * 192,
                        (const uint32_t(*)[3])Zf, entries);
    }
    if (bulk < batch) {
        /* scalar tail for the (normally absent) batch % 16 remainder. */
        int s;
        for (s = bulk; s < batch; s++) {
            int group = s >> 3;
            int lane = s & 7;
            int base = group * 96 + lane * 4;
            uint32_t v0 = load_le32(rand + base + 0);
            uint32_t v1 = load_le32(rand + base + 32);
            uint32_t v2 = load_le32(rand + base + 64);
            volatile int32_t z = 0;
            for (i = 0; i < entries; i++) {
                uint32_t b = ct_lt_u32(v0, Z[i][0]);
                b = ct_lt_u32(v1, Z[i][1] + b);
                b = ct_lt_u32(v2, Z[i][2] + b);
                z = z + (int32_t)b;
            }
            out[s] = z;
        }
    }
#else
    cdt_scan96_scalar(out, rand, Z, entries, batch);
#endif
}

/* Wide masking / BLISS base sampler (RCDT_Z, sigma_s = 825/256),
 * GAUSS_BATCH samples.  The uniform-y / ApproxExp accept / sign are
 * applied by the caller. */
void sampler_sigma2(int32_t *z_out, const uint8_t *rand)
{
    cdt_scan96(z_out, rand, SHUTTLE_RCDT_Z, RCDT_Z_ENTRIES, GAUSS_BATCH);
}

/* Keygen secret-noise magnitude scan (RCDT_NOISE_S / RCDT_NOISE_E),
 * NOISE_BATCH samples.  Returns RAW unsigned magnitudes; the caller
 * applies the sign and the 1/2 zero-fold rejection from the tail bytes.
 * See the contract in sampler.h / ref/sampler.c. */
void noise_magnitude_batch(int32_t *m_out, const uint8_t *rand,
                           const uint32_t Z[][3], int entries)
{
    cdt_scan96(m_out, rand, Z, entries, NOISE_BATCH);
}

/* ===================================================================== *
 *  Wide-Gaussian per-candidate finalize (SampleDGauss step 5-8)        *
 * ===================================================================== *
 * Verbatim from ref/sampler.c.  Per-candidate, cheap (one Q64 compare + a
 * couple of branchless selects); kept scalar.  Constant-time apart from
 * the (public) accept count.  See the contract in sampler.h. */
static uint64_t ct_lt_u64(uint64_t a, uint64_t b)
{
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
    int32_t cand = WIDE_K * x + y;                   /* 256x + y */
    uint64_t u = load_le64(tail);                    /* full 64-bit draw */
    uint32_t accept = (uint32_t)ct_lt_u64(u, p_hat); /* u < p_hat */
    uint32_t z0 = ct_is_zero_u32((uint32_t)cand);    /* cand == 0 */
    uint32_t keep = accept & (1u ^ (z0 & (sign_bit & 1u)));
    *out = ct_sel_i32(sign_bit & 1u, -cand, cand);
    return (int)keep;
}

#if defined(USE_AVX2_SAMPLER) && defined(__AVX2__)
/*
 * ===================================================================== *
 *  gauss_finalize_batch -- AVX2 vectorized SIGN-INDEPENDENT precompute  *
 *  (PT_G_FINAL).  BYTE-EXACT to GAUSS_BATCH scalar gauss_finalize.     *
 * ===================================================================== *
 *
 *  RIGOROUS PRECISION + SECURITY ANALYSIS (the three points; see also    *
 *  the BaseSampler design notes on the gauss finalize).                 *
 *
 *  ----------------------------------------------------------------------
 *  (1) EXACTNESS -- every SIMD op is an EXACT integer op, no float, no
 *      rounding.  Per 4-wide block (avx2 4x 64-bit lanes) we compute:
 *
 *      cand = WIDE_K*x + y : 256x+y is built with vpslld $8 (256*x) +
 *          vpaddd (+y) on int32 lanes.  x in [0,36], y in [0,255] => cand
 *          in [0,9471], no int32 overflow; vpslld/vpaddd are exact 2^32
 *          modular adds and the values are far from the modulus, so the
 *          result is the EXACT integer 256x+y, == the scalar `WIDE_K*x+y`.
 *      negcand = -cand : vpsubd from zero, exact two's-complement negate,
 *          == scalar `-cand`.
 *      z0 = (cand==0) : vpcmpeqd against zero, yields the {0,-1} lane
 *          mask; we AND with 1 to get the {0,1} flag.  Bit-identical to
 *          the scalar ct_is_zero_u32((uint32_t)cand) (cand==0 <=> the
 *          32-bit value is 0).
 *      u = LE64(tail+8j) : the 8 tail bytes per candidate are contiguous
 *          little-endian, so a vpmovzxbd-free plain 256-bit load of 4x
 *          8-byte words IS the 4 LE64 draws u[0..3] (x86 is little-endian;
 *          a 64-bit load of 8 LE bytes == load_le64).  EXACT, byte-for-
 *          byte the scalar load_le64(tail).
 *      accept = (u <_u p_hat) : the PRECISION-CRITICAL op.  AVX2 has no
 *          unsigned 64-bit compare, so we use the EXACT order-preserving
 *          sign-flip:  a <_u b  <=>  (a ^ 2^63) <_s (b ^ 2^63).  Proof:
 *          XOR-ing bit 63 maps the unsigned order [0,2^64) bijectively and
 *          monotonically onto the signed order [-2^63,2^63) (it adds 2^63
 *          mod 2^64, i.e. rotates the wrap point), so unsigned-< on a,b
 *          equals signed-< on the flipped values for ALL 64-bit a,b.  We
 *          flip u and p_hat by vpxor with 0x8000000000000000 and test
 *          (uf <_s pf) via vpcmpgtq(pf, uf) (a {0,-1} lane mask), then AND
 *          with 1.  This is an EXACT 64-bit integer comparison with NO
 *          rounding, bit-identical to the scalar ct_lt_u64(u, p_hat)
 *          (which is the Hacker's-Delight branchless unsigned-< -- also
 *          exact; both compute the same boolean for every (u,p_hat)).
 *
 *      Therefore accept[j], z0[j], cand[j], negcand[j] emitted here are
 *      BIT-IDENTICAL to what the scalar gauss_finalize computes for the
 *      same (x[j], y[j], p_hat[j], tail+8j).  The caller's downstream
 *      keep = accept & ~(z0 & sign_bit) and out = sign?negcand:cand are
 *      then trivially identical -- so the emitted coefficient stream and
 *      the KAT hash are UNCHANGED.
 *
 *  ----------------------------------------------------------------------
 *  (2) DISTRIBUTION / PRECISION PRESERVATION -- because the accept
 *      decision accept[j] = [u < p_hat] is bit-identical to the scalar
 *      reference for every candidate, the Bernoulli acceptance event is
 *      IDENTICAL, hence the emitted discrete-Gaussian distribution is
 *      identical.  p_hat is produced by the UNTOUCHED ApproxExp kernel
 *      (approx_exp_accept_q64{,_x4}); this routine only CONSUMES it.  So
 *      the scalar precision budget carries over UNCHANGED: ApproxExp
 *      worst-case relative error 2^-54.49 vs the per-set accept gates
 *      eta_max = 2^-51.25 / 2^-51.98 / 2^-52.97 (128/256/512); the
 *      BaseSampler RCDT Renyi divergence R_1045 = 1 + 2^-95.40; and the
 *      z==0 zero-fold mass-halving (kept only when sign_bit==0).  This
 *      vectorization introduces ZERO additional error -- the compare is an
 *      exact integer compare, not a re-approximation of exp.
 *
 *  ----------------------------------------------------------------------
 *  (3) SECURITY / CONSTANT-TIME -- the SIMD body is pure data-oblivious
 *      register arithmetic (vpslld / vpaddd / vpsubd / vpcmpeqd / vpxor /
 *      vpcmpgtq / vpand) over the PUBLIC-offset contiguous mini-batch
 *      buffers; NO branch, NO data-dependent index, NO gather/scatter, NO
 *      v-dependent shift, NO division, NO float.  The loads are all at
 *      PUBLIC sequential offsets (the mini-batch cursor).  Its timing is
 *      data-independent.  The ONLY data-dependent control flow stays in
 *      the caller's scalar tail (the accept-count -> coefcnt advance),
 *      which is IDENTICAL to the scalar reference and is the documented,
 *      whitelisted masking-sampler rejection-timing channel (the
 *      isochronous emitted-distribution argument).  No new timing /
 *      cache / branch leak is introduced; the machine-code CT scanner
 *      (ct_scan) stays CLEAN (no gather/scatter emitted here).
 *
 *  Writes accept[j], z0[j] as int32 {0,1} flags and cand[j], negcand[j]
 *  as the signed magnitudes.  `batch` is a whole number of 4 (GAUSS_BATCH
 *  == 32); any batch % 4 remainder finishes on a byte-exact scalar tail.
 */
void gauss_finalize_batch(int32_t *cand, int32_t *negcand, int32_t *accept,
                          int32_t *z0, const int32_t *x, const int32_t *y,
                          const uint64_t *p_hat, const uint8_t *tail,
                          int batch)
{
    const __m256i kflip =
        _mm256_set1_epi64x((long long)0x8000000000000000LL);
    const __m256i one32 = _mm256_set1_epi32(1);
    int j;
    int bulk = batch & ~3; /* 4-candidate-aligned bulk */
    for (j = 0; j < bulk; j += 4) {
        /* cand = 256*x + y (exact int32; x in [0,36], y in [0,255]). */
        __m128i xv = _mm_loadu_si128((const __m128i *)(x + j));
        __m128i yv = _mm_loadu_si128((const __m128i *)(y + j));
        __m128i cv = _mm_add_epi32(_mm_slli_epi32(xv, 8), yv); /* 256x+y */
        __m128i ncv = _mm_sub_epi32(_mm_setzero_si128(), cv);  /* -cand  */
        /* z0 = (cand == 0) as {0,1}. */
        __m128i z0v = _mm_and_si128(
            _mm_cmpeq_epi32(cv, _mm_setzero_si128()), _mm_set1_epi32(1));
        _mm_storeu_si128((__m128i *)(cand + j), cv);
        _mm_storeu_si128((__m128i *)(negcand + j), ncv);
        _mm_storeu_si128((__m128i *)(z0 + j), z0v);
        /* u = 4x LE64 Bernoulli draws (contiguous 8 bytes/candidate). */
        __m256i u =
            _mm256_loadu_si256((const __m256i *)(tail + (size_t)j * 8));
        __m256i ph = _mm256_loadu_si256((const __m256i *)(p_hat + j));
        /* accept = (u <_u ph): sign-flip bit 63 then signed cmpgt. */
        __m256i uf = _mm256_xor_si256(u, kflip);
        __m256i pf = _mm256_xor_si256(ph, kflip);
        __m256i acc64 = _mm256_cmpgt_epi64(pf, uf); /* {0,-1}: u <_u ph */
        /* pack the 4x 64-bit {0,-1} masks to 4x int32 {0,1} flags. */
        __m256i acc_and = _mm256_and_si256(acc64, one32);
        /* lanes (as 32-bit): [a0lo a0hi a1lo a1hi | a2lo a2hi a3lo a3hi];
         * the {0,1} flag sits in each 64-bit lane's LOW 32 bits.  Shuffle
         * the four low dwords (positions 0,2 of each 128-bit half) down to
         * the bottom 128 bits, then store 4x int32. */
        __m256i packed = _mm256_permutevar8x32_epi32(
            acc_and, _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7));
        _mm_storeu_si128((__m128i *)(accept + j),
                         _mm256_castsi256_si128(packed));
    }
    /* byte-exact scalar tail for any batch % 4 (absent for
     * GAUSS_BATCH=32). */
    for (; j < batch; j++) {
        int32_t c = WIDE_K * x[j] + y[j];
        uint64_t u = load_le64(tail + (size_t)j * 8);
        cand[j] = c;
        negcand[j] = -c;
        accept[j] = (int32_t)ct_lt_u64(u, p_hat[j]);
        z0[j] = (int32_t)ct_is_zero_u32((uint32_t)c);
    }
}
#endif /* USE_AVX2_SAMPLER && __AVX2__ */
