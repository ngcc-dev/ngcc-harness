/*
 * prof.h -- macro-gated profiling probes for the SHUTTLE KeyGen / Sign /
 * Verify path.
 *
 * Two independent, opt-in instrumentation systems (BOTH no-ops unless their
 * macro is defined, so a normal / KAT / CT build is byte-identical and
 * zero-cost -- this is the load-bearing invariant: the probes must never
 * perturb the production data path, and `make check-kat` must
 * reproduce the recorded hashes after the probes are inserted):
 *
 *   -DPROF_TIME : per-component cycle accounting.  Disjoint regions are
 *      timed with PROF_START / PROF_STOP (rdtscp).  Top-level buckets sum
 *      to the per-primitive total; sub-buckets (SampleY internals, NTT
 *      stages, IRS SamplerU/ApproxLog) are reported as a breakdown of their
 *      parent.  (The ~50-cycle rdtscp probe overhead is reported and
 *      matters only for the very hot inner buckets.)
 *
 *   -DPROF_RAND : randomness accounting.  PROF_CTX sets the current "who is
 *      pulling randomness" context; PROF_SQ -- dropped into the XOF squeeze
 *      functions (symmetric.c) -- tallies the squeezed bytes against that
 *      context; PROF_USE tallies fine per-field consumption inside the
 *      samplers.  The per-context bytes/sig table directly exposes whether
 *      any XOF buffer is over-squeezed (the design-doc PRNG-waste concern:
 *      NGCC SM3 squeezes 32 B/SM3-block, SHA3 168 B/SHAKE128-block).
 *
 * `make profile` (-DPROF_TIME -DPROF_RAND) builds + runs test/speed_profile.c.
 *
 * The PT_ / PC_ / PU_ enum bucket sets below diverge from a
 * Dilithium-style bucket layout where SHUTTLE's algorithm differs (the
 * principal divergence point):
 *   (1) PT_EXPAND_A (SHUTTLE's public matrix is A, expanded NTT-domain via
 *       tag 0x02 xof128) -- not PT_EXPAND_A0;
 *   (2) the sign rejection bucket is PT_IRS (parent) + PT_SAMPLERU +
 *       PT_APPROXLOG (children): SHUTTLE's inner loop is the Iterative
 *       Rejection Sampler driven by SamplerU/ApproxLog (the base-2 log +
 *       2 r^2 ln2 multiply), NOT a Gaussian rejection;
 *   (3) PT_RANS is broken out from PT_PACK so the profiler can attribute
 *       the rANS encode (the variable-cost block, the only source of
 *       sign's tail);
 *   (4) PT_SAMPLE_Y children are PT_G_SHAKE / PT_G_BASESAMP (96-bit CDT,
 *       the table invariant) / PT_G_APPROXEXP (t7d8 Q64 Bernoulli) /
 *       PT_G_FINAL (fold/sign, the output-indexed sign convention);
 *   (5) Verify's PT_VF_MATMUL reconstructs over only 1+ELL columns (the
 *       A1-hat asymmetry: Verify omits the 2 I_m block), so it is
 *       structurally cheaper than Sign's full-matrix PT_COMMIT.
 */
#ifndef SHUTTLE_PROF_H
#define SHUTTLE_PROF_H

#include <stddef.h>
#include <stdint.h>

/* ---- PROF_TIME buckets (order = report order; parents precede children) ---- */
enum {
    /* ===== SIGN ===== */
    PT_SETUP = 0, /* skDecode / tr / mu derivation (XOF)                 */
    PT_EXPAND_A,  /* ExpandA: uniform A-hat (NTT domain), tag 0x02       */
    PT_SAMPLE_Y,  /* parent: SampleY (wide Gaussian y-vector)            */
    PT_G_SHAKE,   /*   child: XOF squeeze (SM3/Keccak) for SampleY       */
    PT_G_BASESAMP,/*   child: BaseSampler 96-bit CDT (sigma_s)           */
    PT_G_APPROXEXP,/*  child: ApproxExp Bernoulli accept (t7d8 Q64)      */
    PT_G_FINAL,   /*   child: fold/sign/finalize + y-unpack              */
    PT_COMMIT,    /* parent: commitment w = A y mod 2q (NTT mat-mul)     */
    PT_NTT_FWD,   /*   child: forward NTT                                */
    PT_NTT_PW,    /*   child: pointwise Montgomery / basemul             */
    PT_NTT_INV,   /*   child: inverse NTT                                */
    PT_HIGHBITS,  /* CompressY / HighBits / RoundB bucketing             */
    PT_CHALLENGE, /* HashCh + SampleC (binary Fisher-Yates)              */
    PT_IRS,       /* parent: Iterative Rejection Sampler (R loop)        */
    PT_SAMPLERU,  /*   child: SamplerU (CLZ + MSB-first mantissa)        */
    PT_APPROXLOG, /*   child: ApproxLog base-2 (g2 d13 Q62) CT scan      */
    PT_NORMCHECK, /* ||(z1,z2')||_2 <= B_v norm gate                     */
    PT_MAKEHINT,  /* MakeHint                                            */
    PT_RANS,      /* rANS encode (the variable-cost block)               */
    PT_PACK,      /* sigEncode / byte layout (excl. rANS arithmetic)     */
    /* ===== KEYGEN ===== */
    PT_KG_EXPAND_A,  /* ExpandA in keygen                                */
    PT_KG_NOISE,     /* ExpandS: sample s, e (BaseSampler small sigma)   */
    PT_KG_BPRODUCT,  /* b_0 = a_gen + iNTT(A-hat o NTT(s)) + e           */
    PT_KG_ROUNDB,    /* b = RoundB(b_0); e' = e + (b - b_0)              */
    PT_KG_STRETCH,   /* StretchS(1, s, e')                               */
    PT_KG_NORM,      /* norm-window gate [B_k', B_k]                     */
    PT_KG_PACK,      /* pkEncode / skEncode / tr                         */
    /* ===== VERIFY ===== */
    PT_VF_UNPACK,    /* unpack pk + sigDecode (rANS decode + checks)     */
    PT_VF_SETUP,     /* tr / mu derivation                              */
    PT_VF_A,         /* ExpandA + b-hat NTT import                      */
    PT_VF_MATMUL,    /* commitment reconstruction (1+ELL cols, NO 2I_m) */
    PT_VF_HINT,      /* UseHint (live mod-q) / lsb                      */
    PT_VF_CHALLENGE, /* HashCh + SampleC                                */
    PT_VF_NORM,      /* z reconstruction + ||.|| <= B_v check           */
    PT_NBUCKETS
};

/* ---- PROF_RAND contexts (who is pulling randomness) ---- */
enum {
    PC_SETUP = 0, /* tr/mu/rhoprime/seed expansion         */
    PC_A,         /* ExpandA (xof128, tag 0x02)            */
    PC_GAUSS,     /* SampleY wide-Gaussian (xof256, 0x08)  */
    PC_CHALLENGE, /* HashCh + SampleC (0x04 / 0x07)        */
    PC_IRS,       /* SamplerU/R fresh ctx (0x09 || seed_y) */
    PC_NOISE,     /* ExpandS keygen noise (0x03)           */
    PC_OTHER,
    PC_NCTX
};

/* ---- PROF_USE fine consumption (within the samplers; catches PRNG waste) ---- */
enum {
    PU_SIGNS = 0, /* OUTPUT-indexed sign bits                */
    PU_SIGMA_S,   /* BaseSampler RCDT draws (sigma_s)        */
    PU_Y,         /* uniform y bits in the BLISS convolution */
    PU_GTAIL,     /* SampleY rejection-tail / fold bytes     */
    PU_NOISE,     /* ExpandS keygen noise bytes              */
    PU_SAMPLERU,  /* SamplerU exponent(10B)+mantissa(8B)     */
    PU_NCONS
};

#if defined(PROF_TIME) || defined(PROF_RAND)
extern uint64_t prof_cyc[PT_NBUCKETS];
extern uint64_t prof_cnt[PT_NBUCKETS];
extern uint64_t prof_sq[PC_NCTX];
extern uint64_t prof_use[PU_NCONS];
extern int prof_ctx; /* current squeeze attribution context */
void prof_reset(void);
void prof_report(const char *title, uint64_t nsig);
static inline uint64_t prof_rdtsc(void)
{
    unsigned lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    return ((uint64_t)hi << 32) | lo;
}
#endif

#ifdef PROF_TIME
#    define PROF_START(v) uint64_t v = prof_rdtsc()
#    define PROF_STOP(b, v)                      \
        do {                                     \
            prof_cyc[(b)] += prof_rdtsc() - (v); \
            prof_cnt[(b)]++;                     \
        } while (0)
#else
#    define PROF_START(v)
#    define PROF_STOP(b, v)
#endif

#ifdef PROF_RAND
#    define PROF_CTX(c) (prof_ctx = (c))
#    define PROF_CTX_GET() (prof_ctx)
#    define PROF_CTX_SET(c) (prof_ctx = (c))
#    define PROF_SQ(nbytes) (prof_sq[prof_ctx] += (uint64_t)(nbytes))
#    define PROF_USE(t, n) (prof_use[(t)] += (uint64_t)(n))
#else
#    define PROF_CTX(c) ((void)0)
#    define PROF_CTX_GET() (0)
#    define PROF_CTX_SET(c) ((void)0)
#    define PROF_SQ(nbytes) ((void)0)
#    define PROF_USE(t, n) ((void)0)
#endif

#endif /* SHUTTLE_PROF_H */
