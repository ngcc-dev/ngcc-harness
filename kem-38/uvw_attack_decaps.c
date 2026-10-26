/* Attacker-side decapsulation of a UVW-KEM-128 ciphertext from PUBLIC data
 * (pk, ct) plus the structure recovered from pk alone by uvw_key_recovery.
 * usage: uvw_attack_decaps pk.bin struct.txt ct.bin [ss.bin]
 *   ss.bin (sender's secret) is read only at the end, for scoring.
 * Uses the frozen reference code (#include) for unpacking, hashes, H1 expansion,
 * and the frozen Guruswami-Sudan list decoder uvw_rs_list_decode. */
#include "KEM_AlgorithmInstance.c"
#include <time.h>
DRNG_ctx drng_algorithm;
#define QQ UVW_Q
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static uint32_t mi(uint32_t a) { return gf_inv((gf_elem_t)(a % QQ)); }
static unsigned char *rd(const char *p, long *len) {
    FILE *f = fopen(p, "rb"); if (!f) return NULL; fseek(f, 0, SEEK_END); *len = ftell(f); rewind(f);
    unsigned char *b = malloc(*len); if (fread(b, 1, *len, f) != (size_t)*len) return NULL; fclose(f); return b;
}
/* solve r*G_S = c1_S over the coordinate list S (size ns); returns 0 if consistent & full rank */
static int solve_r(const uint32_t *G, const gf_elem_t *c1, const long *S, long ns, gf_elem_t *r) {
    long k = UVW_K, n = UVW_N, w = k + 1;
    uint32_t *A = malloc((size_t)ns * w * 4);
    for (long a = 0; a < ns; ++a) { for (long t = 0; t < k; ++t) A[a * w + t] = G[t * n + S[a]]; A[a * w + k] = c1[S[a]]; }
    long rank = 0, *pc = malloc(w * sizeof *pc);
    for (long col = 0; col < w && rank < ns; ++col) {
        long piv = -1; for (long i = rank; i < ns; ++i) if (A[i * w + col]) { piv = i; break; }
        if (piv < 0) continue;
        if (piv != rank) for (long j = 0; j < w; ++j) { uint32_t t = A[piv * w + j]; A[piv * w + j] = A[rank * w + j]; A[rank * w + j] = t; }
        uint32_t iv = mi(A[rank * w + col]); for (long j = col; j < w; ++j) A[rank * w + j] = A[rank * w + j] * iv % QQ;
        for (long i = 0; i < ns; ++i) if (i != rank && A[i * w + col]) { uint32_t g = QQ - A[i * w + col]; for (long j = col; j < w; ++j) A[i * w + j] = (A[i * w + j] + g * A[rank * w + j]) % QQ; }
        pc[rank++] = col;
    }
    int ok = (rank == k) && pc[k - 1] == k - 1;   /* rank k on coefficient part, no pivot in RHS column */
    if (ok) for (long t = 0; t < k; ++t) r[t] = (gf_elem_t)A[t * w + k];
    free(A); free(pc); return ok ? 0 : -1;
}
int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: uvw_attack_decaps pk.bin struct.txt ct.bin [ss.bin]\n"); return 2; }
    const long n = UVW_N, k = UVW_K, h = UVW_N / 2, W = UVW_W;
    double t0 = now();
    long pl, cl; unsigned char *pkb = rd(argv[1], &pl), *ctb = rd(argv[3], &cl);
    if (!pkb || !ctb || pl != (long)kem_get_pk_len_bytes() || cl != (long)kem_get_ct_len_bytes()) { puts("ERROR: bad input"); return 2; }
    public_key_t *pk = malloc(sizeof *pk); uvw_unpack_pk(pk, pkb);
    uint32_t *G = calloc((size_t)k * n, 4);
    for (long r = 0; r < k; ++r) { G[r * n + r] = 1; for (long j = 0; j < n - k; ++j) G[r * n + k + j] = pk->T[r * (n - k) + j]; }
    kem_ciphertext_t *ct = malloc(sizeof *ct); uvw_unpack_ct(ct, ctb);
    FILE *sf = fopen(argv[2], "r"); long P, nc;
    if (!sf || fscanf(sf, "%ld %ld", &P, &nc) != 2 || P != h || nc < 1) { puts("ERROR: structure file incomplete (need all 430 pairs and >=1 GRS candidate)"); return 2; }
    long *pi = malloc(P * sizeof *pi), *pj = malloc(P * sizeof *pj); uint32_t *rho = malloc(P * 4);
    for (long p = 0; p < P; ++p) if (fscanf(sf, "%ld %ld %u", &pi[p], &pj[p], &rho[p]) != 3) return 2;
    gf_elem_t (*al)[430] = malloc(nc * sizeof *al), (*vv)[430] = malloc(nc * sizeof *vv);
    for (long z = 0; z < nc; ++z) for (long p = 0; p < P; ++p) { unsigned a, v; if (fscanf(sf, "%u %u", &a, &v) != 2) return 2; al[z][p] = a; vv[z][p] = v; }
    fclose(sf);
    double t1 = now();
    gf_elem_t r[UVW_K], e[UVW_N], f[UVW_K], y[430];
    int done = 0; long used_z = -1, errpairs = -1, nS = 0, wt = -1, ldret = 0, tries = 0;
    double t_ld = 0, t_lin = 0;
    for (long z = 0; z < nc && !done; ++z) {
        double ta = now();
        for (long p = 0; p < P; ++p) {
            uint32_t delta = (ct->c1[pi[p]] + QQ - ct->c1[pj[p]] * mi(rho[p]) % QQ) % QQ;   /* c_i - c_j d_i/d_j */
            y[p] = (gf_elem_t)(delta * mi(vv[z][p]) % QQ);
        }
        memset(f, 0, sizeof f);
        ldret = uvw_rs_list_decode(y, al[z], f);
        long ep = 0; char *bad = calloc(P, 1);
        for (long p = 0; p < P; ++p) { uint32_t ev = 0; for (long d = k / 2 - 1; d >= 0; --d) ev = (ev * al[z][p] + f[d]) % QQ; if (ev != y[p]) { ep++; bad[p] = 1; } }
        double tb = now(); t_ld += tb - ta;
        if (ldret <= 0 || ep > W) { free(bad); continue; }
        long *S = malloc(n * sizeof *S); nS = 0;
        for (long p = 0; p < P; ++p) if (!bad[p]) { S[nS++] = pi[p]; S[nS++] = pj[p]; }
        uint64_t rs = 12345;
        for (tries = 1; tries <= 50 && !done; ++tries) {
            long *Su = S, ns = nS, *Sr = NULL;
            if (tries > 1) { /* random subset in case an error cancelled inside a pair */
                Sr = malloc(nS * sizeof *Sr); memcpy(Sr, S, nS * sizeof *Sr);
                for (long a = nS - 1; a > 0; --a) { rs = rs * 6364136223846793005ULL + 1442695040888963407ULL; long b = (long)((rs >> 33) % (a + 1)); long t = Sr[a]; Sr[a] = Sr[b]; Sr[b] = t; }
                Su = Sr; ns = k + 30 < nS ? k + 30 : nS;
            }
            if (solve_r(G, ct->c1, Su, ns, r) == 0) {
                wt = 0;
                for (long c = 0; c < n; ++c) { uint64_t s = 0; for (long t = 0; t < k; ++t) s += (uint64_t)r[t] * G[t * n + c]; e[c] = (gf_elem_t)((ct->c1[c] + QQ - s % QQ) % QQ); wt += e[c] != 0; }
                if (wt <= W) done = 1;
            }
            free(Sr);
        }
        tries--;
        t_lin += now() - tb;
        if (done) { used_z = z; errpairs = ep; }
        free(bad); free(S);
    }
    double t2 = now();
    if (!done) { printf("RESULT: FAIL (no candidate decoded to weight <= %ld) list_decode_ret=%ld t_parse=%.3fs t_listdec=%.3fs t_lin=%.3fs\n", W, ldret, t1 - t0, t_ld, t_lin); return 1; }
    /* m = c2 xor H3(r, e); d check; FO re-derivation; K = H4(m, c1, c2) */
    unsigned char mask[UVW_M / 8], m[UVW_M / 8], d2[UVW_M / 8], K[64], sh1[32];
    kem_hash(H3_PREFIX, (unsigned char *)r, k * sizeof(gf_elem_t) * 8, (unsigned char *)e, n * sizeof(gf_elem_t) * 8, NULL, 0, UVW_M, mask);
    for (int i = 0; i < UVW_M / 8; ++i) m[i] = ct->c2[i] ^ mask[i];
    kem_hash(H2_PREFIX, m, UVW_M, NULL, 0, NULL, 0, UVW_M, d2);
    int d_ok = !memcmp(d2, ct->d, sizeof d2);
    kem_hash(H1_PREFIX, m, UVW_M, NULL, 0, NULL, 0, 256, sh1);
    DRNG_ctx dh; init_random_number(&dh, sh1, 32);
    gf_elem_t r2[UVW_K], e2[UVW_N]; generate_random_matrix(r2, k, &dh); sample_error_vector(e2, n, W, &dh);
    int fo_ok = !memcmp(r2, r, sizeof r2) && !memcmp(e2, e, sizeof e2);
    kem_hash(H4_PREFIX, m, UVW_M, (unsigned char *)ct->c1, sizeof(ct->c1) * 8, (unsigned char *)ct->c2, sizeof(ct->c2) * 8, 512, K);
    double t3 = now();
    printf("RESULT: decoded candidate=%ld list_decode_ret=%ld error_pairs=%ld errfree_coords=%ld solve_tries=%ld wt(e)=%ld d_check=%s FO_reencrypt(r,e)=%s",
           used_z, ldret, errpairs, nS, tries, wt, d_ok ? "ok" : "FAIL", fo_ok ? "ok" : "FAIL");
    if (argc > 4) { long sl; unsigned char *ss = rd(argv[4], &sl); printf(" K==sender_ss: %s", (ss && sl == 64 && !memcmp(ss, K, 64)) ? "YES" : "NO"); }
    printf(" | t_parse=%.3fs t_listdec=%.3fs t_linalg=%.3fs t_hash=%.3fs total=%.3fs\n", t1 - t0, t_ld, t_lin, t3 - t2, t3 - t0);
    return (d_ok && fo_ok) ? 0 : 1;
}
