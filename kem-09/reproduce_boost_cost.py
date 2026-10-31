#!/usr/bin/env python3
"""kem-09-5: failure-boosting cost for CheetahKEM (requires numpy and scipy).

The attacker encapsulates honestly and submits only ciphertexts whose known
(r, f) norm sigma_C^2 is at least x times its mean.  The selection probability
is the Cramer (large-deviation) estimate.  The failure probability of a
selected ciphertext uses a Gaussian model for the selected part, which the
exact norm-tilted law below shows is conservative for Cheetah128.  For each set
the script prints the cheapest x that yields 1,500 failures within 2^80
decapsulation queries, or the cost at the budget if none is below target.
"""
import math
import numpy as np
from math import comb
from scipy.optimize import brentq
from scipy.special import erfc, logsumexp

SETS = (("Cheetah128", 1, 5, 10, 10, 4, 128), ("Cheetah256", 2, 4, 10, 10, 4, 256),
        ("Cheetah384", 3, 3, 11, 11, 4, 384), ("Cheetah512", 4, 2, 11, 11, 8, 512))
NF, BUDGET = 1500, 80

def gaussian_model(name, k, eta, db, du, dv, lam):
    n = 640 * k
    xs = np.arange(-eta, eta + 1); cb = np.array([comb(2 * eta, j + eta) / 4**eta for j in xs])
    w = 2**(13 - du); fx = np.arange(-eta - w // 2 + 1, eta + w // 2 + 1); fp = np.zeros(len(fx))
    for i, x in enumerate(xs):
        for u in range(-w // 2 + 1, w // 2 + 1): fp[x + u - fx[0]] += cb[i] / w
    wpk = 2**(13 - db); var_ep = eta / 2 + (wpk * wpk - 1) / 12; var_s = eta / 2
    def cgf(th):
        return n * (logsumexp(th * var_ep * xs**2, b=cb) + logsumexp(th * var_s * fx**2, b=fp))
    def dcgf(th, h=1e-9): return (cgf(th + h) - cgf(th - h)) / (2 * h)
    mean = n * (var_ep * (cb * xs**2).sum() + var_s * (fp * fx**2).sum())
    top = n * (var_ep * eta**2 + var_s * fx[-1]**2)
    ev = np.arange(-2**(12 - dv) + 1, 2**(12 - dv) + 1)
    best = None
    for x in np.arange(1.0, top / mean, 0.01):
        t = x * mean
        if x == 1.0: lsel = 0.0
        else:
            try: th = brentq(lambda h: dcgf(h) - t, 1e-12, 50.0)
            except ValueError: break
            lsel = -(th * t - cgf(th)) / math.log(2)
        sc = math.sqrt(t + eta / 2)
        lp = logsumexp(np.log(0.5 * erfc((1921 - ev) / sc / math.sqrt(2)))) - math.log(len(ev)) + math.log(2)
        lbeta = (lp + math.log(lam)) / math.log(2)
        lq = math.log2(NF) - lbeta; lenc = lq - lsel
        if x == 1.0: base = lq
        row = (x, lsel, lbeta, lq, lenc)
        if lq <= BUDGET:
            if best is None or lenc < best[4]: best = row
    msg = f"{name}: unboosted 2^{base:.1f} queries for {NF} failures"
    if best:
        x, lsel, lbeta, lq, lenc = best
        msg += (f"; within 2^{BUDGET} queries: x={x:.2f}, selection 2^{lsel:.1f}, failure rate 2^{lbeta:.1f},"
                f" queries 2^{lq:.1f}, encapsulations 2^{lenc:.1f} (target {lam})")
    else:
        msg += f"; no preselection reaches 2^{BUDGET} queries in the model"
    print(msg)

class D:
    def __init__(s, lo, p): s.lo = lo; s.p = np.array(p, dtype=np.float64)
    def conv(a, b): return D(a.lo + b.lo, np.convolve(a.p, b.p))
    def trim(s, eps=2.0**-400):
        nz = np.nonzero(s.p > eps)[0]; return D(s.lo + nz[0], s.p[nz[0]:nz[-1] + 1])
    def values(s): return np.arange(s.lo, s.lo + len(s.p))
def cbd(eta): return D(-eta, [comb(2 * eta, k + eta) / 4**eta for k in range(-eta, eta + 1)])
def unif(lo, hi): return D(lo, [1 / (hi - lo + 1)] * (hi - lo + 1))
def prod(X, Y):
    d = {}
    for i, px in enumerate(X.p):
        for j, py in enumerate(Y.p):
            v = (X.lo + i) * (Y.lo + j); d[v] = d.get(v, 0) + px * py
    lo, hi = min(d), max(d); return D(lo, [d.get(v, 0) for v in range(lo, hi + 1)])
def power(X, n):
    R, B = None, X
    while n:
        if n & 1: R = B if R is None else R.conv(B).trim()
        n >>= 1
        if n: B = B.conv(B).trim()
    return R
def tilt(X, th, w):
    v = X.values(); p = X.p * np.exp(th * w * v * v); return D(X.lo, p / p.sum())
def m2(X): v = X.values(); return (X.p * v * v).sum()

def exact128(x=1.17):
    eta, n = 5, 640; wr, wf = eta / 2 + 5.25, eta / 2
    c = cbd(eta); epk = unif(-3, 4); f = c.conv(unif(-3, 4)); ev = unif(-255, 256)
    mean = n * (wr * m2(c) + wf * m2(f))
    th = brentq(lambda t: n * (wr * m2(tilt(c, t, wr)) + wf * m2(tilt(f, t, wf))) - x * mean, 1e-9, 0.05)
    Z = power(prod(c.conv(epk), tilt(c, th, wr)), n).conv(power(prod(tilt(f, th, wf), c), n)).trim()
    Z = Z.conv(c).conv(ev).trim(); v = Z.values()
    pc = (Z.p[np.abs(2 * v) > 3840].sum() + Z.p[np.abs(2 * v + 1) > 3840].sum()) / 2
    lbeta = math.log2(128 * pc)
    print(f"Cheetah128 exact tilted law at x={x:.2f}: failure rate 2^{lbeta:.2f},"
          f" queries for {NF} failures 2^{math.log2(NF) - lbeta:.1f}")

if __name__ == "__main__":
    for s in SETS: gaussian_model(*s)
    exact128()
