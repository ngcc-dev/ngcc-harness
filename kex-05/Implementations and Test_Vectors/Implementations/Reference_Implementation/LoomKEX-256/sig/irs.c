/*
 * irs.c -- SHUTTLE RejectSample / R reference (scalar, integer-only, CT).
 *
 * The rejection-free inner masking transition (Algorithms alg:RejectSample
 * and alg:Ryv).  See irs.h for the full contract: the fresh 0x09||seed_y
 * context, the ascending-j traversal, the base-2->ln multiply by
 * 2 r^2 ln2, the N=29 / 15-boundary-pair interval test, and the isochrony
 * / secret-vs-public leakage argument.
 *
 * Constant-time discipline (irs.h "ISOCHRONY / LEAKAGE"):
 *   - the ONLY branch on the IRS path is the ascending-j  if (c[j])  gate,
 *     whitelisted because c is PUBLIC (recomputed by the verifier);
 *   - the sign-normalize, the 15-pair interval test, and the z += flag*v
 *     update are branchless (two's-complement masks);
 *   - no division/modulo; the u-vs-boundary comparison is exact in
 * __int128.
 *
 * The inner products run on the RAW signed scheme-domain coefficients (NOT
 * mod-q-centered): sk_tilde and y are small signed integers, so we do NOT
 * use poly_sqnorm (which centers mod q and would corrupt them) -- we
 * accumulate the products directly in int64.
 */
#include "irs.h"

#include <stdint.h>
#include <string.h>

#include "params.h"
#include "poly.h"
#include "sampler_u.h"
#include "symmetric.h"

/* ===================================================================== *
 *  IRS SINGLE-BUFFER PRNG SCHEDULE (the bulk-draw optimization)         *
 * ===================================================================== *
 *
 * RejectSample is rejection-FREE: it runs EXACTLY tau transitions (irs.h),
 * each consuming a FIXED 18 bytes (10 exponent + 8 mantissa).  So the
 * IRS needs EXACTLY tau*18 bytes off the 0x09||seed_y stream
 * (756/1044/2052 for SHUTTLE-128/256/512), DETERMINISTIC and PUBLIC in
 * length.
 *
 * Schedule: draw the WHOLE tau*18-byte buffer in ONE xof256_squeeze, then
 * decode each transition's 18-byte slice (ascending j) via
 * sampler_u_decode. This replaces the old per-transition two-squeeze (10
 * then 8) structure.
 *
 *  - Under SHA3_MODE the Keccak squeeze is a continuous rate-buffered
 *    stream, so the bulk tau*18 squeeze is BYTE-IDENTICAL to tau pairs of
 *    (squeeze 10, squeeze 8): the SHA3 IRS KAT is UNCHANGED by this
 * change.
 *  - Under NGCC_MODE each xof256_squeeze is one SM3 Hash-DRBG generate (a
 *    full SM3 block round + a 55-byte state update); the old path did
 * tau*2 generates (32 B produced / 18 used per transition, 44% waste), the
 * new path does ONE generate of ceil(tau*18/32) blocks (no per-transition
 *    state update, ~no waste).  The byte stream therefore CHANGES -- the
 *    NGCC IRS KAT is RE-RECORDED (authorized).
 *
 * The avx2/avx512 backends fill the SAME tau*18 buffer N-way and
 * BYTE-EXACT to this scalar bulk squeeze (see irs_bulk_fill below).
 */
#define IRS_BLOCK_BYTES 18 /* 10 exponent + 8 mantissa */
#define IRS_BULK_BYTES ((size_t)TAU * IRS_BLOCK_BYTES)
/* SM3-DRBG blocks for the bulk fill: ceil(tau*18 / 32) (24/33/65 for the
 * three sets).  Bounds the per-block message/digest scratch arrays. */
#define IRS_BULK_MAX_BLOCKS (((size_t)TAU * IRS_BLOCK_BYTES + 31) / 32)
_Static_assert(IRS_BLOCK_BYTES == 18, "SamplerU block is 18 bytes");

/* ---- N-way bulk DRBG fill (NGCC only; behind USE_AVX2/AVX512_XOF_NWAY)
 * --
 *
 * A single scalar NGCC xof256_squeeze(ctx, buf, L) == one
 * SM3_DRNG_Generate: it emits m = ceil(L/32) blocks  SM3(V+0), SM3(V+1),
 * ..., SM3(V+m-1)  (each the SM3 hash of the 55-byte working state V
 * incremented i times), copies the first L bytes out (the tail block is
 * truncated; L is a whole number of bytes here so no partial-byte mask
 * fires), THEN advances the state once by  V <- V + SM3(0x03||V) + C +
 * reseed_counter ; reseed_counter++.
 *
 * The m blocks are INDEPENDENT SM3 compressions of distinct 55-byte
 * messages (V+i), so they map directly onto the 8-way sm3hash_avx2 /
 * 16-way sm3hash_avx512 cores (same length per lane).  We compute them
 * N-way, splice them into buf, and do the SINGLE scalar state update
 * verbatim -- producing bytes BIT-IDENTICAL to the scalar
 * get_random_number.  Validated by the ref==avx2==avx512 KAT gate. */
#if !defined(SHA3_MODE) &&                                \
    ((defined(USE_AVX2_XOF_NWAY) && defined(__AVX2__)) || \
     (defined(USE_AVX512_XOF_NWAY) && defined(__AVX512F__)))
#    define IRS_NWAY_BULK 1
#    include "drng.h" /* SEEDLEN, DRNG_ctx layout (== xof_ctx under NGCC) */
#    if defined(USE_AVX512_XOF_NWAY) && defined(__AVX512F__)
#        include "auxfunc_avx512.h" /* sm3hash_avx512 (16-way) */
#        define IRS_SM3_NWAY SM3_WAY_AVX512
#        define irs_sm3hash_nway sm3hash_avx512
#    else
#        include "auxfunc_avx2.h" /* sm3hash_avx2 (8-way) */
#        define IRS_SM3_NWAY SM3_WAY_AVX2
#        define irs_sm3hash_nway sm3hash_avx2
#    endif

#    define IRS_SM3_OUTLEN 32

/* big-endian byte-array increment (BN[len-1] is the LSB), matching drng.c.
 */
static void irs_inc_bn(unsigned char *bn, size_t len)
{
    for (; len != 0; len--) {
        bn[len - 1] += 1;
        if (bn[len - 1])
            break;
    }
}

/* four-way big-number add  bn1 <- bn1 + bn2 + bn3 + bn4  (mod 2^(8*len)),
 * matching drng.c plus_Big_Number (the single per-generate state update).
 */
static void irs_plus_bn(unsigned char *bn1, const unsigned char *bn2,
                        const unsigned char *bn3, const unsigned char *bn4,
                        size_t len)
{
    unsigned int sum;
    unsigned char carry = 0;
    for (; len != 0; len--) {
        sum = (unsigned int)bn1[len - 1] + bn2[len - 1] + bn3[len - 1] +
              bn4[len - 1] + carry;
        carry = (unsigned char)(sum / (0xFFU + 1));
        bn1[len - 1] = (unsigned char)(sum & 0xFFU);
    }
}

/* N-way bulk generate of L (<= IRS_BULK_BYTES) bytes off the NGCC DRBG
 * ctx, byte-exact to the scalar get_random_number(ctx, buf, L*8). */
static void irs_bulk_fill_nway(xof_ctx *ctx, uint8_t *buf, size_t L)
{
    DRNG_ctx *drng = (DRNG_ctx *)ctx;
    const size_t m = (L + IRS_SM3_OUTLEN - 1) / IRS_SM3_OUTLEN;
    /* per-block incremented-V messages (V, V+1, ..., V+m-1) and digests.
     */
    unsigned char data[IRS_BULK_MAX_BLOCKS][SEEDLEN];
    unsigned char dgst[IRS_BULK_MAX_BLOCKS][IRS_SM3_OUTLEN];
    const unsigned char *mp[IRS_SM3_NWAY];
    unsigned char *dp[IRS_SM3_NWAY];
    unsigned char padded_V[1 + SEEDLEN];
    unsigned char H[SEEDLEN];
    size_t i, b, off;

    /* build the m messages V+i (cheap scalar; SEEDLEN=55 bytes each). */
    memcpy(data[0], drng->V, SEEDLEN);
    for (i = 1; i < m; i++) {
        memcpy(data[i], data[i - 1], SEEDLEN);
        irs_inc_bn(data[i], SEEDLEN);
    }

    /* hash all m blocks N-way (pad the trailing lanes with block 0). */
    for (b = 0; b < m; b += IRS_SM3_NWAY) {
        unsigned k;
        for (k = 0; k < IRS_SM3_NWAY; k++) {
            size_t idx = b + k;
            mp[k] = (idx < m) ? data[idx] : data[0]; /* spare lane = V */
            dp[k] =
                (idx < m) ? dgst[idx] : dgst[0]; /* spare lane = blk0 */
        }
        irs_sm3hash_nway(mp, (unsigned long long)SEEDLEN * 8, dp);
    }

    /* splice the digests into buf (tail block truncated; L whole bytes).
     */
    for (i = 0, off = 0; i < m; i++) {
        size_t take =
            (L - off >= IRS_SM3_OUTLEN) ? IRS_SM3_OUTLEN : L - off;
        memcpy(buf + off, dgst[i], take);
        off += take;
    }

    /* the SINGLE state update: V <- V + SM3(0x03||V) + C + ctr; ctr++.
     * SM3(0x03||V) is one scalar SM3 (cheap); reuse dgst[0] as scratch. */
    padded_V[0] = 0x03;
    memcpy(padded_V + 1, drng->V, SEEDLEN);
    {
        const unsigned char *m1[IRS_SM3_NWAY];
        unsigned char *d1[IRS_SM3_NWAY];
        unsigned k;
        for (k = 0; k < IRS_SM3_NWAY; k++) {
            m1[k] = padded_V;
            d1[k] =
                dgst[k % m]; /* any valid scratch; lane 0 is the result */
        }
        irs_sm3hash_nway(m1, (unsigned long long)(1 + SEEDLEN) * 8, d1);
    }
    memset(H, 0, sizeof(H));
    memcpy(H + (SEEDLEN - IRS_SM3_OUTLEN), dgst[0], IRS_SM3_OUTLEN);
    irs_plus_bn(drng->V, H, drng->C, drng->reseed_counter, SEEDLEN);
    irs_inc_bn(drng->reseed_counter, SEEDLEN);
}
#endif /* IRS_NWAY_BULK */

/* Fill the tau*18 IRS PRNG buffer.  Scalar/SHA3: one xof256_squeeze.  NGCC
 * avx2/avx512: the N-way bulk DRBG generate (byte-exact to the scalar). */
static void irs_bulk_fill(xof_ctx *ctx, uint8_t *buf, size_t L)
{
#if defined(IRS_NWAY_BULK)
    irs_bulk_fill_nway(ctx, buf, L);
#else
    xof256_squeeze(ctx, buf, L);
#endif
}

/* The u-vs-boundary comparison and the 2 r^2 ln2 multiply use GNU __int128
 * (a GCC/Clang extension ISO C does not define; -Wpedantic flags it).
 * Localize the suppression to this TU's __int128 use.
 */
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif

/* ===================================================================== *
 *  IRS R-transition fixed-point constant                                *
 *  Owned by tools/gen_irs_consts.py; re-derived by `make check-consts`.  *
 * ===================================================================== */
/* @@AUTOGEN:irs_consts@@ BEGIN */
/* IRS R-transition fixed-point constant (gen_irs_consts.py).
 *
 *   2 r^2 ln 2 = 2*825^2*ln2 = 943546.5995372255524442...
 *   R2LN2_QSHIFT = 44   (the Q-scale F; u is carried at Q44)
 *   R2LN2_QF     = round(2 r^2 ln2 * 2^44) = 16599047320634951608
 *                = 0xE65BA997B45887B8  (fits uint64_t:
 * 16599047320634951608 < 2^64)
 *
 * relative error of the rounded constant = 2^-65.1577; amplified additive
 * natural-log error |ln U|*relerr <= 50.53*2^-65.1577 ~ 2^-59.4986
 * (binding |ln U|); total delta_log ~ 2^-58.6572, accumulated delta_tau ~
 * 2^-46.8043
 * (< 2^-45 budget).  See gen_irs_consts.py +
 * log/irs_consts_derivation.txt.
 */
#define R2LN2_QSHIFT 44
#define R2LN2_QF UINT64_C(16599047320634951608)
/* @@AUTOGEN:irs_consts@@ END */

_Static_assert(TWO_RSQ == 1361250L, "2 r^2 = 2*825^2 = 1361250");

/* ===================================================================== *
 *  Exact integer inner products (raw signed coeffs, int64 accum)        *
 * ===================================================================== */

/*
 * V = <sk_tilde, sk_tilde> = sum over KVEC polys, n coeffs each.  sk_tilde
 * = StretchS(sk) is norm-bounded (||sk_tilde|| <= B_k ~ 296, so V ~ 88000
 * < 2^17); the int64 accumulation cannot overflow.  Computed ONCE per
 * RejectSample and reused for every shift (isometry).
 */
static int64_t sk_tilde_norm2(const poly sk_tilde[KVEC])
{
    int64_t acc = 0;
    unsigned i, k;
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k) {
            int64_t c = (int64_t)sk_tilde[i].coeffs[k];
            acc += c * c;
        }
    return acc;
}

/*
 * t = <y, v> = sum over KVEC polys, n coeffs each.  |y| <= ~5000 (wide
 * Gaussian ~11 sigma of r=825), |v| <= ||sk_tilde||_inf <= ~26 per coeff,
 * so |t| <= KVEC*N*5000*26 ~ 2^28; int64 is safe (the V-tail term -m^2 V
 * with m<=28 dominates the boundary magnitude, still well within int64).
 */
static int64_t inner_y_v(const poly y[KVEC], const poly v[KVEC])
{
    int64_t acc = 0;
    unsigned i, k;
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k)
            acc += (int64_t)y[i].coeffs[k] * (int64_t)v[i].coeffs[k];
    return acc;
}

/*
 * v <- sk_tilde . X^j  in R = Z[X]/(X^n+1): a negacyclic right-rotation by
 * j, negating the part that wraps past degree n.  Per-component over all
 * KVEC polys: v[i].coeffs[k] = (k >= j) ?  sk_tilde[i].coeffs[k-j] :
 * -sk_tilde[i].coeffs[n + k - j]. (j is PUBLIC -- it is a challenge index
 * -- so the index arithmetic is fine; the from-scratch rotation is the KAT
 * reference, S10.)
 */
static void poly_shift_negacyclic(poly v[KVEC], const poly src[KVEC],
                                  unsigned int j)
{
    unsigned i, k;
    for (i = 0; i < KVEC; ++i) {
        for (k = 0; k < j; ++k)
            v[i].coeffs[k] = -src[i].coeffs[N + k - j];
        for (k = j; k < N; ++k)
            v[i].coeffs[k] = src[i].coeffs[k - j];
    }
}

/* z[i] += flag * v[i]  for every coeff, flag in {-1,+1}, branchless. */
static void poly_axpy_flag(poly z[KVEC], const poly v[KVEC], int64_t flag)
{
    unsigned i, k;
    int32_t f = (int32_t)flag; /* -1 or +1 */
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k)
            z[i].coeffs[k] += f * v[i].coeffs[k];
}

/* ===================================================================== *
 *  The u fixed-point form                                                *
 * ===================================================================== */

/*
 * u_q44 = (2 r^2 ln2) * log2(U) at the pinned scale Q44, in __int128.
 * log2(U) = frac_q62/2^62 - a (the unfolded SamplerU pair).  Computed as:
 *   u_frac = round(R2LN2_QF * frac_q62 / 2^62) = (R2LN2_QF*frac + 2^61) >>
 * 62 u_a    = a * R2LN2_QF u      = u_frac - u_a frac_q62 is in [0, 2^62)
 * (nonnegative log2(b)), so R2LN2_QF*frac fits an unsigned __int128 (<
 * 2^126).  The result is a signed Q44 value.
 */
static __int128 sampler_u_to_u_q44(sampler_u_res ell)
{
    unsigned __int128 prod = (unsigned __int128)R2LN2_QF *
                             (unsigned __int128)(uint64_t)ell.frac_q62;
    /* round-to-nearest >> 62 (unbiased), then to signed Q44. */
    unsigned __int128 u_frac = (prod + ((unsigned __int128)1 << 61)) >> 62;
    __int128 u_a = (__int128)ell.a * (__int128)R2LN2_QF;
    return (__int128)u_frac - u_a;
}

/* ===================================================================== *
 *  One R transition (Algorithm alg:Ryv)                                 *
 * ===================================================================== */

/*
 * R_transition: draw ell from SamplerU(ctx) (already drawn and passed in
 * by the caller so the x2 batch can be wired -- see reject_sample), form
 * u, sign-normalize v so t = <y,v> > 0, run the 15 boundary-pair interval
 * tests, and apply z <- z + flag*v.  `v` is the shift sk_tilde.X^j; it is
 * mutated (sign-normalized) in place.  z is mutated.
 *
 * Branchless throughout: the t<=0 flip and the flag selection use
 * two's-complement masks; the loop runs all 15 pairs with no early exit.
 */
static void R_transition(poly z[KVEC], poly v[KVEC], int64_t V,
                         sampler_u_res ell)
{
    __int128 u = sampler_u_to_u_q44(ell);
    int64_t t = inner_y_v(z, v);
    int64_t flagmask; /* sign-normalize: flip iff t <= 0 (spec uses <=) */
    int64_t flag64;
    unsigned i;

    /*
     * Sign-normalize so t > 0.  The spec condition is t <= 0 (inclusive at
     * 0).  flipmask = ((t - 1) >> 63) is all-ones iff (t-1) < 0 iff t <= 0
     * (for in-range t); it MUST flip at t == 0 to match the spec's <=.
     * Negate per-coeff via two's complement (x ^ mask) - mask, and t
     * likewise.
     */
    flagmask = (t - 1) >> 63; /* -1 iff t <= 0, else 0 */
    {
        int32_t m32 = (int32_t)flagmask; /* 0 or -1 */
        unsigned k, ii;
        for (ii = 0; ii < KVEC; ++ii)
            for (k = 0; k < N; ++k) {
                int32_t c = v[ii].coeffs[k];
                v[ii].coeffs[k] = (c ^ m32) - m32; /* c, or -c iff mask */
            }
    }
    t = (t ^ flagmask) - flagmask; /* |t| (now t > 0) */

    /*
     * 15 boundary pairs cover the N=29 truncated terms m=0..28.  Pair i
     * tests lo = -2(2i+1)t - (2i+1)^2 V   (the m=2i+1 side) hi = -4 i t -
     * 4 i^2 V        (the m=2i   side) and sets flag=1 iff (lo<<F) < u <=
     * (hi<<F).  At most one i matches, but we run ALL 15 and OR the
     * condition (CT).  flag starts at -1.
     */
    flag64 = -1;
    for (i = 0; i < IRS_BDRY; ++i) {
        int64_t two_i1 = (int64_t)(2u * i + 1u); /* 2i+1 */
        int64_t four_i = (int64_t)(4u * i);      /* 4i   */
        int64_t lo = -2 * two_i1 * t - two_i1 * two_i1 * V;
        int64_t hi = -four_i * t - (int64_t)(4u * i * i) * V;
        __int128 lo_s = (__int128)lo << R2LN2_QSHIFT;
        __int128 hi_s = (__int128)hi << R2LN2_QSHIFT;
        /* cond = (lo_s < u) && (u <= hi_s), as a 0/-1 mask. */
        int64_t c_lo = (int64_t)((lo_s < u) ? 1 : 0);
        int64_t c_hi = (int64_t)((u <= hi_s) ? 1 : 0);
        int64_t cond =
            -(c_lo & c_hi); /* -1 iff in the half-open interval */
        /* flag = cond ? 1 : flag  (branchless select). */
        flag64 = (flag64 & ~cond) | ((int64_t)1 & cond);
    }

    poly_axpy_flag(z, v, flag64);
}

/* ===================================================================== *
 *  RejectSample (Algorithm alg:RejectSample)                            *
 * ===================================================================== */

void reject_sample(xof_ctx *ctx, poly z[KVEC], const poly y[KVEC],
                   const poly *c, const poly sk_tilde[KVEC])
{
    poly v[KVEC];
    int64_t V;
    unsigned j;
    /* ONE bulk PRNG buffer for the whole IRS (tau*18 bytes, deterministic
     * and public in length).  Drawn once, sliced 18 bytes per transition.
     */
    uint8_t buf[IRS_BULK_BYTES];
    size_t cur =
        0; /* byte cursor into buf; advances by 18 per transition */

    /* z <- y  (copy; y may alias z safely after this). */
    memcpy(z, y, KVEC * sizeof(poly));

    /* V = <sk_tilde, sk_tilde>, computed ONCE (isometry). */
    V = sk_tilde_norm2(sk_tilde);

    /* Draw the ENTIRE IRS randomness in one shot (single 0x09 ctx). The
     * buffer length tau*18 is PUBLIC (tau is the challenge weight), so the
     * single squeeze is over a data-independent length -- no
     * secret-dependent squeeze count.  Backends fill this N-way and
     * byte-exact (irs_bulk_fill).
     */
    irs_bulk_fill(ctx, buf, IRS_BULK_BYTES);

    /*
     * Strict ascending traversal j = 0..n-1, transition iff c[j] == 1.
     * c is PUBLIC, so this branch is a whitelisted public-data
     * branch (irs.h leakage note).  Each matching j consumes the NEXT
     * 18-byte slice of buf (10 exponent + 8 mantissa) in ascending-j
     * order, decodes it to ell via sampler_u_decode (no XOF, pure
     * CLZ/mantissa/ApproxLog), and applies one R transition.  Exactly tau
     * slices are consumed (rejection-free), so the cursor ends at tau*18
     * == IRS_BULK_BYTES.
     */
    for (j = 0; j < N; ++j) {
        if (c->coeffs[j] == 1) {
            sampler_u_res ell =
                sampler_u_decode(buf + cur, buf + cur + 10);
            cur += IRS_BLOCK_BYTES;
            poly_shift_negacyclic(v, sk_tilde, j); /* v = sk_tilde . X^j */
            R_transition(z, v, V, ell);
        }
    }
}

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif
