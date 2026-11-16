/* kem-10-7: C-Multi-UR-AG ignores the status of pseudohash and
 * init_random_number, so one failed allocation makes kem_enc / kem_keygen
 * return 0 with encryption coins or a secret key that anyone can recompute
 * from public data.
 *
 * Linked with -Wl,--wrap=malloc. When armed, the wrapper fails exactly one
 * allocation: the k-th request (k counted from arming) of a given size.
 *   enc-G : size |pk|+|m|+|salt|+2, the first buffer pseudohash_512 allocates
 *           for theta = G(pk, m, salt) (kem.c:123). theta stays {0} (kem.c:94).
 *   enc-S : size 64, the seed buffer of the first init_random_number in
 *           encapsulation, i.e. seeding the coin expander with theta
 *           (cmultiurag.c:152 via random_source.c:30). The expander keeps the
 *           all-zero DRNG state.
 *   kg-S  : size 64, the first init_random_number in key generation, i.e.
 *           the sk seed expander. X and Y are then drawn from the all-zero
 *           DRNG state.
 * The "attacker" functions below use only pk and ct (and the public code). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "drng.h"
#include "auxfunc.h"
#include "parameters.h"
#include "parsing.h"
#include "cmultiurag.h"
#include "rbc_vec.h"
#include "rbc_mat.h"
#include "rbc_vspace.h"
#include "augmented_gabidulin.h"
#include "random_source.h"

DRNG_ctx drng_algorithm;
int kem_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int kem_enc(unsigned char *, unsigned long long, unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int kem_dec(unsigned char *, unsigned long long, unsigned char *, unsigned long long, unsigned char *, unsigned long long *);

void *__real_malloc(size_t);
static size_t fail_size;
static int fail_skip, failed;
void *__wrap_malloc(size_t n)
{
    if (fail_size && n == fail_size) {
        if (fail_skip-- == 0) { fail_size = 0; failed++; return NULL; }
    }
    return __real_malloc(n);
}
static void arm(size_t size, int skip) { fail_size = size; fail_skip = skip; failed = 0; }

#define PK CMULTIURAG_PUBLIC_KEY_BYTES
#define SK CMULTIURAG_SECRET_KEY_BYTES
#define CT CMULTIURAG_CIPHERTEXT_BYTES
#define SS CMULTIURAG_SHARED_SECRET_BYTES
#define N  CMULTIURAG_PARAM_N
#define N1 CMULTIURAG_PARAM_N1
#define N2 CMULTIURAG_PARAM_N2
#define G_ALLOC (PK + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_SALT_BYTES + 2)
#define SEED_ALLOC 64

/* K = H(pk || m || U || V || salt), exactly as kem.c:132-137 */
static void kdf(uint8_t *ss, const uint8_t *pk, const rbc_vec m, const uint8_t *ct)
{
    static uint8_t H_in[PK + CMULTIURAG_VEC_K_BYTES + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES + CMULTIURAG_SALT_BYTES];
    memcpy(H_in, pk, PK);
    rbc_vec_to_string(H_in + PK, m, CMULTIURAG_PARAM_K);
    memcpy(H_in + PK + CMULTIURAG_VEC_K_BYTES, ct, CT);   /* U || V || salt */
    pseudohash(512, H_in, sizeof H_in * 8, ss);
}

static void decode(rbc_vec m, const uint8_t *pk, rbc_mat W)
{
    rbc_vec g, mG;
    rbc_mat H, S;
    rbc_augmented_gabidulin code;
    rbc_vec_init(&g, N1 * N2); rbc_vec_init(&mG, N1 * N2);
    rbc_mat_init(&H, N, N); rbc_mat_init(&S, N, N1);
    cmultiurag_public_key_from_string(g, H, S, pk);
    rbc_mat_unfold(mG, W, N1, N2);
    rbc_augmented_gabidulin_init(&code, g, CMULTIURAG_PARAM_K, N1 * N2);
    rbc_augmented_gabidulin_decode(m, code, mG);
    rbc_vec_clear(g); rbc_vec_clear(mG); rbc_mat_clear(H); rbc_mat_clear(S);
}

/* Attacker 1: guess that the coins came from theta = 0 (mode 0) or from an
 * all-zero expander state (mode 1); confirm on U, then read m off V. */
static int attack_enc(uint8_t *ss, const uint8_t *pk, const uint8_t *ct, int mode)
{
    rbc_mat U0, V0;
    rbc_vec m0, m;
    uint8_t theta[SHA512_BYTES] = {0};
    static uint8_t a[CMULTIURAG_MAT_NN2_BYTES];
    rbc_mat V;
    int ok;

    rbc_mat_init(&U0, N, N2); rbc_mat_init(&V0, N1, N2); rbc_mat_init(&V, N1, N2);
    rbc_vec_init(&m0, CMULTIURAG_PARAM_K); rbc_vec_init(&m, CMULTIURAG_PARAM_K);
    if (mode == 1) arm(SEED_ALLOC, 0);   /* the attacker's own run of the public encryptor */
    cmultiurag_pke_encrypt(U0, V0, m0, theta, pk);
    fail_size = 0;
    rbc_mat_to_string(a, U0, N, N2);
    ok = memcmp(a, ct, sizeof a) == 0;
    if (ok) {
        rbc_mat_from_string(V, N1, N2, ct + CMULTIURAG_MAT_NN2_BYTES);
        rbc_mat_add(V0, V, V0, N1, N2);  /* V - V0 = Fold(m.G) */
        V0[0][0][0] ^= 1;                /* add a known rank-1 error (cf. the BRA
                                            decoder, which mishandles an exact codeword) */
        decode(m, pk, V0);
        kdf(ss, pk, m, ct);
    }
    rbc_mat_clear(U0); rbc_mat_clear(V0); rbc_mat_clear(V);
    rbc_vec_clear(m0); rbc_vec_clear(m);
    return ok;
}

/* Attacker 2: rebuild (X, Y) from the all-zero expander state, check them
 * against pk's S = X + H.Y, and decrypt ct with Y. */
static int attack_key(uint8_t *ss, const uint8_t *pk, const uint8_t *ct)
{
    random_source z;
    rbc_vspace sup;
    rbc_mat X, Y, YT, H, S, T, U, V, W;
    rbc_vec g, m;
    uint8_t salt[CMULTIURAG_SALT_BYTES];
    static uint8_t a[CMULTIURAG_MAT_NN1_BYTES];
    int ok;

    random_source_init(&z, RANDOM_SOURCE_SEEDEXP);
    rbc_vspace_init(&sup, CMULTIURAG_PARAM_W1);
    rbc_mat_init(&X, N, N1); rbc_mat_init(&Y, N, N1); rbc_mat_init(&YT, N1, N);
    rbc_mat_init(&H, N, N); rbc_mat_init(&S, N, N1); rbc_mat_init(&T, N, N1);
    rbc_mat_init(&U, N, N2); rbc_mat_init(&V, N1, N2); rbc_mat_init(&W, N1, N2);
    rbc_vec_init(&g, N1 * N2); rbc_vec_init(&m, CMULTIURAG_PARAM_K);
    rbc_vspace_set_random_full_rank_with_one(&z, sup, CMULTIURAG_PARAM_W1);
    rbc_mat_set_random_from_support(&z, X, N, N1, sup, CMULTIURAG_PARAM_W1, 1);
    rbc_mat_set_random_from_support(&z, Y, N, N1, sup, CMULTIURAG_PARAM_W1, 1);
    cmultiurag_public_key_from_string(g, H, S, pk);
    rbc_mat_mul(T, H, Y, N, N, N1); rbc_mat_add(T, T, X, N, N1);   /* X + H.Y */
    rbc_mat_to_string(a, T, N, N1);
    ok = memcmp(a, pk, sizeof a) == 0;
    if (ok) {
        cmultiurag_kem_ciphertext_from_string(U, V, salt, ct);
        rbc_mat_trans(YT, Y, N, N1);
        rbc_mat_mul(W, YT, U, N1, N, N2); rbc_mat_add(W, V, W, N1, N2);   /* V - Y^T.U */
        decode(m, pk, W);
        kdf(ss, pk, m, ct);
    }
    rbc_vspace_clear(sup);
    rbc_mat_clear(X); rbc_mat_clear(Y); rbc_mat_clear(YT); rbc_mat_clear(H); rbc_mat_clear(S);
    rbc_mat_clear(T); rbc_mat_clear(U); rbc_mat_clear(V); rbc_mat_clear(W);
    rbc_vec_clear(g); rbc_vec_clear(m);
    return ok;
}

static uint8_t pk[PK], sk[SK], ct[CT], ss[SS], ssd[SS], ssa[SS];

/* The key owner's decapsulation, run in a child process (a mismatched key
 * pair can drive the decoder into a crash). Returns 1 if it completed and
 * agrees with ss, 0 if it completed and disagrees, -1 if it crashed. */
static int owner_dec(void)
{
    int fd[2], st;
    unsigned long long a;
    if (pipe(fd)) return -2;
    pid_t p = fork();
    if (p == 0) {
        close(fd[0]);
        kem_dec(sk, SK, ct, CT, ssd, &a);
        if (write(fd[1], ssd, SS) != SS) _exit(3);
        _exit(0);
    }
    close(fd[1]);
    ssize_t n = read(fd[0], ssd, SS);
    close(fd[0]);
    waitpid(p, &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0 || n != SS) return -1;
    return !memcmp(ss, ssd, SS);
}

static void enc_case(const char *name, size_t size, int skip, int mode)
{
    unsigned long long a, b;
    int rc, ok;
    memset(ss, 0, SS);
    failed = 0;
    if (size) arm(size, skip);
    rc = kem_enc(pk, PK, ss, &a, ct, &b);
    int f = failed; fail_size = 0;
    int od = owner_dec();
    memset(ssa, 0xEE, SS);
    ok = attack_enc(ssa, pk, ct, mode);
    printf("%s: faults=%d kem_enc=%d owner_dec_matches=%d attacker_confirms_coins=%d attacker_ss_matches=%d\n",
           name, f, rc, od, ok, ok && !memcmp(ssa, ss, SS));
}

int main(void)
{
    uint8_t seed[64];
    unsigned long long a, b;
    int rc, ok;
    setvbuf(stdout, NULL, _IONBF, 0);
    for (int i = 0; i < 64; i++) seed[i] = (uint8_t)(7 * i + 3);
    init_random_number(&drng_algorithm, seed, 64);

    kem_keygen(pk, &a, sk, &b);
    enc_case("enc control", 0, 0, 0);
    enc_case("enc control (zero-state guess)", 0, 0, 1);
    enc_case("enc G-hash alloc fails", G_ALLOC, 0, 0);
    enc_case("enc coin-seed alloc fails", SEED_ALLOC, 0, 1);

    for (int fault = 0; fault <= 1; fault++) {
        failed = 0;
        if (fault) arm(SEED_ALLOC, 0);
        rc = kem_keygen(pk, &a, sk, &b);
        int f = failed; fail_size = 0;
        kem_enc(pk, PK, ss, &a, ct, &b);              /* honest sender, no fault */
        int od = owner_dec();
        memset(ssa, 0xEE, SS);
        ok = attack_key(ssa, pk, ct);
        printf("%s: faults=%d kem_keygen=%d owner_dec_matches=%d attacker_key_matches_pk=%d attacker_ss_matches=%d\n",
               fault ? "keygen sk-seed alloc fails" : "keygen control", f, rc, od, ok, ok && !memcmp(ssa, ss, SS));
    }
    return 0;
}
