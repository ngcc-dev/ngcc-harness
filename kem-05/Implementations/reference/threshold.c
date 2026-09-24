/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include "threshold.h"
#include <math.h>
#include <stdlib.h>

static double lnbino(size_t n, size_t t);
static double xlny(double x, double y);
static double lnbinomialpmf(size_t n, size_t k, double p, double q);
static double Euh_log(size_t n, size_t w, size_t t, size_t i);
static double iks(size_t r, size_t n, size_t w, size_t t);
static double counters_C0(size_t n, size_t d, size_t w, size_t S, size_t t,
                          double x);
static double counters_C1(size_t n, size_t d, size_t w, size_t S, size_t t,
                          double x);

/* $lnbino(n, t) = \ln {n \choose t}$ */
static double lnbino(size_t n, size_t t) {
    if ((t == 0) || (n == t))
        return 0.0;
    else
        return lgamma(n + 1) - lgamma(t + 1) - lgamma(n - t + 1);
}

static double xlny(double x, double y) {
    if (x == 0.)
        return 0.;
    else
        return x * log(y);
}

/* Log of the probability mass function of a binomial distribution:
 * $lnbinomial(n, k, p, q)) = \ln({n \choose k} p^k q^{n-k})$ */
static double lnbinomialpmf(size_t n, size_t k, double p, double q) {
    return lnbino(n, k) + xlny(k, p) + xlny(n - k, q);
}

static double Euh_log(size_t n, size_t w, size_t t, size_t i) {
    return lnbino(w, i) + lnbino(n - w, t - i) - lnbino(n, t);
}

/* iks = X = sum((l - 1) * E_l, l odd) */
static double iks(size_t r, size_t n, size_t w, size_t t) {
    size_t i;
    double x;
    double denom = 0.;

    // Euh_log(n, w, t, i) decreases fast when 'i' varies.
    // For $i = 10$ it is very likely to be negligible.
    for (x = 0, i = 1; (i < 10) && (i < t); i += 2) {
        x += (i - 1) * exp(Euh_log(n, w, t, i));
        denom += exp(Euh_log(n, w, t, i));
    }

    if (denom == 0.)
        return 0.;
    return x / denom;
}

/* Probability for a bit of the syndrome to be zero, knowing the syndrome
 * weight 'S' and 'X' */
static double counters_C0(size_t n, size_t d, size_t w, size_t S, size_t t,
                          double x) {
    return ((w - 1) * S - x) / (n - t) / d;
}

/* Probability for a bit of the syndrome to be non-zero, knowing the syndrome
 * weight 'S' and 'X' */
static double counters_C1(size_t n, size_t d, size_t w, size_t S, size_t t,
                          double x) {
    return (S + x) / t / d;
}

size_t compute_threshold(size_t r, size_t n, size_t d, size_t w, size_t S,
                         size_t t) {
    double p, q;

    double x = iks(r, n, w, t) * S;
    p = counters_C0(n, d, w, S, t, x);
    q = counters_C1(n, d, w, S, t, x);

    size_t threshold;
    if (p >= 1.0 || p > q) {
        threshold = d;
    }
    else if (q >= 1.) {
        threshold = d + 1;
        double diff = 0.;
        do {
            threshold--;
            diff = -exp(lnbinomialpmf(d, threshold, p, 1. - p)) * (n - t) + 1.;
        } while (diff >= 0. && threshold > (d + 1) / 2);
        threshold = threshold < d ? (threshold + 1) : d;
    }
    else {
        threshold = d + 1;
        double diff = 0.;
        do {
            threshold--;
            diff = (-exp(lnbinomialpmf(d, threshold, p, 1. - p)) * (n - t) +
                    exp(lnbinomialpmf(d, threshold, q, 1. - q)) * t);
        } while (diff >= 0. && threshold > (d + 1) / 2);
        threshold = threshold < d ? (threshold + 1) : d;
    }

    return threshold;
}
