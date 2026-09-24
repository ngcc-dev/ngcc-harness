/*
 * sampler.c -- 96-bit reverse-CDT (RCDT) discrete-Gaussian base sampler
 *              (AVX-512 FORK).
 *
 * =========================================================================
 *  What this is
 * =========================================================================
 * This is the AVX-512 fork of ref/sampler.c.  It is BYTE-EXACT to the
 * scalar reference: the public `cdt_scan96` writes the IDENTICAL `batch`
 * int32 magnitudes the scalar oracle writes for the same grouped PRNG
 * buffer, so `make check-kat` reproduces the EXACT ref NGCC/SHA3 hashes.
 * Only the INSTRUCTION SELECTION inside the RCDT scan changes (a 16-way
 * SIMD borrow chain instead of a per-sample scalar loop); the consumed
 * bytes, the table, the borrow-fold arithmetic and the output values are
 * all unchanged.
 *
 * The scalar body below (`cdt_scan96_scalar`, `sampler_sigma2`,
 * `noise_magnitude_batch`, `gauss_finalize`) is copied VERBATIM from
 * ref/sampler.c (the KAT oracle).  Under -DUSE_AVX512_SAMPLER the public
 * entry point `cdt_scan96` dispatches the 16-sample-aligned bulk through
 * the AVX-512 kernel `cdt_scan96_avx512` and finishes any `batch % 16`
 * remainder with the scalar loop (in practice GAUSS_BATCH == NOISE_BATCH
 * == 32, an exact 2x 16-sample tiling, so the scalar tail never runs for
 * the production mini-batches -- it exists only for
 * completeness/robustness).  When USE_AVX512_SAMPLER is NOT defined the
 * file is bit-for-bit the scalar ref.
 *
 * =========================================================================
 *  The AVX-512 SIMD scan (cdt_scan96_avx512) -- why it is byte-exact
 * =========================================================================
 * The scan replaced is the per-sample scalar loop in cdt_scan96_scalar.
 * For each sample s the scalar code reads three 32-bit limbs at the
 * grouped byte offsets group*96 + lane*4 + {0,32,64} (group = s>>3, lane =
 * s&7) and counts z = #{ i : v <_u Z[i] } via the borrow-FOLD chain
 * b0=[v0<Z0], b1=[v1<Z1+b0], b2=[v2<Z2+b1].
 *
 * In the grouped layout the 8 lanes of one group hold their v0 limbs in
 * the contiguous 32 bytes at group*96+0, their v1 limbs at group*96+32 and
 * their v2 limbs at group*96+64.  TWO consecutive groups (192 bytes) thus
 * hold their 16 v0 limbs at bytes {0..31, 96..127}, etc.  The AVX-512
 * kernel processes those 16 lanes per call by loading the two 32-byte v0
 * halves into one 64-byte __m512i (the SIMD lane l = scalar sample
 * 2*group*8 + l for l<8, and (2*group+1)*8 + (l-8) for l>=8), running ONE
 * 16-lane borrow chain.  This is exactly the validated kernel
 * `avx512_count16` exercised by the basesampler demo (5e5-case
 * fuzz with eq/0xFFFFFFFF corners: 0 mismatches vs the scalar eq|lt
 * reference; the mask31-drop variant).
 *
 * Unlike AVX2 (which has no unsigned 32-bit compare and must flip to the
 * signed domain), AVX-512 has a NATIVE unsigned compare `vpcmpltud`
 * (_mm512_cmplt_epu32_mask).  So NO flip is needed: the table Z is loaded
 * UNFLIPPED, v is loaded UNFLIPPED, and the borrow is folded into the
 * threshold limb as a MASKED +1 via _mm512_mask_add_epi32(Zi, m, Zi, 1).
 * Under INV-NOMAX (mid/high limbs Z1,Z2 != 0xFFFFFFFF) the masked +1 never
 * wraps, so b1=[v1<_u Z1+b0], b2=[v2<_u Z2+b1] are bit-exact to the
 * textbook 96-bit unsigned compare and hence to the scalar oracle (the
 * table invariant).
 * The accumulate is _mm512_mask_add_epi32(z, m, z, 1) (+= the {0,1}
 * borrow), identical to the scalar `z += b`.
 *
 * =========================================================================
 *  Constant-time
 * =========================================================================
 * The SIMD scan is data-oblivious by construction, exactly like the
 * scalar:
 *   - The outer loop count is batch/16 (public) and the inner loop count
 * is the PUBLIC table length `entries`; no early-out on v or z.
 *   - The body is pure SIMD arithmetic (vpcmpltud / vpaddd under a mask)
 * on whole registers -- no branch, no v-indexed memory access (the only
 * loads are the contiguous grouped rand buffer at a PUBLIC sequential
 * offset and the broadcast of the PUBLIC table limb), no v-dependent shift
 * count, no gather/scatter, no division/modulo, no float.  The {0,1} mask
 * is the compare result, not a secret index. The AVX-512 path therefore
 * has the SAME (data-independent) timing profile as the scalar oracle; the
 * only data-dependent timing that escapes is the caller's BLISS/zero-fold
 * accept COUNT (a public masking-sampler property, whitelisted by the
 * constant-time policy), which lives in polyvec.c, not here.
 *
 * NB the scalar fallback path keeps the `volatile z` gather-barrier from
 * the reference (see the long comment below) so that even the `batch % 16`
 * tail (when present) is not auto-vectorized into a secret-index gather.
 */
#include "sampler.h"

#include "rcdt_tables.h" /* SHUTTLE_RCDT_Z, SHUTTLE_RCDT_NOISE_* (static const) */

#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__)
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
 * result is bit-exact (KAT-neutral).  On the AVX-512 path the explicit
 * intrinsics below already pin the instruction selection, so the volatile
 * barrier matters only for the (normally unused) scalar tail.
 */

/*
 * cdt_scan96_scalar -- scalar borrow-FOLD RCDT scan over a public table.
 * Reads the grouped 12-byte-per-sample layout (group*96 + lane*4 +
 * {0,32,64}; group = s>>3, lane = s&7) and writes `batch` int32 magnitudes
 * to `out` in sample order.  This is the BYTE-EXACT oracle the AVX-512
 * kernel reproduces. Only used on the non-AVX-512 fallback build (the
 * AVX-512 dispatch inlines its own scalar tail for the normally-absent
 * batch % 16 remainder).
 */
#if !(defined(USE_AVX512_SAMPLER) && defined(__AVX512F__))
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
#endif /* !(USE_AVX512_SAMPLER && __AVX512F__) */

#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__)
/*
 * cdt_scan96_avx512 -- the 16-sample (two grouped 8-lane half-blocks)
 * AVX-512 borrow-fold scan over the UNFLIPPED public table Z (no flip;
 * AVX-512 has a native unsigned compare).
 *
 * `rand` points at the start of two consecutive 96-byte groups (192
 * bytes): group g   limbs: rand+{0,32,64}        8 lanes -> out[0..7]
 *   group g+1 limbs: rand+{96,128,160}     8 lanes -> out[8..15]
 * The two 32-byte v-limb halves of a given limb index are gathered into
 * one 64-byte __m512i with a 256-bit insert, so SIMD lane l (0..15) is
 * exactly scalar sample (group g, lane l) for l<8 and (group g+1, lane
 * l-8) for l>=8 -- byte-identical to two scalar groups.  Bit-identical to
 * the scalar borrow chain under INV-NOMAX (see the file header and
 * demo_basesampler.c
 * ::avx512_count16).
 */
static void cdt_scan96_avx512(int32_t *out, const uint8_t *rand,
                              const uint32_t (*Z)[3], int entries)
{
    /* Load the 16 lanes of each limb: low half from group g (32 bytes),
     * high half from group g+1 (32 bytes).  The two halves are the SAME
     * grouped bytes the scalar reads, just packed into one ZMM. */
    __m512i v0 = _mm512_inserti64x4(
        _mm512_castsi256_si512(
            _mm256_loadu_si256((const __m256i *)(rand + 0))),
        _mm256_loadu_si256((const __m256i *)(rand + 96)), 1);
    __m512i v1 = _mm512_inserti64x4(
        _mm512_castsi256_si512(
            _mm256_loadu_si256((const __m256i *)(rand + 32))),
        _mm256_loadu_si256((const __m256i *)(rand + 128)), 1);
    __m512i v2 = _mm512_inserti64x4(
        _mm512_castsi256_si512(
            _mm256_loadu_si256((const __m256i *)(rand + 64))),
        _mm256_loadu_si256((const __m256i *)(rand + 160)), 1);
    __m512i z = _mm512_setzero_si512();
    const __m512i one = _mm512_set1_epi32(1);
    int i;
    for (i = 0; i < entries; i++) {
        __m512i Z0 = _mm512_set1_epi32((int)Z[i][0]);
        __m512i Z1 = _mm512_set1_epi32((int)Z[i][1]);
        __m512i Z2 = _mm512_set1_epi32((int)Z[i][2]);
        /* native unsigned compare; fold borrow as a masked +1 on Z (exact
         * while mid/high limbs <= 0xFFFFFFFE, the INV-NOMAX table
         * invariant) */
        __mmask16 m =
            _mm512_cmplt_epu32_mask(v0, Z0); /* b0 = [v0 <_u Z0]   */
        m = _mm512_cmplt_epu32_mask(
            v1,
            _mm512_mask_add_epi32(Z1, m, Z1, one)); /* b1=[v1<_u Z1+b0]  */
        m = _mm512_cmplt_epu32_mask(
            v2,
            _mm512_mask_add_epi32(Z2, m, Z2, one)); /* b2=[v2<_u Z2+b1]  */
        z = _mm512_mask_add_epi32(z, m, z, one);    /* += b2             */
    }
    /* lanes 0..7 -> out[0..7] (group g), lanes 8..15 -> out[8..15] (g+1)
     */
    _mm512_storeu_si512((__m512i *)out, z);
}

/* Max RCDT table length across the suite (RCDT_Z = 36 rows). */
#    define CDT96_MAX_ENTRIES 36
#endif /* USE_AVX512_SAMPLER && __AVX512F__ */

/*
 * cdt_scan96 -- public dispatch.  Under -DUSE_AVX512_SAMPLER the 16-sample
 * aligned bulk runs the AVX-512 kernel, and any `batch % 16` remainder
 * finishes on the scalar loop -- byte-exact to the scalar oracle for every
 * byte consumed.  Without the macro this is the verbatim scalar reference.
 */
void cdt_scan96(int32_t *out, const uint8_t *rand, const uint32_t Z[][3],
                int entries, int batch)
{
#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__)
    int bulk = batch & ~15; /* 16-sample-aligned bulk */
    int blk;
    int i;
    for (blk = 0; blk < bulk; blk += 16) {
        /* two 96-byte groups -> 16 samples; rand stride = 2*96 = 192. */
        cdt_scan96_avx512(out + blk, rand + (size_t)(blk >> 4) * 192,
                          (const uint32_t(*)[3])Z, entries);
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

#if defined(USE_AVX512_SAMPLER) && defined(__AVX512F__)
/*
 * ===================================================================== *
 *  gauss_finalize_batch -- AVX-512 vectorized SIGN-INDEPENDENT precompute*
 *  (PT_G_FINAL).  BYTE-EXACT to GAUSS_BATCH scalar gauss_finalize.     *
 * ===================================================================== *
 *
 *  RIGOROUS PRECISION + SECURITY ANALYSIS (the three points; see also    *
 *  the BaseSampler design notes on the gauss finalize).                 *
 *
 *  ----------------------------------------------------------------------
 *  (1) EXACTNESS -- every SIMD op is an EXACT integer op, no float, no
 *      rounding.  Per 8-wide block we compute:
 *
 *      cand = WIDE_K*x + y : 256x+y via vpslld $8 (256*x) + vpaddd (+y) on
 *          8x int32 lanes.  x in [0,36], y in [0,255] => cand in [0,9471],
 *          no overflow; exact integer 256x+y == scalar `WIDE_K*x+y`.
 *      negcand = -cand : vpsubd from zero, exact two's-complement negate.
 *      z0 = (cand==0) : vpcmpeqd-mask to {0,1} via a masked set, identical
 *          to scalar ct_is_zero_u32((uint32_t)cand).
 *      u = LE64(tail+8j) : 8 contiguous 8-byte little-endian draws loaded
 *          as one 512-bit vector; on little-endian x86 a 64-bit lane load
 *          of 8 LE bytes IS load_le64.  EXACT.
 *      accept = (u <_u p_hat) : the PRECISION-CRITICAL op.  AVX-512 has a
 *          NATIVE unsigned 64-bit compare _mm512_cmplt_epu64_mask, which
 *          is the EXACT 64-bit unsigned less-than (no flip, no rounding),
 *          bit-identical to the scalar ct_lt_u64(u, p_hat) for every
 *          (u, p_hat).  The {0,1} flag is materialized by a masked set.
 *
 *      Hence accept[j]/z0[j]/cand[j]/negcand[j] are BIT-IDENTICAL to the
 *      scalar gauss_finalize; the caller's keep = accept & ~(z0 & sign)
 *      and out = sign?negcand:cand are then identical, so the emitted
 *      coefficient stream and the KAT hash are UNCHANGED.
 *
 *  ----------------------------------------------------------------------
 *  (2) DISTRIBUTION / PRECISION PRESERVATION -- identical accept events
 *      => identical emitted discrete Gaussian.  p_hat comes from the
 *      UNTOUCHED ApproxExp kernel; we only consume it.  The scalar
 *      precision budget carries over UNCHANGED: ApproxExp rel-err 2^-54.49
 *      vs gates 2^-51.25/2^-51.98/2^-52.97; BaseSampler R_1045 =
 *      1+2^-95.40; the z==0 zero-fold mass-halving.  ZERO added error.
 *
 *  ----------------------------------------------------------------------
 *  (3) SECURITY / CONSTANT-TIME -- pure data-oblivious register arithmetic
 *      (vpslld/vpaddd/vpsubd/vpcmpeqd/vpcmpuq) at PUBLIC mini-batch
 *      offsets; NO branch, NO data-dependent index, NO gather/scatter, NO
 *      v-dependent shift, NO division, NO float.  Data-independent timing;
 *      the only data-dependent control flow (accept-count -> coefcnt) is
 *      the caller's scalar tail, identical to the scalar reference (the
 *      whitelisted masking-sampler rejection-timing channel).  No new
 *      timing/cache/branch leak; ct_scan stays CLEAN.
 *
 *  Writes accept[j], z0[j] as int32 {0,1} flags and cand[j], negcand[j]
 *  as the signed magnitudes.  `batch` is a whole number of 8 (GAUSS_BATCH
 *  == 32); any batch % 8 remainder finishes on a byte-exact scalar tail.
 */
void gauss_finalize_batch(int32_t *cand, int32_t *negcand, int32_t *accept,
                          int32_t *z0, const int32_t *x, const int32_t *y,
                          const uint64_t *p_hat, const uint8_t *tail,
                          int batch)
{
    const __m256i one32 = _mm256_set1_epi32(1);
    int j;
    int bulk = batch & ~7; /* 8-candidate-aligned bulk */
    for (j = 0; j < bulk; j += 8) {
        /* cand = 256*x + y (exact int32). */
        __m256i xv = _mm256_loadu_si256((const __m256i *)(x + j));
        __m256i yv = _mm256_loadu_si256((const __m256i *)(y + j));
        __m256i cv =
            _mm256_add_epi32(_mm256_slli_epi32(xv, 8), yv); /* 256x+y */
        __m256i ncv =
            _mm256_sub_epi32(_mm256_setzero_si256(), cv); /* -cand  */
        /* z0 = (cand == 0) -> {0,1} (masked select 1 where equal). */
        __mmask8 zm = _mm256_cmpeq_epi32_mask(cv, _mm256_setzero_si256());
        __m256i z0v = _mm256_maskz_mov_epi32(zm, one32);
        _mm256_storeu_si256((__m256i *)(cand + j), cv);
        _mm256_storeu_si256((__m256i *)(negcand + j), ncv);
        _mm256_storeu_si256((__m256i *)(z0 + j), z0v);
        /* u = 8x LE64 Bernoulli draws (contiguous 8 bytes/candidate). */
        __m512i u =
            _mm512_loadu_si512((const __m512i *)(tail + (size_t)j * 8));
        __m512i ph = _mm512_loadu_si512((const __m512i *)(p_hat + j));
        /* accept = (u <_u ph): NATIVE unsigned 64-bit compare. */
        __mmask8 am = _mm512_cmplt_epu64_mask(u, ph);
        /* materialize the 8x {0,1} int32 accept flags. */
        __m256i accv = _mm256_maskz_mov_epi32(am, one32);
        _mm256_storeu_si256((__m256i *)(accept + j), accv);
    }
    /* byte-exact scalar tail for any batch % 8 (absent for
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
#endif /* USE_AVX512_SAMPLER && __AVX512F__ */
