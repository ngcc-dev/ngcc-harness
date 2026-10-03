#!/usr/bin/env python3
# Reproduces the numerical model posted by Leo Luo on the NGCC PKC Forum.
# The secret-key-dependent message supplies a model lower bound for the
# per-key max_m correctness quantity required by the cited FO theorems.

"""
ZEN (NTRU-based KEM) decryption-failure probability estimator.
"""

from concurrent.futures import ProcessPoolExecutor
from math import factorial as fac, log, ceil
import os

import numpy as np

EXPECTED = {
    "ZEN-light": (-134.94, -118.07),
    "ZEN-128": (-128.13, -104.69),
    "ZEN-256": (-174.93, -132.35),
    "ZEN-256backup": (-257.31, -195.16),
    "ZEN-512": (-179.51, -111.90),
    "ZEN-512backup": (-338.83, -214.41),
}

# =====================================================================
# Basic probability utilities
# =====================================================================

def binomial_coefficient(x, y):
    """Binomial coefficient (x choose y)."""
    try:
        return fac(x) // fac(y) // fac(x - y)
    except ValueError:
        return 0


def centered_binomial_pmf(k, x):
    """PMF of the centered binomial law of parameter ``k`` evaluated at ``x``."""
    return binomial_coefficient(2 * k, x + k) / 2. ** (2 * k)


def build_centered_binomial_law(k):
    """Build the centered binomial law of parameter ``k`` as a dict {x: p(x)}."""
    return {i: centered_binomial_pmf(k, i) for i in range(-k, k + 1)}


def modulus_switch(x, q, rq):
    """Switch a value ``x`` from modulus ``q`` to modulus ``rq``."""
    return int((x * rq + q / 2) / q)


def centered_modulus(x, q):
    """Reduce ``x`` mod ``q``, centered in [-q/2, q/2)."""
    a = x % q
    return a if a < q / 2 else a - q


def build_mod_switching_error_law(q, rq):
    """Law of the error introduced by switching q -> rq -> q."""
    D = {}
    for x in range(q):
        y = modulus_switch(x, q, rq)
        z = modulus_switch(y, rq, q)
        d = centered_modulus(x - z, q)
        D[d] = D.get(d, 0) + 1. / q
    return D


def convolve_distributions(A, B):
    """Convolution (sum of independent r.v.s) of two laws."""
    C = {}
    for a in A:
        for b in B:
            c = a + b
            C[c] = C.get(c, 0) + A[a] * B[b]
    return C


def prune_distribution(A, threshold=2. ** (-500)):
    """Drop support points with probability below ``threshold``."""
    return {x: y for (x, y) in A.items() if y > threshold}


def iterated_convolution(A, i, threshold=2. ** (-500)):
    """``i``-th fold convolution of ``A`` with itself (double-and-add)."""
    D = {0: 1.0}
    for ch in bin(i)[2:]:
        D = prune_distribution(convolve_distributions(D, D), threshold)
        if ch == '1':
            D = prune_distribution(convolve_distributions(D, A), threshold)
    return D


def positive_tail_probability(D, t):
    """P[X >= t] for X ~ D."""
    ma = max(D.keys())
    if t >= ma:
        return 0
    s = 0
    for i in reversed(range(int(ceil(t)), ma + 1)):
        s += D.get(i, 0)
    return s


# =====================================================================
# Small support laws used by the ZEN estimator
# =====================================================================

def build_centered_binary_law(s):
    """Centered binary law: ±1 with probability ``s`` each, 0 with 1 - 2s."""
    return {-1: s, 1: s, 0: 1. - 2 * s}


def build_uniform_binary_law():
    """Uniform law on {0, 1}."""
    return {0: 1 / 2., 1: 1 / 2.}


def rotate_quadruple(v):
    """Cyclic rotation of a 4-vector: [a, b, c, d] -> [-d, a, b, c]."""
    return [-v[3], v[0], v[1], v[2]]


# =====================================================================
# Configuration for the union bound
# =====================================================================

CHECK_EACH_Y = True
CHECK_EACH_U = True

Y_LIST = [[1, 1], [1, -1], [-1, 1], [-1, -1]]

U_LIST = [[0, 1], [0, 3], [1, 2], [2, 3]]

# =====================================================================
# L1-norm distributions of the (t, g, r) components
# =====================================================================

def l1_norm_tgr(Dg, Dr, y, u):
    """Law of the L1-norm contribution from the (t, g, r) part."""
    D = {}
    for s1 in Dg.keys():
        for s2 in Dg.keys():
            for s3 in Dg.keys():
                for s4 in Dg.keys():
                    for e1 in Dr.keys():
                        for e2 in Dr.keys():
                            for e3 in Dr.keys():
                                for e4 in Dr.keys():
                                    v1 = np.array([s1, s2, s3, s4])
                                    v2 = rotate_quadruple(v1)
                                    v3 = rotate_quadruple(v2)
                                    v4 = rotate_quadruple(v3)
                                    mat = np.matrix([v1, v2, v3, v4])
                                    x = np.array([e1 - e3, e2 - e4,
                                                  e1 + e3, e2 + e4])
                                    mat = x * mat
                                    t = y[0] * mat[0, u[0]] + y[1] * mat[0, u[1]]
                                    D[t] = D.get(t, 0) + (
                                        Dg[s1] * Dg[s2] * Dg[s3] * Dg[s4]
                                        * Dr[e1] * Dr[e2] * Dr[e3] * Dr[e4]
                                    )
    return D


def l1_norm_tfe_fwm(Df, De, Dm, y, u):
    """L1-norm contribution from (t, f, e) with an explicit ``m`` law."""
    D = {}
    for f1 in Df.keys():
        for f2 in Df.keys():
            for f3 in Df.keys():
                for f4 in Df.keys():
                    for e1 in De.keys():
                        for e2 in De.keys():
                            for e3 in De.keys():
                                for e4 in De.keys():
                                    for m in Dm.keys():
                                        v1 = np.array([f1, f2, f3, f4])
                                        v2 = rotate_quadruple(v1)
                                        v3 = rotate_quadruple(v2)
                                        v4 = rotate_quadruple(v3)
                                        mat = np.matrix([v1, v2, v3, v4])
                                        x = np.array([e1 - e3, e2 - e4,
                                                      e1 + e3 + m,
                                                      e2 + e4 + m])
                                        mat = x * mat
                                        t = (y[0] * mat[0, u[0]]
                                             + y[1] * mat[0, u[1]])
                                        D[t] = D.get(t, 0) + (
                                            Df[f1] * Df[f2] * Df[f3] * Df[f4]
                                            * De[e1] * De[e2] * De[e3] * De[e4]
                                            * Dm[m]
                                        )
                                        
    return D


def l1_norm_tfe_fwm_worst_m(Df, De, y, u):
    """Use the post's secret-key-dependent diagnostic choice of ``m``."""
    D = {}
    for f1 in Df.keys():
        for f2 in Df.keys():
            for f3 in Df.keys():
                for f4 in Df.keys():
                    for e1 in De.keys():
                        for e2 in De.keys():
                            for e3 in De.keys():
                                for e4 in De.keys():
                                    m = 0 if f2 + f3 + f4 > 0 else 1

                                    v1 = np.array([f1, f2, f3, f4])
                                    v2 = rotate_quadruple(v1)
                                    v3 = rotate_quadruple(v2)
                                    v4 = rotate_quadruple(v3)
                                    mat = np.matrix([v1, v2, v3, v4])
                                    x = np.array([e1 - e3, e2 - e4,
                                                  e1 + e3 + m,
                                                  e2 + e4 + m])
                                    mat = x * mat
                                    t = (y[0] * mat[0, u[0]]
                                         + y[1] * mat[0, u[1]])
                                    D[t] = D.get(t, 0) + (
                                        Df[f1] * Df[f2] * Df[f3] * Df[f4]
                                        * De[e1] * De[e2] * De[e3] * De[e4]
                                    )
    return D


# =====================================================================
# Union bounds
# =====================================================================

def compute_union_bound(n, q, Df, Dg, Dr, De, Dm):
    """Average-case decryption-failure probability union bound."""
    total = 0
    for u in U_LIST:
        union_cc = 0
        for y in Y_LIST:
            D1 = l1_norm_tfe_fwm(Df, De, Dm, y, u)
            D2 = l1_norm_tgr(Dg, Dr, y, u)
            D = convolve_distributions(D1, D2)
            D = iterated_convolution(D, int(n / 4))
            p_cc = positive_tail_probability(D, q) * n / 4
            union_cc += p_cc
            
        total += union_cc
        
    return total


def compute_union_bound_worst_dfr(n, q, Df, Dg, Dr, De):
    """Worst-case (over the DFR-relevant choice of ``m``) union bound."""
    total = 0
    for u in U_LIST:
        union_cc = 0
        for y in Y_LIST:
            D1 = l1_norm_tfe_fwm_worst_m(Df, De, y, u)
            D2 = l1_norm_tgr(Dg, Dr, y, u)
            D = convolve_distributions(D1, D2)
            D = iterated_convolution(D, int(n / 4))
            p_cc = positive_tail_probability(D, q) * n / 4
            union_cc += p_cc
            
        total += union_cc

    return total


# =====================================================================
# Driver
# =====================================================================

def evaluate_parameter_set(label, n, q, rqc,
                           Dg, Df, Dr, De,
                           use_mod_switching=True):
    """Evaluate the averaged model and secret-dependent diagnostic."""
    if use_mod_switching:
        Dc = build_mod_switching_error_law(q, rqc)
        De = convolve_distributions(De, Dc)
    Dm = build_uniform_binary_law()

    union_cc = compute_union_bound(n, q, Df, Dg, Dr, De, Dm)
    average = log(union_cc + 2. ** (-500), 2.)
    print(f"{label}: uniform-message model 2^", average)

    union_cc = compute_union_bound_worst_dfr(n, q, Df, Dg, Dr, De)
    diagnostic = log(union_cc + 2. ** (-500), 2.)
    print(f"{label}: per-key-message lower-bound model 2^", diagnostic)
    expected_average, expected_diagnostic = EXPECTED[label]
    assert abs(average - expected_average) < 0.02
    assert abs(diagnostic - expected_diagnostic) < 0.02
    print()
    print()
    return label


def parameter_laws(label):
    B = build_centered_binary_law
    C = build_centered_binomial_law
    V = convolve_distributions
    if label == "ZEN-light":
        return 512, 769, 128, V(C(1), B(1 / 16)), B(1 / 8), B(3 / 16), V(C(1), B(1 / 8)), True
    if label == "ZEN-128":
        return 512, 769, 256, V(C(1), B(1 / 8)), C(1), V(C(1), B(3 / 32)), C(2), True
    if label == "ZEN-256":
        return 1024, 769, 256, B(3 / 16), B(1 / 8), C(1), C(1), True
    if label == "ZEN-256backup":
        return 1024, 769, None, B(3 / 16), B(1 / 8), C(1), C(1), False
    if label == "ZEN-512":
        return 2048, 769, 256, B(3 / 32), B(3 / 32), B(1 / 8), B(1 / 8), True
    if label == "ZEN-512backup":
        return 2048, 769, None, B(3 / 32), B(3 / 32), B(1 / 8), B(1 / 8), False
    raise ValueError(label)


def evaluate_projection(task):
    label, diagnostic, y, u = task
    n, q, rqc, Dg, Df, Dr, De, use_mod_switching = parameter_laws(label)
    if use_mod_switching:
        De = convolve_distributions(De, build_mod_switching_error_law(q, rqc))
    if diagnostic:
        D1 = l1_norm_tfe_fwm_worst_m(Df, De, y, u)
    else:
        D1 = l1_norm_tfe_fwm(Df, De, build_uniform_binary_law(), y, u)
    D2 = l1_norm_tgr(Dg, Dr, y, u)
    D = iterated_convolution(convolve_distributions(D1, D2), n // 4)
    return label, diagnostic, positive_tail_probability(D, q) * n / 4


def main():
    labels = tuple(EXPECTED)
    tasks = [
        (label, diagnostic, y, u)
        for label in labels
        for diagnostic in (False, True)
        for u in U_LIST
        for y in Y_LIST
    ]
    jobs = min(int(os.environ.get("NGCC_JOBS", os.cpu_count() or 1)), len(tasks))
    totals = {(label, diagnostic): 0.0 for label in labels for diagnostic in (False, True)}
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        for label, diagnostic, value in pool.map(evaluate_projection, tasks):
            totals[label, diagnostic] += value
    for label in labels:
        average = log(totals[label, False] + 2. ** (-500), 2.)
        diagnostic = log(totals[label, True] + 2. ** (-500), 2.)
        print(f"{label}: uniform-message model 2^ {average}")
        print(f"{label}: per-key-message lower-bound model 2^ {diagnostic}")
        expected_average, expected_diagnostic = EXPECTED[label]
        assert abs(average - expected_average) < 0.02
        assert abs(diagnostic - expected_diagnostic) < 0.02
    print("PROOF GAP kem-41-1 CONFIRMED: the numerical model averages message bits while the cited reduction requires a per-key max_m bound")


if __name__ == "__main__":
    main()
