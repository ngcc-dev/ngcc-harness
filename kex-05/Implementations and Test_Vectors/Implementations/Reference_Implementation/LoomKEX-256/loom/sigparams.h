/*
 * params.h - Parameters for the SHUTTLE signature scheme.
 *
 * SHUTTLE is an MLWE Fiat-Shamir-with-aborts signature with a
 * rejection-free inner masking loop (IRS / SamplerU / ApproxLog).  Three
 * SUF parameter sets, selected via config.h by SHUTTLE_MODE:
 *
 *   SHUTTLE-128: lambda=128, n=256,  q=15361, ell=3, m=3, signed-int16 NTT
 *   SHUTTLE-256: lambda=256, n=512,  q=61441, ell=3, m=2, uint16-valley
 * NTT SHUTTLE-512: lambda=512, n=1024, q=59393, ell=3, m=2, uint16-valley
 * NTT
 *
 * EVERY value here is either (a) a PRIMARY constant copied verbatim from
 * the spec table `tab:suf-parameters` (each commented with that
 * provenance) or (b) a DERIVED constant emitted into an @@AUTOGEN@@ region
 * by a reproducible Python generator (DO NOT hand-edit a region;
 * regenerate via `make tables`, verify drift-free via `make
 * check-consts`):
 *
 *   @@AUTOGEN:params@@  <- tools/gen_params.py
 *       DQ_BITS, BQ, DB_BITS, HH, DH_BITS, ZETA, NINV, DN_BITS, BN
 *   @@AUTOGEN:bounds@@  <- tools/gen_bounds.py
 *       BK_SQ, BK_LOW_SQ, BV_SQ  (exact integer squared-norm thresholds)
 *
 * The NTT, packing, sampler, rANS, and sign modules consume these macros;
 * the ApproxExp/ApproxLog structural constants are owned by the autogen
 * headers approx_exp_poly.h / approx_log_poly.h and are NOT re-declared
 * here under shorter names.
 */

#ifndef SHUTTLE_PARAMS_H
#define SHUTTLE_PARAMS_H

#include <stdint.h>

//#include "config.h"

/* ============================================================= *
 *  Primary per-set constants (verbatim from tab:suf-parameters) *
 * ============================================================= */

#if SHUTTLE_MODE == 128
#    define CRYPTO_ALGNAME "SHUTTLE-128"
#    define SHUTTLE_NAMESPACETOP shuttle128_ref
#    define SHUTTLE_NAMESPACE(s) shuttle128_ref_##s
#elif SHUTTLE_MODE == 256
#    define CRYPTO_ALGNAME "SHUTTLE-256"
#    define SHUTTLE_NAMESPACETOP shuttle256_ref
#    define SHUTTLE_NAMESPACE(s) shuttle256_ref_##s
#elif SHUTTLE_MODE == 512
#    define CRYPTO_ALGNAME "SHUTTLE-512"
#    define SHUTTLE_NAMESPACETOP shuttle512_ref
#    define SHUTTLE_NAMESPACE(s) shuttle512_ref_##s
#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

#if SHUTTLE_MODE == 128
#    define LAMBDA 128 /* tab:suf-parameters: security level */
#    define N 256      /* tab:suf-parameters: ring dimension n           */
#    define Q 15361    /* tab:suf-parameters: prime modulus q            */
#    define ELL 3      /* tab:suf-parameters: components of s (ell)      */
#    define EM 3       /* tab:suf-parameters: components of e (m)        */
#    define TAU 42     /* tab:suf-parameters: challenge Hamming weight   */
#    define ALPHA_H \
        1024          /* tab:suf-parameters: hint high-bit compression  */
#    define ALPHA_B 2 /* tab:suf-parameters: pk rounding alpha_b  */
#    define ALPHA_1                                                     \
        90             /* tab:suf-parameters: CompressY/StretchS x0 div \
                        */
#    define ALPHA_S 10 /* tab:suf-parameters: CompressY/StretchS s div */
#    define ALPHA_E 5  /* tab:suf-parameters: CompressY/StretchS e div  */
#    define BS_ENC 9   /* tab:suf-parameters: encode bound for s         */
#    define BE_ENC 10  /* tab:suf-parameters: encode bound for e'  */
#    define DS_BITS 5  /* tab:suf-parameters: s coeff bit-width d_s  */
#    define DE_BITS 5  /* tab:suf-parameters: e' coeff bit-width d_e  */

#elif SHUTTLE_MODE == 256
#    define LAMBDA 256 /* tab:suf-parameters: security level */
#    define N 512      /* tab:suf-parameters: ring dimension n           */
#    define Q 61441    /* tab:suf-parameters: prime modulus q            */
#    define ELL 3      /* tab:suf-parameters: components of s (ell)      */
#    define EM 2       /* tab:suf-parameters: components of e (m)        */
#    define TAU 58     /* tab:suf-parameters: challenge Hamming weight   */
#    define ALPHA_H \
        1024          /* tab:suf-parameters: hint high-bit compression  */
#    define ALPHA_B 2 /* tab:suf-parameters: pk rounding alpha_b */
#    define ALPHA_1 \
        135           /* tab:suf-parameters: CompressY/StretchS x0 div  */
#    define ALPHA_S 5 /* tab:suf-parameters: CompressY/StretchS s div */
#    define ALPHA_E 5 /* tab:suf-parameters: CompressY/StretchS e div */
#    define BS_ENC 10 /* tab:suf-parameters: encode bound for s */
#    define BE_ENC 12 /* tab:suf-parameters: encode bound for e' */
#    define DS_BITS 5 /* tab:suf-parameters: s coeff bit-width d_s */
#    define DE_BITS 5 /* tab:suf-parameters: e' coeff bit-width d_e */

#elif SHUTTLE_MODE == 512
#    define LAMBDA 512 /* tab:suf-parameters: security level */
#    define N 1024     /* tab:suf-parameters: ring dimension n           */
#    define Q 59393    /* tab:suf-parameters: prime modulus q            */
#    define ELL 3      /* tab:suf-parameters: components of s (ell)      */
#    define EM 2       /* tab:suf-parameters: components of e (m)        */
/* TAU: challenge Hamming weight (tab:suf-parameters).  114 -> 115 in the
 * 2026-06-25 spec update, lifting binom(1024,TAU) >= 2^512 (SUF-512). */
#    define TAU 115
#    define ALPHA_H \
        2048          /* tab:suf-parameters: hint high-bit compression  */
#    define ALPHA_B 4 /* tab:suf-parameters: pk rounding alpha_b */
#    define ALPHA_1 \
        144           /* tab:suf-parameters: CompressY/StretchS x0 div  */
#    define ALPHA_S 3 /* tab:suf-parameters: CompressY/StretchS s div */
#    define ALPHA_E 3 /* tab:suf-parameters: CompressY/StretchS e div */
#    define BS_ENC 10 /* tab:suf-parameters: encode bound for s */
#    define BE_ENC 12 /* tab:suf-parameters: encode bound for e' */
#    define DS_BITS 5 /* tab:suf-parameters: s coeff bit-width d_s */
#    define DE_BITS 5 /* tab:suf-parameters: e' coeff bit-width d_e */

#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

/* ---- masking / Gaussian-table selectors (shared form, per-set value)
 * ---- */
/* RY (r): masking-vector Gaussian rate r=825 for all three sets
 * (tab:suf-parameters). */
#define RY 825

/* IRS alternating-series truncation N and boundary-pair count B_bdry
 * (tab:suf-parameters; shared across all sets). */
#define IRS_N 29
#define IRS_BDRY 15 /* = ceil(IRS_N/2) */

/* SamplerU exponent/mantissa widths (tab:suf-parameters; shared).
 * ceil(KAPPA_A/8)=10 exponent bytes, ceil(KAPPA_B/8)=8 mantissa bytes. */
#define KAPPA_A 80
#define KAPPA_B 57

/* ============================================================= *
 *  Shared seed / hash length macros                            *
 * ============================================================= */
#define SEEDBYTES (LAMBDA / 8) /* seedA, xi: 16 / 32 / 64 */
#define CHALLENGESEEDBYTES \
    (LAMBDA / 4) /* seedC,tr,mu,K,seedsk: 32 / 64 / 128    */
/* NOTE: CHALLENGESEEDBYTES (a SEED byte-length) is DISTINCT from
 * CHALLENGE_PACKEDBYTES (the packed binary challenge c, defined below),
 * even where the two byte counts coincide.  Do NOT conflate them. */

/* ============================================================= *
 *  Derived constants (generated by tools/gen_params.py)         *
 *  DQ_BITS, BQ, DB_BITS, HH, DH_BITS, ZETA, NINV, DN_BITS, BN   *
 * ============================================================= */
/* @@AUTOGEN:params@@ BEGIN */
/* DERIVED per-set constants (see tools/gen_params.py +
 * tools/log/params_derivation.txt). Closed-formula functions of (N, Q,
 * alpha_b, alpha_h); asserted in the generator: ZETA^n==-1, ZETA^{2n}==1,
 * n*NINV==1 mod q, H_h integer. */
#if SHUTTLE_MODE == 128
#    define DQ_BITS 14 /* ceil(log2 q): uniform-sample mask width */
#    define BQ 2       /* ceil(DQ_BITS/8): bytes per raw Z_q candidate */
#    define DB_BITS                                                    \
        13            /* ceil(log2 ceil(q/alpha_b)): pk poly bit-width \
                       */
#    define HH 30     /* 2(q-1)/alpha_h: hint high-part range (NON-pow2) */
#    define DH_BITS 5 /* ceil(log2 H_h): EncodeCom w_h bit-width */
#    define ZETA 98   /* smallest primitive 2n-th root of unity mod q */
#    define NINV 15301 /* n^{-1} mod q: inverse-NTT scale */
#    define DN_BITS 8  /* ceil(log2 n): SampleC index bit-width */
#    define BN 1       /* ceil(DN_BITS/8): SampleC index bytes */
#endif

#if SHUTTLE_MODE == 256
#    define DQ_BITS 16 /* ceil(log2 q): uniform-sample mask width */
#    define BQ 2       /* ceil(DQ_BITS/8): bytes per raw Z_q candidate */
#    define DB_BITS                                                    \
        15            /* ceil(log2 ceil(q/alpha_b)): pk poly bit-width \
                       */
#    define HH 120    /* 2(q-1)/alpha_h: hint high-part range (NON-pow2) */
#    define DH_BITS 7 /* ceil(log2 H_h): EncodeCom w_h bit-width */
#    define ZETA 21   /* smallest primitive 2n-th root of unity mod q */
#    define NINV 61321 /* n^{-1} mod q: inverse-NTT scale */
#    define DN_BITS 9  /* ceil(log2 n): SampleC index bit-width */
#    define BN 2       /* ceil(DN_BITS/8): SampleC index bytes */
#endif

#if SHUTTLE_MODE == 512
#    define DQ_BITS 16 /* ceil(log2 q): uniform-sample mask width */
#    define BQ 2       /* ceil(DQ_BITS/8): bytes per raw Z_q candidate */
#    define DB_BITS                                                    \
        14            /* ceil(log2 ceil(q/alpha_b)): pk poly bit-width \
                       */
#    define HH 58     /* 2(q-1)/alpha_h: hint high-part range (NON-pow2) */
#    define DH_BITS 6 /* ceil(log2 H_h): EncodeCom w_h bit-width */
#    define ZETA 3    /* smallest primitive 2n-th root of unity mod q */
#    define NINV 59335 /* n^{-1} mod q: inverse-NTT scale */
#    define DN_BITS 10 /* ceil(log2 n): SampleC index bit-width */
#    define BN 2       /* ceil(DN_BITS/8): SampleC index bytes */
#endif
/* @@AUTOGEN:params@@ END */

/* ============================================================= *
 *  Norm-square bounds (generated by tools/gen_bounds.py)        *
 *  BK_SQ, BK_LOW_SQ, BV_SQ  (compare-of-squares, NO sqrt)       *
 * ============================================================= */
/* @@AUTOGEN:bounds@@ BEGIN */
/* EXACT integer squared-norm thresholds (see tools/gen_bounds.py +
 * tools/log/bounds_derivation.txt).  Inclusive-gate rule:
 *   KeyGen accept iff BK_LOW_SQ <= norm_sq <= BK_SQ  (B_k' <= ||.|| <=
 * B_k) Sign/Verify accept iff norm_sq <= BV_SQ          (||.|| <= B_v,
 * SHARED) SHUTTLE-128: BK_LOW_SQ=84100 <= ||.||^2 <= BK_SQ=87060,
 * BV_SQ=54120740 SHUTTLE-256: BK_LOW_SQ=84100 <= ||.||^2 <= BK_SQ=87728,
 * BV_SQ=107859233 SHUTTLE-512: BK_LOW_SQ=84100 <= ||.||^2 <= BK_SQ=85708,
 * BV_SQ=634782986
 */
#if SHUTTLE_MODE == 128
/* B_k = 295.06  B_k' = 290  B_v = 7356.68 */
#    define BK_SQ                                                        \
        INT64_C(87060) /* floor(B_k^2):  KeyGen upper, accept iff nsq <= \
                          BK_SQ */
#    define BK_LOW_SQ                                                    \
        INT64_C(84100) /* ceil(B_k'^2):  KeyGen lower, accept iff nsq >= \
                          BK_LOW_SQ */
#    define BV_SQ                                                       \
        INT64_C(54120740) /* floor(B_v^2):  Sign+Verify, accept iff nsq \
                             <= BV_SQ */
#endif

#if SHUTTLE_MODE == 256
/* B_k = 296.19  B_k' = 290  B_v = 10385.53 */
#    define BK_SQ                                                        \
        INT64_C(87728) /* floor(B_k^2):  KeyGen upper, accept iff nsq <= \
                          BK_SQ */
#    define BK_LOW_SQ                                                    \
        INT64_C(84100) /* ceil(B_k'^2):  KeyGen lower, accept iff nsq >= \
                          BK_LOW_SQ */
#    define BV_SQ                                                        \
        INT64_C(107859233) /* floor(B_v^2):  Sign+Verify, accept iff nsq \
                              <= BV_SQ */
#endif

#if SHUTTLE_MODE == 512
/* B_k = 292.76  B_k' = 290  B_v = 25194.90 */
#    define BK_SQ                                                        \
        INT64_C(85708) /* floor(B_k^2):  KeyGen upper, accept iff nsq <= \
                          BK_SQ */
#    define BK_LOW_SQ                                                    \
        INT64_C(84100) /* ceil(B_k'^2):  KeyGen lower, accept iff nsq >= \
                          BK_LOW_SQ */
#    define BV_SQ                                                        \
        INT64_C(634782986) /* floor(B_v^2):  Sign+Verify, accept iff nsq \
                              <= BV_SQ */
#endif
/* @@AUTOGEN:bounds@@ END */

/* ============================================================= *
 *  Domain-separation tags (one byte each)                      *
 * ============================================================= */
#define DS_EXPAND_SEEDS 0x00   /* ExpandSeeds                    */
#define DS_EXPAND_SIGNING 0x01 /* ExpandSigningSeeds             */
#define DS_EXPAND_A 0x02       /* ExpandA (public; xof128)       */
#define DS_EXPAND_S 0x03       /* ExpandS                        */
#define DS_HASH_CH 0x04        /* HashCh                         */
#define DS_HASH_PK 0x05        /* HashPK                         */
#define DS_HASH_MSG 0x06       /* HashMsg                        */
#define DS_SAMPLE_C 0x07       /* SampleC                        */
#define DS_SAMPLE_Y 0x08       /* SampleY (seed_y)               */
#define DS_IRS 0x09            /* IRS / R / SamplerU (seed_y)    */

/* ============================================================= *
 *  Shared sampler / convolution constants (no #if)             *
 * ============================================================= */
#define THETA \
    96 /* BaseSampler CDT precision bits (3x32 limbs)            */

/* Wide masking sampler sigma_s = 825/256 = 3.22265625, z = 256*x + y. */
#define WIDE_SIGMA_NUM 825
#define WIDE_SIGMA_DEN 256
#define WIDE_K 256 /* k in SampleDGauss (z = WIDE_K*x + y)          */
#define WIDE_RCDT_LEN \
    36 /* wide RCDT table length = ceil(11*sigma_s)     */

/* IRS R-test constants.  TWO_RSQ = 2 r^2; TWO_RSQ_LN2 = 2 r^2 ln 2
 * (ApproxLog returns log2, a single multiply by this restores ln U).
 * TWO_RSQ_LN2 is documented here as the exact real; the Q-fixed integer
 * form is pinned in irs.c, never a runtime float. */
#define TWO_RSQ \
    (2L * RY * RY) /* = 2 * 825^2 = 1361250                    */
/* TWO_RSQ_LN2 = 943546.5995372256 (2 r^2 ln 2); stored as a fixed-point
 * integer in irs.c, reproducible from RY. */

/* ============================================================= *
 *  ApproxExp / ApproxLog structural cross-checks                *
 * ============================================================= */
/* The authoritative ApproxExp/ApproxLog structural constants live in the
 * autogen headers approx_exp_poly.h (SHUTTLE_EXP_POLY_*) and
 * approx_log_poly.h (SHUTTLE_LOG_POLY_*).  params.h does NOT independently
 * re-declare them.  The EXPECTED-value comments below are an audit anchor
 * for the spec table; the real cross-check _Static_asserts are wired once
 * those headers are included by a TU that also includes params.h:
 *   SHUTTLE_EXP_POLY_SQUARINGS == 7,  _SPLIT == 128,  _DEGREE == 8,
 *   _X_MAX == 36,  _Y_MAX == 255  (ApproxExp t7d8 Q64).
 *   SHUTTLE_LOG_POLY_G == 2,  _SEGMENTS == 4,  _DEGREE == 13,  _QBITS ==
 * 62 (ApproxLog g2d13 Q62). */

/* ============================================================= *
 *  Derived structural-size macros                              *
 * ============================================================= */
#define KVEC (1 + ELL + EM) /* z / secret vector length: 7/6/6 */
#define Z1LEN (1 + ELL)     /* z_1 length: 4/4/4               */
#define CHALLENGE_PACKEDBYTES \
    ((N + 7) / 8)  /* packed BINARY challenge c       */
#define DQ (2 * Q) /* mod-2q ring modulus             */

/* Packed-poly byte sizes.  POLYPK depends on the autogen DB_BITS. */
#define POLYPK_PACKEDBYTES \
    ((N * DB_BITS + 7) / 8) /* pk poly block            */
#define POLYS_PACKEDBYTES \
    ((N * DS_BITS + 7) / 8) /* secret s block (skEncode)*/
#define POLYE_PACKEDBYTES \
    ((N * DE_BITS + 7) / 8) /* secret e' block (skEncode)*/
#define POLYWH_PACKEDBYTES \
    ((N * DH_BITS + 7) / 8)              /* EncodeCom w_h block      */
#define POLYW0_PACKEDBYTES ((N + 7) / 8) /* EncodeCom w_0 block      */

/* ---- public-key / secret-key / signature sizes ---- */
/* pk = SEEDBYTES + EM * POLYPK_PACKEDBYTES = 1264 / 1952 / 3648 (exact).
 */
#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + EM * POLYPK_PACKEDBYTES)

/* CRYPTO_SECRETKEYBYTES.  The realized skEncode layout (packing.c pack_sk)
 * sums EXACTLY to this expression, which equals 2288/3680/7104; pack_sk
 * carries a _Static_assert that its byte cursor lands here, and t_pack
 * asserts/reports the realized length per mode.  Mirrors the skEncode
 * layout: seedA(SEEDBYTES) + EM*POLYPK_PACKEDBYTES (b/alpha_b body)
 *   + K(CHALLENGESEEDBYTES) + tr(CHALLENGESEEDBYTES)
 *   + ELL*POLYS_PACKEDBYTES + EM*POLYE_PACKEDBYTES.
 */
#define CRYPTO_SECRETKEYBYTES                              \
    (SEEDBYTES + CHALLENGESEEDBYTES + CHALLENGESEEDBYTES + \
     ELL * POLYS_PACKEDBYTES + EM * POLYE_PACKEDBYTES +    \
     EM * POLYPK_PACKEDBYTES)

/* Per-set hard size pins (drift gate).  PK_SIZE_EXPECT / SK_SIZE_EXPECT
 * are the spec byte counts; the _Static_asserts below fail the build if
 * any field-width or vector-length edit silently moves a key size. */
#if SHUTTLE_MODE == 128
#    define PK_SIZE_EXPECT 1264
#    define SK_SIZE_EXPECT 2288
#elif SHUTTLE_MODE == 256
#    define PK_SIZE_EXPECT 1952
#    define SK_SIZE_EXPECT 3680
#elif SHUTTLE_MODE == 512
#    define PK_SIZE_EXPECT 3648
#    define SK_SIZE_EXPECT 7104
#endif

/* CRYPTO_BYTES: the rANS signature length returned by
 * sig_get_sn_len_bytes().  RANS_RESERVED_BYTES (the 2^-35 per-stream
 * overflow reserve) is pinned, so the realized compact signature is the
 * FIXED length SIG_PACKED_BYTES = CHALLENGESEEDBYTES + 2 +
 * RANS_RESERVED_BYTES
 *                      + POLYZ_LO_PACKEDBYTES
 *                    = 1183 / 2417 / 5001  (computed by tools/SigSize.py).
 * These MATCH the current spec table (tab:suf-parameters now lists
 * 1183/2417/5001).  The older targets 1005/2155/4552 lay below the source-law
 * entropy of (Q0,Qs,h) plus the raw low bits (even the Shannon floor is
 * ~1115/2316/4866 B) and were dropped from the spec.  CRYPTO_BYTES is
 * the EXACT realized length: it equals SIG_PACKED_BYTES with no head-room
 * padding.  params.h precedes rans.h in the include order, so the literal
 * is mirrored here and packing.c static-asserts SIG_PACKED_BYTES ==
 * CRYPTO_BYTES as the drift gate.  Realized sig length == SIG_PACKED_BYTES
 * == CRYPTO_BYTES. */
#if SHUTTLE_MODE == 128
#    define CRYPTO_BYTES 1183 /* == SIG_PACKED_BYTES */
#elif SHUTTLE_MODE == 256
#    define CRYPTO_BYTES 2417 /* == SIG_PACKED_BYTES */
#elif SHUTTLE_MODE == 512
#    define CRYPTO_BYTES 5001 /* == SIG_PACKED_BYTES */
#endif

/* ============================================================= *
 *  Structural invariants (compile-time)                         *
 * ============================================================= */
_Static_assert(KVEC == 1 + ELL + EM, "KVEC must equal 1+ELL+EM");
_Static_assert(Z1LEN == 1 + ELL, "Z1LEN must equal 1+ELL");
_Static_assert(DQ == 2 * Q, "DQ must equal 2*Q");

/* Packed-field byte-alignment: every PolyToBytes output field in
 * this scheme is a whole number of bytes with NO padding bits (N*d % 8 ==
 * 0 for d in {d_b, d_s, d_e, d_h, 1}).  The generic zero-pad flush in
 * poly_to_bytes is implemented anyway for any future d, but these assert
 * the exactness for the audit trail. */
_Static_assert(N* DB_BITS % 8 == 0,
               "pk poly field (N*d_b) has no padding bits");
_Static_assert(N* DS_BITS % 8 == 0 && N * DE_BITS % 8 == 0,
               "noise fields (N*d_s, N*d_e) have no padding bits");
_Static_assert(N* DH_BITS % 8 == 0,
               "comY_h field (N*d_h) has no padding bits");
_Static_assert(N % 8 == 0, "comY_0 field (N*1) has no padding bits");

/* Hard size pins (drift gate): a field-width / vector-length edit that
 * moves pk or sk away from the spec byte count fails the build here.
 */
_Static_assert(CRYPTO_PUBLICKEYBYTES == PK_SIZE_EXPECT,
               "pk size must match the spec table (1264/1952/3648)");
_Static_assert(CRYPTO_SECRETKEYBYTES == SK_SIZE_EXPECT,
               "sk size must match the spec table (2288/3680/7104)");

#endif /* SHUTTLE_PARAMS_H */
