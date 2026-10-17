/* sign-23-3 Shuttle-128: lambda-bit masking seeds expose the signing key.
 *
 * Built by sign-23/reproduce_seed_collision.sh against a temporary copy of the
 * submitted SHUTTLE-128 reference tree. sign.c is #included for its static
 * helpers (encode_com_vec, build_cached_matrix). With -DNGCC_SEED_BITS=w the
 * copy keeps only w bits of the 128-bit seed_y (reduced-width model).
 *
 *   collide <n> [drbg]  n ordinary sig_sign signatures on distinct messages.
 *       ATTACK (pk, messages, signatures only): sort the verifier commitments
 *       EncodeCom(w_h, w0'); on a match d0 = z0 - z0', d_i = z_i - z_i' satisfy
 *       d_i = s_i * d0 in Z[X]/(X^n+1); solve s_i, complete e' from pk, sign a
 *       fresh message with the equivalent key, verify with the unmodified
 *       verifier.
 *   mt <n>  Multi-target route. T(pk, seed) = EncodeCom computed from pk and a
 *       candidate seed alone is compared with the verifier's EncodeCom of all n
 *       signatures at once. Full width: the true seed of each signature is
 *       regenerated with K (stand-in for the 2^lambda/n enumeration) and must
 *       hit; a one-bit-different seed must miss. Reduced width: the seed space
 *       is actually enumerated until the first table hit, then s1 is solved
 *       from that one signature, completed publicly and used to forge.
 * The oracle secret key is read only after recovery, to report agreement.
 */
#include "sign.c"
#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include <stdio.h>
#include <stdlib.h>

DRNG_ctx drng_algorithm;

#define MSGLEN 32

typedef struct { uint64_t h; unsigned idx; } entry;

static int cmp_entry(const void *a, const void *b)
{
    const entry *x = a, *y = b;
    if (x->h != y->h) return x->h < y->h ? -1 : 1;
    return (int)x->idx - (int)y->idx;
}

static uint64_t fnv(const uint8_t *p, size_t n)
{
    uint64_t h = 1469598103934665603ULL;
    while (n--) { h ^= *p++; h *= 1099511628211ULL; }
    return h;
}

/* Public: the verifier's commitment encoding for (pk, sig). */
static int public_commitment(uint8_t *enc, const uint8_t *pk, const uint8_t *sig)
{
    uint8_t seedA[SEEDBYTES], seedC[CHALLENGESEEDBYTES];
    poly b[EM], z1[Z1LEN], hint[EM], c, comY0p[EM], comY_tilde[EM], comY_h[EM], z2p[EM];
    static poly16 agen[EM], hAgen[EM * ELL], bhat[EM], Ahat[EM * ELL];
    if (unpack_pk(seedA, b, pk) != 0) return -1;
    if (unpack_sig(seedC, z1, hint, sig) != 0) return -1;
    sample_c(&c, seedC);
    expand_a(agen, hAgen, seedA);
    build_cached_matrix(bhat, Ahat, b, agen, hAgen);
    recon_comY0p(&comY0p[0], &z1[0], &c);
    for (int j = 1; j < EM; ++j) memset(&comY0p[j], 0, sizeof comY0p[j]);
    mat_mul_z1_2q(comY_tilde, z1, &c, bhat, Ahat);
    use_hint(comY_h, z2p, hint, comY_tilde, &comY0p[0]);
    encode_com_vec(enc, comY_h, comY0p);
    return 0;
}

/* Public: EncodeCom(w_h, w0) from pk (cached matrix) and a candidate seed. */
static poly16 T_bhat[EM], T_Ahat[EM * ELL];
static void attacker_T_init(const uint8_t *pk)
{
    uint8_t seedA[SEEDBYTES];
    poly b[EM];
    static poly16 agen[EM], hAgen[EM * ELL];
    unpack_pk(seedA, b, pk);
    expand_a(agen, hAgen, seedA);
    build_cached_matrix(T_bhat, T_Ahat, b, agen, hAgen);
}
static void attacker_T(uint8_t out[ENCODECOM_VEC_BYTES], const uint8_t seedY[SEEDBYTES], poly yp_out[KVEC])
{
    poly y[KVEC], yp[KVEC], comY[EM], comY_h[EM], comY_0[EM];
    sample_y(y, seedY);
    compress_y(yp, y);
    mat_mul_2q(comY, yp, T_bhat, T_Ahat);
    for (int j = 0; j < EM; ++j)
        for (int k = 0; k < N; ++k) {
            comY_0[j].coeffs[k] = lsb_coeff(comY[j].coeffs[k]);
            comY_h[j].coeffs[k] = highbits_reduced(comY[j].coeffs[k]);
        }
    encode_com_vec(out, comY_h, comY_0);
    if (yp_out) memcpy(yp_out, yp, sizeof(yp));
}

/* ---- negacyclic arithmetic mod Q by evaluation at odd psi powers ---- */
static int64_t pw(int64_t a, int64_t e)
{
    int64_t r = 1; a %= Q; if (a < 0) a += Q;
    while (e) { if (e & 1) r = r * a % Q; a = a * a % Q; e >>= 1; }
    return r;
}
static int64_t psi_pow[2 * N];
static void init_psi(void)
{
    int64_t psi = 0;
    for (int64_t g = 2; g < Q; ++g) {
        int64_t cand = pw(g, (Q - 1) / (2 * N));
        if (pw(cand, N) == Q - 1) { psi = cand; break; }
    }
    for (int i = 0; i < 2 * N; ++i) psi_pow[i] = pw(psi, i);
}
static void eval(int64_t out[N], const int32_t *a)
{
    for (int k = 0; k < N; ++k) {
        int64_t acc = 0;
        for (int j = 0; j < N; ++j) {
            int64_t v = a[j] % Q; if (v < 0) v += Q;
            acc = (acc + v * psi_pow[((2 * k + 1) * j) % (2 * N)]) % Q;
        }
        out[k] = acc;
    }
}
static void interp(int32_t *a, const int64_t in[N])
{
    int64_t ninv = pw(N, Q - 2);
    for (int j = 0; j < N; ++j) {
        int64_t acc = 0;
        for (int k = 0; k < N; ++k) {
            int e = (int)(((int64_t)(2 * k + 1) * j) % (2 * N));
            acc = (acc + in[k] * psi_pow[(2 * N - e) % (2 * N)]) % Q;
        }
        acc = acc * ninv % Q;
        if (acc > Q / 2) acc -= Q;
        a[j] = (int32_t)acc;
    }
}
/* s_i = d_i / d0 for i = 0..ELL-1, where d_i = A[1+i] - B[1+i], d0 = A[0] - B[0]. */
static int divide_out(poly s[ELL], const poly *A, const poly *B, int *nz, int *mx)
{
    int32_t d[N];
    int64_t D0[N], Di[N], S[N];
    *nz = 0; *mx = 0;
    for (int j = 0; j < N; ++j) {
        d[j] = A[0].coeffs[j] - B[0].coeffs[j];
        if (d[j]) (*nz)++;
        if (abs(d[j]) > *mx) *mx = abs(d[j]);
    }
    eval(D0, d);
    for (int k = 0; k < N; ++k) if (D0[k] == 0) return -2;
    for (int i = 0; i < ELL; ++i) {
        for (int j = 0; j < N; ++j) d[j] = A[1 + i].coeffs[j] - B[1 + i].coeffs[j];
        eval(Di, d);
        for (int k = 0; k < N; ++k) S[k] = Di[k] * pw(D0[k], Q - 2) % Q;
        interp(s[i].coeffs, S);
    }
    return 0;
}

/* Public completion: e' = b - a_gen - A_gen*s must be short and pass the
 * key-generation norm window. */
static int complete_key(uint8_t sk_out[CRYPTO_SECRETKEYBYTES],
                        const uint8_t pk[CRYPTO_PUBLICKEYBYTES], const poly s[ELL])
{
    uint8_t seedA[SEEDBYTES], masterK[CHALLENGESEEDBYTES] = {0}, tr[CHALLENGESEEDBYTES];
    poly b[EM], b0[EM], ep[EM], zero[EM], full[KVEC], stretched[KVEC];
    poly16 agen[EM], hAgen[EM * ELL];
    memset(zero, 0, sizeof zero);
    if (unpack_pk(seedA, b, pk) != 0) return -1;
    expand_a(agen, hAgen, seedA);
    keygen_bproduct(b0, agen, hAgen, s, zero);
    for (unsigned i = 0; i < EM; ++i)
        for (unsigned j = 0; j < N; ++j) {
            int32_t d = freeze(b[i].coeffs[j] - b0[i].coeffs[j]);
            if (d > Q / 2) d -= Q;
            if (d < -BE_ENC || d > BE_ENC) return -1;
            ep[i].coeffs[j] = d;
        }
    memset(full, 0, sizeof full);
    full[0].coeffs[0] = 1;
    for (unsigned i = 0; i < ELL; ++i) full[1 + i] = s[i];
    for (unsigned i = 0; i < EM; ++i) full[1 + ELL + i] = ep[i];
    stretch_s(stretched, full);
    if (!keygen_norm_ok(stretched)) return -1;
    uint8_t in[1 + CRYPTO_PUBLICKEYBYTES];
    xof_ctx ctx;
    in[0] = DS_HASH_PK;
    memcpy(in + 1, pk, CRYPTO_PUBLICKEYBYTES);
    xof256_init(&ctx, in, sizeof in);
    xof256_squeeze(&ctx, tr, CHALLENGESEEDBYTES);
    pack_sk(sk_out, seedA, b, masterK, tr, s, ep);
    return 0;
}

static int oracle_s1_equal(const uint8_t *sk, const poly s[ELL])
{
    uint8_t sA[SEEDBYTES], K[CHALLENGESEEDBYTES], tr[CHALLENGESEEDBYTES];
    poly bb[EM], ts[ELL], te[EM];
    unpack_sk(sA, bb, K, tr, ts, te, sk);
    for (int i = 0; i < ELL; ++i)
        for (int j = 0; j < N; ++j)
            if (ts[i].coeffs[j] != s[i].coeffs[j]) return 0;
    return 1;
}

/* Sign a fresh message with the equivalent key; return verify results. */
static void forge(const uint8_t *eq, unsigned long long skl, const uint8_t *pk,
                  unsigned long long pkl, int *v1, int *v2)
{
    static uint8_t fresh[65536];
    const uint8_t m[] = "sign-23-3: Shuttle masking-seed forgery";
    unsigned long long fl = 0;
    if (sig_sign(eq, skl, (uint8_t *)m, sizeof m - 1, fresh, &fl) != 0) { *v1 = *v2 = 98; return; }
    *v1 = sig_verify(pk, pkl, fresh, fl, (uint8_t *)m, sizeof m - 1);
    uint8_t m2[sizeof m]; memcpy(m2, m, sizeof m); m2[0] ^= 1;
    *v2 = sig_verify(pk, pkl, fresh, fl, m2, sizeof m - 1);
}

static int run_collide(unsigned nsig, unsigned variant)
{
    unsigned char seed[SEEDLEN];
    for (unsigned i = 0; i < SEEDLEN; ++i) seed[i] = (unsigned char)(7 + 13 * i);
    seed[0] ^= (unsigned char)variant;
    if (init_random_number(&drng_algorithm, seed, SEEDLEN) != 0) return 2;
    unsigned long long pkl = 0, skl = 0, sl = 0;
    uint8_t *pk = malloc(CRYPTO_PUBLICKEYBYTES), *sk = malloc(CRYPTO_SECRETKEYBYTES);
    uint8_t *eq = malloc(CRYPTO_SECRETKEYBYTES);
    size_t SL = (size_t)sig_get_sn_len_bytes();
    uint8_t *sigs = malloc(SL * nsig);
    uint8_t (*enc)[ENCODECOM_VEC_BYTES] = malloc((size_t)nsig * ENCODECOM_VEC_BYTES);
    entry *E = malloc(sizeof(entry) * nsig);
    if (!pk || !sk || !eq || !sigs || !enc || !E) return 2;
    if (sig_keygen(pk, &pkl, sk, &skl) != 0) return 2;
    for (unsigned q = 0; q < nsig; ++q) {
        uint8_t msg[MSGLEN] = {0};
        memcpy(msg, &q, sizeof q);
        if (sig_sign(sk, skl, msg, MSGLEN, sigs + SL * q, &sl) != 0 || sl != SL) return 3;
        if (sig_verify(pk, pkl, sigs + SL * q, sl, msg, MSGLEN) != 0) return 4;
    }
    printf("honest: %u signatures on distinct messages, all verify\n", nsig);
    for (unsigned q = 0; q < nsig; ++q) {
        if (public_commitment(enc[q], pk, sigs + SL * q) != 0) return 5;
        E[q].h = fnv(enc[q], ENCODECOM_VEC_BYTES); E[q].idx = q;
    }
    qsort(E, nsig, sizeof(entry), cmp_entry);
    unsigned matches = 0, recovered = 0, shown = 0;
    int done = 0, v1 = 99, v2 = 99, same = -1;
    for (unsigned t = 1; t < nsig; ++t) {
        if (E[t].h != E[t - 1].h) continue;
        unsigned a = E[t - 1].idx, b = E[t].idx;
        if (memcmp(enc[a], enc[b], ENCODECOM_VEC_BYTES) != 0) continue;
        matches++;
        int verbose = shown < 3;
        if (verbose) { printf("commitment match: signatures %u and %u (distinct messages)\n", a, b); shown++; }
        uint8_t sc[CHALLENGESEEDBYTES];
        poly za[Z1LEN], zb[Z1LEN], h[EM], s[ELL];
        int nz, mx;
        if (unpack_sig(sc, za, h, sigs + SL * a) || unpack_sig(sc, zb, h, sigs + SL * b)) continue;
        int rc = divide_out(s, za, zb, &nz, &mx);
        if (verbose) printf("  d0 = z0-z0': %d nonzero coefficients, max |d0_j| = %d%s\n", nz, mx,
                            rc ? " (not invertible mod q)" : "");
        if (rc) continue;
        if (complete_key(eq, pk, s) != 0) { if (verbose) printf("  public completion rejected\n"); continue; }
        if (verbose) printf("  public completion accepted: b - a_gen - A_gen*s is short, key-gen norm window holds\n");
        recovered++;
        if (done) continue;
        forge(eq, skl, pk, pkl, &v1, &v2);
        printf("  fresh-message forgery: verify=%d (0=accept); changed-message control: verify=%d\n", v1, v2);
        same = oracle_s1_equal(sk, s);
        printf("  [evaluation] recovered s1 equals oracle s1: %s\n", same ? "yes" : "no");
        done = 1;
    }
    printf("RESULT matches=%u completed=%u forgery=%d changed=%d s1_equal=%d\n",
           matches, recovered, v1, v2, same);
    {
        uint8_t sc[CHALLENGESEEDBYTES];
        poly za[Z1LEN], zb[Z1LEN], h[EM], s[ELL];
        int nz, mx, cc = -9;
        unpack_sig(sc, za, h, sigs); unpack_sig(sc, zb, h, sigs + SL);
        int rc = divide_out(s, za, zb, &nz, &mx);
        if (rc == 0) cc = complete_key(eq, pk, s);
        printf("RESULT nonmatching_pair d0_nonzero=%d max=%d recover=%d completion=%d\n", nz, mx, rc, cc);
    }
    return 0;
}

static int run_mt(unsigned nsig)
{
    static uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], eq[CRYPTO_SECRETKEYBYTES];
    uint8_t xi[SEEDBYTES], rnd[RNDBYTES];
    for (int i = 0; i < SEEDBYTES; i++) xi[i] = (uint8_t)(0x11 * i + 3);
    if (crypto_sign_keypair_xi(pk, sk, xi)) return 2;
    if (init_random_number(&drng_algorithm, xi, SEEDBYTES) != 0) return 2;
    unsigned long long pkl = CRYPTO_PUBLICKEYBYTES, skl = CRYPTO_SECRETKEYBYTES;
    uint8_t seedA[SEEDBYTES], masterK[CHALLENGESEEDBYTES], tr[CHALLENGESEEDBYTES];
    poly bb[EM], s_true[ELL], ep[EM];
    unpack_sk(seedA, bb, masterK, tr, s_true, ep, sk);
    size_t SL = (size_t)sig_get_sn_len_bytes();
    uint8_t *sig = malloc(SL + 64);
    uint8_t (*table)[ENCODECOM_VEC_BYTES] = malloc((size_t)nsig * ENCODECOM_VEC_BYTES);
    uint8_t (*seeds)[SEEDBYTES] = malloc((size_t)nsig * SEEDBYTES);
    poly (*z1s)[Z1LEN] = malloc(sizeof(poly) * Z1LEN * nsig);
    entry *E = malloc(sizeof(entry) * nsig);
    if (!sig || !table || !seeds || !z1s || !E) return 2;
    int honest = 1;
    for (unsigned t = 0; t < nsig; t++) {
        uint8_t msg[MSGLEN], mu[CHALLENGESEEDBYTES], stage[1 + CHALLENGESEEDBYTES + MSGLEN];
        size_t siglen = 0;
        for (int i = 0; i < MSGLEN; i++) msg[i] = (uint8_t)(t * 7 + i + (t >> 8));
        memcpy(msg, &t, sizeof t);
        for (int i = 0; i < RNDBYTES; i++) rnd[i] = (uint8_t)(t * 13 + i * 5 + 1 + (t >> 8));
        if (crypto_sign_signature_rnd(sig, &siglen, msg, MSGLEN, sk, rnd)) return 3;
        honest &= crypto_sign_verify(sig, siglen, msg, MSGLEN, pk) == 0;
        uint32_t att = shuttle_last_sign_attempts;
        /* evaluation stand-in for the enumeration hit: regenerate seed_y with K */
        xof_ctx h;
        stage[0] = DS_HASH_MSG; memcpy(stage + 1, tr, CHALLENGESEEDBYTES);
        memcpy(stage + 1 + CHALLENGESEEDBYTES, msg, MSGLEN);
        xof256_init(&h, stage, sizeof stage); xof256_squeeze(&h, mu, CHALLENGESEEDBYTES);
        expand_signing_seeds(seeds[t], masterK, rnd, mu, att - 1);
        /* public table entry: the verifier's EncodeCom and z1 of this signature */
        uint8_t seedC[CHALLENGESEEDBYTES];
        poly hint[EM];
        if (public_commitment(table[t], pk, sig) != 0) return 5;
        if (unpack_sig(seedC, z1s[t], hint, sig) != 0) return 5;
        E[t].h = fnv(table[t], ENCODECOM_VEC_BYTES); E[t].idx = t;
    }
    printf("honest: %u signatures, all verify: %s\n", nsig, honest ? "yes" : "no");
    qsort(E, nsig, sizeof(entry), cmp_entry);
    attacker_T_init(pk);

    /* Identity check at the build's width (also the full 128-bit width). */
    unsigned check = nsig < 4 ? nsig : 4, id_ok = 1;
    for (unsigned t = 0; t < check; t++) {
        uint8_t T[ENCODECOM_VEC_BYTES], s2[SEEDBYTES];
        poly yp[KVEC], s[ELL];
        int nz, mx;
        attacker_T(T, seeds[t], yp);
        int hit = !memcmp(T, table[t], ENCODECOM_VEC_BYTES);
        memcpy(s2, seeds[t], SEEDBYTES); s2[0] ^= 1;
        attacker_T(T, s2, NULL);
        int miss = 1;
        for (unsigned j = 0; j < nsig; j++) if (!memcmp(T, table[j], ENCODECOM_VEC_BYTES)) miss = 0;
        /* z1 - y' = (c', s*c'): divide out against y' */
        poly ypz[Z1LEN];
        for (int i = 0; i < Z1LEN; i++) ypz[i] = yp[i];
        int ok = divide_out(s, z1s[t], ypz, &nz, &mx) == 0 && oracle_s1_equal(sk, s);
        printf("seed of sig %u: T(pk,seed) hits its table entry: %s; flipped-bit seed misses all: %s; "
               "c' weight=%d; exact s1 from this signature: %s\n",
               t, hit ? "yes" : "no", miss ? "yes" : "no", nz, ok ? "yes" : "no");
        id_ok &= hit && miss && ok;
    }
    printf("RESULT mt_identity=%u\n", id_ok);

#ifdef NGCC_SEED_BITS
    /* Reduced width: real enumeration of the seed space against all targets. */
    uint64_t space = 1ULL << NGCC_SEED_BITS, g;
    int found = -1; uint8_t cand[SEEDBYTES];
    poly yp[KVEC];
    for (g = 0; g < space && found < 0; g++) {
        uint8_t T[ENCODECOM_VEC_BYTES];
        memset(cand, 0, SEEDBYTES);
        for (int i = 0; i < (NGCC_SEED_BITS + 7) / 8; i++) cand[i] = (uint8_t)(g >> (8 * i));
        attacker_T(T, cand, yp);
        uint64_t hv = fnv(T, ENCODECOM_VEC_BYTES);
        size_t lo = 0, hi = nsig;
        while (lo < hi) { size_t mid = (lo + hi) / 2; if (E[mid].h < hv) lo = mid + 1; else hi = mid; }
        for (; lo < nsig && E[lo].h == hv; lo++)
            if (!memcmp(T, table[E[lo].idx], ENCODECOM_VEC_BYTES)) { found = (int)E[lo].idx; break; }
    }
    int v1 = 99, v2 = 99, same = -1, cc = -9;
    if (found >= 0) {
        poly s[ELL], ypz[Z1LEN];
        int nz, mx;
        for (int i = 0; i < Z1LEN; i++) ypz[i] = yp[i];
        printf("enumeration: table hit on signature %d after %llu guesses (expected about %.0f = 2^%d/%u)\n",
               found, (unsigned long long)g, (double)space / nsig, NGCC_SEED_BITS, nsig);
        if (divide_out(s, z1s[found], ypz, &nz, &mx) == 0) {
            cc = complete_key(eq, pk, s);
            printf("  s1 solved from z1 - y'; public completion: %s\n", cc == 0 ? "accepted" : "rejected");
            if (cc == 0) {
                forge(eq, skl, pk, pkl, &v1, &v2);
                printf("  fresh-message forgery: verify=%d (0=accept); changed-message control: verify=%d\n", v1, v2);
            }
            same = oracle_s1_equal(sk, s);
            printf("  [evaluation] recovered s1 equals oracle s1: %s\n", same ? "yes" : "no");
        }
    }
    printf("RESULT mt_enum guesses=%llu found=%d completion=%d forgery=%d changed=%d s1_equal=%d\n",
           (unsigned long long)g, found, cc, v1, v2, same);
#endif
    return 0;
}

int main(int argc, char **argv)
{
    init_psi();
#ifdef NGCC_SEED_BITS
    printf("model: reduced-width seed_y (%d of %d bits kept)\n", NGCC_SEED_BITS, 8 * SEEDBYTES);
#else
    printf("model: shipped seed_y width (%d bits)\n", 8 * SEEDBYTES);
#endif
    if (argc > 2 && !strcmp(argv[1], "collide"))
        return run_collide((unsigned)strtoul(argv[2], 0, 10), argc > 3 ? (unsigned)strtoul(argv[3], 0, 10) : 0);
    if (argc > 2 && !strcmp(argv[1], "mt"))
        return run_mt((unsigned)strtoul(argv[2], 0, 10));
    fprintf(stderr, "usage: %s collide <n> [drbg] | mt <n>\n", argv[0]);
    return 2;
}
