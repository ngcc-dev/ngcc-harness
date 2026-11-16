/* sign-01-10: Aigis-Sig+ ignores the status of the contest DRBG when it
 * expands the challenge seed, so a failed allocation there replaces the
 * challenge polynomial c by a fixed public constant c0.
 *
 * unpack_c() calls kdf_init() -> init_random_number(), a void wrapper whose
 * status is dropped (hashkdf.c kdf_init, polyvec.c unpack_c). The ICCS
 * DRBG instantiation zeroes its context before its first malloc and returns
 * on failure (drng.c SM3_DRNG_Instantiate), so the following squeeze runs
 * from an all-zero state and yields the same c0 for every seed.
 *
 * Linked with -Wl,--wrap=malloc -Wl,--wrap=init_random_number. When armed,
 * the init_random_number wrapper makes exactly the first allocation of the
 * selected DRBG instantiation fail.
 *
 *  1. Signing: signing is deterministic (y depends on key, mu and the nonce).
 *     A message is signed honestly, then signed again with the allocation
 *     failing in the unpack_c call of the iteration that the honest run
 *     accepted. The second call returns 0 with the same cseed and
 *     z' = y + c0*s1, so z - z' = (c - c0)*s1 and s1 is solved mod q from
 *     public data only and compared with the packed s1 in the secret key.
 *  2. Verification: with only the public key, a signature for a fresh
 *     message is built for challenge c0 (z = 0, h = 0). It is rejected
 *     normally and accepted when the verifier's unpack_c allocation fails.
 *  3. Key generation (same mechanism in the eta sampler): two key pairs from
 *     different DRBG seeds, each with the first allocation of the s1[0]
 *     sampler's DRBG failing, return 0, sign and verify, and carry the same
 *     s1[0] (the zero-state DRBG output).
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "api.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "pspm.h"
#include "hashkdf.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
#ifdef AIGIS_AVX2   /* AVX2 tree: sample.h, int32_t lengths, renamed helpers */
#include "sample.h"
#include "sign.h"
#define polyvecl_pointwise_acc_invmontgomery polyvecl_pointwise_acc_montgomery
#define emulate_ct1 emulate_ct
typedef int32_t smlen_t;
void randombytes(unsigned char *x, size_t xlen) { get_random_number(&drng_algorithm, x, xlen * 8); }
#else
int msig_keygen(unsigned char *pk, unsigned char *sk);
int msig_sign(unsigned char *sk, unsigned char *m, unsigned long long mlen,
              unsigned char *sm, unsigned long long *smlen);
int msig_verf(unsigned char *pk, unsigned char *sm, unsigned long long smlen,
              unsigned char *m, unsigned long long mlen);
typedef unsigned long long smlen_t;
#endif

void *__real_malloc(size_t);
int __real_init_random_number(DRNG_ctx *, const unsigned char *, unsigned long long);
static int fail_next, n_irn, fail_irn = -1, n_failed;

void *__wrap_malloc(size_t n)
{
    if (fail_next) { fail_next = 0; n_failed++; return NULL; }
    return __real_malloc(n);
}

int __wrap_init_random_number(DRNG_ctx *d, const unsigned char *s, unsigned long long l)
{
    if (n_irn++ == fail_irn) fail_next = 1;
    return __real_init_random_number(d, s, l);
}

#define Q PARAM_Q
#define N PARAM_N
static int64_t md(int64_t a) { a %= Q; return a < 0 ? a + Q : a; }
static int64_t pw(int64_t b, int64_t e) { int64_t r = 1; b = md(b); while (e) { if (e & 1) r = r * b % Q; b = b * b % Q; e >>= 1; } return r; }

/* Solve d * s_k = r_k in Z_q[x]/(x^N+1) for k < PARAM_L. Returns 0 if singular. */
static int ring_solve(const int64_t d[N], int64_t r[PARAM_L][N], int64_t s[PARAM_L][N])
{
    static int64_t M[N][N + PARAM_L];
    for (int j = 0; j < N; j++)          /* column j = d * x^j */
        for (int i = 0; i < N; i++) {
            int k = i - j;
            M[i][j] = k >= 0 ? md(d[k]) : md(-d[k + N]);
        }
    for (int i = 0; i < N; i++)
        for (int k = 0; k < PARAM_L; k++) M[i][N + k] = md(r[k][i]);
    for (int c = 0; c < N; c++) {
        int p = c;
        while (p < N && M[p][c] == 0) p++;
        if (p == N) return 0;
        if (p != c) for (int j = 0; j < N + PARAM_L; j++) { int64_t t = M[p][j]; M[p][j] = M[c][j]; M[c][j] = t; }
        int64_t inv = pw(M[c][c], Q - 2);
        for (int j = c; j < N + PARAM_L; j++) M[c][j] = M[c][j] * inv % Q;
        for (int i = 0; i < N; i++)
            if (i != c && M[i][c]) {
                int64_t f = M[i][c];
                for (int j = c; j < N + PARAM_L; j++) M[i][j] = md(M[i][j] - f * M[c][j]);
            }
    }
    for (int k = 0; k < PARAM_L; k++)
        for (int i = 0; i < N; i++) { int64_t v = M[i][N + k]; s[k][i] = v > Q / 2 ? v - Q : v; }
    return 1;
}

int main(void)
{
    static uint8_t pk[SIG_PUBLICKEYBYTES], sk[SIG_SECRETKEYBYTES];
    static uint8_t sig[SIG_BYTES], fsig[SIG_BYTES], forged[SIG_BYTES];
    smlen_t siglen, fsiglen;
    uint8_t seed[48], m[32];
    const int nmat = PARAM_K * PARAM_L;   /* expand_mat instantiations before unpack_c */

    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(7 * i + 3);
    __real_init_random_number(&drng_algorithm, seed, 48);
    if (msig_keygen(pk, sk) != 0) { puts("keygen failed"); return 1; }

    /* c0 is public: the faulted expansion of two unrelated seeds agrees. */
    poly c0, c0b;
    uint8_t sa[64], sb[64];
    memset(sa, 0x00, sizeof sa); memset(sb, 0xA7, sizeof sb);
    fail_next = 1; unpack_c(&c0, sa);
    fail_next = 1; unpack_c(&c0b, sb);
    int c0_public = memcmp(&c0, &c0b, sizeof c0) == 0;

    /* 1. Signing. */
    int tries = 0, recovered = 0, rc_h = -9, rc_f = -9, v_h = -9, v_f = -9, same_cseed = 0;
    for (int t = 0; t < 64 && !recovered; t++) {
        tries = t + 1;
        for (int i = 0; i < 32; i++) m[i] = (uint8_t)(i + 31 * t);
        n_irn = 0; fail_irn = -1;
        rc_h = msig_sign(sk, m, sizeof m, sig, &siglen);
        int last = n_irn - 1;              /* unpack_c of the accepting iteration */
        n_irn = 0; fail_irn = last; n_failed = 0;
        rc_f = msig_sign(sk, m, sizeof m, fsig, &fsiglen);
        fail_irn = -1;
        if (rc_h || rc_f || n_failed != 1 || last < nmat) continue;
        same_cseed = memcmp(sig + PARAM_L * POLZ_SIZE_PACKED, fsig + PARAM_L * POLZ_SIZE_PACKED, SEEDBYTES) == 0;
        if (!same_cseed) continue;         /* faulted iteration was rejected; try another message */
        v_h = msig_verf(pk, sig, siglen, m, sizeof m);
        v_f = msig_verf(pk, fsig, fsiglen, m, sizeof m);

        /* Attacker: public data only (the two signatures and c0). */
        polyvecl z, zf; polyveck h; uint8_t cs[SEEDBYTES]; poly c;
        unpack_sig(&z, &h, cs, sig);
        unpack_sig(&zf, &h, cs, fsig);
        unpack_c(&c, cs);
        static int64_t d[N], r[PARAM_L][N], s[PARAM_L][N];
        for (int i = 0; i < N; i++) d[i] = (int64_t)c.coeffs[i] - c0.coeffs[i];
        for (int k = 0; k < PARAM_L; k++)
            for (int i = 0; i < N; i++) r[k][i] = (int64_t)z.vec[k].coeffs[i] - zf.vec[k].coeffs[i];
        if (!ring_solve(d, r, s)) continue;
        int small = 1;
        uint8_t packed[PARAM_L * POLETA1_SIZE_PACKED];
        for (int k = 0; k < PARAM_L; k++) {
            poly p;
            for (int i = 0; i < N; i++) { if (s[k][i] < -ETA1 || s[k][i] > ETA1) small = 0; p.coeffs[i] = (int32_t)s[k][i]; }
            if (small) polyeta1_pack(packed + k * POLETA1_SIZE_PACKED, &p);
        }
        recovered = small && memcmp(packed, sk + 2 * SEEDBYTES + CRHBYTES, sizeof packed) == 0;
    }
    printf("c0 seed-independent=%d; sign: tries=%d honest rc=%d verify=%d, faulted rc=%d verify=%d same cseed=%d, s1 recovered=%d\n",
           c0_public, tries, rc_h, v_h, rc_f, v_f, same_cseed, recovered);

    /* 2. Verification: forge for a fresh message from pk alone, for challenge c0. */
    uint8_t msg[] = "a message the key holder never signed";
    uint8_t rho[SEEDBYTES], cseed[SEEDBYTES];
    polyveck t1, tmp1, tmp3, w1, h0;
    polyvecl z0, mat[PARAM_K];
    memset(&z0, 0, sizeof z0); memset(&h0, 0, sizeof h0);
    unpack_pk(rho, &t1, pk);
    uint8_t *buf = malloc(CRHBYTES + sizeof msg);
    memcpy(buf + CRHBYTES, msg, sizeof msg);
    KDF(buf, CRHBYTES, pk, SIG_PUBLICKEYBYTES);
    KDF(buf, CRHBYTES, buf, CRHBYTES + sizeof msg);
    expand_mat(mat, rho);
    polyvecl tz = z0; polyvecl_ntt(&tz);
    for (int i = 0; i < PARAM_K; ++i) polyvecl_pointwise_acc_invmontgomery(tmp1.vec + i, mat + i, &tz);
    polyveck_invntt_montgomery(&tmp1);
    emulate_ct1(&tmp3, &c0, &t1);
    polyveck_shiftl(&tmp3, PARAM_D);
    polyveck_sub(&tmp1, &tmp1, &tmp3);
    polyveck_g_reduce(&tmp1);
    polyveck_use_hint(&w1, &tmp1, &h0);
    challenge(cseed, buf, &w1);
    free(buf);
    smlen_t flen = pack_sig(forged, &z0, cseed, &h0);
    n_irn = 0; fail_irn = -1;
    int fv_plain = msig_verf(pk, forged, flen, msg, sizeof msg);
    n_irn = 0; fail_irn = nmat; n_failed = 0;   /* verifier: expand_mat, then unpack_c */
    int fv_fault = msig_verf(pk, forged, flen, msg, sizeof msg);
    fail_irn = -1;
    printf("verify: forged signature without fault=%d, with verifier unpack_c allocation failing=%d (failed allocations=%d)\n",
           fv_plain, fv_fault, n_failed);

    /* 3. Key generation: keygen calls expand_mat (nmat instantiations), then the s1 sampler. */
    static uint8_t pk2[2][SIG_PUBLICKEYBYTES], sk2[2][SIG_SECRETKEYBYTES];
    int kg_rc[2], kg_rt[2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(11 * i + 97 * k + 5);
        __real_init_random_number(&drng_algorithm, seed, 48);
        n_irn = 0; fail_irn = nmat; n_failed = 0;
        kg_rc[k] = msig_keygen(pk2[k], sk2[k]);
        fail_irn = -1;
        kg_rt[k] = msig_sign(sk2[k], m, sizeof m, sig, &siglen) == 0 && msig_verf(pk2[k], sig, siglen, m, sizeof m) == 0;
    }
    int s10_same = memcmp(sk2[0] + 2 * SEEDBYTES + CRHBYTES, sk2[1] + 2 * SEEDBYTES + CRHBYTES, POLETA1_SIZE_PACKED) == 0;
    int pk_differ = memcmp(pk2[0], pk2[1], SIG_PUBLICKEYBYTES) != 0;
    printf("keygen: rc=%d,%d round-trip=%d,%d public keys differ=%d, s1[0] identical=%d\n",
           kg_rc[0], kg_rc[1], kg_rt[0], kg_rt[1], pk_differ, s10_same);
    return 0;
}
