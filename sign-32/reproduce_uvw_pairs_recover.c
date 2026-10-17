/* Public UVW key recovery from scored candidate pairs (cand.tsv from
   pair_stats.py: a, b, c, score) and the public key only.
   1. span S = <h_a + c h_b> over top candidates (expect dim n/2-k1);
   2. reduce every public column modulo S; hidden partners become
      proportional, recovering all n/2 pairs and relative scales;
   3. build an equivalent secret key (A,B | -Z,Z) D' and sign a FRESH message
      with the frozen uvw_sign; check it with the intended uvw_verify.
   truth.bin is read only to report the pair-recovery accuracy.
   usage: recover pk.bin cand.tsv NCAND truth.bin [ZTHR] */
#include "SIG_AlgorithmInstance.c"
#include <stdint.h>
#include <time.h>
DRNG_ctx drng_algorithm;
static size_t nk, n, h;
static vf3_e **Bs; static size_t *piv; static size_t rank_ = 0;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static void reduce(vf3_e *u) {
    for (size_t k = 0; k < rank_; k++) { uint8_t c = vf3_get_element(piv[k], u); if (c) vf3_vector_add_multiple_inplace(u, 3 - c, Bs[k]); }
}
static int first_nz(const vf3_e *u) {
    for (size_t i = 0; i < u->alloc; i++) if (u->r0[i]) return i * 64 + __builtin_ctzll(u->r0[i]);
    return -1;
}
static int insert(vf3_e *u) {     /* u already reduced */
    int p = first_nz(u); if (p < 0) return 0;
    if (vf3_get_element(p, u) == 2) vf3_vector_scalarmul_inplace(u, 2);
    for (size_t k = 0; k < rank_; k++) { uint8_t c = vf3_get_element(p, Bs[k]); if (c) vf3_vector_add_multiple_inplace(Bs[k], 3 - c, u); }
    Bs[rank_] = u; piv[rank_] = p; rank_++; return 1;
}
static uint64_t mix(uint64_t z) { z += 0x9e3779b97f4a7c15ULL; z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL; z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL; return z ^ (z >> 31); }
static uint64_t hashv(const vf3_e *u) { uint64_t x = 0; for (size_t i = 0; i < u->alloc; i++) { x = mix(x ^ u->r0[i]); x = mix(x ^ u->r1[i]); } return x; }
typedef struct { uint64_t hv; size_t j; } hent;
static int hcmp(const void *a, const void *b) { const hent *x = a, *y = b; return x->hv < y->hv ? -1 : x->hv > y->hv; }
int main(int argc, char **argv) {
    double t0 = now();
    FILE *f = fopen(argv[1], "rb"); fseek(f, 0, SEEK_END); size_t pl = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char *pkb = malloc(pl); if (fread(pkb, 1, pl, f) != pl) return 2; fclose(f);
    uvw_public_key pk; deserialize_pk(&pk, pkb); uvw_param P = pk.param;
    n = P.n; nk = P.n - P.k; h = n / 2; size_t want = h - P.k1;
    mf3_e *Rt = mf3_alloc(P.k, nk); mf3_transpose_and_copy(Rt, pk.r);       /* columns of R */
    vf3_e **col = malloc(n * sizeof *col);
    for (size_t j = 0; j < n; j++) { col[j] = vf3_alloc(nk); if (j < nk) vf3_set_coeff(j, col[j], 1); else vf3_copy(col[j], &Rt->rows[j - nk]); vf3_trim(col[j]); }
    Bs = malloc(nk * sizeof *Bs); piv = malloc(nk * sizeof *piv);
    /* step 1 */
    f = fopen(argv[2], "r"); size_t ncand = atol(argv[3]), used = 0, zero = 0, skipped = 0; long a, b, c; double sc, zs, zthr = argc > 5 ? atof(argv[5]) : 0;
    while (used < ncand && fscanf(f, "%ld %ld %ld %lf %lf", &a, &b, &c, &sc, &zs) == 5) {
        if (zs < zthr) { skipped++; continue; }
        vf3_e *u = vf3_alloc(nk); vf3_copy(u, col[a]); vf3_vector_add_multiple_inplace(u, c == 1 ? 1 : 2, col[b]);
        reduce(u); if (!insert(u)) { vf3_free(u); zero++; } used++;
    }
    fclose(f);
    printf("step1: %zu candidates (|S| z >= %.1f; %zu lower-z skipped) -> span dim %zu (expected n/2-k1 = %zu); %zu already in span  [%.1fs]\n", used, zthr, skipped, rank_, want, zero, now() - t0);
    /* A few false candidates only enlarge W = span >= S; modulo W hidden partners stay
       proportional, so step 2 still works when rank_ > want.  rank_ < want means S is not yet spanned. */
    if (rank_ < want) { printf("span too small: need more candidates\n"); return 1; }
    if (rank_ > want) printf("step1: span W exceeds n/2-k1 by %zu (false candidates); continuing with W >= S\n", rank_ - want);
    if (getenv("DEBUG_TRUTH")) {   /* diagnostic only: which true pair combinations lie in S */
        FILE *ft = fopen(argv[4], "rb"); uint32_t tp_[1]; uint8_t td_[1]; uint32_t *tpp = malloc(n*4); uint8_t *tdd = malloc(n);
        for (size_t i = 0; i < n; i++) { if (fread(tp_, 4, 1, ft) != 1 || fread(td_, 1, 1, ft) != 1) return 2; tpp[i] = tp_[0]; tdd[i] = td_[0]; } fclose(ft);
        size_t inS[2] = {0,0};
        for (size_t i = 0; i < h; i++) for (int sgn = 0; sgn < 2; sgn++) {
            vf3_e *u = vf3_alloc(nk); vf3_copy(u, col[tpp[i]]); vf3_vector_add_multiple_inplace(u, sgn ? 2 : 1, col[tpp[i+h]]);
            reduce(u); if (first_nz(u) < 0) inS[sgn]++; vf3_free(u); }
        printf("debug: true pairs with h_a + h_b in S: %zu, h_a - h_b in S: %zu\n", inS[0], inS[1]);
    }
    hent *H = malloc(n * sizeof *H); vf3_e **res = calloc(n, sizeof *res); uint8_t *nrm = malloc(n);
    long *pa = malloc(n * sizeof(long)); int8_t *rel = malloc(n); size_t npairs = 0;
    for (int round = 1; round <= 3; round++) {
    /* step 2: quotient */
    for (size_t j = 0; j < n; j++) {
        if (!res[j]) res[j] = vf3_alloc(nk); vf3_copy(res[j], col[j]); reduce(res[j]);
        int p = first_nz(res[j]); nrm[j] = 1;
        if (p >= 0 && vf3_get_element(p, res[j]) == 2) { vf3_vector_scalarmul_inplace(res[j], 2); nrm[j] = 2; }
        vf3_trim(res[j]); H[j].hv = hashv(res[j]); H[j].j = j;
    }
    qsort(H, n, sizeof *H, hcmp);
    npairs = 0; size_t bad = 0;
    for (size_t j = 0; j < n; j++) pa[j] = -1;
    for (size_t i = 0; i < n; ) {
        size_t e = i; while (e < n && H[e].hv == H[i].hv) e++;
        if (e - i == 2 && vf3_equal(res[H[i].j], res[H[i + 1].j])) {
            size_t x = H[i].j, y = H[i + 1].j; pa[x] = y; pa[y] = x;
            /* res_x*nrm_x = col_x mod S, so col_x - s col_y in S with s = nrm_x/nrm_y; c = -s */
            int s = (nrm[x] == nrm[y]) ? 1 : -1; rel[x] = rel[y] = -s; npairs++;
        } else bad += e - i;
        i = e;
    }
    printf("step2 (round %d): quotient pairs found %zu of %zu; columns not uniquely paired %zu  [%.1fs]\n", round, npairs, h, bad, now() - t0);
    if (npairs == h || round == 3) break;
    /* refine: rebuild the span from the pairs found so far (removes false candidates) */
    for (size_t k = 0; k < rank_; k++) vf3_free(Bs[k]);
    rank_ = 0;
    for (size_t j = 0; j < n; j++) if (pa[j] > (long)j) { vf3_e *u = vf3_alloc(nk); vf3_copy(u, col[j]); vf3_vector_add_multiple_inplace(u, rel[j] == 1 ? 1 : 2, col[pa[j]]); reduce(u); if (!insert(u)) vf3_free(u); }
    printf("round %d: span rebuilt from %zu recovered pairs: dim %zu\n", round, npairs, rank_);
    }
    /* score against truth */
    f = fopen(argv[4], "rb"); uint32_t *tp = malloc(n * 4); uint8_t *td = malloc(n);
    for (size_t i = 0; i < n; i++) { if (fread(&tp[i], 4, 1, f) != 1 || fread(&td[i], 1, 1, f) != 1) return 2; } fclose(f);
    if (getenv("DEBUG_TRUTH")) { int shown = 0; size_t gs[5] = {0};
        for (size_t i = 0; i < n; ) { size_t e = i; while (e < n && H[e].hv == H[i].hv) e++; gs[(e-i) > 4 ? 4 : (e-i)]++; i = e; }
        for (size_t i = 0, sh = 0; i < n && sh < 2; ) { size_t e = i; while (e < n && H[e].hv == H[i].hv) e++;
            if (e - i > 2) { printf("debug: group size %zu:", e - i); for (size_t q = i; q < e && q < i + 6; q++) printf(" col %zu (w=%zu first=%d)", H[q].j, vf3_hamming_weight(res[H[q].j]), first_nz(res[H[q].j])); printf("\n"); sh++; } i = e; }
        printf("debug: hash group sizes 1:%zu 2:%zu 3:%zu 4+:%zu\n", gs[1], gs[2], gs[3], gs[4]);
        for (size_t i = 0; i < h && shown < 3; i++) { long x = tp[i], y = tp[i + h]; if (pa[x] == y) continue;
            size_t diff = 0, nz = 0; for (size_t q = 0; q < nk; q++) { uint8_t u = vf3_get_element(q, res[x]), v = vf3_get_element(q, res[y]); diff += (u != v); nz += (u != 0); }
            printf("debug: failing true pair (%ld,%ld): residual weight %zu, coords differing %zu, pa[x]=%ld\n", x, y, nz, diff, pa[x]); shown++; } }
    size_t okp = 0, oks = 0;
    for (size_t i = 0; i < h; i++) {
        long x = tp[i], y = tp[i + h];
        if (pa[x] == y) { okp++; int tr = ((td[i] == 1) ? 1 : -1) * ((td[i + h] == 1) ? 1 : -1); if (rel[x] == tr) oks++; }
    }
    printf("truth check: %zu/%zu hidden pairs recovered, %zu with correct relative scale\n", okp, h, oks);
    if (npairs != h) return 1;
    /* rebuild S exactly from the n/2 recovered pairs */
    for (size_t k = 0; k < rank_; k++) vf3_free(Bs[k]);
    rank_ = 0;
    for (size_t j = 0; j < n; j++) if ((long)j < pa[j]) { vf3_e *u = vf3_alloc(nk); vf3_copy(u, col[j]); vf3_vector_add_multiple_inplace(u, rel[j] == 1 ? 1 : 2, col[pa[j]]); reduce(u); if (!insert(u)) vf3_free(u); }
    printf("step2b: S rebuilt from all recovered pairs: dim %zu (expected %zu)  [%.1fs]\n", rank_, want, now() - t0);
    if (rank_ != want) return 1;
    /* step 3: equivalent key.  x-space basis: rows e_p (p pivot of S) and the annihilator of S. */
    mf3_e *Hp = mf3_alloc(nk, n);
    for (size_t r = 0; r < nk; r++) { vf3_set_coeff(r, &Hp->rows[r], 1); for (size_t j = 0; j < P.k; j++) { uint8_t v = mf3_coeff(pk.r, r, j); if (v) vf3_set_coeff(nk + j, &Hp->rows[r], v); } }
    uint8_t *ispiv = calloc(nk, 1); for (size_t k = 0; k < rank_; k++) ispiv[piv[k]] = 1;
    mf3_e *Hs = mf3_alloc(nk, n); size_t rt = 0, rb = h - P.k1;
    for (size_t k = 0; k < rank_; k++) vf3_copy(&Hs->rows[rt++], &Hp->rows[piv[k]]);
    for (size_t fcol = 0; fcol < nk; fcol++) if (!ispiv[fcol]) {
        vf3_e *row = &Hs->rows[rb++]; vf3_copy(row, &Hp->rows[fcol]);
        for (size_t k = 0; k < rank_; k++) { uint8_t bk = vf3_get_element(fcol, Bs[k]); if (bk) vf3_vector_add_multiple_inplace(row, 3 - bk, &Hp->rows[piv[k]]); }
    }
    /* pair layout: secret index i -> (first = a_i, second = b_i), D' = (p,d) */
    dp_e *D = dp_alloc(n); size_t ii = 0;
    for (size_t j = 0; j < n; j++) if ((long)j < pa[j]) { D->p[ii] = j; vf3_set_coeff(ii, D->d, 1); D->p[ii + h] = pa[j]; vf3_set_coeff(ii + h, D->d, rel[j] == 1 ? 1 : 2); ii++; }
    size_t r1 = h - P.k1, r2 = h - P.k2;
    mf3_e *HX = mf3_alloc(r1, h), *HY = mf3_alloc(r1, h), *HZ = mf3_alloc(r2, h); size_t zbad = 0;
    for (size_t i = 0; i < h; i++) {
        uint8_t ci = vf3_get_element(i + h, D->d);
        for (size_t r = 0; r < nk; r++) {
            uint8_t L = mf3_coeff(Hs, r, D->p[i]), Rr = (mf3_coeff(Hs, r, D->p[i + h]) * ci) % 3;
            if (r < r1) { mf3_setcoeff(HX, r, i, L); mf3_setcoeff(HY, r, i, Rr); }
            else { mf3_setcoeff(HZ, r - r1, i, Rr); if ((L + Rr) % 3) zbad++; }
        }
    }
    printf("step3: equivalent key built; bottom block (-Z|Z) violations: %zu  [%.1fs]\n", zbad, now() - t0);
    uvw_secret_key sk; sk.param = P; sk.h_x = HX; sk.h_y = HY; sk.h_z = HZ; sk.d = D;
    mf3_e *H1 = mf3_alloc(nk, nk);
    for (size_t r = 0; r < nk; r++) for (size_t j = 0; j < nk; j++) { uint8_t v = mf3_coeff(Hs, r, j); if (v) mf3_setcoeff(H1, r, j, v); }
    sk.sti = mf3_alloc(nk, nk); mf3_transpose_and_copy(sk.sti, H1);
    unsigned char sd[48] = "sign-32-3 forger randomness"; init_random_number(uvw_drng, sd, 48);
    const char *msg = "sign-32-3: fresh message never signed by the key owner";
    shake256_ctx ctx; uvw_hash_init(&ctx); uvw_hash_update(&ctx, (unsigned char*)msg, strlen(msg));
    uvw_signature sig = uvw_sign(sk, &ctx); uvw_hash_free(&ctx);
    shake256_ctx v1; uvw_hash_init(&v1); uvw_hash_update(&v1, (unsigned char*)msg, strlen(msg));
    int ok1 = uvw_verify(pk, &v1, sig); uvw_hash_free(&v1);
    const char *msg2 = "sign-32-3: fresh message never signed by the key owner!";
    shake256_ctx v2; uvw_hash_init(&v2); uvw_hash_update(&v2, (unsigned char*)msg2, strlen(msg2));
    int ok2 = uvw_verify(pk, &v2, sig); uvw_hash_free(&v2);
    unsigned long long snl = sn_serialized_size(&P); unsigned char *sn = malloc(snl); serialize_sn(sn, &sig);
    f = fopen("forgery.sig", "wb"); fwrite(sn, 1, snl, f); fclose(f);
    printf("forgery: intended uvw_verify on fresh message = %d, on changed message = %d, API signature %llu bytes  [%.1fs]\n", ok1, ok2, snl, now() - t0);
    return !(ok1 && !ok2);
}
