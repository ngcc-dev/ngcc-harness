/* sign-22-8 driver: Rhyme challenge space and generic challenge-guessing forgery.
 *
 * Compiled inside one submitted instance directory.  It #includes the
 * unmodified src/sign.c so that the forger can reuse the public commitment
 * computation (compute_w, add_jterm, expand_A) and the verdict comes from the
 * unmodified crypto_sign_verify.
 *
 *   alphabet T   sample T challenges from distinct c_tilde values; print N,
 *                TAU, the coefficient alphabet and log2 binomial(N,TAU)
 *   forge T      deterministic key pair; honest sign/verify control; then a
 *                forger that never touches the signing key fixes a short z
 *                and a target challenge c*, derives w = A z - q j c* mod 2q
 *                from the public key, and grinds up to T messages until
 *                SampleChallenge(H(w, H(pk, m))) == c*.  The resulting
 *                (c_tilde, z) is checked with crypto_sign_verify.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "src/sign.c"

DRNG_ctx drng_algorithm;

static long double log2_binom(int n, int k) {
    long double s = 0.0L;
    for (int i = 0; i < k; i++)
        s += log2l((long double)(n - i) / (long double)(i + 1));
    return s;
}

static void hash2(uint8_t *out, size_t outlen, const uint8_t *a, size_t alen,
                  const uint8_t *b, size_t blen) {
    keccak_state ks;
    rhyme_shake256_init(&ks);
    rhyme_shake256_absorb(&ks, a, alen);
    rhyme_shake256_absorb(&ks, b, blen);
    rhyme_shake256_finalize(&ks);
    rhyme_shake256_squeeze(out, outlen, &ks);
#ifdef RHYME_SM3_XOF_H
    sm3_xof_clear(&ks);
#endif
}

/* returns weight, or -1 if a coefficient is outside {0,1} */
static int binary_weight(const poly *c) {
    int w = 0;
    for (int i = 0; i < N; i++) {
        if (c->coeffs[i] == 1) w++;
        else if (c->coeffs[i] != 0) return -1;
    }
    return w;
}

static int alphabet(unsigned long trials) {
    uint8_t ct[CTILDEBYTES];
    poly c;
    unsigned long bad = 0, wrong_weight = 0;
    for (unsigned long t = 0; t < trials; t++) {
        for (int i = 0; i < CTILDEBYTES; i++) ct[i] = (uint8_t)(0xA5 ^ i);
        for (int i = 0; i < 8; i++) ct[i] = (uint8_t)(t >> (8 * i));
        SampleChallenge(&c, ct);
        int w = binary_weight(&c);
        if (w < 0) bad++;
        else if (w != TAU) wrong_weight++;
    }
    printf("mode=%d N=%d TAU=%d samples=%lu non_binary=%lu wrong_weight=%lu log2C=%.2Lf\n",
           RHYME_MODE, N, TAU, trials, bad, wrong_weight, log2_binom(N, TAU));
    return (bad || wrong_weight) ? 1 : 0;
}

static uint32_t lcg;
static int32_t small(int32_t r) {   /* uniform-ish in [-r, r] */
    lcg = lcg * 1103515245u + 12345u;
    return (int32_t)((lcg >> 8) % (uint32_t)(2 * r + 1)) - r;
}

static int forge(unsigned long trials) {
    uint8_t seed[48], pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    uint8_t sig[CRYPTO_BYTES], ct[CTILDEBYTES], mu[CRHBYTES];
    size_t siglen = 0;
    poly z1, zr[D_REST], c, cstar;

    for (size_t i = 0; i < sizeof seed; i++) seed[i] = (uint8_t)(7 * i + 1);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    if (crypto_sign_keypair(pk, sk) != 0) return 2;

    /* honest control: sign, verify, and decode the honest challenge */
    static const uint8_t hm[] = "honest control message";
    if (crypto_sign_signature(sig, &siglen, hm, sizeof hm, sk) != 0) return 2;
    int honest = crypto_sign_verify(sig, siglen, hm, sizeof hm, pk);
    if (unpack_sig(ct, &z1, zr, sig, siglen) != 0) return 2;
    SampleChallenge(&c, ct);
    int honest_weight = binary_weight(&c);

    /* the forger has only pk */
    memset(sk, 0, sizeof sk);

    lcg = 0x5eed;
    for (int i = 0; i < N; i++) z1.coeffs[i] = small(100);
    for (int r = 0; r < D_REST; r++)
        for (int i = 0; i < N; i++) zr[r].coeffs[i] = small(100);
    for (int i = 0; i < CTILDEBYTES; i++) ct[i] = (uint8_t)(0x3c + i);
    SampleChallenge(&cstar, ct);                    /* target challenge c* */

    uint8_t seedA[SEEDBYTES], wbuf[W_PACKEDBYTES];
    poly Agen[K][M_SGEN], b[K], w[K];
    unpack_pk(seedA, b, pk);
    expand_A(Agen, seedA);
    for (int i = 0; i < K; i++) poly_ntt(&b[i]);
    compute_w(w, (const poly (*)[M_SGEN])Agen, (const poly *)b, &z1, zr, 1);
    add_jterm(w, &z1, &cstar);                      /* w = A z - q j c* mod 2q */
    pack_w(wbuf, w);

    uint8_t msg[32];
    memcpy(msg, "Rhyme forged message, trial ....", 32);
    unsigned long t;
    int hit = 0;
    for (t = 1; t <= trials; t++) {
        for (int i = 0; i < 8; i++) msg[24 + i] = (uint8_t)(t >> (8 * i));
        hash2(mu, CRHBYTES, pk, CRYPTO_PUBLICKEYBYTES, msg, sizeof msg);
        hash2(ct, CTILDEBYTES, wbuf, W_PACKEDBYTES, mu, CRHBYTES);
        SampleChallenge(&c, ct);
        if (memcmp(c.coeffs, cstar.coeffs, sizeof c.coeffs) == 0) { hit = 1; break; }
    }
    long double lc = log2_binom(N, TAU);
    if (!hit) {
        printf("mode=%d TAU=%d log2C=%.2Lf honest_verify=%d honest_weight=%d trials=%lu forged=none\n",
               RHYME_MODE, TAU, lc, honest, honest_weight, trials);
        return 3;
    }
    if (pack_sig(sig, &siglen, ct, &z1, zr) != 0) return 2;
    int forged = crypto_sign_verify(sig, siglen, msg, sizeof msg, pk);
    uint8_t other[32];
    memcpy(other, msg, 32);
    other[31] ^= 0x80;
    int other_rc = crypto_sign_verify(sig, siglen, other, sizeof other, pk);
    printf("mode=%d TAU=%d log2C=%.2Lf honest_verify=%d honest_weight=%d trials=%lu log2trials=%.2f forged_verify=%d other_message_verify=%d\n",
           RHYME_MODE, TAU, lc, honest, honest_weight, t, log2((double)t), forged,
           other_rc == 0 ? 0 : -1);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    unsigned long n = strtoul(argv[2], NULL, 10);
    if (strcmp(argv[1], "alphabet") == 0) return alphabet(n);
    if (strcmp(argv[1], "forge") == 0) return forge(n);
    return 2;
}
