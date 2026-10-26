/* UVW-KEM: an order-2 cubic "jet" count distinguishes the public code.
 *
 * usage: jet n k_l mode seed [pk-file]
 *   mode 0  model key [[GU,GU],[GRS+GW,GW]]D (k1 = k2 = n/4)
 *   mode 1  same twin shape with GRS replaced by a random block (control)
 *   mode 2  uniform random [n, n/2] code (control)
 *   mode 3  real public key (I_k | T), T packed MSB-first as in the archive
 * Compile with -DQ=<q> -DQBITS=<ceil(log2 q)> (default UVW-128: 433, 9).
 *
 * The code is shortened at n/2 - k_l random positions, giving n' columns as
 * points in F_q^k_l. The program counts cubic forms in k_l variables whose
 * gradient vanishes at every column; by Euler's identity they also vanish
 * there, to order two. For random columns the count is
 * max(0, C(k_l+2,3) - n' k_l). The public computation is one rank.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __AVX512VNNI__
#include <immintrin.h>
#endif

#ifndef Q
#define Q 433u
#endif
#ifndef QBITS
#define QBITS 9
#endif

static uint64_t rs;
static uint64_t rnd64(void) {
    uint64_t z = (rs += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
static uint32_t rq(void) { return (uint32_t)(rnd64() % Q); }
static uint32_t inv(uint32_t a) {
    uint32_t r = 1, e = Q - 2;
    while (e) { if (e & 1) r = r * a % Q; a = a * a % Q; e >>= 1; }
    return r;
}
static void shuffle(long *p, long n) {
    for (long j = 0; j < n; ++j) p[j] = j;
    for (long j = n - 1; j > 0; --j) {
        long r = (long)(rnd64() % (unsigned long)(j + 1)), t = p[j];
        p[j] = p[r]; p[r] = t;
    }
}

/* Rank of an r x c matrix over F_Q (row-major uint16, destroyed).
 * Blocked elimination: each panel of PB columns is reduced on a copy to find
 * its pivot rows; the other rows are then cleared of the panel by one
 * matrix product with the original pivot rows, accumulated without
 * reduction (PB (Q-1)^2 + Q < 2^32 for Q < 2^11) and reduced once per tile.
 */
#define PB 128
#define TJ 1024
static long rank_mod(uint16_t *M, long r, long c) {
    uint16_t **row = malloc(r * sizeof *row);
    uint32_t *P = malloc((size_t)r * PB * 4), *P0 = malloc((size_t)r * PB * 4);
    uint32_t *mul = malloc((size_t)r * PB * 4);
    char *is_piv = malloc(r);
    long live = r, rank = 0;
    for (long i = 0; i < r; ++i) row[i] = M + (size_t)i * c;
    for (long c0 = 0; c0 < c && live > 0; c0 += PB) {
        long w = c - c0 < PB ? c - c0 : PB, b = 0, prow[PB], pcol[PB];
        #pragma omp parallel for schedule(static)
        for (long i = 0; i < live; ++i) {
            is_piv[i] = 0;
            for (long t = 0; t < w; ++t) P[i * PB + t] = P0[i * PB + t] = row[i][c0 + t];
        }
        /* Pivot rows of the panel. */
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
        if (b == 0) continue;
        /* A = original pivot block; multipliers m_i = P0_i[pcol] A^-1. */
        uint32_t A[PB][2 * PB];
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
        #pragma omp parallel for schedule(static)
        for (long i = 0; i < live; ++i) {
            if (is_piv[i]) continue;
            for (long e = 0; e < b; ++e) {
                uint64_t s = 0;
                for (long a = 0; a < b; ++a) s += (uint64_t)P0[i * PB + pcol[a]] * A[a][b + e];
                mul[i * PB + e] = (uint32_t)((Q - s % Q) % Q);
            }
        }
        /* Trailing update with the original pivot rows. */
        long cs = c0 + w;
#ifdef __AVX512VNNI__
        /* Pair consecutive pivot rows so one vpdpwssd adds 32 products. */
        long pairs = (b + 1) / 2, cw = ((c - cs) + 15) & ~15L;
        int16_t *IL = aligned_alloc(64, (size_t)pairs * cw * 2 * sizeof *IL);
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
                        const int16_t *pp = IL + (e2 * cw + j0) * 2;
                        for (long j = 0; j < jv; j += 16) {
                            __m512i a = _mm512_load_si512((const void *)(acc + j));
                            __m512i v = _mm512_loadu_si512((const void *)(pp + 2 * j));
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
        /* Drop the pivot rows. */
        long o = 0;
        for (long i = 0; i < live; ++i) if (!is_piv[i]) row[o++] = row[i];
        live = o; rank += b;
    }
    free(row); free(P); free(P0); free(mul); free(is_piv);
    return rank;
}

static int read_pk(uint32_t *T, long n, long k, const char *path) {
    long tc = n - k, tot = k * tc, idx = 0, bits = 0;
    uint32_t acc = 0, *t = malloc(tot * sizeof *t);
    int ch;
    FILE *f = fopen(path, "rb");
    if (!f || !t) return 0;
    while (idx < tot && (ch = fgetc(f)) != EOF) {
        acc = (acc << 8) | (uint32_t)ch; bits += 8;
        if (bits >= QBITS) { t[idx++] = (acc >> (bits - QBITS)) & ((1u << QBITS) - 1); bits -= QBITS; }
    }
    fclose(f);
    if (idx != tot) return 0;
    for (long i = 0; i < tot; ++i) if (t[i] >= Q) return 0;
    for (long i = 0; i < k; ++i) {
        T[i * n + i] = 1;
        for (long j = 0; j < tc; ++j) T[i * n + k + j] = t[i * tc + j];
    }
    free(t);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 5) return 2;
    long n = atol(argv[1]), kl = atol(argv[2]);
    int mode = atoi(argv[3]);
    rs = strtoull(argv[4], NULL, 0) * 0x1234567ULL + 99;
    long h = n / 2, k = h, k1 = h / 2, k2 = h - k1, l = k - kl, np = n - l;
    if (kl < 1 || kl > k || (mode == 3 && argc < 6) || (mode != 3 && h > (long)Q)) return 2;
    uint32_t *T = calloc((size_t)k * n, 4), *G = calloc((size_t)k * n, 4);
    long *perm = malloc(n * sizeof *perm), *ord = malloc(n * sizeof *ord);

    if (mode == 3) {
        if (!read_pk(T, n, k, argv[5])) { puts("ERROR: unreadable public key"); return 2; }
    } else if (mode == 2) {
        for (long i = 0; i < k * n; ++i) T[i] = rq();
    } else {
        uint32_t *al = malloc(h * 4), *v = malloc(h * 4);
        char *used = calloc(Q, 1);
        for (long j = 0; j < h; ++j) {
            uint32_t a;
            do a = rq(); while (used[a]);
            used[a] = 1; al[j] = a; v[j] = 1 + rq() % (Q - 1);
        }
        for (long i = 0; i < k1; ++i)
            for (long j = 0; j < h; ++j) T[i * n + j] = T[i * n + h + j] = rq();
        for (long i = 0; i < k2; ++i)
            for (long j = 0; j < h; ++j) {
                uint32_t w = rq(), x = v[j];
                if (mode == 0) for (long e = 0; e < i; ++e) x = x * al[j] % Q;
                else x = rq();
                T[(k1 + i) * n + j] = (x + w) % Q;
                T[(k1 + i) * n + h + j] = w;
            }
    }
    /* Monomial map D (a no-op for the counts; also applied to a real key). */
    shuffle(perm, n);
    for (long j = 0; j < n; ++j) {
        uint32_t d = 1 + rq() % (Q - 1);
        for (long i = 0; i < k; ++i) G[i * n + j] = T[i * n + perm[j]] * d % Q;
    }
    /* Shorten at the first l positions of a random order. */
    shuffle(ord, n);
    long rank = 0;
    for (long s = 0; s < l; ++s) {
        long col = ord[s], piv = -1;
        for (long i = rank; i < k; ++i) if (G[i * n + col]) { piv = i; break; }
        if (piv < 0) { puts("ERROR: degenerate shortening"); return 2; }
        for (long j = 0; j < n; ++j) {
            uint32_t t = G[piv * n + j]; G[piv * n + j] = G[rank * n + j]; G[rank * n + j] = t;
        }
        uint32_t iv = inv(G[rank * n + col]);
        for (long j = 0; j < n; ++j) G[rank * n + j] = G[rank * n + j] * iv % Q;
        for (long i = 0; i < k; ++i)
            if (i != rank && G[i * n + col]) {
                uint32_t f = Q - G[i * n + col];
                for (long j = 0; j < n; ++j) G[i * n + j] = (G[i * n + j] + f * G[rank * n + j]) % Q;
            }
        ++rank;
    }
    /* Columns of the shortened code: rows rank..k-1 at positions ord[l..]. */
    uint32_t *X = malloc((size_t)np * kl * 4);
    for (long j = 0; j < np; ++j)
        for (long t = 0; t < kl; ++t) X[j * kl + t] = G[(rank + t) * n + ord[l + j]];
    free(G); free(T);

    long nm = kl * (kl + 1) * (kl + 2) / 6, R = np * kl, u = 0;
    long (*mon)[3] = malloc(nm * sizeof *mon);
    for (long a = 0; a < kl; ++a)
        for (long b = a; b < kl; ++b)
            for (long c = b; c < kl; ++c) { mon[u][0] = a; mon[u][1] = b; mon[u][2] = c; ++u; }
    uint16_t *M = calloc((size_t)R * nm, 2);
    if (!M) { puts("ERROR: out of memory"); return 2; }
    for (long j = 0; j < np; ++j) {
        const uint32_t *x = X + j * kl;
        for (long t = 0; t < kl; ++t) {
            uint16_t *row = M + (size_t)(j * kl + t) * nm;
            for (long m = 0; m < nm; ++m) {
                long a = mon[m][0], b = mon[m][1], c = mon[m][2];
                uint32_t s = 0;
                if (a == t) s += x[b] * x[c] % Q;
                if (b == t) s += x[a] * x[c] % Q;
                if (c == t) s += x[a] * x[b] % Q;
                row[m] = (uint16_t)(s % Q);
            }
        }
    }
    long kernel = nm - rank_mod(M, R, nm), generic = nm > R ? nm - R : 0;
    printf("n=%ld q=%u k_l=%ld mode=%d rows=%ld cubic_monomials=%ld kernel=%ld generic=%ld excess=%ld\n",
           n, Q, kl, mode, R, nm, kernel, generic, kernel - generic);
    return 0;
}
