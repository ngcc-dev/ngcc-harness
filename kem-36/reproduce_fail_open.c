/* kem-36-10: TRIKE ignores the status of pseudohash, init_random_number and
 * its own calloc in generate_error_vector, so one failed allocation makes
 * kem_keygen / kem_enc / kem_dec return 0 with secrets computable from public
 * data.
 *
 * Linked with -Wl,--wrap=malloc,--wrap=calloc. When armed, the wrapper fails
 * exactly one allocation: the first malloc (or calloc) of a given size.
 *   enc ss-hash   : malloc |m|+|ct|+2 inside pseudohash for K = H(m || ct);
 *                   hash_length_m copies its zeroed temp, so K = 0.
 *   enc err-hash  : malloc |e|+2 inside pseudohash for H(e); c2 = m ^ 0 = m.
 *   enc errvec    : calloc |m|+|r2| in generate_error_vector (sample.c:232);
 *                   it returns early and e = 0, so H(e) is public.
 *   enc errvec-S  : malloc |m|+|r2| in init_random_number (sample.c:239);
 *                   e is drawn from the all-zero DRNG state.
 *   keygen        : malloc max(|m|, 55), the first init_random_number in key generation
 *                   (generate_secret_key, sample.c:184); h0, h1, h2 are drawn
 *                   from the all-zero DRNG state.
 *   dec ss-hash   : as enc ss-hash, inside kem_dec; K = 0 for any ciphertext.
 * The attacker functions use only pk and ct. Where the attacker must evaluate
 * the public sampler on the all-zero DRNG state it does so by running the same
 * public sampler with its own seeding allocation failed. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "auxfunc.h"
#include "KEM_AlgorithmInstance.h"
#include "trike_params.h"
#include "trike_types.h"
#include "sample.h"
#include "gf2x.h"

DRNG_ctx drng_algorithm;

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
static size_t fail_malloc, fail_calloc;
static int failed;
void *__wrap_malloc(size_t n)
{
    if (fail_malloc && n == fail_malloc) { fail_malloc = 0; failed++; return NULL; }
    return __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    if (fail_calloc && n * s == fail_calloc) { fail_calloc = 0; failed++; return NULL; }
    return __real_calloc(n, s);
}

#define PK sizeof(public_key_t)
#define SK sizeof(secret_key_t)
#define CT sizeof(ciphertext_t)
#define SS sizeof(shared_secret_t)
#define SS_HASH_ALLOC (M_SIZE_BYTES + sizeof(ciphertext_t) + 2)
#define E_HASH_ALLOC (N_ZMM_SIZE_BYTES + 2)
#define ERRVEC_ALLOC (M_SIZE_BYTES + R_SIZE_BYTES)
#define KEYGEN_SEED_ALLOC ((M_SIZE_BYTES) > 55 ? (M_SIZE_BYTES) : 55)   /* MAX(M_SIZE_BYTES, SEEDLEN = 55) */

static void hash_m(const uint8_t *in, size_t bits, uint8_t *out)
{
    uint8_t t[64] = {0};
    pseudohash(512, in, bits, t);
    memcpy(out, t, M_SIZE_BYTES);
}

static uint8_t *pad(void) { uint8_t *p = aligned_alloc(64, PADDED_R_SIZE_BYTES); memset(p, 0, PADDED_R_SIZE_BYTES); return p; }

/* u = e0 + e1*r1 + e2*r2 */
static int u_matches(const uint8_t *e, const uint8_t *r1, const uint8_t *r2, const uint8_t *u)
{
    uint8_t *e0 = pad(), *e1 = pad(), *e2 = pad(), *acc = pad(), *t = pad();
    trike_assign(e, e0, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
    trike_assign(e + R_ZMM_SIZE_BYTES, e1, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
    trike_assign(e + 2 * R_ZMM_SIZE_BYTES, e2, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
    memcpy(acc, e0, PADDED_R_SIZE_BYTES);
    gf2x_mul(t, e1, r1); gf2x_add(acc, acc, t);
    gf2x_mul(t, e2, r2); gf2x_add(acc, acc, t);
    int ok = memcmp(acc, u, R_SIZE_BYTES) == 0;
    free(e0); free(e1); free(e2); free(acc); free(t);
    return ok;
}

/* Attacker against a faulted encapsulation: try the three public error-vector
 * hypotheses, confirm on u, recover m and K = H(m || ct). Returns the
 * hypothesis that matched (1, 2, 3) or 0. */
static int attack_enc(uint8_t *ss, const uint8_t *pkb, const uint8_t *ctb)
{
    const public_key_t *pk = (const public_key_t *)pkb;
    const ciphertext_t *ct = (const ciphertext_t *)ctb;
    uint8_t *t1 = pad(), *t2 = pad(), *r1 = pad(), *r2 = pad();
    uint8_t *e = aligned_alloc(64, N_ZMM_SIZE_BYTES);
    uint8_t m[M_SIZE_BYTES], eh[M_SIZE_BYTES], hin[M_SIZE_BYTES + CT];
    int hyp = 0;

    generate_hash_vectors(t1, t2, r1, pk->sigma);
    memcpy(r2, pk->r2, R_SIZE_BYTES);
    for (int h = 1; h <= 3 && !hyp; h++) {
        memset(e, 0, N_ZMM_SIZE_BYTES);
        if (h == 1)       /* H(e) failed: c2 is m in the clear */
            generate_error_vector(e, e + R_ZMM_SIZE_BYTES, e + 2 * R_ZMM_SIZE_BYTES, ct->c2, r2);
        else if (h == 3) { /* sampler seeded from the all-zero DRNG state */
            fail_malloc = ERRVEC_ALLOC;
            generate_error_vector(e, e + R_ZMM_SIZE_BYTES, e + 2 * R_ZMM_SIZE_BYTES, ct->c2, r2);
            fail_malloc = 0;
        }                  /* h == 2: e = 0 */
        if (!u_matches(e, r1, r2, ct->u)) continue;
        if (h == 1) memcpy(m, ct->c2, M_SIZE_BYTES);
        else {
            hash_m(e, N_ZMM_SIZE_BYTES * 8, eh);
            for (int i = 0; i < M_SIZE_BYTES; i++) m[i] = ct->c2[i] ^ eh[i];
        }
        memcpy(hin, m, M_SIZE_BYTES);
        memcpy(hin + M_SIZE_BYTES, ct, CT);
        hash_m(hin, sizeof hin * 8, ss);
        hyp = h;
    }
    free(t1); free(t2); free(r1); free(r2); free(e);
    return hyp;
}

/* Attacker against a faulted key generation: rebuild the whole secret key
 * from the all-zero DRNG state and the public sigma; check against pk.r2. */
static int attack_key(secret_key_t *forged, const uint8_t *pkb)
{
    const public_key_t *pk = (const public_key_t *)pkb;
    uint8_t *h0 = pad(), *h1 = pad(), *h2 = pad(), *t0 = pad(), *t1 = pad(), *t2 = pad(), *r1 = pad(), *r2 = pad();
    uint8_t *a = pad(), *b = pad();
    uint8_t any[M_SIZE_BYTES] = {0};
    memset(forged, 0, sizeof *forged);
    fail_malloc = KEYGEN_SEED_ALLOC;
    generate_secret_key(h0, h1, h2, forged->h0_idx, forged->h1_idx, forged->h2_idx, any);
    fail_malloc = 0;
    generate_hash_vectors(t1, t2, r1, pk->sigma);
    gf2x_add(a, t1, r1); gf2x_inv(b, a); gf2x_mul(a, h0, r1); gf2x_add(a, a, h1); gf2x_mul(t0, a, b);  /* t0 */
    memset(a, 0, PADDED_R_SIZE_BYTES); memset(b, 0, PADDED_R_SIZE_BYTES);
    gf2x_add(a, t0, h0); gf2x_inv(b, a); gf2x_mul(a, t0, t2); gf2x_add(a, a, h2); gf2x_mul(r2, a, b);  /* r2 */
    int ok = memcmp(r2, pk->r2, R_SIZE_BYTES) == 0;
    memcpy(forged->h0, h0, R_SIZE_BYTES);
    memcpy(forged->t0, t0, R_SIZE_BYTES);
    memcpy(forged->r2, pk->r2, R_SIZE_BYTES);
    memcpy(forged->sigma, pk->sigma, M_SIZE_BYTES);    /* sigma2 left zero: unknown, unused on accept */
    free(h0); free(h1); free(h2); free(t0); free(t1); free(t2); free(r1); free(r2); free(a); free(b);
    return ok;
}

static uint8_t pk[PK], sk[SK], ct[CT], ss[SS], ssd[SS], ssa[SS], zero[SS];

static void enc_case(const char *name, size_t m_size, size_t c_size)
{
    unsigned long long a, b;
    memset(ss, 0xA5, SS);
    failed = 0; fail_malloc = m_size; fail_calloc = c_size;
    int rc = kem_enc(pk, PK, ss, &a, ct, &b);
    int f = failed; fail_malloc = fail_calloc = 0;
    kem_dec(sk, SK, ct, CT, ssd, &a);
    memset(ssa, 0xEE, SS);
    int hyp = attack_enc(ssa, pk, ct);
    printf("%s: faults=%d kem_enc=%d ss_is_zero=%d owner_dec_matches=%d attacker_hypothesis=%d attacker_ss_matches=%d\n",
           name, f, rc, !memcmp(ss, zero, SS), !memcmp(ss, ssd, SS), hyp, hyp && !memcmp(ssa, ss, SS));
}

int main(void)
{
    uint8_t seed[48];
    unsigned long long a, b;
    static secret_key_t forged;
    setvbuf(stdout, NULL, _IONBF, 0);
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(5 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);

    kem_keygen(pk, &a, sk, &b);
    enc_case("enc control", 0, 0);
    enc_case("enc ss-hash alloc fails", SS_HASH_ALLOC, 0);
    enc_case("enc err-hash alloc fails", E_HASH_ALLOC, 0);
    enc_case("enc errvec calloc fails", 0, ERRVEC_ALLOC);
    enc_case("enc errvec seed alloc fails", ERRVEC_ALLOC, 0);

    /* decapsulation: K = 0 for an arbitrary (garbage) ciphertext */
    for (int fault = 0; fault <= 1; fault++) {
        for (size_t i = 0; i < CT; i++) ct[i] = (uint8_t)(i * 131 + 7);
        memset(ssd, 0xA5, SS);
        failed = 0; fail_malloc = fault ? SS_HASH_ALLOC : 0;
        int rc = kem_dec(sk, SK, ct, CT, ssd, &a);
        int f = failed; fail_malloc = 0;
        printf("%s: faults=%d kem_dec=%d ss_is_zero=%d\n", fault ? "dec ss-hash alloc fails (garbage ct)" : "dec control (garbage ct)",
               f, rc, !memcmp(ssd, zero, SS));
    }

    /* key generation: control, then the secret-key seeding fails */
    for (int fault = 0; fault <= 1; fault++) {
        failed = 0; fail_malloc = fault ? KEYGEN_SEED_ALLOC : 0;
        int rc = kem_keygen(pk, &a, sk, &b);
        int f = failed; fail_malloc = 0;
        kem_enc(pk, PK, ss, &a, ct, &b);              /* honest sender */
        kem_dec(sk, SK, ct, CT, ssd, &a);             /* honest owner */
        int ok = attack_key(&forged, pk);
        memset(ssa, 0xEE, SS);
        kem_dec((unsigned char *)&forged, SK, ct, CT, ssa, &a);
        printf("%s: faults=%d kem_keygen=%d owner_dec_matches=%d attacker_key_matches_pk=%d attacker_ss_matches=%d\n",
               fault ? "keygen sk-seed alloc fails" : "keygen control", f, rc, !memcmp(ss, ssd, SS), ok, !memcmp(ssa, ss, SS));
    }
    return 0;
}
