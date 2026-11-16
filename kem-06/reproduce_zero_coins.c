/* kem-06-7: BRA ignores the status of pseudohash and init_random_number, so
 * one failed allocation makes kem_enc / kem_keygen return 0 with encryption
 * coins or a secret key that anyone can recompute from public data.
 *
 * Linked with -Wl,--wrap=malloc. When armed, the wrapper fails exactly one
 * allocation: the k-th request (k counted from arming) of a given size.
 *   enc-G : size |pk|+|m|+|salt|+2, the first buffer pseudohash_512 allocates
 *           for theta = G(pk, m, salt) (kem.c:126). theta stays {0} (kem.c:95).
 *   enc-S : size 64, the seed buffer of the first init_random_number in
 *           encapsulation, i.e. seeding the coin expander with theta
 *           (bra.c:171 via random_source.c:30). The expander keeps the
 *           all-zero DRNG state left by random_source_init / Instantiate.
 *   kg-S  : size 64, the first init_random_number in key generation, i.e.
 *           the sk seed expander (bra.c:63). x and y are then drawn from the
 *           all-zero DRNG state.
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
#include "bra.h"
#include "rbc_qre.h"
#include "rbc_vec.h"
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

#define PK BRA_PUBLIC_KEY_BYTES
#define SK BRA_SECRET_KEY_BYTES
#define CT BRA_CIPHERTEXT_BYTES
#define SS BRA_SHARED_SECRET_BYTES
#define G_ALLOC (BRA_PUBLIC_KEY_BYTES + BRA_VEC_K_BYTES + BRA_SALT_BYTES + 2)
#define SEED_ALLOC 64

/* K = H(pk || m || u || v || salt), exactly as kem.c:136-141 */
static void kdf(uint8_t *ss, const uint8_t *pk, const rbc_vec m, const uint8_t *ct)
{
    static uint8_t H_in[PK + BRA_VEC_K_BYTES + 2 * BRA_VEC_N_BYTES + BRA_SALT_BYTES];
    memcpy(H_in, pk, PK);
    rbc_vec_to_string(H_in + PK, m, BRA_PARAM_K);
    memcpy(H_in + PK + BRA_VEC_K_BYTES, ct, 2 * BRA_VEC_N_BYTES + BRA_SALT_BYTES); /* u || v || salt */
    pseudohash(512, H_in, sizeof H_in * 8, ss);
}

/* Attacker 1: guess that the coins came from theta = 0 (mode 0) or from an
 * all-zero expander state (mode 1); confirm on u, then read m off v. */
static int attack_enc(uint8_t *ss, const uint8_t *pk, const uint8_t *ct, int mode)
{
    rbc_qre u, v, u0, v0, g, h, s;
    rbc_vec m0, m;
    uint8_t salt[BRA_SALT_BYTES], theta[SHA512_BYTES] = {0};
    uint8_t a[BRA_VEC_N_BYTES], b[BRA_VEC_N_BYTES];
    rbc_augmented_gabidulin code;
    int ok;

    rbc_field_init();
    rbc_qre_init_modulus(BRA_PARAM_N);
    rbc_qre_init(&u); rbc_qre_init(&v); rbc_qre_init(&u0); rbc_qre_init(&v0);
    rbc_qre_init(&g); rbc_qre_init(&h); rbc_qre_init(&s);
    rbc_vec_init(&m0, BRA_PARAM_K); rbc_vec_init(&m, BRA_PARAM_K);
    bra_kem_ciphertext_from_string(u, v, salt, ct);
    if (mode == 1) arm(SEED_ALLOC, 0);   /* the attacker's own run of the public encryptor */
    bra_pke_encrypt(u0, v0, m0, theta, pk);
    fail_size = 0;
    rbc_qre_init_modulus(BRA_PARAM_N);
    rbc_qre_to_string(a, u); rbc_qre_to_string(b, u0);
    ok = memcmp(a, b, sizeof a) == 0;
    if (ok) {
        rbc_qre_add(v0, v, v0);          /* v - v0 = Encode(m), a codeword */
        v0->v[0][0] ^= 1;                /* add a known rank-1 error: the decoder
                                            mishandles an exact codeword */
        bra_public_key_from_string(g, h, s, pk);
        rbc_augmented_gabidulin_init(&code, g->v, BRA_PARAM_K, BRA_PARAM_N);
        rbc_augmented_gabidulin_decode(m, code, v0->v);
        kdf(ss, pk, m, ct);
    }
    rbc_qre_clear(u); rbc_qre_clear(v); rbc_qre_clear(u0); rbc_qre_clear(v0);
    rbc_qre_clear(g); rbc_qre_clear(h); rbc_qre_clear(s);
    rbc_vec_clear(m0); rbc_vec_clear(m);
    rbc_qre_clear_modulus();
    return ok;
}

/* Attacker 2: rebuild (x, y) from the all-zero expander state, check them
 * against pk's syndrome, and decrypt ct with y. */
static int attack_key(uint8_t *ss, const uint8_t *pk, const uint8_t *ct)
{
    random_source z;
    rbc_vspace sx, sy, sxy;
    rbc_qre x, y, g, h, s, t, u, v;
    rbc_vec m;
    uint8_t salt[BRA_SALT_BYTES], a[BRA_VEC_N_BYTES], b[BRA_VEC_N_BYTES];
    rbc_augmented_gabidulin code;
    int ok;

    rbc_field_init();
    rbc_qre_init_modulus(BRA_PARAM_N);
    random_source_init(&z, RANDOM_SOURCE_SEEDEXP);
    rbc_vspace_init(&sx, BRA_PARAM_W_x); rbc_vspace_init(&sy, BRA_PARAM_W_y);
    rbc_vspace_init(&sxy, BRA_PARAM_W_x + BRA_PARAM_W_y);
    rbc_qre_init(&x); rbc_qre_init(&y); rbc_qre_init(&g); rbc_qre_init(&h);
    rbc_qre_init(&s); rbc_qre_init(&t); rbc_qre_init(&u); rbc_qre_init(&v);
    rbc_vec_init(&m, BRA_PARAM_K);
    rbc_vspace_set_random_full_rank(&z, sxy, BRA_PARAM_W_x + BRA_PARAM_W_y);
    for (size_t i = 0; i < BRA_PARAM_W_x; i++) rbc_elt_set(sx[i], sxy[i]);
    for (size_t i = 0; i < BRA_PARAM_W_y; i++) rbc_elt_set(sy[i], sxy[BRA_PARAM_W_x + i]);
    rbc_qre_set_random_from_support(&z, x, sx, BRA_PARAM_W_x, 1);
    rbc_qre_set_random_from_support(&z, y, sy, BRA_PARAM_W_y, 1);
    bra_public_key_from_string(g, h, s, pk);
    rbc_qre_mul(t, h, y); rbc_qre_add(t, t, x);           /* x + h*y */
    rbc_qre_to_string(a, t); rbc_qre_to_string(b, s);
    ok = memcmp(a, b, sizeof a) == 0;
    if (ok) {
        bra_kem_ciphertext_from_string(u, v, salt, ct);
        rbc_qre_mul(t, u, y); rbc_qre_add(t, v, t);       /* v - u*y */
        rbc_augmented_gabidulin_init(&code, g->v, BRA_PARAM_K, BRA_PARAM_N);
        rbc_augmented_gabidulin_decode(m, code, t->v);
        kdf(ss, pk, m, ct);
    }
    rbc_vspace_clear(sx); rbc_vspace_clear(sy); rbc_vspace_clear(sxy);
    rbc_qre_clear(x); rbc_qre_clear(y); rbc_qre_clear(g); rbc_qre_clear(h);
    rbc_qre_clear(s); rbc_qre_clear(t); rbc_qre_clear(u); rbc_qre_clear(v);
    rbc_vec_clear(m);
    rbc_qre_clear_modulus();
    return ok;
}

static uint8_t pk[PK], sk[SK], ct[CT], ss[SS], ssd[SS], ssa[SS];

/* The key owner's decapsulation, run in a child process: with a key pair from
 * the faulted key generation the decoder sees a high-rank error and may crash
 * (see kem-06-1). Returns 1 if it completed and agrees with ss, 0 if it
 * completed and disagrees, -1 if it crashed. */
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

/* encapsulate with an optional single fault, then attack */
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

    /* key generation: control, then the sk expander seeding fails */
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
               fault ? "keygen sk-seed alloc fails" : "keygen control", f, rc,
               od, ok, ok && !memcmp(ssa, ss, SS));
    }
    return 0;
}
