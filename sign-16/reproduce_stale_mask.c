/* sign-16: when the allocation inside Octarine's rho'' hash fails, signing
 * still returns a valid signature, made with the previous signature's rho''.
 *
 * RRLWR_SIGN_HASH_RHOPP (hash_domain.h) is void and discards pseudoXOF's
 * status; sig_sign then expands every mask y from the unwritten rhopp_kappa
 * buffer, which still holds the rho'' of the previous signing call in the same
 * stack frame. Two signatures accepted at the same rejection round then share
 * y, so z2 - z1 = (c2 - c1) * s1 exactly.
 *
 * Linked with -Wl,--wrap=malloc. For each round the driver makes one ordinary
 * signature and then, with no other call in between, one with exactly one
 * allocation failing: pseudoXOF's copy of the encoded (K, rnd, mu) input plus
 * a 4-byte counter (fixed size, independent of the message). From the two
 * public signatures it solves (c2 - c1) * x = z2 - z1 in Z_p[X]/(X^N+1) for a
 * small NTT prime p and accepts x if every coefficient lies in the secret range
 * [-eta, eta-1]. With the first such x it rebuilds a signing key from pk alone
 * (as sig_keygen does from s1) and signs a fresh message, checked by the
 * unmodified verifier. The real secret key is read only to report agreement.
 * Control: the same recovery on two consecutive ordinary signatures. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sign.h"
#define RRLWR_HASH_DOMAIN_SIGN
#include "hash_domain.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
static size_t fail_size;

void *__wrap_malloc(size_t n)
{
    if (fail_size && n == fail_size) {
        fail_size = 0;
        return NULL;
    }
    return __real_malloc(n);
}

#define N RRLWR_N
#define K RRLWR_K
#define ROUNDS 8
#define MLEN 32
#define RHOPP_ALLOC (RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_RHOPP, \
                     RRLWR_SIGN_K_LEN + RRLWR_SIGN_RND_LEN + RRLWR_SIGN_MU_LEN, 3) + 4)
#define S1_OFF (RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN + RRLWR_SIGN_TR_LEN)

static unsigned char pk[RRLWR_SIGN_PK_LEN], sk[RRLWR_SIGN_SK_LEN], sk2[RRLWR_SIGN_SK_LEN];
static unsigned long long pkl, skl;

/* All signing goes through one non-inlined call, so sig_sign's frame sits at
 * the same stack address each time. hit reports whether the allocation was reached. */
__attribute__((noinline)) static int sign(unsigned char *s, unsigned char *key, unsigned char *m, size_t n, int *hit)
{
    unsigned long long sl;
    int r;
    fail_size = n;
    r = sig_sign(key, RRLWR_SIGN_SK_LEN, m, MLEN, s, &sl);
    *hit = n && !fail_size;
    fail_size = 0;
    return r;
}

static uint32_t pw(uint64_t a, uint64_t e, uint32_t p)
{
    uint64_t r = 1;
    a %= p;
    while (e) { if (e & 1) r = r * a % p; a = a * a % p; e >>= 1; }
    return (uint32_t)r;
}

/* Solve dc * x_k = dz_k in Z_p[X]/(X^N+1) by evaluation at the odd powers of a
 * primitive 2N-th root of unity; returns 0 and centred x on success. */
static int ring_divide(int32_t x[K][N], const int32_t dc[N], int32_t dz[K][N])
{
    static const uint32_t primes[] = {12289, 40961, 61441, 65537};
    static uint32_t root[2 * N], D[N], Z[N];
    for (unsigned q = 0; q < sizeof primes / sizeof primes[0]; q++) {
        uint32_t p = primes[q], psi = 0;
        for (uint32_t a = 2; a < p && !psi; a++) {
            uint32_t c = pw(a, (p - 1) / (2 * N), p);
            if (pw(c, N, p) == p - 1) psi = c;
        }
        root[0] = 1;
        for (int e = 1; e < 2 * N; e++) root[e] = (uint32_t)((uint64_t)root[e - 1] * psi % p);
        int ok = 1;
        for (int i = 0; i < N && ok; i++) {
            uint64_t acc = 0;
            for (int j = 0; j < N; j++)
                acc = (acc + (uint64_t)(((dc[j] % (int32_t)p) + p) % p) * root[(size_t)(2 * i + 1) * j % (2 * N)]) % p;
            D[i] = (uint32_t)acc;
            ok = acc != 0;
        }
        if (!ok) continue;
        uint32_t ninv = pw(N, p - 2, p);
        for (int k = 0; k < K; k++) {
            for (int i = 0; i < N; i++) {
                uint64_t acc = 0;
                for (int j = 0; j < N; j++)
                    acc = (acc + (uint64_t)(((dz[k][j] % (int32_t)p) + p) % p) * root[(size_t)(2 * i + 1) * j % (2 * N)]) % p;
                Z[i] = (uint32_t)(acc * pw(D[i], p - 2, p) % p);
            }
            for (int j = 0; j < N; j++) {
                uint64_t acc = 0;
                for (int i = 0; i < N; i++)
                    acc = (acc + (uint64_t)Z[i] * root[(2 * N - (size_t)(2 * i + 1) * j % (2 * N)) % (2 * N)]) % p;
                acc = acc * ninv % p;
                x[k][j] = (int32_t)(acc > p / 2 ? (int64_t)acc - p : (int64_t)acc);
            }
        }
        return 0;
    }
    return -1;
}

/* From two public signatures: x = (z2 - z1) / (c2 - c1); 1 if x is short. */
static int recover(int32_t x[K][N], unsigned char *a, unsigned char *b)
{
    static poly ca, cb;
    static ring_element za, zb;
    static int32_t dc[N], dz[K][N];
    if (poly_sampleInBall(&ca, a) || poly_sampleInBall(&cb, b)) return 0;
    ring_unpack(&za, a + RRLWR_SIGN_CTILDE_LEN, RRLWR_SIGN_LOG_GAMMA1 + 1);
    ring_unpack(&zb, b + RRLWR_SIGN_CTILDE_LEN, RRLWR_SIGN_LOG_GAMMA1 + 1);
    for (int j = 0; j < N; j++) dc[j] = cb.coeffs[j] - ca.coeffs[j];
    for (int k = 0; k < K; k++)
        for (int j = 0; j < N; j++) dz[k][j] = zb.x[k].coeffs[j] - za.x[k].coeffs[j];
    if (ring_divide(x, dc, dz) != 0) return 0;
    for (int k = 0; k < K; k++)
        for (int j = 0; j < N; j++)
            if (x[k][j] < -RRLWR_SIGN_ETA || x[k][j] > RRLWR_SIGN_ETA - 1) return 0;
    return 1;
}

/* Rebuild a signing key from pk and s1, as sig_keygen does (K left zero). */
static void rebuild_key(unsigned char *out, int32_t x[K][N])
{
    static ring_element s1, a, t, b, s2;
    static unsigned char pk2[RRLWR_SIGN_PK_LEN];
    for (int k = 0; k < K; k++)
        for (int j = 0; j < N; j++) s1.x[k].coeffs[j] = x[k][j];
    memset(out, 0, RRLWR_SIGN_SK_LEN);
    memcpy(out, pk, RRLWR_SIGN_RHO_LEN);
    ring_uniform_public_x4(&a, RRLWR_SIGN_LOGQ, pk, RRLWR_SIGN_RHO_LEN);
    ring_mul32x2(&t, &a, &s1);
    ring_pack(out + S1_OFF, &s1, RRLWR_SIGN_LOG_ETA + 1);
    ring_round_xtoy(&b, &t, RRLWR_SIGN_LOGQ, RRLWR_SIGN_LOGP);
    ring_shift_and_sub(&s2, &t, &b, RRLWR_SIGN_LOGQ - RRLWR_SIGN_LOGP);
    ring_pack(out + S1_OFF + RRLWR_SIGN_S_LEN, &s2, RRLWR_SIGN_LOG_ETA + 1);
    ring_power2round_and_pack(out + S1_OFF + 2 * RRLWR_SIGN_S_LEN, pk2 + RRLWR_SIGN_RHO_LEN, &b);
    RRLWR_SIGN_HASH_TR(out + RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_K_LEN, pk);
}

int main(void)
{
    static unsigned char ordinary[ROUNDS][RRLWR_SIGN_SIG_LEN], faulty[ROUNDS][RRLWR_SIGN_SIG_LEN];
    static unsigned char fresh[RRLWR_SIGN_SIG_LEN];
    static int32_t x[K][N];
    static ring_element s1;
    unsigned char seed[48], m[MLEN];
    int hit, reached = 0, valid = 0, recovered = 0, equal = -1, forged = -9, other = -9, control = 0;

    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(5 * i + 3);
    init_random_number(&drng_algorithm, seed, 48);
    if (sig_keygen(pk, &pkl, sk, &skl) != 0) { printf("keygen failed\n"); return 1; }
    ring_unpack(&s1, sk + S1_OFF, RRLWR_SIGN_LOG_ETA + 1);   /* evaluation only */

    for (int t = 0; t < ROUNDS; t++) {
        memset(m, 0x80 + t, MLEN);                           /* ordinary signature, then at once */
        if (sign(ordinary[t], sk, m, 0, &hit) != 0) { printf("ordinary signing failed\n"); return 1; }
        memset(m, t, MLEN);                                  /* one whose rho'' allocation fails */
        int r = sign(faulty[t], sk, m, RHOPP_ALLOC, &hit);
        reached += hit;
        valid += r == 0 && sig_verify(pk, pkl, faulty[t], RRLWR_SIGN_SIG_LEN, m, MLEN) == 0;
    }
    for (int t = 0; t < ROUNDS; t++) {
        if (!recover(x, ordinary[t], faulty[t])) continue;
        if (recovered++) continue;
        equal = 1;
        for (int k = 0; k < K; k++)
            for (int j = 0; j < N; j++) equal &= x[k][j] == s1.x[k].coeffs[j];
        rebuild_key(sk2, x);
        unsigned char fm[MLEN];
        memset(fm, 0x3c, MLEN);
        if (sign(fresh, sk2, fm, 0, &hit) != 0) { forged = -2; continue; }
        forged = sig_verify(pk, pkl, fresh, RRLWR_SIGN_SIG_LEN, fm, MLEN);
        fm[0] ^= 1;
        other = sig_verify(pk, pkl, fresh, RRLWR_SIGN_SIG_LEN, fm, MLEN);
    }
    for (int t = 0; t + 1 < ROUNDS; t++) control += recover(x, ordinary[t], ordinary[t + 1]);
    printf("allocation=%d reached=%d/%d faulty_valid=%d short_s1_recovered=%d s1_equal=%d "
           "forgery_verify=%d changed_message_verify=%d ordinary_pairs_recovered=%d\n",
           (int)RHOPP_ALLOC, reached, ROUNDS, valid, recovered, equal, forged, other, control);
    return 0;
}
