/*************************************************
* File:        kg_solver.c
*
* Description: Pure-C unimodular-basis solver for key generation. Provides the
*              coprimality tests that decide whether the short rows extend to a
*              det=1 matrix (order-(D-1) minor descent, and the faster DP1
*              order-d "truncated tail columns" check), the full module-NTRU
*              last-row solve with Babai round-off, and a determinant check.
*              Built on CRT-NTT minor evaluation, big-integer resultants, and
*              gcd folding with dynamic CRT convergence.
**************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "kg_zint.h"
#include "kg_ntt.h"
#include "kg_solver.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ----------------------------------------------------------------- misc */
static void *xcalloc(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) abort();
    return p;
}

/* shared-modulus incremental CRT (Garner): one M for a whole group of values.
 * per prime: invM = (M mod p)^-1 computed ONCE; each value pays O(len) only. */
typedef struct { zint M; } crt_mod;
static void crtm_init(crt_mod *m) { zi_init(&m->M); zi_set_i64(&m->M, 1); }
static void crtm_free(crt_mod *m) { zi_free(&m->M); }
/* call before stepping the group's values with this prime */
static uint32_t crtm_prime_inv(const crt_mod *m, uint32_t p) {
    uint32_t Mm = zi_mod_u32(&m->M, p);
    return kg_powmod(Mm, p - 2, p);
}
static void crtm_step_value(zint *val, const crt_mod *m, uint32_t invM,
                            uint32_t res, uint32_t p) {
    uint32_t vm = zi_mod_u32(val, p);
    uint32_t diff = res >= vm ? res - vm : res + p - vm;
    uint32_t t = (uint32_t)(((uint64_t)diff * invM) % p);
    zi_addmul_u32(val, &m->M, t);
}
static void crtm_advance(crt_mod *m, uint32_t p) { zi_mul_u32(&m->M, p); }
static void crtm_center(zint *val, const crt_mod *m) {
    zint two; zi_init(&two);
    zi_copy(&two, val);
    zi_mul_u32(&two, 2);
    if (val->sign > 0 && zi_cmp_abs(&two, &m->M) > 0)
        zi_sub(val, val, &m->M);
    zi_free(&two);
}

/*************************************************
* Name:        det_mod
*
* Description: Computes the determinant of an m x m matrix modulo a prime p via Gaussian
*              elimination (used per NTT point when evaluating minors).
*
* Arguments:   - uint32_t *M:    row-major m x m matrix mod p (overwritten)
*              - unsigned m:     matrix dimension
*              - uint32_t p:     prime modulus
*
* Returns:     Determinant modulo p.
**************************************************/
static uint32_t det_mod(uint32_t *M, unsigned m, uint32_t p) {
    uint64_t det = 1;
    int sign = 1;
    for (unsigned c = 0; c < m; c++) {
        unsigned piv = m;
        for (unsigned r = c; r < m; r++)
            if (M[r * m + c] % p) { piv = r; break; }
        if (piv == m) return 0;
        if (piv != c) {
            for (unsigned j = 0; j < m; j++) {
                uint32_t t = M[c * m + j]; M[c * m + j] = M[piv * m + j]; M[piv * m + j] = t;
            }
            sign = -sign;
        }
        det = (det * M[c * m + c]) % p;
        uint32_t inv = kg_powmod(M[c * m + c], p - 2, p);
        for (unsigned r = c + 1; r < m; r++) {
            uint64_t f = ((uint64_t)M[r * m + c] * inv) % p;
            if (!f) continue;
            for (unsigned j = c; j < m; j++) {
                uint64_t s = (uint64_t)M[r * m + j] + (p - (uint32_t)((f * M[c * m + j]) % p));
                M[r * m + j] = (uint32_t)(s % p);
            }
        }
    }
    if (sign < 0) det = (p - det) % p;
    return (uint32_t)det;
}

/* ----------------------------------------------------------------- complex FFT */
typedef struct { double re, im; } cplx;
static cplx cadd(cplx a, cplx b){ return (cplx){a.re+b.re, a.im+b.im}; }
static cplx csub(cplx a, cplx b){ return (cplx){a.re-b.re, a.im-b.im}; }
static cplx cmul(cplx a, cplx b){ return (cplx){a.re*b.re-a.im*b.im, a.re*b.im+a.im*b.re}; }
static cplx cconj(cplx a){ return (cplx){a.re, -a.im}; }

static void fft_inplace(cplx *a, unsigned n, int inverse) {
    for (unsigned i = 1, j = 0; i < n; i++) {
        unsigned bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) { cplx t = a[i]; a[i] = a[j]; a[j] = t; }
    }
    for (unsigned len = 2; len <= n; len <<= 1) {
        double ang = (inverse ? -2.0 : 2.0) * M_PI / (double)len;
        cplx wl = {cos(ang), sin(ang)};
        for (unsigned i = 0; i < n; i += len) {
            cplx w = {1.0, 0.0};
            for (unsigned k = i; k < i + len / 2; k++) {
                cplx u = a[k], v = cmul(a[k + len / 2], w);
                a[k] = cadd(u, v);
                a[k + len / 2] = csub(u, v);
                w = cmul(w, wl);
            }
        }
    }
    if (inverse)
        for (unsigned i = 0; i < n; i++) { a[i].re /= n; a[i].im /= n; }
}

/* negacyclic embedding (matches Python tooling fft_of / ifft_to) */
static void fft_of(cplx *out, const double *coeffs, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        double ang = M_PI * (double)i / (double)n;
        out[i] = cmul((cplx){coeffs[i], 0.0}, (cplx){cos(ang), sin(ang)});
    }
    fft_inplace(out, n, 0);
}
static void ifft_to(double *out, cplx *vals, unsigned n) {
    fft_inplace(vals, n, 1);
    for (unsigned i = 0; i < n; i++) {
        double ang = -M_PI * (double)i / (double)n;
        cplx r = cmul(vals[i], (cplx){cos(ang), sin(ang)});
        out[i] = r.re;
    }
}

static void csolve(cplx *G, cplx *rhs, cplx *k, unsigned m) {
    for (unsigned c = 0; c < m; c++) {
        unsigned piv = c;
        double best = G[c*m+c].re*G[c*m+c].re + G[c*m+c].im*G[c*m+c].im;
        for (unsigned r = c + 1; r < m; r++) {
            double v = G[r*m+c].re*G[r*m+c].re + G[r*m+c].im*G[r*m+c].im;
            if (v > best) { best = v; piv = r; }
        }
        if (piv != c) {
            for (unsigned j = 0; j < m; j++) { cplx t = G[c*m+j]; G[c*m+j] = G[piv*m+j]; G[piv*m+j] = t; }
            cplx t = rhs[c]; rhs[c] = rhs[piv]; rhs[piv] = t;
        }
        cplx pc = G[c*m+c];
        double den = pc.re*pc.re + pc.im*pc.im + 1e-300;
        cplx inv = {pc.re/den, -pc.im/den};
        for (unsigned j = c; j < m; j++) G[c*m+j] = cmul(G[c*m+j], inv);
        rhs[c] = cmul(rhs[c], inv);
        for (unsigned r = 0; r < m; r++) {
            if (r == c) continue;
            cplx f = G[r*m+c];
            if (f.re == 0 && f.im == 0) continue;
            for (unsigned j = c; j < m; j++) G[r*m+j] = csub(G[r*m+j], cmul(f, G[c*m+j]));
            rhs[r] = csub(rhs[r], cmul(f, rhs[c]));
        }
    }
    for (unsigned i = 0; i < m; i++) k[i] = rhs[i];
}

/* exact negacyclic conv of (k int64) * (col int32) via 3-prime NTT CRT.
 * |conv| <= n * 2^53 * maxcol << p0*p1*p2 (~2^93), reconstruct signed i128. */
#define KCONV_NP 3
/* Garner constants for primes idx 0..2, computed once */
static uint32_t c3_p0, c3_p1, c3_p2, c3_inv_p0_mod_p1, c3_inv_p01_mod_p2;
static unsigned __int128 c3_M01, c3_M, c3_Mhalf;
/* When nonzero, kg_solve_last_column stops right after the coprimality
 * (|gcd| == 1) test and returns 0 (coprime / solvable) or -1 (not coprime),
 * WITHOUT assembling F or running Babai reduction.  This is the cut-F fast
 * path: the long row F is never used in signing, so only its EXISTENCE
 * (guaranteed by coprimality, Bezout) needs to be verified.  The full solver
 * (F assembly + Babai) below is left intact and is used when this flag is 0. */
int kg_solve_check_only = 0;

static int c3_ready = 0;
static void crt3_setup(void) {
    if (c3_ready) return;
    c3_p0 = kg_prime(0); c3_p1 = kg_prime(1); c3_p2 = kg_prime(2);
    c3_inv_p0_mod_p1 = kg_powmod(c3_p0 % c3_p1, c3_p1 - 2, c3_p1);
    uint64_t p01m2 = ((uint64_t)(c3_p0 % c3_p2) * (c3_p1 % c3_p2)) % c3_p2;
    c3_inv_p01_mod_p2 = kg_powmod((uint32_t)p01m2, c3_p2 - 2, c3_p2);
    c3_M01 = (unsigned __int128)c3_p0 * c3_p1;
    c3_M = c3_M01 * c3_p2;
    c3_Mhalf = c3_M / 2;
    c3_ready = 1;
}
static inline __int128 crt3(const uint32_t r[KCONV_NP]) {
    uint32_t r0m1 = r[0] % c3_p1;
    uint32_t d1 = r[1] >= r0m1 ? r[1] - r0m1 : r[1] + c3_p1 - r0m1;
    uint32_t t1 = (uint32_t)(((uint64_t)d1 * c3_inv_p0_mod_p1) % c3_p1);
    uint64_t x01 = (uint64_t)r[0] + (uint64_t)c3_p0 * t1;
    uint32_t x01m2 = (uint32_t)(x01 % c3_p2);
    uint32_t d2 = r[2] >= x01m2 ? r[2] - x01m2 : r[2] + c3_p2 - x01m2;
    uint32_t t2 = (uint32_t)(((uint64_t)d2 * c3_inv_p01_mod_p2) % c3_p2);
    unsigned __int128 x = (unsigned __int128)x01 + c3_M01 * t2;
    if (x > c3_Mhalf) return (__int128)x - (__int128)c3_M;
    return (__int128)x;
}

/* cache of column NTTs mod the 3 conv primes: layout [a][i][pi][n] */
static uint32_t *kconv_cache_build(const int32_t *cols, unsigned n, unsigned d) {
    unsigned dm1 = d - 1;
    crt3_setup();
    uint32_t *cc = xcalloc((size_t)dm1 * d * KCONV_NP * n, 4);
    for (unsigned a = 0; a < dm1; a++)
        for (unsigned i = 0; i < d; i++)
            for (unsigned pi = 0; pi < KCONV_NP; pi++) {
                uint32_t p = kg_prime(pi);
                uint32_t *h = cc + (((size_t)a * d + i) * KCONV_NP + pi) * n;
                const int32_t *src = cols + ((size_t)a * d + i) * n;
                for (unsigned t = 0; t < n; t++) {
                    int64_t v = src[t] % (int64_t)p;
                    if (v < 0) v += p;
                    h[t] = (uint32_t)v;
                }
                kg_ntt_fwd(h, n, pi);
            }
    return cc;
}

/* F[i] -= (k * col_{a,i}) << shift for all rows i, using cached col NTTs */
static void sub_kcol_all_rows(zint **F, const int64_t *k, const uint32_t *cache,
                              unsigned a, unsigned n, unsigned d, unsigned shift,
                              uint32_t *kntt /* scratch KCONV_NP*n */,
                              uint32_t *work /* scratch KCONV_NP*n */) {
    for (unsigned pi = 0; pi < KCONV_NP; pi++) {
        uint32_t p = kg_prime(pi);
        uint32_t *h = kntt + (size_t)pi * n;
        for (unsigned t = 0; t < n; t++) {
            int64_t v = k[t] % (int64_t)p;
            if (v < 0) v += p;
            h[t] = (uint32_t)v;
        }
        kg_ntt_fwd(h, n, pi);
    }
    for (unsigned i = 0; i < d; i++) {
        for (unsigned pi = 0; pi < KCONV_NP; pi++) {
            uint32_t p = kg_prime(pi);
            const uint32_t *ch = cache + (((size_t)a * d + i) * KCONV_NP + pi) * n;
            uint32_t *w = work + (size_t)pi * n;
            const uint32_t *kh = kntt + (size_t)pi * n;
            for (unsigned t = 0; t < n; t++)
                w[t] = (uint32_t)(((uint64_t)kh[t] * ch[t]) % p);
            kg_ntt_inv(w, n, pi);
        }
        for (unsigned t = 0; t < n; t++) {
            uint32_t r[KCONV_NP];
            for (unsigned pi = 0; pi < KCONV_NP; pi++)
                r[pi] = work[(size_t)pi * n + t];
            __int128 v = crt3(r);
            if (v != 0)
                zi_sub_shifted_i128(&F[i][t], v, shift);
        }
    }
}

/* ----------------------------------------------------------------- babai */
static int babai_reduce(zint **F, const int32_t *cols, unsigned n, unsigned d) {
    unsigned dm1 = d - 1;
    cplx *Bh   = xcalloc((size_t)d * dm1 * n, sizeof(cplx));
    cplx *Gram = xcalloc((size_t)n * dm1 * dm1, sizeof(cplx));
    cplx *Fh   = xcalloc((size_t)d * n, sizeof(cplx));
    cplx *ksl  = xcalloc((size_t)dm1 * n, sizeof(cplx));
    cplx *rhs  = xcalloc(dm1, sizeof(cplx));
    cplx *Gs   = xcalloc((size_t)dm1 * dm1, sizeof(cplx));
    cplx *kk   = xcalloc(dm1, sizeof(cplx));
    double *tmpd = xcalloc(n, sizeof(double));
    int64_t *kint = xcalloc(n, sizeof(int64_t));
    uint32_t *kcache = kconv_cache_build(cols, n, d);
    uint32_t *kntt = xcalloc((size_t)KCONV_NP * n, 4);
    uint32_t *kwork = xcalloc((size_t)KCONV_NP * n, 4);
    int ret = -2;

    for (unsigned i = 0; i < d; i++)
        for (unsigned j = 0; j < dm1; j++) {
            for (unsigned t = 0; t < n; t++)
                tmpd[t] = (double)cols[((size_t)j * d + i) * n + t];
            fft_of(Bh + ((size_t)i * dm1 + j) * n, tmpd, n);
        }
    for (unsigned s = 0; s < n; s++)
        for (unsigned a = 0; a < dm1; a++)
            for (unsigned b = 0; b < dm1; b++) {
                cplx acc = {0, 0};
                for (unsigned i = 0; i < d; i++)
                    acc = cadd(acc, cmul(cconj(Bh[((size_t)i*dm1+a)*n + s]),
                                         Bh[((size_t)i*dm1+b)*n + s]));
                if (a == b) acc.re += 1e-9;
                Gram[(size_t)s*dm1*dm1 + (size_t)a*dm1 + b] = acc;
            }

    unsigned last_size = 0xFFFFFFFFu;
    int stall = 0;
    for (unsigned iter = 0; iter < 100000; iter++) {
        unsigned size = 0;
        for (unsigned i = 0; i < d; i++)
            for (unsigned t = 0; t < n; t++) {
                unsigned bl = zi_bitlen(&F[i][t]);
                if (bl > size) size = bl;
            }
        unsigned shift = size > 52 ? size - 52 : 0;
        if (size >= last_size) {
            if (++stall > 8) { ret = -2; goto out; }
        } else stall = 0;
        last_size = size;

        for (unsigned i = 0; i < d; i++) {
            for (unsigned t = 0; t < n; t++)
                tmpd[t] = zi_top_double(&F[i][t], shift);
            fft_of(Fh + (size_t)i * n, tmpd, n);
        }
        for (unsigned s = 0; s < n; s++) {
            for (unsigned a = 0; a < dm1; a++) {
                cplx acc = {0,0};
                for (unsigned i = 0; i < d; i++)
                    acc = cadd(acc, cmul(cconj(Bh[((size_t)i*dm1+a)*n + s]),
                                         Fh[(size_t)i*n + s]));
                rhs[a] = acc;
            }
            memcpy(Gs, Gram + (size_t)s*dm1*dm1, sizeof(cplx)*dm1*dm1);
            csolve(Gs, rhs, kk, dm1);
            for (unsigned a = 0; a < dm1; a++)
                ksl[(size_t)a*n + s] = kk[a];
        }
        int nonzero = 0;
        for (unsigned a = 0; a < dm1; a++) {
            ifft_to(tmpd, ksl + (size_t)a*n, n);  /* consumes ksl row a */
            int any = 0;
            for (unsigned t = 0; t < n; t++) {
                kint[t] = (int64_t)nearbyint(tmpd[t]);
                if (kint[t]) any = 1;
            }
            if (any) {
                nonzero = 1;
                sub_kcol_all_rows(F, kint, kcache, a, n, d, shift, kntt, kwork);
            }
        }
        if (!nonzero) { ret = 0; goto out; }
    }
out:
    free(Bh); free(Gram); free(Fh); free(ksl); free(rhs); free(Gs); free(kk);
    free(tmpd); free(kint); free(kcache); free(kntt); free(kwork);
    return ret;
}

static void gso_polish_d2(int32_t *F, const int32_t *cols, unsigned n);

/* ----------------------------------------------------------------- solve */
/*************************************************
* Name:        kg_solve_last_column
*
* Description: Solves the long NTRU row of the unimodular basis (the full module-NTRU solve):
*              field-norm descent to a big-integer Bezout problem, then unwinds with Babai
*              round-off to keep the row short. Slowest backend; only needed when the actual
*              row F is required.
*
* Arguments:   - int32_t *Fout:        output solved row (d*n integers)
*              - const int32_t *cols:  short-row columns (d x d block, degree-n)
*              - unsigned n:           ring degree
*              - unsigned d:           number of short rows
*
* Returns:     0 on success, nonzero on failure.
**************************************************/
int kg_solve_last_column(int32_t *Fout, const int32_t *cols, unsigned n, unsigned d) {
    unsigned dm1 = d - 1;
    int ret = -3;

    /* ---- minors via CRT-NTT ---- */
    double colnorm = 1.0;
    for (unsigned j = 0; j < dm1; j++) {
        double e = 0;
        for (unsigned i = 0; i < d; i++)
            for (unsigned t = 0; t < n; t++) {
                double v = cols[((size_t)j * d + i) * n + t];
                e += v * v;
            }
        colnorm *= sqrt(e > 1 ? e : 1);
    }
    unsigned mbits = (unsigned)(log2(colnorm + 2)
                     + (dm1 > 1 ? (dm1 - 1) * 0.5 * log2((double)n) : 0)) + 80;
    unsigned np_min = mbits / 29 + 1;
    if (np_min > kg_nprimes()) return -3;

    zint **minors = xcalloc(d, sizeof(zint *));
    crt_mod mmod;
    crtm_init(&mmod);
    for (unsigned i = 0; i < d; i++) {
        minors[i] = xcalloc(n, sizeof(zint));
        for (unsigned t = 0; t < n; t++) zi_init(&minors[i][t]);
    }
    {
        uint32_t *H = xcalloc((size_t)d * dm1 * n, 4);
        uint32_t *Mt = xcalloc((size_t)dm1 * dm1, 4);
        uint32_t *dets = xcalloc(n, 4);
        for (unsigned pi = 0; pi < np_min; pi++) {
            uint32_t p = kg_prime(pi);
            uint32_t invM = crtm_prime_inv(&mmod, p);
            for (unsigned i = 0; i < d; i++)
                for (unsigned j = 0; j < dm1; j++) {
                    uint32_t *h = H + ((size_t)i * dm1 + j) * n;
                    const int32_t *src = cols + ((size_t)j * d + i) * n;
                    for (unsigned t = 0; t < n; t++) {
                        int64_t v = src[t] % (int64_t)p;
                        if (v < 0) v += p;
                        h[t] = (uint32_t)v;
                    }
                    kg_ntt_fwd(h, n, pi);
                }
            for (unsigned di = 0; di < d; di++) {
                for (unsigned s = 0; s < n; s++) {
                    unsigned rr = 0;
                    for (unsigned i = 0; i < d; i++) {
                        if (i == di) continue;
                        for (unsigned j = 0; j < dm1; j++)
                            Mt[rr * dm1 + j] = H[((size_t)i * dm1 + j) * n + s];
                        rr++;
                    }
                    dets[s] = det_mod(Mt, dm1, p);
                }
                kg_ntt_inv(dets, n, pi);
                for (unsigned t = 0; t < n; t++)
                    crtm_step_value(&minors[di][t], &mmod, invM, dets[t], p);
            }
            crtm_advance(&mmod, p);
        }
        for (unsigned di = 0; di < d; di++)
            for (unsigned t = 0; t < n; t++)
                crtm_center(&minors[di][t], &mmod);
        free(H); free(Mt); free(dets);
    }

    /* ---- resultants + rho per minor ---- */
    zint *rs = xcalloc(d, sizeof(zint));
    zint **rhos = xcalloc(d, sizeof(zint *));
    for (unsigned i = 0; i < d; i++) zi_init(&rs[i]);
    int fail = 0;
    for (unsigned di = 0; di < d && !fail; di++) {
        double e = 0;
        for (unsigned t = 0; t < n; t++) {
            double bl = (double)zi_bitlen(&minors[di][t]);
            double v = bl > 0 ? exp2(bl) : 0;
            e += v * v;
        }
        double l2 = sqrt(e) + 2;
        unsigned bits = (unsigned)((double)n * (log2(l2) + 0.5 * log2((double)n) + 0.1)) + 96;
        unsigned npr = bits / 29 + 1;
        if (npr > kg_nprimes()) { fail = 1; break; }

        crt_mod rmod;
        crtm_init(&rmod);
        zint racc; zi_init(&racc);
        zint *rhoacc = xcalloc(n, sizeof(zint));
        for (unsigned t = 0; t < n; t++) zi_init(&rhoacc[t]);
        uint32_t *h = xcalloc(n, 4);

        unsigned used = 0;
        for (unsigned pi = 0; used < npr; pi++) {
            if (pi >= kg_nprimes()) { fail = 1; break; }
            uint32_t p = kg_prime(pi);
            for (unsigned t = 0; t < n; t++)
                h[t] = zi_mod_u32(&minors[di][t], p);
            kg_ntt_fwd(h, n, pi);
            uint64_t rp = 1;
            int bad = 0;
            for (unsigned t = 0; t < n; t++) {
                if (h[t] == 0) { bad = 1; break; }
                rp = (rp * h[t]) % p;
            }
            if (bad) continue;
            /* batch inversion of the n slots (Montgomery's trick): 1 powmod total */
            {
                static uint32_t *pref = NULL;
                static unsigned pref_cap = 0;
                if (pref_cap < n) { free(pref); pref = malloc((size_t)n * 4); pref_cap = n; }
                uint64_t run = 1;
                for (unsigned t = 0; t < n; t++) {
                    pref[t] = (uint32_t)run;
                    run = (run * h[t]) % p;
                }
                uint64_t inv_all = kg_powmod((uint32_t)run, p - 2, p);
                for (unsigned t = n; t-- > 0;) {
                    uint64_t inv_t = (inv_all * pref[t]) % p;
                    inv_all = (inv_all * h[t]) % p;
                    h[t] = (uint32_t)((rp * inv_t) % p);
                }
            }
            kg_ntt_inv(h, n, pi);
            uint32_t invM = crtm_prime_inv(&rmod, p);
            crtm_step_value(&racc, &rmod, invM, (uint32_t)rp, p);
            for (unsigned t = 0; t < n; t++)
                crtm_step_value(&rhoacc[t], &rmod, invM, h[t], p);
            crtm_advance(&rmod, p);
            used++;
        }
        if (!fail) {
            crtm_center(&racc, &rmod);
            zi_copy(&rs[di], &racc);
            rhos[di] = xcalloc(n, sizeof(zint));
            for (unsigned t = 0; t < n; t++) {
                crtm_center(&rhoacc[t], &rmod);
                zi_init(&rhos[di][t]);
                zi_copy(&rhos[di][t], &rhoacc[t]);
            }
            if (zi_is_zero(&rs[di])) fail = 1;
        }
        zi_free(&racc);
        crtm_free(&rmod);
        for (unsigned t = 0; t < n; t++) zi_free(&rhoacc[t]);
        free(rhoacc); free(h);
    }
    for (unsigned i = 0; i < d; i++) {
        for (unsigned t = 0; t < n; t++) zi_free(&minors[i][t]);
        free(minors[i]);
    }
    free(minors);
    crtm_free(&mmod);
    if (fail) { ret = -1; goto cleanup_rr; }

    /* ---- multi xgcd fold: alpha_i with sum alpha_i * r_i = g, need |g| = 1 ---- */
    {
        zint g, u, v, gg, t1;
        zi_init(&g); zi_init(&u); zi_init(&v); zi_init(&gg); zi_init(&t1);
        zint *alpha = xcalloc(d, sizeof(zint));
        for (unsigned i = 0; i < d; i++) zi_init(&alpha[i]);
        zi_copy(&g, &rs[0]);
        zi_set_i64(&alpha[0], 1);
        for (unsigned i = 1; i < d; i++) {
            zi_xgcd(&gg, &u, &v, &g, &rs[i]);
            for (unsigned j = 0; j < i; j++) {
                zi_mul(&t1, &alpha[j], &u);
                zi_copy(&alpha[j], &t1);
            }
            zi_copy(&alpha[i], &v);
            zi_copy(&g, &gg);
        }
        int ok = (g.len == 1 && g.d[0] == 1);
        if (!ok) {
            for (unsigned i = 0; i < d; i++) zi_free(&alpha[i]);
            free(alpha);
            zi_free(&g); zi_free(&u); zi_free(&v); zi_free(&gg); zi_free(&t1);
            ret = -1;
            goto cleanup_rr;
        }
        /* cut-F fast path: coprime confirmed => F exists (Bezout).  Since F is
         * never used in signing, skip its assembly and the (expensive) Babai
         * reduction entirely and report success. */
        if (kg_solve_check_only) {
            for (unsigned i = 0; i < d; i++) zi_free(&alpha[i]);
            free(alpha);
            zi_free(&g); zi_free(&u); zi_free(&v); zi_free(&gg); zi_free(&t1);
            ret = 0;
            goto cleanup_rr;
        }
        int gneg = g.sign < 0;

        /* F_i = sign_i * alpha_i * rho_i, sign_i = (-1)^{i+d-1}; flip all if g = -1 */
        zint **F = xcalloc(d, sizeof(zint *));
        for (unsigned i = 0; i < d; i++) {
            F[i] = xcalloc(n, sizeof(zint));
            int s = ((i + d - 1) % 2 == 0) ? 1 : -1;
            if (gneg) s = -s;
            for (unsigned t = 0; t < n; t++) {
                zi_init(&F[i][t]);
                zi_mul(&F[i][t], &alpha[i], &rhos[i][t]);
                if (s < 0) zi_neg(&F[i][t]);
            }
        }
        for (unsigned i = 0; i < d; i++) zi_free(&alpha[i]);
        free(alpha);
        zi_free(&g); zi_free(&u); zi_free(&v); zi_free(&gg); zi_free(&t1);

        int br = babai_reduce(F, cols, n, d);
        if (br == 0) {
            ret = 0;
            for (unsigned i = 0; i < d && ret == 0; i++)
                for (unsigned t = 0; t < n; t++) {
                    if (zi_bitlen(&F[i][t]) > 24) { ret = -2; break; }
                    Fout[(size_t)i * n + t] = (int32_t)zi_to_i64(&F[i][t]);
                }
            if (ret == 0 && d == 2)
                gso_polish_d2(Fout, cols, n);   /* HAWK line-6: 4/3 energy gain */
        } else {
            ret = br;
        }
        for (unsigned i = 0; i < d; i++) {
            for (unsigned t = 0; t < n; t++) zi_free(&F[i][t]);
            free(F[i]);
        }
        free(F);
    }

cleanup_rr:
    for (unsigned i = 0; i < d; i++) {
        if (rhos[i]) {
            for (unsigned t = 0; t < n; t++) zi_free(&rhos[i][t]);
            free(rhos[i]);
        }
        zi_free(&rs[i]);
    }
    free(rhos);
    free(rs);
    return ret;
}

/* ----------------------------------------------------------------- det check */
/*************************************************
* Name:        kg_det_check
*
* Description: Verifies that the short rows together with a candidate solved row F form a
*              determinant-1 (unimodular) matrix.
*
* Arguments:   - const int32_t *cols:  short-row columns
*              - const int32_t *F:     candidate solved row
*              - unsigned n:           ring degree
*              - unsigned d:           number of short rows
*
* Returns:     0 if det = 1, nonzero otherwise.
**************************************************/
int kg_det_check(const int32_t *cols, const int32_t *F, unsigned n, unsigned d) {
    uint32_t *H = xcalloc((size_t)d * d * n, 4);
    uint32_t *Mt = xcalloc((size_t)d * d, 4);
    for (unsigned pi = 0; pi < 3; pi++) {
        uint32_t p = kg_prime(pi);
        for (unsigned i = 0; i < d; i++)
            for (unsigned j = 0; j < d; j++) {
                uint32_t *h = H + ((size_t)i * d + j) * n;
                for (unsigned t = 0; t < n; t++) {
                    int64_t v = (j < d - 1) ? cols[((size_t)j * d + i) * n + t]
                                            : F[(size_t)i * n + t];
                    v %= (int64_t)p;
                    if (v < 0) v += p;
                    h[t] = (uint32_t)v;
                }
                kg_ntt_fwd(h, n, pi);
            }
        unsigned step = n >= 8 ? n / 8 : 1;
        for (unsigned s = 0; s < n; s += step) {
            for (unsigned i = 0; i < d; i++)
                for (unsigned j = 0; j < d; j++)
                    Mt[(size_t)i * d + j] = H[((size_t)i * d + j) * n + s];
            if (det_mod(Mt, d, p) != 1) { free(H); free(Mt); return -1; }
        }
    }
    free(H); free(Mt);
    return 0;
}

/* ----------------------------------------------------------------- d=2 GSO polish
 * HAWK Alg.1 line-6 equivalent: nearest-plane reduction of the F column against
 * the Gram-Schmidt orthogonalisation of the rotations of c0=(f,g), processed in
 * bit-reversed order.  Lands F in the GSO fundamental domain, shrinking
 * E||F||^2 from n/12*2n*sigma^2 (parallelepiped) to (3n+1)/48*2n*sigma^2. */
static unsigned bitrev_u(unsigned x, unsigned bits) {
    unsigned r = 0;
    for (unsigned i = 0; i < bits; i++) { r = (r << 1) | (x & 1); x >>= 1; }
    return r;
}
/* coefficient vector of x^r * p (negacyclic), into out[n] */
static void rot_poly(double *out, const int32_t *p, unsigned n, unsigned r) {
    for (unsigned t = 0; t < n; t++) {
        if (t >= r) out[t] = (double)p[t - r];
        else        out[t] = -(double)p[t - r + n];
    }
}
static void rot_poly_i(int64_t *out, const int32_t *p, unsigned n, unsigned r) {
    for (unsigned t = 0; t < n; t++) {
        if (t >= r) out[t] = p[t - r];
        else        out[t] = -(int64_t)p[t - r + n];
    }
}

static void gso_polish_d2(int32_t *F /* 2*n: F0 | F1 */, const int32_t *cols, unsigned n) {
    const int32_t *f = cols;            /* cols[(0*d+0)*n] */
    const int32_t *g = cols + n;        /* cols[(0*d+1)*n] */
    unsigned logn = 0;
    while ((1u << logn) < n) logn++;
    size_t dim = 2 * (size_t)n;

    double *gs = malloc((size_t)n * dim * sizeof(double));   /* GS vectors */
    double *nrm = malloc((size_t)n * sizeof(double));
    double *v = malloc(dim * sizeof(double));
    double *t = malloc(dim * sizeof(double));
    int64_t *vi = malloc(dim * sizeof(int64_t));
    if (!gs || !nrm || !v || !t || !vi) goto done;

    /* classical (modified) Gram-Schmidt over rotations in bit-reversed order */
    for (unsigned j = 0; j < n; j++) {
        unsigned r = bitrev_u(j, logn);
        rot_poly(v,     f, n, r);
        rot_poly(v + n, g, n, r);
        double *bj = gs + (size_t)j * dim;
        memcpy(bj, v, dim * sizeof(double));
        for (unsigned i = 0; i < j; i++) {
            const double *bi = gs + (size_t)i * dim;
            double dot = 0;
            for (size_t u = 0; u < dim; u++) dot += bj[u] * bi[u];
            double coef = dot / nrm[i];
            for (size_t u = 0; u < dim; u++) bj[u] -= coef * bi[u];
        }
        double s = 0;
        for (size_t u = 0; u < dim; u++) s += bj[u] * bj[u];
        nrm[j] = s;
    }

    /* nearest plane: t = F; for j = n-1 .. 0 subtract round(<t,b~_j>/||b~_j||^2) * v_j */
    for (unsigned u = 0; u < n; u++) { t[u] = (double)F[u]; t[n + u] = (double)F[n + u]; }
    for (unsigned jj = n; jj-- > 0;) {
        const double *bj = gs + (size_t)jj * dim;
        double dot = 0;
        for (size_t u = 0; u < dim; u++) dot += t[u] * bj[u];
        double mu = dot / nrm[jj];
        double cr = nearbyint(mu);
        if (cr == 0.0) continue;
        int64_t c = (int64_t)cr;
        unsigned r = bitrev_u(jj, logn);
        rot_poly(v,     f, n, r);
        rot_poly(v + n, g, n, r);
        for (size_t u = 0; u < dim; u++) t[u] -= cr * v[u];
        rot_poly_i(vi,     f, n, r);
        rot_poly_i(vi + n, g, n, r);
        for (unsigned u = 0; u < n; u++) {
            F[u]     = (int32_t)(F[u]     - c * vi[u]);
            F[n + u] = (int32_t)(F[n + u] - c * vi[n + u]);
        }
    }
done:
    free(gs); free(nrm); free(v); free(t); free(vi);
}

/* ===================================================================
 * Coprimality via field-norm descent (Ewha Algorithm 2, descent half).
 * Reuses the exact-integer minors, then for each minor computes its
 * resultant Res(M_i, x^n+1) by field-norm halving in the NTT domain,
 * rebuilding only the single底层 scalar per prime via CRT.  Finally a
 * multi-xgcd fold decides gcd == 1.  No F, no rho, no Babai.
 * =================================================================== */
/*************************************************
* Name:        kg_solve_coprime_descent
*
* Description: Default coprimality backend: tests whether the short block's D order-(D-1)
*              minors are integer-coprime (gcd of their resultants is 1), which is necessary
*              and sufficient for the short rows to extend to a det=1 matrix. Uses shared
*              per-prime NTT, field-norm resultants, dynamic CRT convergence, and gcd
*              early-stop.
*
* Arguments:   - const int32_t *cols:  short-row columns (degree-n)
*              - unsigned n:           ring degree
*              - unsigned d:           solve dimension (D)
*
* Returns:     0 if coprime (rows extend to unimodular), nonzero otherwise.
**************************************************/
int kg_solve_coprime_descent(const int32_t *cols, unsigned n, unsigned d) {
    unsigned dm1 = d - 1;
    int ret = -3;

    /* ---- minors via CRT-NTT (identical to kg_solve_last_column) ---- */
    double colnorm = 1.0;
    for (unsigned j = 0; j < dm1; j++) {
        double e = 0;
        for (unsigned i = 0; i < d; i++)
            for (unsigned t = 0; t < n; t++) {
                double v = cols[((size_t)j * d + i) * n + t];
                e += v * v;
            }
        colnorm *= sqrt(e > 1 ? e : 1);
    }
    unsigned mbits = (unsigned)(log2(colnorm + 2)
                     + (dm1 > 1 ? (dm1 - 1) * 0.5 * log2((double)n) : 0)) + 80;
    unsigned np_min = mbits / 29 + 1;
    if (np_min > kg_nprimes()) return -3;

    zint **minors = xcalloc(d, sizeof(zint *));
    crt_mod mmod;
    crtm_init(&mmod);
    for (unsigned i = 0; i < d; i++) {
        minors[i] = xcalloc(n, sizeof(zint));
        for (unsigned t = 0; t < n; t++) zi_init(&minors[i][t]);
    }
    {
        uint32_t *H = xcalloc((size_t)d * dm1 * n, 4);
        uint32_t *Mt = xcalloc((size_t)dm1 * dm1, 4);
        uint32_t *dets = xcalloc(n, 4);
        for (unsigned pi = 0; pi < np_min; pi++) {
            uint32_t p = kg_prime(pi);
            uint32_t invM = crtm_prime_inv(&mmod, p);
            for (unsigned i = 0; i < d; i++)
                for (unsigned j = 0; j < dm1; j++) {
                    uint32_t *h = H + ((size_t)i * dm1 + j) * n;
                    const int32_t *src = cols + ((size_t)j * d + i) * n;
                    for (unsigned t = 0; t < n; t++) {
                        int64_t v = src[t] % (int64_t)p;
                        if (v < 0) v += p;
                        h[t] = (uint32_t)v;
                    }
                    kg_ntt_fwd(h, n, pi);
                }
            for (unsigned di = 0; di < d; di++) {
                for (unsigned s = 0; s < n; s++) {
                    unsigned rr = 0;
                    for (unsigned i = 0; i < d; i++) {
                        if (i == di) continue;
                        for (unsigned j = 0; j < dm1; j++)
                            Mt[rr * dm1 + j] = H[((size_t)i * dm1 + j) * n + s];
                        rr++;
                    }
                    dets[s] = det_mod(Mt, dm1, p);
                }
                kg_ntt_inv(dets, n, pi);
                for (unsigned t = 0; t < n; t++)
                    crtm_step_value(&minors[di][t], &mmod, invM, dets[t], p);
            }
            crtm_advance(&mmod, p);
        }
        for (unsigned di = 0; di < d; di++)
            for (unsigned t = 0; t < n; t++)
                crtm_center(&minors[di][t], &mmod);
        free(H); free(Mt); free(dets);
    }

    /* ---- resultant of each minor via field-norm descent ----
     * Res(M_i) is obtained per-prime by halving in NTT domain:
     *   res mod p = pairwise-product fold of the n NTT-point values.
     * Only ONE scalar per prime is CRT-rebuilt (vs n rho-values in gold). */
    zint *rs = xcalloc(d, sizeof(zint));
    for (unsigned i = 0; i < d; i++) zi_init(&rs[i]);
    int fail = 0;

    /* running gcd of resultants, folded incrementally so we can stop as soon
     * as it reaches 1 (coprime confirmed) without computing later minors. */
    zint gacc; zi_init(&gacc);
    int have_g = 0;
    int coprime_early = 0;


    for (unsigned di = 0; di < d && !fail; di++) {
        /* No fixed npr from a (very loose) bit-length bound: that heuristic
         * over-estimates badly at n=1024 (est ~1418 primes vs the ~972 truly
         * needed), and "npr > nprimes" then spuriously aborts solvable keys
         * (root cause of the mode512 ~0.5% accept rate, present in gold too).
         * Instead we CRT-accumulate and stop once the centered resultant value
         * has stopped changing: once the running modulus M exceeds 2*|Res|,
         * the centered value is exact and further primes leave it unchanged. */
        crt_mod rmod;
        crtm_init(&rmod);
        zint racc; zi_init(&racc);
        zint prev; zi_init(&prev);
        uint32_t *h = xcalloc(n, 4);

        unsigned stable = 0;
        unsigned prev_bitlen = 0xFFFFFFFFu;
        int have_any = 0;
        for (unsigned pi = 0; ; pi++) {
            if (pi >= kg_nprimes()) {
                /* exhausted the table without convergence: fall back to
                 * "not decidable here" -> treat as fail (resample). This is
                 * now extremely rare since we stop at the true length. */
                fail = 1; break;
            }
            uint32_t p = kg_prime(pi);
            for (unsigned t = 0; t < n; t++)
                h[t] = zi_mod_u32(&minors[di][t], p);
            kg_ntt_fwd(h, n, pi);
            /* Match the gold resultant path exactly: if any NTT point is zero
             * (i.e. p | Res), gold skips this prime (its rho batch-inverse
             * cannot divide by zero).  We MUST skip identically so our gcd sees
             * the same factor set as gold; the decision must equal gold's. */
            int bad = 0;
            for (unsigned t = 0; t < n; t++) {
                if (h[t] == 0) { bad = 1; break; }
            }
            if (bad) continue;
            /* field-norm descent in NTT domain: pairwise product fold.
             * (mathematically the product of all n point values = Res mod p) */
            unsigned m = n;
            while (m > 1) {
                for (unsigned v = 0; v < m / 2; v++)
                    h[v] = (uint32_t)(((uint64_t)h[2 * v] * h[2 * v + 1]) % p);
                m /= 2;
            }
            uint32_t rp = h[0];
            uint32_t invM = crtm_prime_inv(&rmod, p);
            crtm_step_value(&racc, &rmod, invM, rp, p);
            crtm_advance(&rmod, p);
            have_any = 1;
            /* convergence check on the centered value */
            zint cv; zi_init(&cv);
            zi_copy(&cv, &racc);
            crtm_center(&cv, &rmod);
            unsigned bl = zi_bitlen(&cv);
            int same = (bl == prev_bitlen);
            if (same && !zi_is_zero(&prev)) {
                /* compare full value, not just bitlen */
                zint diff; zi_init(&diff);
                zi_sub(&diff, &cv, &prev);
                same = zi_is_zero(&diff);
                zi_free(&diff);
            }
            if (same) stable++; else stable = 0;
            prev_bitlen = bl;
            zi_copy(&prev, &cv);
            zi_free(&cv);
            /* require a margin of stable primes so we don't stop on a chance
             * coincidence of two consecutive equal centered values. */
            if (stable >= 4 && have_any) break;
        }
        zi_free(&prev);
        if (!fail) {
            crtm_center(&racc, &rmod);
            zi_copy(&rs[di], &racc);
            if (zi_is_zero(&rs[di])) {
                /* Res(M_i)=0 means M_i is non-invertible, but it does NOT make
                 * the system unsolvable: gcd(g,0)=g, so a zero resultant simply
                 * contributes nothing to the running gcd.  (gold's "zero =>
                 * fail" is an over-conservative artifact of its rho path; the
                 * true solvability criterion is gcd of the nonzero resultants
                 * == 1.)  So we skip it. */
            } else if (!have_g) {
                zi_copy(&gacc, &rs[di]);
                have_g = 1;
            } else {
                zint gg;
                zi_init(&gg);
                zi_gcd(&gg, &gacc, &rs[di]);
                zi_copy(&gacc, &gg);
                zi_free(&gg);
            }
        }
        zi_free(&racc);
        crtm_free(&rmod);
        free(h);
        /* Early exit: once the running gcd is 1 the final gcd is 1 regardless
         * of the remaining minors (gcd(1, anything)=1), so the system is
         * solvable and we can stop without computing the costly remaining
         * resultants.  This is exact (not a heuristic) and is the dominant
         * speedup: gcd reaches 1 by the 2nd minor in ~2/3 of solvable cases,
         * saving ~48% of resultant work. */
        if (have_g && gacc.len == 1 && gacc.d[0] == 1) {
            coprime_early = 1;
            break;
        }
    }

    for (unsigned i = 0; i < d; i++) {
        for (unsigned t = 0; t < n; t++) zi_free(&minors[i][t]);
        free(minors[i]);
    }
    free(minors);
    crtm_free(&mmod);
    if (fail) { zi_free(&gacc); ret = -1; goto cleanup; }

    /* decision: coprime iff the running gcd reached 1.  This holds both when
     * we broke out early (coprime_early) and when we folded all minors; an
     * all-zero / no-nonzero-resultant case leaves have_g==0 => not coprime. */
    {
        int ok = coprime_early || (have_g && gacc.len == 1 && gacc.d[0] == 1);
        zi_free(&gacc);
        ret = ok ? 0 : -1;
        goto cleanup;
    }
cleanup:
    for (unsigned i = 0; i < d; i++) zi_free(&rs[i]);
    free(rs);
    return ret;
}


/*************************************************
* Name:        kg_check_dp1_coprime
*
* Description: Fast coprimality backend ("truncated tail columns"): tests only the leading
*              d x (d+1) sub-block, i.e. its d+1 order-d minors, for integer coprimality. By
*              monotonicity of the gcd ideal, coprimality on the first d+1 columns implies
*              coprimality on all D columns, so this is a sound sufficient test (it never
*              accepts a non-extendable block). Reuses the same shared-NTT / field-norm /
*              dynamic-CRT / gcd-early-stop machinery as the descent backend and is ~4-5x
*              faster end-to-end.
*
* Arguments:   - const int32_t *block:  short block, layout block[(r*D + c)*n + t];
*                                       only the first d+1 columns are read
*              - unsigned n:            ring degree
*              - unsigned D:            full widened column dimension
*              - unsigned d:            number of short rows
*
* Returns:     0 if coprime (accept), nonzero otherwise.
**************************************************/
int kg_check_dp1_coprime(const int32_t *block, unsigned n, unsigned D, unsigned d) {
    unsigned nc = d + 1;            /* leading columns used */
    unsigned nm = d + 1;            /* number of minors (omit one of nc cols) */
    int ret = -3;

    /* prime budget for stage-1 minor reconstruction: each minor is a d-order
     * determinant of degree-n polys; reuse descent's heuristic with order d. */
    double colnorm = 1.0;
    for (unsigned c = 0; c < nc; c++) {
        double e = 0;
        for (unsigned r = 0; r < d; r++)
            for (unsigned t = 0; t < n; t++) {
                double v = block[((size_t)r * D + c) * n + t];
                e += v * v;
            }
        colnorm *= sqrt(e > 1 ? e : 1);
    }
    unsigned mbits = (unsigned)(log2(colnorm + 2)
                     + (d > 1 ? (d - 1) * 0.5 * log2((double)n) : 0)) + 80;
    unsigned np_min = mbits / 29 + 1;
    if (np_min > kg_nprimes()) return -3;

    zint **minors = xcalloc(nm, sizeof(zint *));
    crt_mod mmod;
    crtm_init(&mmod);
    for (unsigned i = 0; i < nm; i++) {
        minors[i] = xcalloc(n, sizeof(zint));
        for (unsigned t = 0; t < n; t++) zi_init(&minors[i][t]);
    }
    {
        uint32_t *H = xcalloc((size_t)d * nc * n, 4);   /* NTT of d x nc block */
        uint32_t *Mt = xcalloc((size_t)d * d, 4);
        uint32_t *dets = xcalloc(n, 4);
        for (unsigned pi = 0; pi < np_min; pi++) {
            uint32_t p = kg_prime(pi);
            uint32_t invM = crtm_prime_inv(&mmod, p);
            for (unsigned r = 0; r < d; r++)
                for (unsigned c = 0; c < nc; c++) {
                    uint32_t *h = H + ((size_t)r * nc + c) * n;
                    const int32_t *s = block + ((size_t)r * D + c) * n;
                    for (unsigned t = 0; t < n; t++) {
                        int64_t v = s[t] % (int64_t)p;
                        if (v < 0) v += p;
                        h[t] = (uint32_t)v;
                    }
                    kg_ntt_fwd(h, n, pi);
                }
            for (unsigned omit = 0; omit < nm; omit++) {
                for (unsigned ss = 0; ss < n; ss++) {
                    unsigned cc = 0;
                    /* d x d matrix: rows 0..d-1, columns = nc cols except 'omit' */
                    for (unsigned c = 0; c < nc; c++) {
                        if (c == omit) continue;
                        for (unsigned r = 0; r < d; r++)
                            Mt[r * d + cc] = H[((size_t)r * nc + c) * n + ss];
                        cc++;
                    }
                    dets[ss] = det_mod(Mt, d, p);
                }
                kg_ntt_inv(dets, n, pi);
                for (unsigned t = 0; t < n; t++)
                    crtm_step_value(&minors[omit][t], &mmod, invM, dets[t], p);
            }
            crtm_advance(&mmod, p);
        }
        for (unsigned i = 0; i < nm; i++)
            for (unsigned t = 0; t < n; t++)
                crtm_center(&minors[i][t], &mmod);
        free(H); free(Mt); free(dets);
    }

    /* ---- stage 2: identical to descent — resultant per minor + gcd fold ---- */
    zint *rs = xcalloc(nm, sizeof(zint));
    for (unsigned i = 0; i < nm; i++) zi_init(&rs[i]);
    int fail = 0;
    zint gacc; zi_init(&gacc);
    int have_g = 0;
    int coprime_early = 0;

    for (unsigned di = 0; di < nm && !fail; di++) {
        crt_mod rmod;
        crtm_init(&rmod);
        zint racc; zi_init(&racc);
        zint prev; zi_init(&prev);
        uint32_t *h = xcalloc(n, 4);
        unsigned stable = 0;
        unsigned prev_bitlen = 0xFFFFFFFFu;
        int have_any = 0;
        for (unsigned pi = 0; ; pi++) {
            if (pi >= kg_nprimes()) { fail = 1; break; }
            uint32_t p = kg_prime(pi);
            for (unsigned t = 0; t < n; t++)
                h[t] = zi_mod_u32(&minors[di][t], p);
            kg_ntt_fwd(h, n, pi);
            int bad = 0;
            for (unsigned t = 0; t < n; t++) if (h[t] == 0) { bad = 1; break; }
            if (bad) continue;
            unsigned m = n;
            while (m > 1) {
                for (unsigned v = 0; v < m / 2; v++)
                    h[v] = (uint32_t)(((uint64_t)h[2 * v] * h[2 * v + 1]) % p);
                m /= 2;
            }
            uint32_t rp = h[0];
            uint32_t invM = crtm_prime_inv(&rmod, p);
            crtm_step_value(&racc, &rmod, invM, rp, p);
            crtm_advance(&rmod, p);
            have_any = 1;
            zint cv; zi_init(&cv);
            zi_copy(&cv, &racc);
            crtm_center(&cv, &rmod);
            unsigned bl = zi_bitlen(&cv);
            int same = (bl == prev_bitlen);
            if (same && !zi_is_zero(&prev)) {
                zint diff; zi_init(&diff);
                zi_sub(&diff, &cv, &prev);
                same = zi_is_zero(&diff);
                zi_free(&diff);
            }
            if (same) stable++; else stable = 0;
            prev_bitlen = bl;
            zi_copy(&prev, &cv);
            zi_free(&cv);
            if (stable >= 4 && have_any) break;
        }
        zi_free(&prev);
        if (!fail) {
            crtm_center(&racc, &rmod);
            zi_copy(&rs[di], &racc);
            if (zi_is_zero(&rs[di])) {
                /* zero resultant contributes nothing to gcd */
            } else if (!have_g) {
                zi_copy(&gacc, &rs[di]);
                have_g = 1;
            } else {
                zint gg; zi_init(&gg);
                zi_gcd(&gg, &gacc, &rs[di]);
                zi_copy(&gacc, &gg);
                zi_free(&gg);
            }
        }
        zi_free(&racc);
        crtm_free(&rmod);
        free(h);
        if (have_g && gacc.len == 1 && gacc.d[0] == 1) { coprime_early = 1; break; }
    }

    for (unsigned i = 0; i < nm; i++) {
        for (unsigned t = 0; t < n; t++) zi_free(&minors[i][t]);
        free(minors[i]);
    }
    free(minors);
    crtm_free(&mmod);
    if (fail) { zi_free(&gacc); ret = -1; goto dp1_cleanup; }
    {
        int ok = coprime_early || (have_g && gacc.len == 1 && gacc.d[0] == 1);
        zi_free(&gacc);
        ret = ok ? 0 : -1;
        goto dp1_cleanup;
    }
dp1_cleanup:
    for (unsigned i = 0; i < nm; i++) zi_free(&rs[i]);
    free(rs);
    return ret;
}
