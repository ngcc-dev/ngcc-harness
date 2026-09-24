#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

#ifndef TEST_DIGEST_BITS
#error TEST_DIGEST_BITS must be defined
#endif

#define ALPHA 0.01
#define NUM_SEQ 100
#define NBITS 1000000
#define BLOCK_M 10000
#define SERIAL_M 16
#define APEN_M 10
#define NUM_TESTS 9

static const char *TEST_NAMES[NUM_TESTS] = {
    "Frequency",
    "BlockFrequency",
    "Runs",
    "LongestRun",
    "CusumForward",
    "CusumReverse",
    "SerialP1",
    "SerialP2",
    "ApproxEntropy"
};

typedef struct {
    int pass;
    double min_p;
    double uniform_p;
    int bins[10];
} summary_t;

static double normal_cdf(double x)
{
    return 0.5 * erfc(-x / sqrt(2.0));
}

static double gammp_series(double a, double x)
{
    const int itmax = 10000;
    const double eps = 3.0e-14;
    const double gln = lgamma(a);
    double ap = a;
    double sum = 1.0 / a;
    double del = sum;

    if (x <= 0.0)
        return 0.0;
    for (int n = 1; n <= itmax; n++) {
        ap += 1.0;
        del *= x / ap;
        sum += del;
        if (fabs(del) < fabs(sum) * eps)
            break;
    }
    return sum * exp(-x + a * log(x) - gln);
}

static double gammq_cf(double a, double x)
{
    const int itmax = 10000;
    const double eps = 3.0e-14;
    const double fpmin = DBL_MIN / eps;
    const double gln = lgamma(a);
    double b = x + 1.0 - a;
    double c = 1.0 / fpmin;
    double d = 1.0 / b;
    double h = d;

    for (int i = 1; i <= itmax; i++) {
        double an = -1.0 * i * (i - a);
        double del;

        b += 2.0;
        d = an * d + b;
        if (fabs(d) < fpmin)
            d = fpmin;
        c = b + an / c;
        if (fabs(c) < fpmin)
            c = fpmin;
        d = 1.0 / d;
        del = d * c;
        h *= del;
        if (fabs(del - 1.0) < eps)
            break;
    }
    return exp(-x + a * log(x) - gln) * h;
}

static double igamc(double a, double x)
{
    double q;

    if (a <= 0.0 || x < 0.0)
        return 0.0;
    if (x == 0.0)
        return 1.0;
    q = (x < a + 1.0) ? 1.0 - gammp_series(a, x) : gammq_cf(a, x);
    if (q < 0.0)
        q = 0.0;
    if (q > 1.0)
        q = 1.0;
    return q;
}

static void put64be(unsigned char out[8], uint64_t x)
{
    for (int i = 7; i >= 0; i--) {
        out[i] = (unsigned char)(x & 0xffU);
        x >>= 8;
    }
}

static void make_bits(unsigned char *bits, int seq)
{
    unsigned char msg[16];
    unsigned char digest[TEST_DIGEST_BITS / 8];
    int out = 0;
    uint64_t ctr = 0;

    put64be(msg, (uint64_t)seq);
    while (out < NBITS) {
        put64be(msg + 8, ctr++);
        if (CryptHash(TEST_DIGEST_BITS, msg, 128ULL, digest) != 0) {
            fprintf(stderr, "CryptHash failed\n");
            exit(2);
        }
        for (int j = 0; j < TEST_DIGEST_BITS / 8 && out < NBITS; j++) {
            for (int b = 7; b >= 0 && out < NBITS; b--)
                bits[out++] = (unsigned char)((digest[j] >> b) & 1U);
        }
    }
}

static double test_frequency(const unsigned char *bits)
{
    long sum = 0;

    for (int i = 0; i < NBITS; i++)
        sum += bits[i] ? 1 : -1;
    return erfc(fabs((double)sum) / sqrt(2.0 * NBITS));
}

static double test_block_frequency(const unsigned char *bits)
{
    const int nblocks = NBITS / BLOCK_M;
    double chi2 = 0.0;

    for (int b = 0; b < nblocks; b++) {
        int ones = 0;
        int off = b * BLOCK_M;
        for (int i = 0; i < BLOCK_M; i++)
            ones += bits[off + i];
        double pi = (double)ones / (double)BLOCK_M;
        chi2 += (pi - 0.5) * (pi - 0.5);
    }
    chi2 *= 4.0 * BLOCK_M;
    return igamc(nblocks / 2.0, chi2 / 2.0);
}

static double test_runs(const unsigned char *bits)
{
    int ones = 0;
    int runs = 1;

    for (int i = 0; i < NBITS; i++)
        ones += bits[i];
    double pi = (double)ones / (double)NBITS;
    if (fabs(pi - 0.5) >= 2.0 / sqrt((double)NBITS))
        return 0.0;
    for (int i = 1; i < NBITS; i++) {
        if (bits[i] != bits[i - 1])
            runs++;
    }
    double num = fabs((double)runs - 2.0 * NBITS * pi * (1.0 - pi));
    double den = 2.0 * sqrt(2.0 * NBITS) * pi * (1.0 - pi);
    return erfc(num / den);
}

static double test_longest_run(const unsigned char *bits)
{
    const int k = 6;
    const double pi[7] = {0.0882, 0.2092, 0.2483, 0.1933,
                          0.1208, 0.0675, 0.0727};
    int v[7] = {0, 0, 0, 0, 0, 0, 0};
    const int nblocks = NBITS / BLOCK_M;
    double chi2 = 0.0;

    for (int b = 0; b < nblocks; b++) {
        int max_run = 0;
        int run = 0;
        int off = b * BLOCK_M;
        for (int i = 0; i < BLOCK_M; i++) {
            if (bits[off + i]) {
                run++;
                if (run > max_run)
                    max_run = run;
            } else {
                run = 0;
            }
        }
        if (max_run <= 10)
            v[0]++;
        else if (max_run == 11)
            v[1]++;
        else if (max_run == 12)
            v[2]++;
        else if (max_run == 13)
            v[3]++;
        else if (max_run == 14)
            v[4]++;
        else if (max_run == 15)
            v[5]++;
        else
            v[6]++;
    }

    for (int i = 0; i <= k; i++) {
        double expv = nblocks * pi[i];
        double d = v[i] - expv;
        chi2 += d * d / expv;
    }
    return igamc(k / 2.0, chi2 / 2.0);
}

static double test_cusum_mode(const unsigned char *bits, int reverse)
{
    int s = 0;
    int z = 0;
    double sqrt_n = sqrt((double)NBITS);
    double sum1 = 0.0;
    double sum2 = 0.0;
    int k_start;
    int k_end;

    for (int i = 0; i < NBITS; i++) {
        int bit = reverse ? bits[NBITS - 1 - i] : bits[i];
        int a;
        s += bit ? 1 : -1;
        a = s < 0 ? -s : s;
        if (a > z)
            z = a;
    }
    if (z == 0)
        return 1.0;

    k_start = (int)floor((-(double)NBITS / z + 1.0) / 4.0);
    k_end = (int)floor(((double)NBITS / z - 1.0) / 4.0);
    for (int k = k_start; k <= k_end; k++) {
        sum1 += normal_cdf((4.0 * k + 1.0) * z / sqrt_n)
              - normal_cdf((4.0 * k - 1.0) * z / sqrt_n);
    }

    k_start = (int)floor((-(double)NBITS / z - 3.0) / 4.0);
    k_end = (int)floor(((double)NBITS / z - 1.0) / 4.0);
    for (int k = k_start; k <= k_end; k++) {
        sum2 += normal_cdf((4.0 * k + 3.0) * z / sqrt_n)
              - normal_cdf((4.0 * k + 1.0) * z / sqrt_n);
    }

    double p = 1.0 - sum1 + sum2;
    if (p < 0.0)
        p = 0.0;
    if (p > 1.0)
        p = 1.0;
    return p;
}

static double psi2(const unsigned char *bits, int m)
{
    int size = 1 << m;
    int mask = size - 1;
    int *count = (int *)calloc((size_t)size, sizeof(int));
    int pat = 0;
    double sum = 0.0;

    if (!count)
        exit(3);
    for (int i = 0; i < m; i++)
        pat = (pat << 1) | bits[i];
    for (int i = 0; i < NBITS; i++) {
        count[pat]++;
        pat = ((pat << 1) & mask) | bits[(i + m) % NBITS];
    }
    for (int i = 0; i < size; i++)
        sum += (double)count[i] * (double)count[i];
    free(count);
    return sum * (double)size / (double)NBITS - (double)NBITS;
}

static void test_serial(const unsigned char *bits, double *p1, double *p2)
{
    double psim = psi2(bits, SERIAL_M);
    double psim1 = psi2(bits, SERIAL_M - 1);
    double psim2 = psi2(bits, SERIAL_M - 2);
    double del1 = psim - psim1;
    double del2 = psim - 2.0 * psim1 + psim2;

    *p1 = igamc(pow(2.0, SERIAL_M - 1) / 2.0, del1 / 2.0);
    *p2 = igamc(pow(2.0, SERIAL_M - 2) / 2.0, del2 / 2.0);
}

static double phi_approx(const unsigned char *bits, int m)
{
    int size = 1 << m;
    int mask = size - 1;
    int *count = (int *)calloc((size_t)size, sizeof(int));
    int pat = 0;
    double phi = 0.0;

    if (!count)
        exit(3);
    for (int i = 0; i < m; i++)
        pat = (pat << 1) | bits[i];
    for (int i = 0; i < NBITS; i++) {
        count[pat]++;
        pat = ((pat << 1) & mask) | bits[(i + m) % NBITS];
    }
    for (int i = 0; i < size; i++) {
        if (count[i] > 0) {
            double c = (double)count[i] / (double)NBITS;
            phi += c * log(c);
        }
    }
    free(count);
    return phi;
}

static double test_approx_entropy(const unsigned char *bits)
{
    double apen = phi_approx(bits, APEN_M) - phi_approx(bits, APEN_M + 1);
    double chi2 = 2.0 * NBITS * (log(2.0) - apen);

    return igamc(pow(2.0, APEN_M - 1), chi2 / 2.0);
}

static void update_summary(summary_t *s, double p)
{
    int bin;

    if (p >= ALPHA)
        s->pass++;
    if (p < s->min_p)
        s->min_p = p;
    bin = (p >= 1.0) ? 9 : (int)(p * 10.0);
    if (bin < 0)
        bin = 0;
    if (bin > 9)
        bin = 9;
    s->bins[bin]++;
}

static void finalize_summary(summary_t *s)
{
    double expected = NUM_SEQ / 10.0;
    double chi2 = 0.0;

    for (int i = 0; i < 10; i++) {
        double d = s->bins[i] - expected;
        chi2 += d * d / expected;
    }
    s->uniform_p = igamc(9.0 / 2.0, chi2 / 2.0);
}

int main(void)
{
    unsigned char *bits = (unsigned char *)malloc(NBITS);
    summary_t s[NUM_TESTS];
    int min_pass;

    if (!bits)
        return 3;
    memset(s, 0, sizeof(s));
    for (int i = 0; i < NUM_TESTS; i++) {
        s[i].min_p = 1.0;
        s[i].uniform_p = 0.0;
    }

    for (int seq = 0; seq < NUM_SEQ; seq++) {
        double p[NUM_TESTS];
        make_bits(bits, seq);
        p[0] = test_frequency(bits);
        p[1] = test_block_frequency(bits);
        p[2] = test_runs(bits);
        p[3] = test_longest_run(bits);
        p[4] = test_cusum_mode(bits, 0);
        p[5] = test_cusum_mode(bits, 1);
        test_serial(bits, &p[6], &p[7]);
        p[8] = test_approx_entropy(bits);
        for (int i = 0; i < NUM_TESTS; i++)
            update_summary(&s[i], p[i]);
        if ((seq + 1) % 10 == 0) {
            fprintf(stderr, "Neulaser-%d: %d/%d sequences\n",
                    TEST_DIGEST_BITS, seq + 1, NUM_SEQ);
        }
    }

    min_pass = (int)ceil(NUM_SEQ *
        (1.0 - ALPHA - 3.0 * sqrt(ALPHA * (1.0 - ALPHA) / NUM_SEQ)));
    printf("Variant,Test,Sequences,BitsPerSequence,Pass,Required,MinP,UniformP,Result\n");
    for (int i = 0; i < NUM_TESTS; i++) {
        int ok;
        finalize_summary(&s[i]);
        ok = (s[i].pass >= min_pass) && (s[i].uniform_p >= 0.0001);
        printf("Neulaser-%d,%s,%d,%d,%d,%d,%.6g,%.6g,%s\n",
               TEST_DIGEST_BITS, TEST_NAMES[i], NUM_SEQ, NBITS,
               s[i].pass, min_pass, s[i].min_p, s[i].uniform_p,
               ok ? "PASS" : "FAIL");
    }

    free(bits);
    return 0;
}
