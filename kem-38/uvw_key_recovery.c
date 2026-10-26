/* Full-size UVW-KEM-128 twin pairing + twin ratios + GRS (alpha, multiplier)
 * recovery from the PUBLIC KEY ONLY, via left-kernel blocks of the order-2
 * cubic jet system on shortened codes.
 *
 * usage: uvw_key_recovery pk.bin k_l seed maxrounds [truth.txt|-] [struct.txt]
 *   truth.txt (optional) is read ONLY after recovery has finished, for scoring;
 *   struct.txt receives the recovered pairs, ratios and GRS descriptions.
 *
 * Blocked elimination (panel + lazy-reduced trailing update, AVX-512 VNNI)
 * copied from kem-38/reproduce_jet_distinguisher.c and extended to record the
 * block-echelon pivot rows so that the right kernel of M^T (= left kernel of
 * the jet system M) can be obtained by back-substitution.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#ifdef __AVX512VNNI__
#include <immintrin.h>
#endif

#define Q 433u
#define QBITS 9
#define INF Q
static uint64_t rs;
static uint64_t rnd64(void) {
    uint64_t z = (rs += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
static uint32_t INV[Q];
static uint32_t inv(uint32_t a) { return INV[a % Q]; }
static void init_inv(void) {
    for (uint32_t a = 1; a < Q; ++a) { uint32_t r = 1, e = Q - 2, b = a; while (e) { if (e & 1) r = r * b % Q; b = b * b % Q; e >>= 1; } INV[a] = r; }
    INV[0] = 0;
}
static double now(void) { return omp_get_wtime(); }

/* ---------- small dense helpers ---------- */
static long rref32(uint32_t *M, long r, long c, long *pivcol) {
    long rank = 0;
    for (long col = 0; col < c && rank < r; ++col) {
        long piv = -1;
        for (long i = rank; i < r; ++i) if (M[i * c + col]) { piv = i; break; }
        if (piv < 0) continue;
        if (piv != rank) for (long j = 0; j < c; ++j) { uint32_t t = M[piv * c + j]; M[piv * c + j] = M[rank * c + j]; M[rank * c + j] = t; }
        uint32_t *p = M + rank * c, iv = inv(p[col]);
        for (long j = col; j < c; ++j) p[j] = p[j] * iv % Q;
        #pragma omp parallel for schedule(dynamic, 32) if (r * c > 200000)
        for (long i = 0; i < r; ++i) {
            if (i == rank) continue;
            uint32_t *row = M + i * c, f = row[col];
            if (!f) continue;
            uint32_t g = Q - f;
            for (long j = col; j < c; ++j) row[j] = (row[j] + g * p[j]) % Q;
        }
        if (pivcol) pivcol[rank] = col;
        ++rank;
    }
    return rank;
}
static long rank_small(const uint32_t *A, long r, long c) {
    uint32_t *M = malloc(r * c * 4 + 4); memcpy(M, A, r * c * 4);
    long rank = 0;
    for (long col = 0; col < c && rank < r; ++col) {
        long piv = -1;
        for (long i = rank; i < r; ++i) if (M[i * c + col]) { piv = i; break; }
        if (piv < 0) continue;
        if (piv != rank) for (long j = 0; j < c; ++j) { uint32_t t = M[piv * c + j]; M[piv * c + j] = M[rank * c + j]; M[rank * c + j] = t; }
        uint32_t *p = M + rank * c, iv = inv(p[col]);
        for (long j = col; j < c; ++j) p[j] = p[j] * iv % Q;
        for (long i = rank + 1; i < r; ++i) {
            uint32_t *row = M + i * c, f = row[col];
            if (!f) continue;
            uint32_t g = Q - f;
            for (long j = col; j < c; ++j) row[j] = (row[j] + g * p[j]) % Q;
        }
        ++rank;
    }
    free(M); return rank;
}

/* ---------- blocked elimination with recorded panels ---------- */
#define PB 128
#define TJ 1024
typedef struct { long c0, w, b; long pcol[PB]; uint16_t *prow[PB]; uint32_t Ainv[PB][PB]; } panel_t;

/* Eliminates M (r x c, uint16, row-major); records panels. Returns rank. */
static long elim_record(uint16_t *M, long r, long c, panel_t **pan_out, long *npan_out) {
    uint16_t **row = malloc(r * sizeof *row);
    uint32_t *P = malloc((size_t)r * PB * 4), *P0 = malloc((size_t)r * PB * 4);
    uint32_t *mul = malloc((size_t)r * PB * 4);
    char *is_piv = malloc(r);
    long npmax = (c + PB - 1) / PB, npan = 0;
    panel_t *pan = calloc(npmax, sizeof *pan);
    long live = r, rank = 0;
    for (long i = 0; i < r; ++i) row[i] = M + (size_t)i * c;
    for (long c0 = 0; c0 < c && live > 0; c0 += PB) {
        long w = c - c0 < PB ? c - c0 : PB, b = 0, prow[PB], pcol[PB];
        panel_t *pp = &pan[npan++]; pp->c0 = c0; pp->w = w;
        #pragma omp parallel for schedule(static)
        for (long i = 0; i < live; ++i) {
            is_piv[i] = 0;
            for (long t = 0; t < w; ++t) P[i * PB + t] = P0[i * PB + t] = row[i][c0 + t];
        }
        for (long t = 0; t < w; ++t) {
            long piv = -1;
            for (long i = 0; i < live; ++i) if (!is_piv[i] && P[i * PB + t]) { piv = i; break; }
            if (piv < 0) continue;
            is_piv[piv] = 1; prow[b] = piv; pcol[b] = t; ++b;
            uint32_t *p = P + piv * PB, iv = inv(p[t]);
            for (long u = t; u < w; ++u) p[u] = p[u] * iv % Q;
            #pragma omp parallel for schedule(static)
            for (long i = 0; i < live; ++i) {
                uint32_t *x = P + i * PB, g = Q - x[t];
                if (is_piv[i] || g == Q) continue;
                for (long u = t; u < w; ++u) x[u] = (x[u] + g * p[u]) % Q;
            }
        }
        pp->b = b;
        if (b == 0) continue;
        static uint32_t A[PB][2 * PB];
        for (long a = 0; a < b; ++a)
            for (long e = 0; e < 2 * b; ++e)
                A[a][e] = e < b ? P0[prow[a] * PB + pcol[e]] : (e - b == a);
        for (long col = 0; col < b; ++col) {
            long piv = col;
            while (!A[piv][col]) ++piv;
            for (long e = 0; e < 2 * b; ++e) { uint32_t t = A[piv][e]; A[piv][e] = A[col][e]; A[col][e] = t; }
            uint32_t iv = inv(A[col][col]);
            for (long e = 0; e < 2 * b; ++e) A[col][e] = A[col][e] * iv % Q;
            for (long a = 0; a < b; ++a)
                if (a != col && A[a][col]) {
                    uint32_t g = Q - A[a][col];
                    for (long e = 0; e < 2 * b; ++e) A[a][e] = (A[a][e] + g * A[col][e]) % Q;
                }
        }
        for (long a = 0; a < b; ++a) { pp->pcol[a] = pcol[a]; pp->prow[a] = row[prow[a]]; for (long e = 0; e < b; ++e) pp->Ainv[a][e] = A[a][b + e]; }
        #pragma omp parallel for schedule(static)
        for (long i = 0; i < live; ++i) {
            if (is_piv[i]) continue;
            for (long e = 0; e < b; ++e) {
                uint64_t s = 0;
                for (long a = 0; a < b; ++a) s += (uint64_t)P0[i * PB + pcol[a]] * A[a][b + e];
                mul[i * PB + e] = (uint32_t)((Q - s % Q) % Q);
            }
        }
        long cs = c0 + w;
#ifdef __AVX512VNNI__
        long pairs = (b + 1) / 2, cw = ((c - cs) + 15) & ~15L;
        int16_t *IL = aligned_alloc(64, ((size_t)pairs * cw * 2 * sizeof(int16_t) + 63) & ~(size_t)63);
        #pragma omp parallel for schedule(static)
        for (long e2 = 0; e2 < pairs; ++e2)
            for (long j = 0; j < cw; ++j) {
                long e = 2 * e2;
                IL[(e2 * cw + j) * 2] = j < c - cs ? (int16_t)row[prow[e]][cs + j] : 0;
                IL[(e2 * cw + j) * 2 + 1] = (e + 1 < b && j < c - cs) ? (int16_t)row[prow[e + 1]][cs + j] : 0;
            }
        #pragma omp parallel
        {
            int32_t acc[TJ] __attribute__((aligned(64)));
            #pragma omp for schedule(static)
            for (long i = 0; i < live; ++i) {
                if (is_piv[i]) continue;
                const uint32_t *m = mul + i * PB;
                for (long j0 = 0; j0 < c - cs; j0 += TJ) {
                    long jn = c - cs - j0 < TJ ? c - cs - j0 : TJ, jv = (jn + 15) & ~15L;
                    uint16_t *x = row[i] + cs + j0;
                    for (long j = 0; j < jn; ++j) acc[j] = x[j];
                    for (long j = jn; j < jv; ++j) acc[j] = 0;
                    for (long e2 = 0; e2 < pairs; ++e2) {
                        uint32_t g0 = m[2 * e2], g1 = 2 * e2 + 1 < b ? m[2 * e2 + 1] : 0;
                        if (!g0 && !g1) continue;
                        __m512i gg = _mm512_set1_epi32((int)(g0 | g1 << 16));
                        const int16_t *pp2 = IL + (e2 * cw + j0) * 2;
                        for (long j = 0; j < jv; j += 16) {
                            __m512i a = _mm512_load_si512((const void *)(acc + j));
                            __m512i v = _mm512_loadu_si512((const void *)(pp2 + 2 * j));
                            _mm512_store_si512((void *)(acc + j), _mm512_dpwssd_epi32(a, gg, v));
                        }
                    }
                    for (long j = 0; j < jn; ++j) x[j] = (uint16_t)((uint32_t)acc[j] % Q);
                }
            }
        }
        free(IL);
#else
        #pragma omp parallel
        {
            uint32_t acc[TJ];
            #pragma omp for schedule(static)
            for (long i = 0; i < live; ++i) {
                if (is_piv[i]) continue;
                const uint32_t *m = mul + i * PB;
                for (long j0 = cs; j0 < c; j0 += TJ) {
                    long jn = c - j0 < TJ ? c - j0 : TJ;
                    uint16_t *x = row[i] + j0;
                    for (long j = 0; j < jn; ++j) acc[j] = x[j];
                    for (long e = 0; e < b; ++e) {
                        uint32_t g = m[e];
                        if (!g) continue;
                        const uint16_t *p = row[prow[e]] + j0;
                        for (long j = 0; j < jn; ++j) acc[j] += g * p[j];
                    }
                    for (long j = 0; j < jn; ++j) x[j] = (uint16_t)(acc[j] % Q);
                }
            }
        }
#endif
        long o = 0;
        for (long i = 0; i < live; ++i) if (!is_piv[i]) row[o++] = row[i];
        live = o; rank += b;
    }
    free(row); free(P); free(P0); free(mul); free(is_piv);
    *pan_out = pan; *npan_out = npan;
    return rank;
}

/* Right kernel of the eliminated c-column matrix: returns F, KX = F x c (uint16). */
static long kernel_backsub(panel_t *pan, long npan, long c, uint16_t **KX_out) {
    char *isfree = malloc(c); memset(isfree, 1, c);
    for (long p = 0; p < npan; ++p) for (long a = 0; a < pan[p].b; ++a) isfree[pan[p].c0 + pan[p].pcol[a]] = 0;
    long F = 0; for (long j = 0; j < c; ++j) F += isfree[j];
    long *fc = malloc((F + 1) * sizeof *fc), u = 0;
    for (long j = 0; j < c; ++j) if (isfree[j]) fc[u++] = j;
    uint16_t *KX = calloc((size_t)F * c + 1, 2);
    for (long f = 0; f < F; ++f) KX[(size_t)f * c + fc[f]] = 1;
    for (long p = npan - 1; p >= 0; --p) {
        panel_t *pp = &pan[p]; long b = pp->b, c0 = pp->c0, len = c - c0;
        if (!b) continue;
        #pragma omp parallel for schedule(dynamic, 1)
        for (long f = 0; f < F; ++f) {
            uint16_t *x = KX + (size_t)f * c;
            uint32_t rhs[PB];
            for (long a = 0; a < b; ++a) {
                const uint16_t *U = pp->prow[a] + c0, *xx = x + c0;
                uint64_t s = 0;
                for (long j0 = 0; j0 < len; j0 += 16384) {
                    long jn = len - j0 < 16384 ? len - j0 : 16384; uint32_t s32 = 0;
                    for (long j = 0; j < jn; ++j) s32 += (uint32_t)U[j0 + j] * xx[j0 + j];
                    s += s32 % Q;
                }
                rhs[a] = (uint32_t)((Q - s % Q) % Q);
            }
            for (long e = 0; e < b; ++e) {
                uint64_t s = 0;
                for (long a = 0; a < b; ++a) s += (uint64_t)pp->Ainv[e][a] * rhs[a];
                x[c0 + pp->pcol[e]] = (uint16_t)(s % Q);
            }
        }
    }
    free(isfree); free(fc);
    *KX_out = KX; return F;
}

static int read_pk(uint32_t *G, long n, long k, const char *path) {
    long tc = n - k, tot = k * tc, idx = 0, bits = 0;
    uint32_t acc = 0, *t = malloc(tot * sizeof *t);
    int ch; FILE *f = fopen(path, "rb");
    if (!f) return 0;
    while (idx < tot && (ch = fgetc(f)) != EOF) {
        acc = (acc << 8) | (uint32_t)ch; bits += 8;
        if (bits >= QBITS) { t[idx++] = (acc >> (bits - QBITS)) & ((1u << QBITS) - 1); bits -= QBITS; }
    }
    fclose(f);
    if (idx != tot) return 0;
    for (long i = 0; i < tot; ++i) if (t[i] >= Q) return 0;
    for (long i = 0; i < k; ++i) { G[i * n + i] = 1; for (long j = 0; j < tc; ++j) G[i * n + k + j] = t[i * tc + j]; }
    free(t); return 1;
}

/* Mobius x -> (a x + b)/(c x + d) on P^1 (INF = Q) */
static uint32_t mob(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x) {
    if (x == INF) return c ? a * inv(c) % Q : INF;
    uint32_t den = (c * x + d) % Q, num = (a * x + b) % Q;
    if (!den) return INF;
    return num * inv(den) % Q;
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: uvw_key_recovery pk.bin k_l seed maxrounds [truth.txt|-] [struct_out]\n"); return 2; }
    init_inv();
    const long n = 860, k = 430, h = 430, k2 = 215;
    long kl = atol(argv[2]), maxr = atol(argv[4]);
    rs = strtoull(argv[3], NULL, 0) * 0x1234567ULL + 99;
    long l = k - kl, np = n - l;
    uint32_t *G0 = calloc((size_t)k * n, 4), *G = malloc((size_t)k * n * 4);
    if (!read_pk(G0, n, k, argv[1])) { puts("ERROR: unreadable pk"); return 2; }
    double t_start = now();
    long nm = kl * (kl + 1) * (kl + 2) / 6, R = np * kl;
    printf("pk=%s n=%ld k=%ld k_l=%ld n'=%ld rows(R)=%ld monomials=%ld generic_kernel=%ld threads=%d\n",
           argv[1], n, k, kl, np, R, nm, nm > R ? nm - R : 0, omp_get_max_threads());
    long (*mon)[3] = malloc(nm * sizeof *mon), u = 0;
    for (long a = 0; a < kl; ++a) for (long b = a; b < kl; ++b) for (long c = b; c < kl; ++c) { mon[u][0] = a; mon[u][1] = b; mon[u][2] = c; ++u; }
    long *ftw = malloc(n * sizeof *ftw); uint32_t *frat = calloc(n, 4);
    for (long j = 0; j < n; ++j) ftw[j] = -1;
    long found = 0, incons = 0, round;
    long *ord = malloc(n * sizeof *ord); uint32_t *X = malloc((size_t)np * kl * 4);
    uint16_t *MT = malloc((size_t)nm * R * 2);
    if (!MT) { puts("ERROR: oom"); return 2; }
    for (round = 1; round <= maxr && found < h; ++round) {
        double t0 = now();
        memcpy(G, G0, (size_t)k * n * 4);
        for (long j = 0; j < n; ++j) ord[j] = j;
        for (long j = n - 1; j > 0; --j) { long r = (long)(rnd64() % (j + 1)), t = ord[j]; ord[j] = ord[r]; ord[r] = t; }
        { long w = 0; long *tmpo = malloc(n * sizeof *tmpo);  /* adaptive: paired columns are shortened first */
          for (long j = 0; j < n; ++j) if (ftw[ord[j]] >= 0) tmpo[w++] = ord[j];
          for (long j = 0; j < n; ++j) if (ftw[ord[j]] < 0) tmpo[w++] = ord[j];
          memcpy(ord, tmpo, n * sizeof *ord); free(tmpo); }
        long unknown_kept = 0; for (long j = l; j < n; ++j) unknown_kept += ftw[ord[j]] < 0;
        long rank = 0, bad = 0;
        for (long s = 0; s < l; ++s) {
            long col = ord[s], piv = -1;
            for (long i = rank; i < k; ++i) if (G[i * n + col]) { piv = i; break; }
            if (piv < 0) { bad = 1; break; }
            for (long j = 0; j < n; ++j) { uint32_t t = G[piv * n + j]; G[piv * n + j] = G[rank * n + j]; G[rank * n + j] = t; }
            uint32_t iv = inv(G[rank * n + col]);
            for (long j = 0; j < n; ++j) G[rank * n + j] = G[rank * n + j] * iv % Q;
            #pragma omp parallel for schedule(static)
            for (long i = 0; i < k; ++i) if (i != rank && G[i * n + col]) {
                uint32_t f = Q - G[i * n + col];
                for (long j = 0; j < n; ++j) G[i * n + j] = (G[i * n + j] + f * G[rank * n + j]) % Q;
            }
            ++rank;
        }
        if (bad) { printf("round %ld: degenerate shortening, skipped\n", round); continue; }
        for (long j = 0; j < np; ++j) for (long t = 0; t < kl; ++t) X[j * kl + t] = G[(rank + t) * n + ord[l + j]];
        /* M^T: rows = monomials, columns = (column j, coordinate t) */
        #pragma omp parallel for schedule(static)
        for (long m = 0; m < nm; ++m) {
            uint16_t *row = MT + (size_t)m * R; memset(row, 0, R * 2);
            long a = mon[m][0], b = mon[m][1], c = mon[m][2];
            for (long j = 0; j < np; ++j) {
                const uint32_t *x = X + j * kl;
                row[j * kl + a] = (row[j * kl + a] + x[b] * x[c]) % Q;
                row[j * kl + b] = (row[j * kl + b] + x[a] * x[c]) % Q;
                row[j * kl + c] = (row[j * kl + c] + x[a] * x[b]) % Q;
            }
        }
        double t1 = now();
        panel_t *pan; long npan;
        long rk = elim_record(MT, nm, R, &pan, &npan);
        double t2 = now();
        uint16_t *KX; long dL = kernel_backsub(pan, npan, R, &KX);
        free(pan);
        double t3 = now();
        /* verify every kernel vector: sum_j Sym(x_j x_j lambda_j) == 0 on all monomials */
        long kbad = 0;
        #pragma omp parallel for schedule(dynamic, 1) reduction(+:kbad)
        for (long f = 0; f < dL; ++f) {
            const uint16_t *lam = KX + (size_t)f * R;
            for (long m = 0; m < nm; ++m) {
                long a = mon[m][0], b = mon[m][1], c = mon[m][2]; uint64_t s = 0;
                for (long j = 0; j < np; ++j) {
                    const uint32_t *x = X + j * kl; const uint16_t *L = lam + j * kl;
                    s += (uint64_t)L[a] * (x[b] * x[c] % Q) + (uint64_t)L[b] * (x[a] * x[c] % Q) + (uint64_t)L[c] * (x[a] * x[b] % Q);
                }
                if (s % Q) { kbad++; break; }
            }
        }
        double t4 = now();
        /* column blocks */
        long *rb = malloc(np * sizeof *rb); uint32_t *cv = calloc((size_t)np * (dL + 1), 4), *mu = calloc((size_t)np * kl, 4);
        long rhist[8] = {0};
        #pragma omp parallel for schedule(dynamic, 4)
        for (long m = 0; m < np; ++m) {
            uint32_t *Lc = malloc((size_t)dL * kl * 4 + 4);
            for (long d = 0; d < dL; ++d) for (long t = 0; t < kl; ++t) Lc[d * kl + t] = KX[(size_t)d * R + m * kl + t];
            rb[m] = rank_small(Lc, dL, kl);
            if (rb[m] == 1) {
                long t0_ = -1, d0 = -1;
                for (long t = 0; t < kl && t0_ < 0; ++t) for (long d = 0; d < dL; ++d) if (Lc[d * kl + t]) { t0_ = t; break; }
                for (long d = 0; d < dL; ++d) if (Lc[d * kl + t0_]) { d0 = d; break; }
                uint32_t iv = inv(Lc[d0 * kl + t0_]);
                for (long d = 0; d < dL; ++d) cv[m * (dL + 1) + d] = Lc[d * kl + t0_] * iv % Q;
                memcpy(mu + m * kl, Lc + d0 * kl, kl * 4);
            }
            free(Lc);
        }
        for (long m = 0; m < np; ++m) rhist[rb[m] < 7 ? rb[m] : 7]++;
        long newp = 0, grp2 = 0, grpbad = 0, linefail = 0, lineamb = 0;
        for (long a = 0; a < np; ++a) if (rb[a] == 1) {
            long partner = -1, cnt = 0;
            for (long b = 0; b < np; ++b) if (b != a && rb[b] == 1 && !memcmp(cv + a * (dL + 1), cv + b * (dL + 1), dL * 4)) { partner = b; cnt++; }
            if (cnt != 1) { grpbad++; continue; }
            if (partner < a) continue;
            grp2++;
            long i = a, j = partner; uint32_t *xi = X + i * kl, *xj = X + j * kl, *mi = mu + i * kl;
            /* line check: x_j in span(x_i, mu_i), and x_j is the only such column */
            long hits = 0, hitj = -1;
            for (long j2 = 0; j2 < np; ++j2) if (j2 != i) {
                uint32_t B3[3 * 128]; for (long t = 0; t < kl; ++t) { B3[t] = xi[t]; B3[kl + t] = mi[t]; B3[2 * kl + t] = X[j2 * kl + t]; }
                if (rank_small(B3, 3, kl) == 2) { hits++; hitj = j2; }
            }
            if (hitj != j && hits >= 1) linefail++;
            if (hits == 0) linefail++;
            if (hits > 1) lineamb++;
            uint32_t ra = 0, rbb = 0; int got = 0;
            for (long p = 0; p < kl && !got; ++p) for (long r2 = p + 1; r2 < kl && !got; ++r2) {
                uint32_t det = (xi[p] * xj[r2] % Q + Q - xi[r2] * xj[p] % Q) % Q; if (!det) continue;
                uint32_t id = inv(det);
                ra = (mi[p] * xj[r2] % Q + Q - mi[r2] * xj[p] % Q) % Q * id % Q;
                rbb = (xi[p] * mi[r2] % Q + Q - xi[r2] * mi[p] % Q) % Q * id % Q; got = 1;
            }
            if (!got || !rbb) { grpbad++; continue; }
            uint32_t rho = (Q - 3 * ra % Q) % Q * inv(rbb) % Q; /* d_j / d_i */
            long gi = ord[l + i], gj = ord[l + j];
            if (ftw[gi] < 0 && ftw[gj] < 0) { ftw[gi] = gj; ftw[gj] = gi; frat[gi] = rho; frat[gj] = inv(rho); found++; newp++; }
            else if (ftw[gi] != gj || frat[gi] != rho) incons++;
        }
        double t5 = now();
        printf("round %ld: rank=%ld leftker=%ld kernel_check_fail=%ld | block ranks 1:%ld 2:%ld other:%ld | size2groups=%ld nonpair_rank1=%ld line_fail=%ld line_ambig=%ld | unknown_cols_kept=%ld new=%ld total=%ld/%ld inconsistent=%ld\n",
               round, rk, dL, kbad, rhist[1], rhist[2], np - rhist[1] - rhist[2], grp2, grpbad, linefail, lineamb, unknown_kept, newp, found, h, incons);
        printf("  timing: shorten+build %.1fs  elimination %.1fs  backsub %.1fs  verify %.1fs  blocks+pairing %.1fs  round total %.1fs\n",
               t1 - t0, t2 - t1, t3 - t2, t4 - t3, t5 - t4, t5 - t0);
        fflush(stdout);
        free(rb); free(cv); free(mu); free(KX);
    }
    free(MT);
    double t_pair = now();
    /* ---------- difference code ---------- */
    long P = 0, *pi_ = malloc(h * sizeof *pi_), *pj_ = malloc(h * sizeof *pj_);
    for (long i = 0; i < n; ++i) if (ftw[i] > i) { pi_[P] = i; pj_[P] = ftw[i]; ++P; }
    uint32_t *Dc = malloc((size_t)k * P * 4 + 4);
    for (long p = 0; p < P; ++p) { uint32_t f = inv(frat[pi_[p]]); for (long r = 0; r < k; ++r) Dc[r * P + p] = (G0[r * n + pi_[p]] + Q - G0[r * n + pj_[p]] * f % Q) % Q; }
    long *pc = malloc((P + 1) * sizeof *pc); long rD = rref32(Dc, k, P, pc);
    long ns = rD * (rD + 1) / 2; uint32_t *Sq = malloc((size_t)ns * P * 4 + 4); u = 0;
    for (long a = 0; a < rD; ++a) for (long b = a; b < rD; ++b) { for (long c = 0; c < P; ++c) Sq[u * P + c] = Dc[a * P + c] * Dc[b * P + c] % Q; ++u; }
    long rS = rref32(Sq, ns, P, NULL); free(Sq);
    printf("PAIRING DONE: pairs=%ld/%ld rounds=%ld inconsistent=%ld time=%.1fs\n", P, h, round - 1, incons, t_pair - t_start);
    printf("DIFFERENCE CODE: length=%ld dim=%ld (k2=%ld) square_dim=%ld (GRS: 2*dim-1=%ld; random: min(len,dim(dim+1)/2)=%ld)\n",
           P, rD, k2, rS, 2 * rD - 1, P < ns ? P : ns);
    /* ---------- Sidelnikov-Shestakov on the difference code ---------- */
    long ncand = 0; uint32_t (*alc)[430] = NULL, (*vc)[430] = NULL; long *cand_ok = NULL;
    if (P == h && rD == k2) {
        /* Dc rows 0..rD-1 are systematic at pivots pc[i]. */
        char *isinfo = calloc(P, 1); for (long i = 0; i < rD; ++i) isinfo[pc[i]] = 1;
        long *nq = malloc(P * sizeof *nq), nnq = 0; for (long q = 0; q < P; ++q) if (!isinfo[q]) nq[nnq++] = q;
        uint32_t *beta = malloc(P * 4); long ssbad = 0;
        for (long t = 0; t < nnq; ++t) { long q = nq[t]; if (!Dc[q] || !Dc[P + q]) ssbad++; beta[q] = Dc[q] * inv(Dc[P + q]) % Q; }
        beta[pc[0]] = INF; beta[pc[1]] = 0;
        long qa = nq[0], qb = nq[1];
        for (long i = 2; i < rD; ++i) {
            uint32_t ra = Dc[qa] * inv(Dc[i * P + qa]) % Q, rb2 = Dc[qb] * inv(Dc[i * P + qb]) % Q;
            uint32_t E = (ra + Q - rb2) % Q * inv((beta[qa] + Q - beta[qb]) % Q) % Q;
            uint32_t bi = (beta[qa] + Q - ra * inv(E) % Q) % Q; beta[pc[i]] = bi;
            for (long t = 2; t < nnq; ++t) { long q = nq[t]; if (Dc[q] * inv(Dc[i * P + q]) % Q != E * ((beta[q] + Q - bi) % Q) % Q) { ssbad++; break; } }
        }
        char used[Q + 1] = {0}; long dup = 0; for (long p = 0; p < P; ++p) { if (used[beta[p]]) dup++; used[beta[p]] = 1; }
        uint32_t comp[8]; long nc = 0; for (uint32_t x = 0; x <= Q; ++x) if (!used[x] && nc < 8) comp[nc++] = x;
        printf("SS: cauchy/linearity violations=%ld duplicate_beta=%ld complement_size=%ld\n", ssbad, dup, nc);
        /* Mobius maps psi with psi(complement) = {0, 431, 432, INF} (true complement of alpha = 1..430) */
        uint32_t tgt[4] = {0, 431, 432, INF};
        alc = malloc(64 * sizeof *alc); vc = malloc(64 * sizeof *vc); cand_ok = calloc(64, sizeof *cand_ok);
        if (nc == 4) {
            for (uint32_t c = 0; c <= 1; ++c) for (uint32_t a = 0; a < Q; ++a) for (uint32_t b = 0; b < Q; ++b) for (uint32_t d = 0; d < Q; ++d) {
                if (c == 0 && d != 1) continue;
                if ((a * d % Q + Q - b * c % Q) % Q == 0) continue;
                int ok = 1;
                for (int z = 0; z < 4 && ok; ++z) { uint32_t y = mob(a, b, c, d, comp[z]); ok = (y == tgt[0] || y == tgt[1] || y == tgt[2] || y == tgt[3]); }
                if (!ok || ncand >= 64) continue;
                for (long p = 0; p < P; ++p) alc[ncand][p] = mob(a, b, c, d, beta[p]);
                /* multipliers: row 0 codeword = v * f0(alpha), f0 = prod_{i>=1}(x - alpha_{p_i}) */
                uint32_t *al = alc[ncand], *v = vc[ncand];
                for (long q = 0; q < P; ++q) {
                    if (isinfo[q] && q != pc[0]) continue;
                    uint32_t f = 1; for (long i = 1; i < rD; ++i) f = f * ((al[q] + Q - al[pc[i]]) % Q) % Q;
                    v[q] = Dc[q] * inv(f) % Q;
                }
                for (long i = 1; i < rD; ++i) {
                    long q = qa; uint32_t fq = 1, fp = 1;
                    for (long l2 = 0; l2 < rD; ++l2) if (l2 != i) { fq = fq * ((al[q] + Q - al[pc[l2]]) % Q) % Q; fp = fp * ((al[pc[i]] + Q - al[pc[l2]]) % Q) % Q; }
                    uint32_t si = Dc[i * P + q] * inv(v[q] * fq % Q) % Q;
                    v[pc[i]] = inv(si * fp % Q);
                }
                /* verify: rank [Dc ; diag(v) Vandermonde_k2(alpha)] == k2 */
                uint32_t *VV = malloc((size_t)2 * rD * P * 4);
                memcpy(VV, Dc, (size_t)rD * P * 4);
                for (long p = 0; p < P; ++p) { uint32_t x = v[p]; for (long r = 0; r < rD; ++r) { VV[(rD + r) * P + p] = x; x = x * al[p] % Q; } }
                long rv = rref32(VV, 2 * rD, P, NULL); free(VV);
                cand_ok[ncand] = (rv == rD);
                ++ncand;
            }
        }
        long nok = 0; for (long z = 0; z < ncand; ++z) nok += cand_ok[z];
        printf("SS: Mobius candidates mapping complement->{0,431,432,inf}: %ld, of which GRS(alpha,v)==difference code verified: %ld\n", ncand, nok);
        free(isinfo); free(nq); free(beta);
    }
    double t_end = now();
    if (argc > 6) { /* recovered structure: public-key-derived only */
        FILE *of = fopen(argv[6], "w");
        long nok = 0; for (long z = 0; z < ncand; ++z) nok += cand_ok[z];
        fprintf(of, "%ld %ld\n", P, nok);
        for (long p = 0; p < P; ++p) fprintf(of, "%ld %ld %u\n", pi_[p], pj_[p], frat[pi_[p]]);
        for (long z = 0; z < ncand; ++z) if (cand_ok[z]) {
            for (long p = 0; p < P; ++p) fprintf(of, "%u %u\n", alc[z][p], vc[z][p]);
        }
        fclose(of);
    }
    printf("TOTAL attack time %.1fs (pairing %.1fs, difference code + SS %.1fs)\n", t_end - t_start, t_pair - t_start, t_end - t_pair);
    /* ---------- scoring (truth read only now) ---------- */
    if (argc > 5 && strcmp(argv[5], "-")) {
        FILE *tf = fopen(argv[5], "r"); long Dp[860]; uint32_t Dd[860];
        for (long c = 0; c < n; ++c) { long cc, p_; unsigned dd; if (fscanf(tf, "%ld %ld %u", &cc, &p_, &dd) != 3) return 4; Dp[cc] = p_; Dd[cc] = dd; }
        fclose(tf);
        long *ttw = malloc(n * sizeof *ttw), inv_perm[860];
        for (long c = 0; c < n; ++c) inv_perm[Dp[c]] = c;
        for (long c = 0; c < n; ++c) ttw[c] = inv_perm[Dp[c] < h ? Dp[c] + h : Dp[c] - h];
        long okp = 0, okr = 0, wrong = 0;
        for (long p = 0; p < P; ++p) { long i = pi_[p], j = pj_[p]; if (ttw[i] == j) { okp++; if (frat[i] == Dd[j] * inv(Dd[i]) % Q) okr++; } else wrong++; }
        printf("SCORE pairing: recovered=%ld correct=%ld wrong=%ld twin_ratios_correct=%ld (of %ld true pairs)\n", P, okp, wrong, okr, h);
        for (long z = 0; z < ncand; ++z) {
            long aok = 0, vconst = 1; uint32_t ratio0 = 0;
            for (long p = 0; p < P; ++p) {
                long i = pi_[p], left = Dp[i] < h ? i : pj_[p];
                uint32_t atrue = (uint32_t)(Dp[left] % h) + 1;
                if (alc[z][p] == atrue) aok++;
                uint32_t vtrue = Dp[i] < h ? Dd[i] : (Q - Dd[i]) % Q;   /* delta_p = c_i - c_j d_i/d_j = +-d_i * GRS column */
                uint32_t r = vc[z][p] * inv(vtrue) % Q;
                if (p == 0) ratio0 = r; else if (r != ratio0) vconst = 0;
            }
            printf("SCORE SS candidate %ld (verified=%ld): alpha correct %ld/%ld, multipliers proportional to secret: %s\n", z, cand_ok[z], aok, P, vconst ? "yes" : "no");
        }
    }
    return 0;
}
