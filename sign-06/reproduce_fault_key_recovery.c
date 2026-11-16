/* sign-06-8: COMPASS-SIG signing ignores pseudoXOF failures; one faulted
 * deterministic signature plus the honest signature on the same message
 * reveal s1, and the recovered key forges.
 *
 * Linked with -Wl,--wrap=malloc. When armed, the wrapper fails exactly one
 * allocation: the first malloc(SEEDBYTES + 2 + 4) of a signing call, which is
 * pseudoXOF's message copy for the first polynomial of the matrix A. The
 * stream buffer then keeps uninitialised heap bytes, so the signer uses a
 * wrong A, a wrong w1 and a wrong challenge c', but the same deterministic
 * mask y, and returns 0 with (c~', z' = y + c' s1). The attacker computes
 * c = SampleInBall(c~) and c' = SampleInBall(c~') from the two signatures and
 * solves (c' - c) s1 = z' - z modulo q. A solution with all coefficients in
 * [-ETA_S, ETA_S] is accepted; it is then compared with the real s1, and a
 * secret key rebuilt from pk and the recovered s1 signs a fresh message.
 * Control: without the fault the re-signature equals the honest one.
 *
 * Mode "keygen": the wrapper fails pseudoXOF's message copy (SEEDBYTES + 4
 * bytes) in the first shake256() of key generation, which expands the DRBG
 * seed into rho || rho' || key. shake256() then copies 128 bytes of an
 * unwritten heap buffer into seedbuf and keygen returns 0 with a working key
 * pair whose s1, s2 come from those stale bytes. The attacker rebuilds the
 * whole secret key from public data: rho' || key = zero bytes in a fresh
 * process, or bytes 32..127 of the 128-byte XOF of the previous public key
 * when one key was generated earlier in the process (that buffer is the last
 * freed chunk of its size). Controls: two fresh processes with different DRBG
 * seeds give the same faulted public key; the earlier honest key does not
 * match the zero-byte rebuild. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "sign.h"
#include "packing.h"
#include "poly.h"
#include "polyvec.h"
#include "symmetric.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
static size_t fail_size;
void *__wrap_malloc(size_t n)
{
    if (fail_size && n == fail_size) { fail_size = 0; return NULL; }
    return __real_malloc(n);
}

static int64_t md(int64_t a) { a %= Q; return a < 0 ? a + Q : a; }
static int32_t ctr(int64_t a) { a = md(a); return (int32_t)(a > Q / 2 ? a - Q : a); }
static int64_t pw(int64_t b, int64_t e) { int64_t r = 1; b = md(b); while (e) { if (e & 1) r = r * b % Q; b = b * b % Q; e >>= 1; } return r; }

/* solve a * x = d in Z_Q[X]/(X^N + 1); returns 0 if a is not invertible */
static int ring_div(int32_t *x, const int32_t *a, const int32_t *d)
{
    static int64_t M[N][N + 1];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {          /* coefficient of X^i in a * X^j */
            int k = i - j;
            M[i][j] = k >= 0 ? md(a[k]) : md(-(int64_t)a[k + N]);
        }
        M[i][N] = md(d[i]);
    }
    for (int c = 0; c < N; c++) {
        int p = c;
        while (p < N && M[p][c] == 0) p++;
        if (p == N) return 0;
        if (p != c) for (int j = 0; j <= N; j++) { int64_t t = M[c][j]; M[c][j] = M[p][j]; M[p][j] = t; }
        int64_t inv = pw(M[c][c], Q - 2);
        for (int j = c; j <= N; j++) M[c][j] = M[c][j] * inv % Q;
        for (int r = 0; r < N; r++) if (r != c && M[r][c]) {
            int64_t f = M[r][c];
            for (int j = c; j <= N; j++) M[r][j] = md(M[r][j] - f * M[c][j]);
        }
    }
    for (int i = 0; i < N; i++) x[i] = ctr(M[i][N]);
    return 1;
}

/* attacker: the key pair crypto_sign_keypair derives from seedbuf = rho || rhop || key */
static void rebuild(uint8_t *pk2, uint8_t *sk2, const uint8_t rho[SEEDBYTES],
                    const uint8_t rhop[CRHBYTES], const uint8_t key[SEEDBYTES])
{
    polyvecl mat[K], s1, s1h; polyveck s2, t, t1, t0; uint8_t tr[TRBYTES];
    polyvec_matrix_expand(mat, rho);
    polyvecl_uniform_eta_s(&s1, rhop, 0);
    polyveck_uniform_eta_e(&s2, rhop, L);
    s1h = s1; polyvecl_ntt(&s1h);
    polyvec_matrix_pointwise_montgomery(&t, mat, &s1h);
    polyveck_reduce(&t); polyveck_invntt_tomont(&t);
    polyveck_add(&t, &t, &s2); polyveck_caddq(&t);
    polyveck_power2round(&t1, &t0, &t);
    pack_pk(pk2, rho, &t1);
    shake256(tr, TRBYTES, pk2, CRYPTO_PUBLICKEYBYTES);
    pack_sk(sk2, rho, tr, key, &s1, &s2, &t0);
}

static int keygen_mode(int prior, int variant)
{
    static uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], pk0[CRYPTO_PUBLICKEYBYTES], sk0[CRYPTO_SECRETKEYBYTES];
    static uint8_t pa[CRYPTO_PUBLICKEYBYTES], sa[CRYPTO_SECRETKEYBYTES], sg[CRYPTO_BYTES];
    uint8_t fresh[32] = "a message the owner never signed", stale[2 * SEEDBYTES + CRHBYTES] = {0};
    uint8_t seed[48];
    size_t len;
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(7 * i + 1 + 50 * variant);
    init_random_number(&drng_algorithm, seed, 48);
    if (prior) {
        crypto_sign_keypair(pk0, sk0);                           /* an earlier, honest key */
        shake256(stale, sizeof stale, pk0, CRYPTO_PUBLICKEYBYTES);
    }
    fail_size = SEEDBYTES + 4;                                   /* pseudoXOF copy of the seed */
    int rc = crypto_sign_keypair(pk, sk);
    fail_size = 0;
    rebuild(pa, sa, pk, stale + SEEDBYTES, stale + SEEDBYTES + CRHBYTES);
    int pk_match = !memcmp(pa, pk, sizeof pk), sk_match = !memcmp(sa, sk, sizeof sk);
    crypto_sign_signature(sg, &len, fresh, 32, NULL, 0, sa);
    int forged = crypto_sign_verify(sg, CRYPTO_BYTES, fresh, 32, NULL, 0, pk);
    uint32_t h = 2166136261u;                                    /* FNV-1a of pk, compared across seeds */
    for (size_t i = 0; i < sizeof pk; i++) { h ^= pk[i]; h *= 16777619u; }
    int control = 1;
    if (prior) {   /* control: the earlier honest key is not reproduced the same way */
        uint8_t z[2 * SEEDBYTES + CRHBYTES] = {0};
        rebuild(pa, sa, pk0, z + SEEDBYTES, z + SEEDBYTES + CRHBYTES);
        control = memcmp(sa, sk0, sizeof sk0) != 0;
        printf("faulted_keygen_rc=%d honest_key_not_rebuilt=%d rebuilt_pk_match=%d rebuilt_sk_match=%d forged_verify=%d\n",
               rc, control, pk_match, sk_match, forged);
    } else
        printf("faulted_keygen_rc=%d rebuilt_pk_match=%d rebuilt_sk_match=%d forged_verify=%d pk_fnv=%08x\n",
               rc, pk_match, sk_match, forged, h);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2 && !strcmp(argv[1], "keygen")) return keygen_mode(0, atoi(argv[2]));
    if (argc > 1 && !strcmp(argv[1], "keygen-after-key")) return keygen_mode(1, 0);
    static uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], sk2[CRYPTO_SECRETKEYBYTES];
    static uint8_t sh[CRYPTO_BYTES], sf[CRYPTO_BYTES], sc[CRYPTO_BYTES], sg[CRYPTO_BYTES];
    uint8_t seed[48], m[32], fresh[32] = "a message the owner never signed";
    size_t len;
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(7 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
    crypto_sign_keypair(pk, sk);

    uint8_t rho[SEEDBYTES], tr[TRBYTES], key[SEEDBYTES];
    polyvecl s1, rec; polyveck s2, t0;
    unpack_sk(rho, tr, key, &s1, &s2, &t0, sk);

    int tries, found = 0, rcf = -9, vf = -9, control_same = 0;
    for (tries = 1; tries <= 32 && !found; tries++) {
        for (int i = 0; i < 32; i++) m[i] = (uint8_t)(i + 32 * tries);
        crypto_sign_signature(sh, &len, m, 32, NULL, 0, sk);
        crypto_sign_signature(sc, &len, m, 32, NULL, 0, sk);          /* control: no fault */
        control_same = !memcmp(sh, sc, CRYPTO_BYTES);
        fail_size = SEEDBYTES + 2 + 4;                                 /* pseudoXOF copy of rho||nonce */
        rcf = crypto_sign_signature(sf, &len, m, 32, NULL, 0, sk);
        fail_size = 0;
        vf = crypto_sign_verify(sf, CRYPTO_BYTES, m, 32, NULL, 0, pk);

        /* attacker: only public data and the two released signatures */
        uint8_t ch[CTILDEBYTES], cf[CTILDEBYTES];
        polyvecl zh, zf; poly cph, cpf; int32_t dc[N];
        unpack_sig(ch, &zh, sh); unpack_sig(cf, &zf, sf);
        poly_challenge(&cph, ch); poly_challenge(&cpf, cf);
        for (int i = 0; i < N; i++) dc[i] = cpf.coeffs[i] - cph.coeffs[i];
        int small = 1;
        for (int l = 0; l < L && small; l++) {
            int32_t d[N];
            for (int i = 0; i < N; i++) d[i] = ctr((int64_t)zf.vec[l].coeffs[i] - zh.vec[l].coeffs[i]);
            if (!ring_div(rec.vec[l].coeffs, dc, d)) { small = 0; break; }
            for (int i = 0; i < N; i++) if (abs(rec.vec[l].coeffs[i]) > ETA_S) small = 0;
        }
        found = small;
    }
    tries--;
    int match = found;
    for (int l = 0; l < L && match; l++)
        for (int i = 0; i < N; i++) if (rec.vec[l].coeffs[i] != s1.vec[l].coeffs[i]) match = 0;

    /* rebuild a signing key from pk and the recovered s1: e = t1*2^D - A s1 = s2 - t0 */
    int forged = -9;
    if (found) {
        polyvecl mat[K], s1h = rec; polyveck as, t1, s2p, t0p; uint8_t rhop[SEEDBYTES], trp[TRBYTES], kp[SEEDBYTES] = {0};
        unpack_pk(rhop, &t1, pk);
        polyvec_matrix_expand(mat, rhop);
        polyvecl_ntt(&s1h);
        polyvec_matrix_pointwise_montgomery(&as, mat, &s1h);
        polyveck_reduce(&as); polyveck_invntt_tomont(&as);
        for (int k = 0; k < K; k++) for (int i = 0; i < N; i++) {
            int32_t e = ctr(((int64_t)t1.vec[k].coeffs[i] << D) - as.vec[k].coeffs[i]);
            int32_t s = 0, lo = -((1 << (D - 1)) - 1), hi = 1 << (D - 1);
            if (-e < lo) s = 1; else if (-e > hi) s = -1;
            s2p.vec[k].coeffs[i] = s; t0p.vec[k].coeffs[i] = s - e;   /* s2' - t0' = e */
        }
        shake256(trp, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);
        pack_sk(sk2, rhop, trp, kp, &rec, &s2p, &t0p);
        crypto_sign_signature(sg, &len, fresh, 32, NULL, 0, sk2);
        forged = crypto_sign_verify(sg, CRYPTO_BYTES, fresh, 32, NULL, 0, pk);
    }
    printf("control_resign_identical=%d faulted_sign_rc=%d faulted_verify=%d messages_tried=%d "
           "s1_small_solution=%d s1_matches_secret=%d forged_verify=%d\n",
           control_same, rcf, vf, tries, found, match, forged);
    return 0;
}
