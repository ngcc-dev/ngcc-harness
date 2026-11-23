/* sign-31-9: TSUOV sig_verify keeps the expanded public key in a static
 * buffer, so honest verifications under different keys in different threads
 * overwrite each other's key and fail, and a verification under a public key
 * the attacker supplies can make a signature that is invalid for the victim's
 * key verify under it.
 *
 * sig_verify (SIG_AlgorithmInstance.c:100-121) declares `static TSUOV_P3 P3`
 * (:109), fills it from the caller's public key with restore_TSUOV_P3 (:112;
 * tsuov_core.c:1060-1071) and reads it throughout TSUOV_Verify (:118;
 * tsuov_core.c:1011-1012). Every call in the process shares that one buffer.
 * A thread verifying under another public key overwrites it while the first
 * thread is still reading, and the first thread's check fails.
 *
 * Honest inputs only: two key pairs from sig_keygen and one signature from
 * sig_sign under each, all made before any thread starts. The argument is the
 * number of rounds per thread. Prints failed/total honest verifications for:
 * one thread taking the two keys in turn (control); four threads all under
 * key 0 (control: the racing writes store identical values); four threads, two
 * per key, released together and then free-running; and one thread again
 * afterwards (control), with a check that keys and signatures are unchanged.
 *
 * With a second argument "forge", the same keys are made and then, instead,
 * the attacker part runs. The verifier's accumulated value acc
 * (tsuov_core.c:1028-1039) is affine in the P3 entries: Expand_PK copies P3[i]
 * into the oil-oil block of Pi (:570-572), each U[.][i] is a bilinear form in
 * the signature through Pi (:1017-1024), and multiply_E_add_m1_m2 is linear
 * (:242-270). P3 is stored explicitly in the public key, so the attacker,
 * from pk_V's seed only (no secret key), picks a random signature value S on
 * a fresh message, computes acc = acc0 + A x by the model below (checked first
 * against both honest signatures), solves A x = target - acc0 over F_31 for
 * the P3 entries x, and stores them with store_TSUOV_P3 behind arbitrary seed
 * bytes as pk_X. Checks, single thread: the unmodified TSUOV_Verify(seed_V,
 * P3_X, m, S) accepts, and sig_verify(pk_V, S, m) rejects. Then counts how
 * often sig_verify(pk_V, S, m) returns 0 in one thread (control), in four
 * threads with three verifying the honest signature under key 1 (control),
 * and in four threads with three calling sig_verify(pk_X, junk, junk); then
 * one thread again afterwards (control), with a check that inputs are
 * unchanged. */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "tsuov_core.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

#define THREADS 4
#define MLEN 32

static unsigned char pk[2][CRYPTO_PUBLICKEYBYTES], sk[2][CRYPTO_SECRETKEYBYTES];
static unsigned char sn[2][CRYPTO_BYTES], msg[2][MLEN];
static unsigned char pk0[2][CRYPTO_PUBLICKEYBYTES], sn0[2][CRYPTO_BYTES];
static unsigned long long pkl[2], skl[2], snl[2];
static pthread_barrier_t gate;
static int rounds = 20;

struct job { int key, fail; };

static int honest_verify(int k)
{
    return sig_verify(pk[k], pkl[k], sn[k], snl[k], msg[k], MLEN);
}

static void *worker(void *arg)
{
    struct job *j = arg;
    pthread_barrier_wait(&gate);
    for (int r = 0; r < rounds; r++)
        j->fail += honest_verify(j->key) != 0;
    return NULL;
}

/* THREADS threads; thread t verifies the honest signature under key t % keys */
static int concurrent(int keys)
{
    pthread_t th[THREADS];
    struct job job[THREADS];
    int fail = 0;
    if (pthread_barrier_init(&gate, NULL, THREADS)) exit(2);
    for (int t = 0; t < THREADS; t++) {
        job[t].key = t % keys;
        job[t].fail = 0;
        if (pthread_create(&th[t], NULL, worker, &job[t])) exit(2);
    }
    for (int t = 0; t < THREADS; t++) {
        if (pthread_join(th[t], NULL)) exit(2);
        fail += job[t].fail;
    }
    pthread_barrier_destroy(&gate);
    return fail;
}

static int sequential(int n)
{
    int fail = 0;
    for (int i = 0; i < n; i++) fail += honest_verify(i & 1) != 0;
    return fail;
}

/* ---------------- attacker part ("forge") ---------------- */

/* F_31[w]/(w^2 - 3), as in VECTOR_mul_MATRIX (tsuov_core.c:30-49) */
typedef struct { int a, b; } F;
static int md(int x) { x %= 31; return x < 0 ? x + 31 : x; }
static F fadd(F x, F y) { F r = { md(x.a + y.a), md(x.b + y.b) }; return r; }
static F fmul(F x, F y) { F r = { md(x.a * y.a + 3 * x.b * y.b), md(x.a * y.b + x.b * y.a) }; return r; }

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

/* target Hash(Expand_mu(seed_pk, m), salt) (tsuov_core.c:415-439, 996-999) */
static void target(const TSUOV_SEED seed, const unsigned char *m, const TSUOV_SALT salt, int t[TSUOV_m2])
{
    uint8_t buf[TSUOV_SEED_LEN + MLEN], mu[TSUOV_MU_LEN], in[TSUOV_MU_LEN + TSUOV_SALT_LEN];
    uint8_t x[TSUOV_t_RS_LEN], h[TSUOV_m2];
    memcpy(buf, seed, TSUOV_SEED_LEN); memcpy(buf + TSUOV_SEED_LEN, m, MLEN);
    pseudohash(512, buf, (TSUOV_SEED_LEN + MLEN) * 8ull, mu);
    memcpy(in, mu, TSUOV_MU_LEN); memcpy(in + TSUOV_MU_LEN, salt, TSUOV_SALT_LEN);
    pseudoXOF(TSUOV_t_RS_LEN * 8ull, in, (TSUOV_MU_LEN + TSUOV_SALT_LEN) * 8ull, x);
    rejsamp(x, TSUOV_t_RS_LEN, h, TSUOV_m2);
    for (int r = 0; r < TSUOV_m2; r++) t[r] = h[r];
}

/* P3 entries P3[i][j][n][t], t >= j, in store_TSUOV_P3 order (tsuov_core.c:1050-1058) */
#define NVI (TSUOV_O * (TSUOV_O + 1) / 2 * TSUOV_L)
#define NV (TSUOV_m1 * NVI)
static int A[TSUOV_m2][NV + 1];

/* v[d] += c * z^d mod f(z), z^m2 = F_COEF[0] + F_COEF[1] z + F_COEF[2] z^2,
 * as multiply_E_vec_scalar applies it (tsuov_core.c:242-260) */
static void addz(int *v, int stride, int d, int c)
{
    static const int fc[] = F_COEF;
    if (d < TSUOV_m2) { v[d * stride] += c; return; }
    d -= TSUOV_m2;
    for (int e = 0; e < 3; e++) v[(d + e) * stride] += fc[e] * c;
}

/* acc of TSUOV_Verify (tsuov_core.c:1001-1039) as acc0 + A x for the signature
 * sg and the public seed: Pi from Expand_PK (:535-580) with P3 = 0 gives
 * acc0; U[(k1,k2)][i] = lambda(s_k1^T Pi s_k2), lambda(a + b w) = a + b, which
 * is vector_dot of the phi_t2_q31_planar images (:1017-1024); acc =
 * sum z^ctr U(z) over pairs in the order of :1032-1039. The P3 part of
 * s_k1^T Pi s_k2 is sum over j <= t of P3[j][t] (o1[j] o2[t] + [j != t] o1[t] o2[j]),
 * o = oil part of s; lambda(c) = c.a + c.b for entry n = 0, c.a + 3 c.b for n = 1. */
static void model(const TSUOV_SEED seed, const TSUOV_SIGNATURE_s *sg, int acc0[TSUOV_m2])
{
    static uint8_t r1[TSUOV_Pi1_LEN], r2[TSUOV_Pi2_LEN];
    static F Pi[TSUOV_N][TSUOV_N], s[TSUOV_k][TSUOV_N], x[TSUOV_k][TSUOV_N];
    const F zero = { 0, 0 };
    memset(acc0, 0, TSUOV_m2 * sizeof(int));
    memset(A, 0, sizeof A);
    for (int k = 0; k < TSUOV_k; k++)
        for (int a = 0; a < TSUOV_N; a++) { F v = { sg->s[k][0][a], sg->s[k][1][a] }; s[k][a] = v; }
    MGF_CTX_s *c0 = PRG_init_sm3(seed);
    for (int i = 0; i < TSUOV_m1; i++) {
        RejSampPRG_sm3(c0, 2 * i, TSUOV_Pi1_LEN, TSUOV_Pi1_RS_LEN, r1);
        RejSampPRG_sm3(c0, 2 * i + 1, TSUOV_Pi2_LEN, TSUOV_Pi2_RS_LEN, r2);
        const uint8_t *p = r1;
        for (int a = 0; a < TSUOV_V; a++)
            for (int b = a; b < TSUOV_V; b++, p += 2) { F v = { md(-p[0]), md(-p[1]) }; Pi[a][b] = Pi[b][a] = v; }
        p = r2;
        for (int a = TSUOV_V; a < TSUOV_N; a++)
            for (int b = 0; b < TSUOV_V; b++, p += 2) { F v = { p[0], p[1] }; Pi[a][b] = Pi[b][a] = v; }
        for (int a = TSUOV_V; a < TSUOV_N; a++)
            for (int b = TSUOV_V; b < TSUOV_N; b++) Pi[a][b] = zero;
        for (int k = 0; k < TSUOV_k; k++)
            for (int b = 0; b < TSUOV_N; b++) {
                F t = zero;
                for (int a = 0; a < TSUOV_N; a++) t = fadd(t, fmul(s[k][a], Pi[a][b]));
                x[k][b] = t;
            }
        int ctr = 0;
        for (int k1 = 0; k1 < TSUOV_k; k1++)
            for (int k2 = TSUOV_k - 1; k2 >= k1; k2--, ctr++) {
                F u = zero;
                for (int b = 0; b < TSUOV_N; b++) u = fadd(u, fmul(x[k1][b], s[k2][b]));
                addz(acc0, 1, ctr + i, u.a + u.b);
                int v = i * NVI;
                for (int j = 0; j < TSUOV_O; j++)
                    for (int t = j; t < TSUOV_O; t++, v += 2) {
                        F c = fmul(s[k1][TSUOV_V + j], s[k2][TSUOV_V + t]);
                        if (t != j) c = fadd(c, fmul(s[k1][TSUOV_V + t], s[k2][TSUOV_V + j]));
                        addz(&A[0][v], NV + 1, ctr + i, c.a + c.b);
                        addz(&A[0][v + 1], NV + 1, ctr + i, c.a + 3 * c.b);
                    }
            }
    }
    PRG_final_sm3(c0);
    for (int r = 0; r < TSUOV_m2; r++) {
        acc0[r] = md(acc0[r]);
        for (int v = 0; v < NV; v++) A[r][v] = md(A[r][v]);
    }
}

static void p3_to_x(const TSUOV_P3 P, int x[NV])
{
    int v = 0;
    for (int i = 0; i < TSUOV_m1; i++)
        for (int j = 0; j < TSUOV_O; j++)
            for (int t = j; t < TSUOV_O; t++)
                for (int n = 0; n < TSUOV_L; n++) x[v++] = P[i][j][n][t];
}

/* model check on honest key k: acc0 + A x(P3 of pk[k]) equals the target */
static int model_matches(int k)
{
    static TSUOV_SIGNATURE_s sg;
    static TSUOV_P3 P;
    static int x[NV];
    int acc0[TSUOV_m2], t[TSUOV_m2];
    TSUOV_SEED seed;
    size_t pb = 0;
    restore_TSUOV_SEED(pk[k], &pb, seed);
    restore_TSUOV_P3(pk[k], &pb, P);
    pb = 0; restore_TSUOV_SIGNATURE(sn[k], &pb, &sg);
    model(seed, &sg, acc0);
    target(seed, msg[k], sg.r, t);
    p3_to_x(P, x);
    for (int r = 0; r < TSUOV_m2; r++) {
        int a = acc0[r];
        for (int v = 0; v < NV; v++) a += A[r][v] * x[v];
        if (md(a) != t[r]) return 0;
    }
    return 1;
}

/* solve A x = A[.][NV] over F_31 (free entries random); returns the rank, -1 if inconsistent */
static uint64_t xs = 0x9E3779B97F4A7C15ull;
static int rnd31(void) { xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17; return (int)(xs % 31); }
static int solve(int x[NV])
{
    int piv[TSUOV_m2], rank = 0;
    static int isp[NV];
    for (int c = 0; c < NV && rank < TSUOV_m2; c++) {
        int p = -1;
        for (int r = rank; r < TSUOV_m2; r++) if (A[r][c]) { p = r; break; }
        if (p < 0) continue;
        for (int q = 0; q <= NV; q++) { int t = A[p][q]; A[p][q] = A[rank][q]; A[rank][q] = t; }
        int iv = Fq_inv((Fq)A[rank][c]);
        for (int q = 0; q <= NV; q++) A[rank][q] = A[rank][q] * iv % 31;
        for (int r = 0; r < TSUOV_m2; r++)
            if (r != rank && A[r][c]) {
                int f = A[r][c];
                for (int q = 0; q <= NV; q++) A[r][q] = md(A[r][q] - f * A[rank][q]);
            }
        piv[rank++] = c;
        isp[c] = 1;
    }
    for (int r = rank; r < TSUOV_m2; r++) if (A[r][NV]) return -1;
    for (int v = 0; v < NV; v++) x[v] = isp[v] ? 0 : rnd31();
    for (int r = 0; r < rank; r++) {
        int a = A[r][NV];
        for (int v = 0; v < NV; v++) if (!isp[v]) a -= A[r][v] * x[v];
        x[piv[r]] = md(a);
    }
    return rank;
}

static unsigned char pkX[CRYPTO_PUBLICKEYBYTES], snS[CRYPTO_BYTES], mF[MLEN], junk[CRYPTO_BYTES];
static unsigned char pkX0[CRYPTO_PUBLICKEYBYTES], snS0[CRYPTO_BYTES], mF0[MLEN];

static int victim_verify(void)
{
    return sig_verify(pk[0], pkl[0], snS, CRYPTO_BYTES, mF, MLEN);
}

/* role 0: victim, counts sig_verify(pk_V, S, m) == 0; role 1: sig_verify(pk_X,
 * junk, junk); role 2: honest signature under key 1 */
struct fjob { int role, ok; };

static void *forge_worker(void *arg)
{
    struct fjob *j = arg;
    pthread_barrier_wait(&gate);
    for (int r = 0; r < rounds; r++) {
        if (j->role == 0) j->ok += victim_verify() == 0;
        else if (j->role == 1) sig_verify(pkX, CRYPTO_PUBLICKEYBYTES, junk, CRYPTO_BYTES, junk, 7);
        else honest_verify(1);
    }
    return NULL;
}

/* the victim thread and nothers threads of the given role, released together */
static int forge_race(int role, int nothers)
{
    pthread_t th[THREADS];
    struct fjob job[THREADS];
    int n = 1 + nothers;
    if (pthread_barrier_init(&gate, NULL, (unsigned)n)) exit(2);
    for (int t = 0; t < n; t++) {
        job[t].role = t ? role : 0;
        job[t].ok = 0;
        if (pthread_create(&th[t], NULL, forge_worker, &job[t])) exit(2);
    }
    for (int t = 0; t < n; t++)
        if (pthread_join(th[t], NULL)) exit(2);
    pthread_barrier_destroy(&gate);
    return job[0].ok;
}

static int forge(void)
{
    static TSUOV_SIGNATURE_s sg;
    static TSUOV_P3 PX;
    static int x[NV];
    int acc0[TSUOV_m2], t[TSUOV_m2];
    int matches = model_matches(0) + model_matches(1);

    /* attacker: pk_V's seed, a fresh message and a random signature value S */
    TSUOV_SEED seedV, any;
    size_t pb = 0;
    restore_TSUOV_SEED(pk[0], &pb, seedV);
    for (int i = 0; i < MLEN; i++) mF[i] = (unsigned char)(i ^ 0xa7);
    memset(&sg, 0, sizeof sg);
    for (int i = 0; i < TSUOV_SALT_LEN; i++) sg.r[i] = (uint8_t)(rnd31() * 8 + 5);
    for (int k = 0; k < TSUOV_k; k++)
        for (int l = 0; l < TSUOV_L; l++)
            for (int a = 0; a < TSUOV_N; a++) sg.s[k][l][a] = (Fq)rnd31();
    pb = 0; store_TSUOV_SIGNATURE(&sg, snS, &pb);
    pb = 0; restore_TSUOV_SIGNATURE(snS, &pb, &sg);
    model(seedV, &sg, acc0);
    target(seedV, mF, sg.r, t);
    for (int r = 0; r < TSUOV_m2; r++) A[r][NV] = md(t[r] - acc0[r]);
    int rank = solve(x);
    if (rank < 0) { printf("forgery: the system for P3_X is inconsistent\n"); return 0; }
    int v = 0;
    for (int i = 0; i < TSUOV_m1; i++)
        for (int j = 0; j < TSUOV_O; j++)
            for (int u = j; u < TSUOV_O; u++)
                for (int n = 0; n < TSUOV_L; n++, v++)
                    PX[i][j][n][u] = PX[i][u][n][j] = (Fq)x[v];
    memset(any, 0xA5, sizeof any);
    pb = 0; store_TSUOV_SEED(any, pkX, &pb); store_TSUOV_P3(PX, pkX, &pb);
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = (unsigned char)(37 * i + 11);

    /* single thread: the composition accepts, pk_V alone rejects */
    static TSUOV_P3 PXr;
    TSUOV_SEED s2;
    pb = 0; restore_TSUOV_SEED(pkX, &pb, s2); restore_TSUOV_P3(pkX, &pb, PXr);
    int comp = TSUOV_Verify(seedV, PXr, mF, MLEN, &sg);
    int alone = victim_verify();
    memcpy(pkX0, pkX, sizeof pkX); memcpy(snS0, snS, sizeof snS); memcpy(mF0, mF, sizeof mF);
    memcpy(pk0, pk, sizeof pk);

    int one = forge_race(0, 0);
    int ctl = forge_race(2, THREADS - 1);
    int hit = forge_race(1, THREADS - 1);
    int after = victim_verify() == 0;
    int kept = !memcmp(pk0, pk, sizeof pk) && !memcmp(pkX0, pkX, sizeof pkX) &&
               !memcmp(snS0, snS, sizeof snS) && !memcmp(mF0, mF, sizeof mF);
    printf("attacker model matches %d/2 honest signatures; random S on a fresh message, "
           "P3_X solved from %d equations in %d unknowns (rank %d); single thread "
           "TSUOV_Verify(seed_V, P3_X, m, S)=%d, sig_verify(pk_V, S, m)=%d; sig_verify(pk_V, S, m) accepted: "
           "one thread %d/%d; four threads, three under honest key 1, %d/%d; "
           "four threads, three under pk_X, %d/%d; one thread afterwards %d/1, inputs unchanged=%d\n",
           matches, TSUOV_m2, NV, rank, comp, alone, one, rounds, ctl, rounds, hit, rounds, after, kept);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned char seed[48];
    if (argc > 1) rounds = atoi(argv[1]);
    if (rounds < 1 || rounds > 100000) return 2;
    int n = THREADS * rounds;
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(7 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < MLEN; i++) msg[k][i] = (unsigned char)(i + 100 * k);
        if (sig_keygen(pk[k], &pkl[k], sk[k], &skl[k])) return 2;
        if (sig_sign(sk[k], skl[k], msg[k], MLEN, sn[k], &snl[k])) return 2;
    }
    if (!memcmp(pk[0], pk[1], sizeof pk[0])) return 2;
    memcpy(pk0, pk, sizeof pk);
    memcpy(sn0, sn, sizeof sn);
    if (argc > 2 && !strcmp(argv[2], "forge")) return forge();

    int one = sequential(n);
    int same = concurrent(1);
    int race = concurrent(2);
    int after = sequential(2);
    int kept = !memcmp(pk0, pk, sizeof pk) && !memcmp(sn0, sn, sizeof sn);
    printf("one thread %d/%d fail; four threads one key %d/%d fail; "
           "four threads two keys %d/%d fail; one thread afterwards %d/2 fail, inputs unchanged=%d\n",
           one, n, same, n, race, n, after, kept);
    return 0;
}
