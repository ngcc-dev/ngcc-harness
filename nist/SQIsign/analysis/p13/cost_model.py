#!/usr/bin/env python3
"""Refined cost model for Wesolowski's p^(1/3) attack and its 'going under'
(memory-capped) variant at the SQIsign Round-3 primes.

Lattice input (validated by conj_lattice.sage / analyze_lat.py):
  cyclic isogenies E -> E^(p) live in a ternary lattice of degree-form
  determinant exactly p/4, so for uniformly random E
     E[# cyclic phi, deg phi <= D] = c3 * D^(3/2) / sqrt(p),   c3 = 4 pi / (3 zeta(3)),
  i.e. density (3/2) c3 sqrt(n)/sqrt(p) per unit degree; the degrees are
  B-smooth like random integers up to a factor SMOOTH_CORR (measured 0.91-0.99).

Attack (one instance, start curve E' random):
  S = B-smooth integers <= X; table of all cyclic isogenies of degree in S:
     G(X,B) = psibar * sum_{s in S} s   (psibar ~ mean psi(s)/s)
  success iff some phi has deg phi = s1*s2 with s1,s2 in S:
     lam = SMOOTH_CORR*(3/2)c3/sqrt(p) * [ I_{1/2}(X^2/B) + f_win*(I_{1/2}(X^2)-I_{1/2}(X^2/B)) ]
  (n <= X^2/B always splits [Wes26, Lemma 3.4]; f_win = fraction of smooth n in
   (X^2/B, X^2] with a divisor in [n/X, X]; f_win=0 lower / 1 upper bound, or MC value)
  P = 1 - exp(-lam); expected entries generated T = G/P; memory = G entries.

Conventions reported:
  Wes  : 1 Fp2 operation per generated entry (Wes26 Section 4.1)
  spec : log2(B)^2 * log2(p)^2 bit operations per generated entry (Remark 8.2.1)
  gate : Fp2-multiplication gate count per entry (fp2_gates.py), as a floor.
"""
import json, math, sys
import numpy as np

ZETA3 = 1.2020569031595942
C3 = 4 * math.pi / (3 * ZETA3)
SMOOTH_CORR = 0.95
PSIBAR = 15 / math.pi ** 2          # attacker-favourable (smooth numbers have larger psi(s)/s)
LN2 = math.log(2)

SETS = {
    "p324_3": dict(c=3, f=324, target=143, cap=80, spec_cj=27.3),
    "p500_27": dict(c=27, f=500, target=207, cap=120, spec_cj=29.8),
    "p664_17": dict(c=17, f=664, target=272, cap=160, spec_cj=31.4),
}

# ---- Dickman rho on a grid -------------------------------------------------
H = 1e-3
UMAX = 80.0
_n = int(UMAX / H) + 1
_u = np.arange(_n) * H
_rho = np.ones(_n)
_k = int(round(1 / H))
for i in range(_k + 1, _n):
    # rho(u) = rho(u-h) - int_{u-h}^{u} rho(t-1)/t dt  (trapezoid)
    a = _rho[i - 1 - _k] / _u[i - 1]
    b = _rho[i - _k] / _u[i]
    _rho[i] = _rho[i - 1] - H * (a + b) / 2
_lrho = np.log(np.maximum(_rho, 1e-300))


def log_rho(u):
    if u <= 1:
        return 0.0
    return float(np.interp(u, _u, _lrho))


def log2_I(alpha, log2Y, log2B):
    """log2 of int_1^Y t^alpha rho(ln t / ln B) dt."""
    if log2Y <= 0:
        return -math.inf
    L = log2B * LN2
    S = log2Y / log2B
    m = max(2, int(S / H))
    s = np.linspace(0, S, m + 1)
    lr = np.interp(s, _u, _lrho)
    expo = (alpha + 1) * L * s + lr
    mx = expo.max()
    val = np.trapezoid(np.exp(expo - mx), s) * L
    return (mx + math.log(val)) / LN2


def log2_add(a, b):
    if a == -math.inf:
        return b
    if b == -math.inf:
        return a
    hi, lo = max(a, b), min(a, b)
    return hi + math.log2(1 + 2 ** (lo - hi))


def instance(log2p, log2X, log2B, f_win):
    log2Beff = min(log2B, log2X)
    log2G = math.log2(PSIBAR) + log2_I(1.0, log2X, log2Beff)
    pref = math.log2(SMOOTH_CORR * 1.5 * C3) - log2p / 2
    lo = log2_I(0.5, 2 * log2X - log2Beff, log2Beff)
    hi = log2_I(0.5, 2 * log2X, log2Beff)
    if f_win > 0 and hi > lo:
        tail = hi + math.log2(1 - 2 ** (lo - hi)) + math.log2(f_win)
        lam_l2 = pref + log2_add(lo, tail)
    else:
        lam_l2 = pref + lo
    lam = 2 ** lam_l2
    P_l2 = math.log2(-math.expm1(-lam)) if lam < 700 else 0.0
    return log2G, lam_l2, log2G - P_l2


def optimise_free(log2p, f_win, grid_B, grid_X):
    best = None
    cB = [b for b in grid_B if b == int(b) and int(b) % 2 == 0]
    cX = [x for x in grid_X if x == int(x) and int(x) % 2 == 0]
    for lb in cB:
        for lx in cX:
            g, lam, T = instance(log2p, lx, lb, f_win)
            if best is None or T < best["T"]:
                best = dict(T=T, mem=g, log2B=lb, log2X=lx, log2lam=lam)
    b0, x0 = best["log2B"], best["log2X"]
    for lb in [b0 + d / 4 for d in range(-8, 9)]:
        for lx in [x0 + d / 4 for d in range(-8, 9)]:
            g, lam, T = instance(log2p, lx, lb, f_win)
            if best is None or T < best["T"]:
                best = dict(T=T, mem=g, log2B=lb, log2X=lx, log2lam=lam)
    return best


def optimise_capped(log2p, f_win, log2w, grid_B, metric):
    """memory cap w entries: choose B, X with G(X,B) <= w; metric 'wes' or 'spec'."""
    best = None
    for lb in grid_B:
        # largest X with G <= w
        lo, hi = 1.0, log2w
        for _ in range(50):
            mid = (lo + hi) / 2
            if instance(log2p, mid, lb, f_win)[0] <= log2w:
                lo = mid
            else:
                hi = mid
        lx = lo
        g, lam, T = instance(log2p, lx, lb, f_win)
        cost = T + (2 * math.log2(lb) + 2 * math.log2(log2p) if metric == "spec" else 0.0)
        kappa = T - (log2p - g) / 2
        if best is None or cost < best["cost"]:
            best = dict(cost=cost, T=T, mem=g, log2B=lb, log2X=lx, log2lam=lam,
                        log2_kappa=kappa)
    return best


def main():
    f_wins = [0.0, 1.0]
    if len(sys.argv) > 1:
        f_wins = [float(x) for x in sys.argv[1].split(",")]
    res = {"c3": C3, "smooth_corr": SMOOTH_CORR, "psibar": PSIBAR, "sets": {}}
    for name, v in SETS.items():
        p = v["c"] * 2 ** v["f"] - 1
        log2p = math.log2(p)
        r = {"log2p": log2p, "target": v["target"]}
        gB = [x / 4 for x in range(32, 4 * 64)]
        gX = [x / 4 for x in range(4 * int(log2p / 6), 4 * int(log2p / 3 + 30))]
        for fw in f_wins:
            fr = optimise_free(log2p, fw, gB, gX)
            fr["spec_bits"] = fr["T"] + 2 * math.log2(fr["log2B"]) + 2 * math.log2(log2p)
            r[f"free_fwin{fw}"] = fr
            cap = optimise_capped(log2p, fw, v["cap"], [x for x in range(8, 81)], "spec")
            r[f"cap_spec_fwin{fw}"] = cap
            capw = optimise_capped(log2p, fw, v["cap"], [x for x in range(8, 81)], "wes")
            r[f"cap_wes_fwin{fw}"] = capw
        # spec's own formula at the cap for reference
        r["spec_formula_cap"] = (log2p - v["cap"]) / 2 + v["spec_cj"]
        res["sets"][name] = r
    print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()
