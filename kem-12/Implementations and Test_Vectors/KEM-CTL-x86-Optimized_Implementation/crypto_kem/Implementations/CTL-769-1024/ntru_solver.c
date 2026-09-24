#include "ntru_solver.h"
#ifdef CTL_ENABLE_PERF
#include "perf.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <limits.h>
#include <quadmath.h>
#include <gmp.h>

typedef long double complex cplx;
typedef __float128 qflt;
typedef __complex128 qcplx;

#ifndef NTRU_SOLVER_VERBOSE
#define NTRU_SOLVER_VERBOSE 0
#endif

#if NTRU_SOLVER_VERBOSE
#define SOLVER_LOG(...)   printf(__VA_ARGS__)
#else
#define SOLVER_LOG(...)   do { } while (0)
#endif

static const long double PI_L = 3.141592653589793238462643383279502884L;
static const SolveParams default_params = SOLVE_PARAMS_DEFAULT;
static const unsigned BABAI_FG_WINDOW_BITS = 24u;
static const unsigned BABAI_K_CHUNK_BITS   = 24u;

static void stats_zero(SolveStats *stats)
{
    if (stats != NULL) {
        memset(stats, 0, sizeof(*stats));
    }
}

static mpz_t *poly_alloc(size_t n)
{
    size_t i;
    mpz_t *a;

    a = (mpz_t *)malloc(n * sizeof(*a));
    if (a == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        mpz_init(a[i]);
    }
    return a;
}

static void poly_free(mpz_t *a, size_t n)
{
    size_t i;

    if (a == NULL) {
        return;
    }
    for (i = 0; i < n; i++) {
        mpz_clear(a[i]);
    }
    free(a);
}

static void poly_copy(mpz_t *dst, const mpz_t *src, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        mpz_set(dst[i], src[i]);
    }
}

static void poly_from_i8(mpz_t *dst, const int8_t *src, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        mpz_set_si(dst[i], (long)src[i]);
    }
}

static unsigned mpz_abs_bitlen(const mpz_t x)
{
    mpz_t t;
    unsigned r;

    if (mpz_sgn(x) == 0) {
        return 0;
    }
    mpz_init(t);
    mpz_abs(t, x);
    r = (unsigned)mpz_sizeinbase(t, 2);
    mpz_clear(t);
    return r;
}

static unsigned poly_max_bitlen(const mpz_t *a, size_t n)
{
    size_t i;
    unsigned mb;

    mb = 0;
    for (i = 0; i < n; i++) {
        unsigned b;
        b = mpz_abs_bitlen(a[i]);
        if (b > mb) {
            mb = b;
        }
    }
    return mb;
}

static unsigned poly_pair_max_bitlen(const mpz_t *A, const mpz_t *B, size_t n)
{
    unsigned a_bits, b_bits;
    a_bits = poly_max_bitlen(A, n);
    b_bits = poly_max_bitlen(B, n);
    return (a_bits > b_bits) ? a_bits : b_bits;
}

static void poly_sqnorm(mpz_t acc, const mpz_t *a, size_t n)
{
    size_t i;
    mpz_t t;

    mpz_set_ui(acc, 0);
    mpz_init(t);

    for (i = 0; i < n; i++) {
        mpz_mul(t, a[i], a[i]);
        mpz_add(acc, acc, t);
    }

    mpz_clear(t);
}

static void poly_pair_sqnorm(mpz_t acc, const mpz_t *A, const mpz_t *B, size_t n)
{
    mpz_t t;

    mpz_init(t);
    poly_sqnorm(acc, A, n);
    poly_sqnorm(t, B, n);
    mpz_add(acc, acc, t);
    mpz_clear(t);
}

static int poly_fit_i32(const mpz_t *a, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (!mpz_fits_sint_p(a[i])) {
            return 0;
        }
    }
    return 1;
}

static void poly_to_i32(int32_t *dst, const mpz_t *src, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        dst[i] = (int32_t)mpz_get_si(src[i]);
    }
}

static long double mpz_to_ld_shifted(const mpz_t x, unsigned right_shift_bits)
{
    mpz_t y;
    signed long exp2;
    double m;
    long double v;

    if (mpz_sgn(x) == 0) {
        return 0.0L;
    }
    mpz_init(y);
    mpz_tdiv_q_2exp(y, x, right_shift_bits);
    if (mpz_sgn(y) == 0) {
        mpz_clear(y);
        return 0.0L;
    }
    m = mpz_get_d_2exp(&exp2, y);
    v = ldexpl((long double)m, (int)exp2);
    mpz_clear(y);
    return v;
}

static long double *poly_to_ld_with_shift(const mpz_t *a, size_t n, unsigned right_shift_bits)
{
    size_t i;
    long double *out;

    out = (long double *)malloc(n * sizeof(*out));
    if (out == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        out[i] = mpz_to_ld_shifted(a[i], right_shift_bits);
    }
    return out;
}

static void fft(cplx *a, size_t n, int invert)
{
    size_t i, j;
    for (i = 1, j = 0; i < n; i++) {
        size_t bit;
        bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            cplx tmp = a[i];
            a[i] = a[j];
            a[j] = tmp;
        }
    }

    {
        size_t len;
        for (len = 2; len <= n; len <<= 1) {
            long double ang;
            cplx wlen;
            size_t base;

            ang = 2.0L * PI_L / (long double)len;
            if (!invert) {
                ang = -ang;
            }
            wlen = cosl(ang) + sinl(ang) * I;
            for (base = 0; base < n; base += len) {
                cplx w;
                size_t u;
                w = 1.0L + 0.0L * I;
                for (u = 0; u < (len >> 1); u++) {
                    cplx x, y;
                    x = a[base + u];
                    y = a[base + u + (len >> 1)] * w;
                    a[base + u] = x + y;
                    a[base + u + (len >> 1)] = x - y;
                    w *= wlen;
                }
            }
        }
    }

    if (invert) {
        long double inv_n;
        inv_n = 1.0L / (long double)n;
        for (i = 0; i < n; i++) {
            a[i] *= inv_n;
        }
    }
}

static cplx *negacyclic_fft(const long double *a, size_t n)
{
    size_t i;
    cplx *v;
    cplx zeta;
    cplx pw;

    v = (cplx *)malloc(n * sizeof(*v));
    if (v == NULL) {
        return NULL;
    }
    zeta = cosl(PI_L / (long double)n) + sinl(PI_L / (long double)n) * I;
    pw = 1.0L + 0.0L * I;
    for (i = 0; i < n; i++) {
        v[i] = a[i] * pw;
        pw *= zeta;
    }
    fft(v, n, 0);
    return v;
}

static long double *negacyclic_ifft_real(const cplx *A, size_t n)
{
    size_t i;
    cplx *v;
    long double *a;
    cplx zeta_inv;
    cplx pw;

    v = (cplx *)malloc(n * sizeof(*v));
    a = (long double *)malloc(n * sizeof(*a));
    if (v == NULL || a == NULL) {
        free(v);
        free(a);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        v[i] = A[i];
    }
    fft(v, n, 1);
    zeta_inv = cosl(-PI_L / (long double)n) + sinl(-PI_L / (long double)n) * I;
    pw = 1.0L + 0.0L * I;
    for (i = 0; i < n; i++) {
        a[i] = creall(v[i] * pw);
        pw *= zeta_inv;
    }
    free(v);
    return a;
}

static long double *poly_i8_to_ld(const int8_t *src, size_t n)
{
    size_t i;
    long double *dst;

    dst = (long double *)malloc(n * sizeof(long double));
    if (dst == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        dst[i] = (long double)src[i];
    }
    return dst;
}

static long double *poly_i32_to_ld(const int32_t *src, size_t n)
{
    size_t i;
    long double *dst;

    dst = (long double *)malloc(n * sizeof(long double));
    if (dst == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        dst[i] = (long double)src[i];
    }
    return dst;
}

static long double *poly_adjoint_ld(const long double *a, size_t n)
{
    size_t i;
    long double *r;

    r = (long double *)calloc(n, sizeof(*r));
    if (r == NULL) {
        return NULL;
    }
    if (n == 0) {
        return r;
    }
    r[0] = a[0];
    for (i = 1; i < n; i++) {
        r[n - i] = -a[i];
    }
    return r;
}

static qflt *
poly_i8_to_q(const int8_t *src, size_t n)
{
    size_t i;
    qflt *out;

    out = (qflt *)malloc(n * sizeof(*out));
    if (out == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        out[i] = (qflt)src[i];
    }
    return out;
}

static qflt *
poly_i32_to_q(const int32_t *src, size_t n)
{
    size_t i;
    qflt *out;

    out = (qflt *)malloc(n * sizeof(*out));
    if (out == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        out[i] = (qflt)src[i];
    }
    return out;
}

static qflt *
poly_adjoint_q(const qflt *a, size_t n)
{
    size_t i;
    qflt *r;

    r = (qflt *)calloc(n, sizeof(*r));
    if (r == NULL) {
        return NULL;
    }
    if (n == 0) {
        return r;
    }
    r[0] = a[0];
    for (i = 1; i < n; i++) {
        r[n - i] = -a[i];
    }
    return r;
}

static void
fft_q(qcplx *a, size_t n, int invert)
{
    static const qflt PI_Q = 3.1415926535897932384626433832795028841971693993751Q;
    size_t i, j;

    for (i = 1, j = 0; i < n; i++) {
        size_t bit;

        bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            qcplx tmp;

            tmp = a[i];
            a[i] = a[j];
            a[j] = tmp;
        }
    }

    {
        size_t len;

        for (len = 2; len <= n; len <<= 1) {
            qflt ang;
            qcplx wlen;
            size_t base;

            ang = 2.0Q * PI_Q / (qflt)len;
            if (!invert) {
                ang = -ang;
            }
            wlen = cosq(ang) + sinq(ang) * I;
            for (base = 0; base < n; base += len) {
                qcplx w;
                size_t u;

                w = 1.0Q + 0.0Q * I;
                for (u = 0; u < (len >> 1); u++) {
                    qcplx x, y;

                    x = a[base + u];
                    y = a[base + u + (len >> 1)] * w;
                    a[base + u] = x + y;
                    a[base + u + (len >> 1)] = x - y;
                    w *= wlen;
                }
            }
        }
    }

    if (invert) {
        qflt inv_n;

        inv_n = 1.0Q / (qflt)n;
        for (i = 0; i < n; i++) {
            a[i] *= inv_n;
        }
    }
}

static qcplx *
negacyclic_fft_q(const qflt *a, size_t n)
{
    static const qflt PI_Q = 3.1415926535897932384626433832795028841971693993751Q;
    size_t i;
    qcplx *v;
    qcplx zeta;
    qcplx pw;

    v = (qcplx *)malloc(n * sizeof(*v));
    if (v == NULL) {
        return NULL;
    }
    zeta = cosq(PI_Q / (qflt)n) + sinq(PI_Q / (qflt)n) * I;
    pw = 1.0Q + 0.0Q * I;
    for (i = 0; i < n; i++) {
        v[i] = a[i] * pw;
        pw *= zeta;
    }
    fft_q(v, n, 0);
    return v;
}

static qflt *
negacyclic_ifft_real_q(const qcplx *A, size_t n)
{
    static const qflt PI_Q = 3.1415926535897932384626433832795028841971693993751Q;
    size_t i;
    qcplx *v;
    qflt *a;
    qcplx zeta_inv;
    qcplx pw;

    v = (qcplx *)malloc(n * sizeof(*v));
    a = (qflt *)malloc(n * sizeof(*a));
    if (v == NULL || a == NULL) {
        free(v);
        free(a);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        v[i] = A[i];
    }
    fft_q(v, n, 1);
    zeta_inv = cosq(-PI_Q / (qflt)n) + sinq(-PI_Q / (qflt)n) * I;
    pw = 1.0Q + 0.0Q * I;
    for (i = 0; i < n; i++) {
        a[i] = crealq(v[i] * pw);
        pw *= zeta_inv;
    }
    free(v);
    return a;
}


static int64_t egcd_i64(int64_t a, int64_t b, int64_t *u, int64_t *v)
{
    int64_t u1, v1, u2, v2, t;

    u1 = 1;
    v1 = 0;
    u2 = 0;
    v2 = 1;
    while (b != 0) {
        t = a / b;
        a = a % b;
        u1 -= t * u2;
        v1 -= t * v2;
        t = a;
        a = b;
        b = t;
        t = u1;
        u1 = u2;
        u2 = t;
        t = v1;
        v1 = v2;
        v2 = t;
    }
    if (u != NULL) {
        *u = u1;
    }
    if (v != NULL) {
        *v = v1;
    }
    return a;
}

static void poly_adjoint_big(mpz_t *r, const mpz_t *a, size_t n)
{
    size_t i;
    if (n == 0) {
        return;
    }
    mpz_set(r[0], a[0]);
    for (i = 1; i < n; i++) {
        mpz_neg(r[n - i], a[i]);
    }
}

static void poly_shift_x(mpz_t *r, const mpz_t *a, size_t n)
{
    size_t i;
    if (n == 0) {
        return;
    }
    mpz_neg(r[0], a[n - 1]);
    for (i = 1; i < n; i++) {
        mpz_set(r[i], a[i - 1]);
    }
}

static void split_even_odd(const mpz_t *a, mpz_t *even, mpz_t *odd, size_t n)
{
    size_t h, i;
    h = n >> 1;
    for (i = 0; i < h; i++) {
        mpz_set(even[i], a[(i << 1) + 0]);
        mpz_set(odd[i], a[(i << 1) + 1]);
    }
}

static mpz_t *negacyclic_mul(const mpz_t *a, const mpz_t *b, size_t n)
{
    size_t i, j;
    mpz_t *r;
    mpz_t tmp;

    r = poly_alloc(n);
    if (r == NULL) {
        return NULL;
    }
    mpz_init(tmp);
    for (i = 0; i < n; i++) {
        if (mpz_sgn(a[i]) == 0) {
            continue;
        }
        for (j = 0; j < n; j++) {
            size_t k;
            if (mpz_sgn(b[j]) == 0) {
                continue;
            }
            mpz_mul(tmp, a[i], b[j]);
            k = i + j;
            if (k < n) {
                mpz_add(r[k], r[k], tmp);
            } else {
                mpz_sub(r[k - n], r[k - n], tmp);
            }
        }
    }
    mpz_clear(tmp);
    return r;
}

static void poly_sub_mul_smallk_inplace_scaled(mpz_t *dst, const mpz_t *a,
    const long long *kvec, unsigned scale_k, size_t n)
{
    size_t i, j;
    mpz_t tmp;

    mpz_init(tmp);
    for (i = 0; i < n; i++) {
        if (mpz_sgn(a[i]) == 0) {
            continue;
        }
        for (j = 0; j < n; j++) {
            size_t k;
            if (kvec[j] == 0) {
                continue;
            }

            mpz_mul_si(tmp, a[i], (long)kvec[j]);
            if (scale_k != 0) {
                mpz_mul_2exp(tmp, tmp, scale_k);
            }

            k = i + j;
            if (k < n) {
                mpz_sub(dst[k], dst[k], tmp);
            } else {
                mpz_add(dst[k - n], dst[k - n], tmp);
            }
        }
    }
    mpz_clear(tmp);
}

static mpz_t *poly_embed_x2(const mpz_t *a, size_t h)
{
    size_t i;
    size_t n;
    mpz_t *r;

    n = h << 1;
    r = poly_alloc(n);
    if (r == NULL) {
        return NULL;
    }
    for (i = 0; i < h; i++) {
        mpz_set(r[i << 1], a[i]);
    }
    return r;
}

static mpz_t *poly_norm_down(const mpz_t *a, size_t n)
{
    size_t h, i;
    mpz_t *a0, *a1, *s0, *s1, *xs1;

    h = n >> 1;
    a0 = poly_alloc(h);
    a1 = poly_alloc(h);
    if (a0 == NULL || a1 == NULL) {
        poly_free(a0, h);
        poly_free(a1, h);
        return NULL;
    }
    split_even_odd(a, a0, a1, n);
    s0 = negacyclic_mul(a0, a0, h);
    s1 = negacyclic_mul(a1, a1, h);
    poly_free(a0, h);
    poly_free(a1, h);
    if (s0 == NULL || s1 == NULL) {
        poly_free(s0, h);
        poly_free(s1, h);
        return NULL;
    }
    xs1 = poly_alloc(h);
    if (xs1 == NULL) {
        poly_free(s0, h);
        poly_free(s1, h);
        return NULL;
    }
    poly_shift_x(xs1, s1, h);
    for (i = 0; i < h; i++) {
        mpz_sub(s0[i], s0[i], xs1[i]);
    }
    poly_free(xs1, h);
    poly_free(s1, h);
    return s0;
}


static int solve_base_case(const mpz_t *f, const mpz_t *g, mpz_t *F, mpz_t *G)
{
    mpz_t d, u, v;

    mpz_inits(d, u, v, NULL);
    mpz_gcdext(d, u, v, f[0], g[0]);

    if (mpz_cmp_ui(d, 1) != 0) {
        SOLVER_LOG("[base] gcd != 1, bits(f0)=%zu, bits(g0)=%zu\n",
            mpz_sizeinbase(f[0], 2),
            mpz_sizeinbase(g[0], 2));
        mpz_clears(d, u, v, NULL);
        return 0;
    }

    mpz_mul_si(G[0], u, NTRU_Q);
    mpz_mul_si(F[0], v, -NTRU_Q);

    mpz_clears(d, u, v, NULL);
    return 1;
}

static int babai_reduce_once(
    const mpz_t *f,
    const mpz_t *g,
    mpz_t *F,
    mpz_t *G,
    size_t n,
    unsigned target_bits,
    unsigned max_rounds,
    unsigned stall_limit,
    int fine_mode,
    SolveStats *stats,
    unsigned *rounds_done)
{
    unsigned round;
    unsigned stall;
    unsigned prev_bits;
    mpz_t prev_norm2, now_norm2;

    stall = 0;
    prev_bits = poly_pair_max_bitlen(F, G, n);

    mpz_init(prev_norm2);
    mpz_init(now_norm2);
    poly_pair_sqnorm(prev_norm2, F, G, n);

    for (round = 0; round < max_rounds; round++) {
        unsigned bits_small, bits_big, k_bits_est;
        unsigned shift_small, shift_big, scale_k;
        long double *f_ld, *g_ld, *F_ld, *G_ld;
        long double *af_ld, *ag_ld;
        cplx *FFT_f, *FFT_g, *FFT_F, *FFT_G, *FFT_af, *FFT_ag;
        cplx *den, *num;
        long double *k_ld;
        long long *kvec;
        size_t i;
        int all_zero;
        unsigned now_bits;
        int improved;

        if (stats != NULL) {
            stats->babai_rounds_total += 1;
        }

        /*
         * 普通模式：达到 target_bits 就可以停
         * 精修模式：不因为 bits 达标而退出，还要继续压 norm
         */
        if (!fine_mode && prev_bits <= target_bits) {
            if (rounds_done != NULL) {
                *rounds_done = round;
            }
            mpz_clear(prev_norm2);
            mpz_clear(now_norm2);
            return 1;
        }

        bits_small = poly_max_bitlen(f, n);
        {
            unsigned gb;
            gb = poly_max_bitlen(g, n);
            if (gb > bits_small) {
                bits_small = gb;
            }
        }

        bits_big = prev_bits;
        k_bits_est = (bits_big > bits_small) ? (bits_big - bits_small) : 0u;

        shift_small = (bits_small > BABAI_FG_WINDOW_BITS)
            ? (bits_small - BABAI_FG_WINDOW_BITS)
            : 0u;

        scale_k = (k_bits_est > BABAI_K_CHUNK_BITS)
            ? (k_bits_est - BABAI_K_CHUNK_BITS)
            : 0u;

        shift_big = shift_small + scale_k;

        SOLVER_LOG("[babai] round=%u bits_small=%u bits_big=%u k_bits_est=%u "
               "shift_small=%u shift_big=%u scale_k=%u fine=%d\n",
            round, bits_small, bits_big, k_bits_est,
            shift_small, shift_big, scale_k, fine_mode);

        f_ld = poly_to_ld_with_shift(f, n, shift_small);
        g_ld = poly_to_ld_with_shift(g, n, shift_small);
        F_ld = poly_to_ld_with_shift(F, n, shift_big);
        G_ld = poly_to_ld_with_shift(G, n, shift_big);
        if (f_ld == NULL || g_ld == NULL || F_ld == NULL || G_ld == NULL) {
            free(f_ld);
            free(g_ld);
            free(F_ld);
            free(G_ld);
            if (rounds_done != NULL) {
                *rounds_done = round;
            }
            mpz_clear(prev_norm2);
            mpz_clear(now_norm2);
            return 0;
        }

        af_ld = poly_adjoint_ld(f_ld, n);
        ag_ld = poly_adjoint_ld(g_ld, n);
        FFT_f = negacyclic_fft(f_ld, n);
        FFT_g = negacyclic_fft(g_ld, n);
        FFT_F = negacyclic_fft(F_ld, n);
        FFT_G = negacyclic_fft(G_ld, n);
        FFT_af = negacyclic_fft(af_ld, n);
        FFT_ag = negacyclic_fft(ag_ld, n);

        free(f_ld);
        free(g_ld);
        free(F_ld);
        free(G_ld);
        free(af_ld);
        free(ag_ld);

        if (FFT_f == NULL || FFT_g == NULL || FFT_F == NULL || FFT_G == NULL
            || FFT_af == NULL || FFT_ag == NULL)
        {
            free(FFT_f);
            free(FFT_g);
            free(FFT_F);
            free(FFT_G);
            free(FFT_af);
            free(FFT_ag);
            if (rounds_done != NULL) {
                *rounds_done = round;
            }
            mpz_clear(prev_norm2);
            mpz_clear(now_norm2);
            return 0;
        }

        den = (cplx *)malloc(n * sizeof(*den));
        num = (cplx *)malloc(n * sizeof(*num));
        if (den == NULL || num == NULL) {
            free(FFT_f);
            free(FFT_g);
            free(FFT_F);
            free(FFT_G);
            free(FFT_af);
            free(FFT_ag);
            free(den);
            free(num);
            if (rounds_done != NULL) {
                *rounds_done = round;
            }
            mpz_clear(prev_norm2);
            mpz_clear(now_norm2);
            return 0;
        }

        for (i = 0; i < n; i++) {
            den[i] = FFT_f[i] * FFT_af[i] + FFT_g[i] * FFT_ag[i];
            num[i] = FFT_F[i] * FFT_af[i] + FFT_G[i] * FFT_ag[i];
        }

        free(FFT_f);
        free(FFT_g);
        free(FFT_F);
        free(FFT_G);
        free(FFT_af);
        free(FFT_ag);

        for (i = 0; i < n; i++) {
            if (cabsl(den[i]) < 1e-30L) {
                free(den);
                free(num);
                if (rounds_done != NULL) {
                    *rounds_done = round;
                }
                mpz_clear(prev_norm2);
                mpz_clear(now_norm2);
                return 0;
            }
            num[i] /= den[i];
        }
        free(den);

        k_ld = negacyclic_ifft_real(num, n);
        free(num);
        if (k_ld == NULL) {
            if (rounds_done != NULL) {
                *rounds_done = round;
            }
            mpz_clear(prev_norm2);
            mpz_clear(now_norm2);
            return 0;
        }

        kvec = (long long *)malloc(n * sizeof(*kvec));
        if (kvec == NULL) {
            free(k_ld);
            if (rounds_done != NULL) {
                *rounds_done = round;
            }
            mpz_clear(prev_norm2);
            mpz_clear(now_norm2);
            return 0;
        }

        all_zero = 1;
        for (i = 0; i < n; i++) {
            long double rz;

            rz = llroundl(k_ld[i]);
            if (!isfinite((double)rz)
                || rz > (long double)LLONG_MAX
                || rz < (long double)LLONG_MIN)
            {
                free(k_ld);
                free(kvec);
                if (rounds_done != NULL) {
                    *rounds_done = round;
                }
                mpz_clear(prev_norm2);
                mpz_clear(now_norm2);
                return 0;
            }

            kvec[i] = (long long)rz;
            if (kvec[i] != 0) {
                all_zero = 0;
            }
        }
        free(k_ld);

        if (all_zero) {
            free(kvec);
            if (stats != NULL) {
                stats->babai_zero_k_breaks += 1;
            }
            if (rounds_done != NULL) {
                *rounds_done = round + 1u;
            }
            /*
             * k 已经全 0 了，再做也没意义。
             * 普通模式：看是不是已达 bits 目标
             * 精修模式：直接返回 1，表示“这一批正常结束”
             */
            mpz_clear(prev_norm2);
            mpz_clear(now_norm2);
            return fine_mode ? 1 : (prev_bits <= target_bits);
        }

        poly_sub_mul_smallk_inplace_scaled(F, f, kvec, scale_k, n);
        poly_sub_mul_smallk_inplace_scaled(G, g, kvec, scale_k, n);
        free(kvec);

        now_bits = poly_pair_max_bitlen(F, G, n);
        poly_pair_sqnorm(now_norm2, F, G, n);

        SOLVER_LOG("[babai] round=%u after update: maxbits=%u\n", round, now_bits);

        /*
         * 普通模式：
         *   bits 下降算进展
         *   bits 不变但 norm 下降，也算进展
         *
         * 精修模式：
         *   只要 norm 下降就算进展
         */
        if (!fine_mode) {
            improved = (now_bits < prev_bits)
                    || (now_bits == prev_bits && mpz_cmp(now_norm2, prev_norm2) < 0);
        } else {
            improved = (mpz_cmp(now_norm2, prev_norm2) < 0);
        }

        if (improved) {
            stall = 0;
        } else {
            stall += 1u;
            if (stall >= stall_limit) {
                if (stats != NULL) {
                    stats->babai_stall_breaks += 1;
                }
                if (rounds_done != NULL) {
                    *rounds_done = round + 1u;
                }
                mpz_clear(prev_norm2);
                mpz_clear(now_norm2);
                return fine_mode ? 1 : (now_bits <= target_bits);
            }
        }

        prev_bits = now_bits;
        mpz_set(prev_norm2, now_norm2);
    }

    if (rounds_done != NULL) {
        *rounds_done = max_rounds;
    }

    mpz_clear(prev_norm2);
    mpz_clear(now_norm2);
    return fine_mode ? 1 : (prev_bits <= target_bits);
}

static int solve_ntru_recursive(
    const mpz_t *f,
    const mpz_t *g,
    size_t n,
    mpz_t *F,
    mpz_t *G,
    const SolveParams *params,
    SolveStats *stats)
{
    mpz_t *fp, *gp;
    mpz_t *Fp, *Gp;
    mpz_t *f_neg, *g_neg;
    mpz_t *Fp_x2, *Gp_x2;
    mpz_t *F_lift, *G_lift;
    unsigned target_bits;
    unsigned tmp_rounds;
    size_t i;
    int ok;

    if (stats != NULL) {
        stats->recursive_levels += 1u;
    }

    if (n == 1u) {
        ok = solve_base_case(f, g, F, G);
        if (!ok) {
            SOLVER_LOG("[rec] base failed at n=1\n");
        }
        return ok;
    }

    fp = poly_norm_down(f, n);
    gp = poly_norm_down(g, n);
    if (fp == NULL || gp == NULL) {
        SOLVER_LOG("[rec] poly_norm_down failed at n=%zu\n", n);
        poly_free(fp, n >> 1);
        poly_free(gp, n >> 1);
        return 0;
    }

    Fp = poly_alloc(n >> 1);
    Gp = poly_alloc(n >> 1);
    if (Fp == NULL || Gp == NULL) {
        SOLVER_LOG("[rec] poly_alloc child FG failed at n=%zu\n", n);
        poly_free(fp, n >> 1);
        poly_free(gp, n >> 1);
        poly_free(Fp, n >> 1);
        poly_free(Gp, n >> 1);
        return 0;
    }

    ok = solve_ntru_recursive(fp, gp, n >> 1, Fp, Gp, params, stats);
    poly_free(fp, n >> 1);
    poly_free(gp, n >> 1);
    if (!ok) {
        SOLVER_LOG("[rec] child recursion failed at n=%zu -> child=%zu\n", n, n >> 1);
        poly_free(Fp, n >> 1);
        poly_free(Gp, n >> 1);
        return 0;
    }

    f_neg = poly_alloc(n);
    g_neg = poly_alloc(n);
    if (f_neg == NULL || g_neg == NULL) {
        SOLVER_LOG("[rec] alloc f_neg/g_neg failed at n=%zu\n", n);
        poly_free(Fp, n >> 1);
        poly_free(Gp, n >> 1);
        poly_free(f_neg, n);
        poly_free(g_neg, n);
        return 0;
    }

    poly_copy(f_neg, f, n);
    poly_copy(g_neg, g, n);
    for (i = 1; i < n; i += 2u) {
        mpz_neg(f_neg[i], f_neg[i]);
        mpz_neg(g_neg[i], g_neg[i]);
    }

    Fp_x2 = poly_embed_x2(Fp, n >> 1);
    Gp_x2 = poly_embed_x2(Gp, n >> 1);
    poly_free(Fp, n >> 1);
    poly_free(Gp, n >> 1);
    if (Fp_x2 == NULL || Gp_x2 == NULL) {
        SOLVER_LOG("[rec] poly_embed_x2 failed at n=%zu\n", n);
        poly_free(f_neg, n);
        poly_free(g_neg, n);
        poly_free(Fp_x2, n);
        poly_free(Gp_x2, n);
        return 0;
    }

    F_lift = negacyclic_mul(Fp_x2, g_neg, n);
    G_lift = negacyclic_mul(Gp_x2, f_neg, n);
    poly_free(Fp_x2, n);
    poly_free(Gp_x2, n);
    poly_free(f_neg, n);
    poly_free(g_neg, n);
    if (F_lift == NULL || G_lift == NULL) {
        SOLVER_LOG("[rec] negacyclic_mul lift failed at n=%zu\n", n);
        poly_free(F_lift, n);
        poly_free(G_lift, n);
        return 0;
    }

    target_bits = poly_max_bitlen(f, n);
    {
        unsigned gb;
        gb = poly_max_bitlen(g, n);
        if (gb > target_bits) {
            target_bits = gb;
        }
    }
    target_bits += params->target_margin_bits;

    /*
     * 不再只做一批 Babai。
     * 只要还在变短，就继续做下一批。
     */
    {
            unsigned pass;
        mpz_t before_norm2, after_norm2;

        mpz_init(before_norm2);
        mpz_init(after_norm2);

        for (pass = 0; pass < 128u; pass++) {
            unsigned before_bits, after_bits;
            int reached;
            int improved;

            before_bits = poly_pair_max_bitlen(F_lift, G_lift, n);
            poly_pair_sqnorm(before_norm2, F_lift, G_lift, n);

            reached = babai_reduce_once(
                f, g, F_lift, G_lift, n,
                target_bits,
                params->babai_max_rounds_per_level,
                params->babai_stall_limit_per_level,
                0,
                stats,
                &tmp_rounds);

            after_bits = poly_pair_max_bitlen(F_lift, G_lift, n);
            poly_pair_sqnorm(after_norm2, F_lift, G_lift, n);

            improved = (after_bits < before_bits)
                    || (after_bits == before_bits
                        && mpz_cmp(after_norm2, before_norm2) < 0);

            SOLVER_LOG("[rec] n=%zu babai pass=%u rounds=%u before=%u after=%u target=%u reached=%d improved=%d\n",
                n, pass, tmp_rounds, before_bits, after_bits, target_bits, reached, improved);

            if (reached) {
                break;
            }

            /*
            * bits 不降，但 norm 还在降，也允许继续下一批。
            */
            if (!improved) {
                break;
            }
        }

        mpz_clear(before_norm2);
        mpz_clear(after_norm2);
    }

    poly_copy(F, F_lift, n);
    poly_copy(G, G_lift, n);
    poly_free(F_lift, n);
    poly_free(G_lift, n);
    return 1;
}

static int verify_ntru_exact_big(
    const mpz_t *f,
    const mpz_t *g,
    const mpz_t *F,
    const mpz_t *G,
    size_t n,
    int32_t q)
{
    size_t i, j;
    mpz_t *t;
    mpz_t z;
    int ok;

    t = poly_alloc(n);
    if (t == NULL) {
        return 0;
    }
    mpz_init(z);
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            size_t k;
            mpz_mul(z, f[i], G[j]);
            mpz_submul(z, g[i], F[j]);
            k = i + j;
            if (k < n) {
                mpz_add(t[k], t[k], z);
            } else {
                mpz_sub(t[k - n], t[k - n], z);
            }
        }
    }

    ok = (mpz_cmp_si(t[0], q) == 0);
    for (i = 1; ok && i < n; i++) {
        if (mpz_sgn(t[i]) != 0) {
            ok = 0;
        }
    }
    mpz_clear(z);
    poly_free(t, n);
    return ok;
}

bool verify_ntru_i32(
    const int8_t *f,
    const int8_t *g,
    const int32_t *F,
    const int32_t *G,
    unsigned logn,
    int32_t q)
{
    size_t n;
    size_t i;
    mpz_t *fb, *gb, *Fb, *Gb;
    int ok;

    if (f == NULL || g == NULL || F == NULL || G == NULL) {
        return false;
    }
    if (q != NTRU_Q || logn != NTRU_LOGN) {
        return false;
    }
    n = (size_t)1u << logn;

    fb = poly_alloc(n);
    gb = poly_alloc(n);
    Fb = poly_alloc(n);
    Gb = poly_alloc(n);
    if (fb == NULL || gb == NULL || Fb == NULL || Gb == NULL) {
        poly_free(fb, n);
        poly_free(gb, n);
        poly_free(Fb, n);
        poly_free(Gb, n);
        return false;
    }
    for (i = 0; i < n; i++) {
        mpz_set_si(fb[i], (long)f[i]);
        mpz_set_si(gb[i], (long)g[i]);
        mpz_set_si(Fb[i], (long)F[i]);
        mpz_set_si(Gb[i], (long)G[i]);
    }
    ok = verify_ntru_exact_big(fb, gb, Fb, Gb, n, q);
    poly_free(fb, n);
    poly_free(gb, n);
    poly_free(Fb, n);
    poly_free(Gb, n);
    return ok ? true : false;
}

bool compute_w_3329_2048_clean(
    const int8_t *f,
    const int8_t *g,
    const int32_t *F,
    const int32_t *G,
    int32_t *w_out)
{
    static const qflt qp = 64513.0Q;
    static const qflt gamma2 = 21.0Q;
    size_t i;
    qflt *f_q, *g_q, *F_q, *G_q;
    qflt *af_q, *ag_q;
    qcplx *fft_f, *fft_g, *fft_F, *fft_G, *fft_af, *fft_ag;
    qcplx *num, *den;
    qflt *w_q;
    int ok;

    if (f == NULL || g == NULL || F == NULL || G == NULL || w_out == NULL) {
        return false;
    }

    f_q = poly_i8_to_q(f, NTRU_N);
    g_q = poly_i8_to_q(g, NTRU_N);
    F_q = poly_i32_to_q(F, NTRU_N);
    G_q = poly_i32_to_q(G, NTRU_N);
    if (f_q == NULL || g_q == NULL || F_q == NULL || G_q == NULL) {
        free(f_q);
        free(g_q);
        free(F_q);
        free(G_q);
        return false;
    }

    af_q = poly_adjoint_q(f_q, NTRU_N);
    ag_q = poly_adjoint_q(g_q, NTRU_N);
    fft_f = negacyclic_fft_q(f_q, NTRU_N);
    fft_g = negacyclic_fft_q(g_q, NTRU_N);
    fft_F = negacyclic_fft_q(F_q, NTRU_N);
    fft_G = negacyclic_fft_q(G_q, NTRU_N);
    fft_af = negacyclic_fft_q(af_q, NTRU_N);
    fft_ag = negacyclic_fft_q(ag_q, NTRU_N);

    free(f_q);
    free(g_q);
    free(F_q);
    free(G_q);
    free(af_q);
    free(ag_q);

    if (fft_f == NULL || fft_g == NULL || fft_F == NULL || fft_G == NULL
        || fft_af == NULL || fft_ag == NULL)
    {
        free(fft_f);
        free(fft_g);
        free(fft_F);
        free(fft_G);
        free(fft_af);
        free(fft_ag);
        return false;
    }

    num = (qcplx *)malloc(NTRU_N * sizeof(*num));
    den = (qcplx *)malloc(NTRU_N * sizeof(*den));
    if (num == NULL || den == NULL) {
        free(fft_f);
        free(fft_g);
        free(fft_F);
        free(fft_G);
        free(fft_af);
        free(fft_ag);
        free(num);
        free(den);
        return false;
    }

    ok = 1;
    for (i = 0; i < NTRU_N; i++) {
        qflt den_re, den_im, den_norm2;

        den[i] = fft_g[i] * fft_ag[i] + gamma2 * fft_f[i] * fft_af[i];
        den_re = crealq(den[i]);
        den_im = cimagq(den[i]);
        den_norm2 = (den_re * den_re) + (den_im * den_im);
        if (den_norm2 < 1e-60Q) {
            ok = 0;
            break;
        }
        num[i] = qp * (fft_G[i] * fft_ag[i] + gamma2 * fft_F[i] * fft_af[i]) / den[i];
    }

    free(fft_f);
    free(fft_g);
    free(fft_F);
    free(fft_G);
    free(fft_af);
    free(fft_ag);
    free(den);

    if (!ok) {
        free(num);
        return false;
    }

    w_q = negacyclic_ifft_real_q(num, NTRU_N);
    free(num);
    if (w_q == NULL) {
        return false;
    }

    for (i = 0; i < NTRU_N; i++) {
        long long rz;

        rz = llroundq(w_q[i]);
        if (rz < (long long)INT32_MIN
            || rz > (long long)INT32_MAX)
        {
            free(w_q);
            return false;
        }
        w_out[i] = (int32_t)rz;
    }

    free(w_q);
    return true;
}

// bool solve_ntru_3329_2048_clean(
//     const int8_t *f,
//     const int8_t *g,
//     int32_t *F_out,
//     int32_t *G_out,
//     SolveStats *stats,
//     const SolveParams *params_in)
// {
//     const SolveParams *params;
//     mpz_t *fb, *gb, *F, *G;
//     unsigned final_rounds;
//     int ok;

//     if (f == NULL || g == NULL || F_out == NULL || G_out == NULL) {
//         return false;
//     }

//     stats_zero(stats);
//     params = (params_in != NULL) ? params_in : &default_params;

//     fb = poly_alloc(NTRU_N);
//     gb = poly_alloc(NTRU_N);
//     F = poly_alloc(NTRU_N);
//     G = poly_alloc(NTRU_N);
//     if (fb == NULL || gb == NULL || F == NULL || G == NULL) {
//         poly_free(fb, NTRU_N);
//         poly_free(gb, NTRU_N);
//         poly_free(F, NTRU_N);
//         poly_free(G, NTRU_N);
//         return false;
//     }
//     poly_from_i8(fb, f, NTRU_N);
//     poly_from_i8(gb, g, NTRU_N);

//     ok = solve_ntru_recursive(fb, gb, NTRU_N, F, G, params, stats);
//     if (!ok) {
//         poly_free(fb, NTRU_N);
//         poly_free(gb, NTRU_N);
//         poly_free(F, NTRU_N);
//         poly_free(G, NTRU_N);
//         return false;
//     }

//     ok = babai_reduce_once(
//         fb, gb, F, G, NTRU_N,
//         30u,
//         params->babai_max_rounds_final,
//         params->babai_stall_limit_final,
//         stats,
//         &final_rounds);
//     if (stats != NULL) {
//         stats->final_cleanup_rounds = final_rounds;
//     }
//     if (!ok) {
//         poly_free(fb, NTRU_N);
//         poly_free(gb, NTRU_N);
//         poly_free(F, NTRU_N);
//         poly_free(G, NTRU_N);
//         return false;
//     }

//     if (!poly_fit_i32(F, NTRU_N) || !poly_fit_i32(G, NTRU_N)) {
//         poly_free(fb, NTRU_N);
//         poly_free(gb, NTRU_N);
//         poly_free(F, NTRU_N);
//         poly_free(G, NTRU_N);
//         return false;
//     }
//     if (!verify_ntru_exact_big(fb, gb, F, G, NTRU_N, NTRU_Q)) {
//         poly_free(fb, NTRU_N);
//         poly_free(gb, NTRU_N);
//         poly_free(F, NTRU_N);
//         poly_free(G, NTRU_N);
//         return false;
//     }

//     poly_to_i32(F_out, F, NTRU_N);
//     poly_to_i32(G_out, G, NTRU_N);

//     poly_free(fb, NTRU_N);
//     poly_free(gb, NTRU_N);
//     poly_free(F, NTRU_N);
//     poly_free(G, NTRU_N);
//     return true;
// }
bool solve_ntru_3329_2048_clean(
    const int8_t *f,
    const int8_t *g,
    int32_t *F_out,
    int32_t *G_out,
    SolveStats *stats,
    const SolveParams *params_in)
{
    const SolveParams *params;
    mpz_t *fb, *gb, *F, *G;
    unsigned final_rounds;
    unsigned total_final_rounds;
    int ok;

    if (f == NULL || g == NULL || F_out == NULL || G_out == NULL) {
        SOLVER_LOG("[solver] null input\n");
        return false;
    }

    stats_zero(stats);
    params = (params_in != NULL) ? params_in : &default_params;

    fb = poly_alloc(NTRU_N);
    gb = poly_alloc(NTRU_N);
    F = poly_alloc(NTRU_N);
    G = poly_alloc(NTRU_N);
    if (fb == NULL || gb == NULL || F == NULL || G == NULL) {
        SOLVER_LOG("[solver] alloc failed\n");
        poly_free(fb, NTRU_N);
        poly_free(gb, NTRU_N);
        poly_free(F, NTRU_N);
        poly_free(G, NTRU_N);
        return false;
    }
    poly_from_i8(fb, f, NTRU_N);
    poly_from_i8(gb, g, NTRU_N);

    SOLVER_LOG("[solver] start recursive solve\n");
    ok = solve_ntru_recursive(fb, gb, NTRU_N, F, G, params, stats);
    if (!ok) {
        SOLVER_LOG("[solver] solve_ntru_recursive failed\n");
        poly_free(fb, NTRU_N);
        poly_free(gb, NTRU_N);
        poly_free(F, NTRU_N);
        poly_free(G, NTRU_N);
        return false;
    }

    SOLVER_LOG("[solver] recursive solve ok, maxbits F=%u G=%u\n",
        poly_max_bitlen(F, NTRU_N), poly_max_bitlen(G, NTRU_N));

    total_final_rounds = 0u;
    {
        unsigned pass;
        mpz_t before_norm2, after_norm2;

        mpz_init(before_norm2);
        mpz_init(after_norm2);

        for (pass = 0; pass < 1024u; pass++) {
            unsigned before_bits, after_bits;
            int improved;

            before_bits = poly_pair_max_bitlen(F, G, NTRU_N);
            poly_pair_sqnorm(before_norm2, F, G, NTRU_N);

            ok = babai_reduce_once(
                fb, gb, F, G, NTRU_N,
                30u,
                params->babai_max_rounds_final,
                params->babai_stall_limit_final,
                0,
                stats,
                &final_rounds);

            total_final_rounds += final_rounds;

            after_bits = poly_pair_max_bitlen(F, G, NTRU_N);
            poly_pair_sqnorm(after_norm2, F, G, NTRU_N);

            improved = (after_bits < before_bits)
                    || (after_bits == before_bits
                        && mpz_cmp(after_norm2, before_norm2) < 0);

            SOLVER_LOG("[solver] final pass=%u rounds=%u before=%u after=%u target=30 reached=%d improved=%d\n",
                pass, final_rounds, before_bits, after_bits, ok, improved);

            if (ok) {
                break;
            }

            if (!improved) {
                break;
            }
        }

        mpz_clear(before_norm2);
        mpz_clear(after_norm2);
    }


    if (poly_pair_max_bitlen(F, G, NTRU_N) > 30u) {
        SOLVER_LOG("[solver] final babai still too large, total_final_rounds=%u, maxbits F=%u G=%u\n",
            total_final_rounds,
            poly_max_bitlen(F, NTRU_N),
            poly_max_bitlen(G, NTRU_N));
        poly_free(fb, NTRU_N);
        poly_free(gb, NTRU_N);
        poly_free(F, NTRU_N);
        poly_free(G, NTRU_N);
        return false;
    }



    unsigned fine_pass;
    unsigned fine_rounds;

    for (fine_pass = 0; fine_pass < 32u; fine_pass++) {
        mpz_t before_norm2, after_norm2;

        mpz_init(before_norm2);
        mpz_init(after_norm2);

        poly_pair_sqnorm(before_norm2, F, G, NTRU_N);

        (void)babai_reduce_once(
            fb, gb, F, G, NTRU_N,
            0u, /* fine_mode 里这个值不重要 */
            params->babai_max_rounds_final,
            params->babai_stall_limit_final * 2u,
            1,
            stats,
            &fine_rounds);

        total_final_rounds += fine_rounds;

        poly_pair_sqnorm(after_norm2, F, G, NTRU_N);

        if (mpz_cmp(after_norm2, before_norm2) >= 0) {
            mpz_clear(before_norm2);
            mpz_clear(after_norm2);
            break;
        }

        mpz_clear(before_norm2);
        mpz_clear(after_norm2);
    }

    if (stats != NULL) {
        stats->final_cleanup_rounds = total_final_rounds;
    }

    SOLVER_LOG("[solver] final cleanup ok, maxbits F=%u G=%u\n",
        poly_max_bitlen(F, NTRU_N), poly_max_bitlen(G, NTRU_N));

    if (!poly_fit_i32(F, NTRU_N) || !poly_fit_i32(G, NTRU_N)) {
        SOLVER_LOG("[solver] poly_fit_i32 failed\n");
        poly_free(fb, NTRU_N);
        poly_free(gb, NTRU_N);
        poly_free(F, NTRU_N);
        poly_free(G, NTRU_N);
        return false;
    }

    if (!verify_ntru_exact_big(fb, gb, F, G, NTRU_N, NTRU_Q)) {
        SOLVER_LOG("[solver] verify_ntru_exact_big failed\n");
        poly_free(fb, NTRU_N);
        poly_free(gb, NTRU_N);
        poly_free(F, NTRU_N);
        poly_free(G, NTRU_N);
        return false;
    }

    SOLVER_LOG("[solver] exact verify ok\n");

    poly_to_i32(F_out, F, NTRU_N);
    poly_to_i32(G_out, G, NTRU_N);

    poly_free(fb, NTRU_N);
    poly_free(gb, NTRU_N);
    poly_free(F, NTRU_N);
    poly_free(G, NTRU_N);
    return true;
}

#ifdef NTRU_SOLVER_DEMO
int main(void)
{
    static int8_t f[NTRU_N];
    static int8_t g[NTRU_N];
    static int32_t F[NTRU_N];
    static int32_t G[NTRU_N];
    SolveStats stats = SOLVE_STATS_ZERO;
    SolveParams params = SOLVE_PARAMS_DEFAULT;
    int ok;

    memset(f, 0, sizeof(f));
    memset(g, 0, sizeof(g));
    f[0] = 1;
    g[0] = 0;

    ok = solve_ntru_3329_2048_clean(f, g, F, G, &stats, &params);
    SOLVER_LOG("ok = %d\n", ok);
    if (ok) {
        SOLVER_LOG("F[0] = %d, G[0] = %d\n", F[0], G[0]);
        SOLVER_LOG("verify = %d\n", verify_ntru_i32(f, g, F, G, NTRU_LOGN, NTRU_Q));
        SOLVER_LOG("recursive_levels=%u, babai_rounds_total=%u, final_cleanup_rounds=%u\n",
            stats.recursive_levels,
            stats.babai_rounds_total,
            stats.final_cleanup_rounds);
    }
    return ok ? 0 : 1;
}
#endif
