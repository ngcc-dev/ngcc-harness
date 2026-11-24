#!/usr/bin/env python3
"""Maximal-minors cost certificates for kem-03-6, kem-04-3 and kem-06-8.

Recomputes the rank-decoding cost estimates that Wang, Wang, Li, Teng and
Pang (ePrint 2026/2994, Section 4 and Tables 6, 7, 13-15) give for BAG-Loong,
BAG-Piglet and BRA, with integer arithmetic and the standard library only.
Three kinds of check are made for each candidate:

  - the specification's own maximal-minors (MM) formula or table is
    reproduced, so the cost model is the candidate's own;
  - an independent parameter search of ordinary MM (guessed zero coordinates
    and punctures) gives the paper's "Ordinary MM" exponents;
  - the paper's zero-direction (ZD-MM) rows are recomputed from the stated
    guesses, punctures and contraction block.

Costs are log2 of binary operations. No decoding is run: these are cost
estimates, and the equation counts assume the generic independence that the
specifications and the paper assume.

Model (paper Section 4.4). Blocks have length n_i and rank r_i over F_(q0^m),
q0 = 2^d; a_i coordinates of block i are guessed zero and p_i punctured,
leaving u_i = n_i - a_i - p_i and dimension K' = K - sum(a). V counts minor
variables and E equations; P is the probability of the guessed zeros.

  ordinary MM (E >= V - 1):  d^2/P * (m^2 S^3 + E V^(w-1) + recovery)
  ZD-MM       (E <  V - 1):  d^2/P * (m^2 S^3 + V E^(w-1) + d L F D + Q G D
                                      + Q G D^(w-1) + Q recovery),  D = V - E
"""
import argparse
import sys
from itertools import permutations
from math import comb, gcd, log2

TOL = 0.002  # exponents are compared with the paper's three decimals


def phi(q, n, r):
    v = 1.0
    for t in range(r):
        v *= 1.0 - float(q) ** (t - n)
    return v


def neg_log2_p(d, n, a, p, r, packed):
    """-log2 of the probability that the guessed coordinates are zero."""
    q0 = 2 ** d
    total = 0.0
    for ni, ai, pi, ri in zip(n, a, p, r):
        ui = ni - ai - pi
        num = phi(q0, ui, ri) if ui > 0 else phi(q0, ni - ai, ri)
        den = phi(2, d * ni, ri) if packed else phi(q0, ni, ri)
        total += d * ai * ri - log2(num) + log2(den)
    return total


def direct_counts(m, u, r, kprime):
    """Disjoint supports: V = prod C(u_i, r_i); E from the blockwise count."""
    act = [i for i in range(len(u)) if u[i] > 0]
    v = 1
    for i in act:
        v *= comb(u[i], r[i])
    tot = sum(r[i] for i in act)
    best = None
    for order in permutations(act):
        left = kprime + 1
        poly = [1]
        for i in order:
            si = min(left, u[i])
            left -= si
            b = [0] * (u[i] - si + 1)
            for k in range(max(0, r[i] - si), u[i] - si + 1):
                b[k] = comb(u[i] - si, k)
            new = [0] * min(len(poly) + len(b) - 1, tot + 1)
            for x, cx in enumerate(poly):
                if cx:
                    for y, cy in enumerate(b):
                        if cy and x + y <= tot:
                            new[x + y] += cx * cy
            poly = new
        c = poly[tot] if left == 0 and len(poly) > tot else 0
        best = c if best is None else min(best, c)
    return v, m * (best or 0)


def overlap_counts(m, u, r, g, kprime):
    """Two supports meeting in dimension g (BAG-Loong), joint rank R."""
    big_r = r[0] + r[1] - g
    v = sum(comb(u[0], s) * comb(u[1], big_r - s) for s in range(r[0] - g, r[0] + 1))
    h = u[0] + u[1] - kprime - 1
    x = (max(0, h - u[1]), max(0, h - u[0]))
    c = h - x[0] - x[1]
    e = m * sum(comb(x[0], i) * comb(x[1], j) * comb(c, big_r - i - j)
                for i in range(r[0] + 1) for j in range(r[1] + 1) if big_r - i - j >= 0)
    return v, e


def shape(d, m, K, n, r, a, p, g):
    u = [ni - ai - pi for ni, ai, pi in zip(n, a, p)]
    kprime = K - sum(a)
    if kprime < 0 or any(x < 0 for x in u):
        return None
    if g is None:
        if any(0 < u[i] < r[i] or (u[i] == 0 and n[i] - a[i] < r[i]) for i in range(len(u))):
            return None
        v, e = direct_counts(m, u, r, kprime)
        rank = sum(r[i] for i in range(len(u)) if u[i] > 0)
    else:
        if any(u[i] < r[i] for i in range(2)):
            return None
        v, e = overlap_counts(m, u, r, g, kprime)
        rank = r[0] + r[1] - g
    return u, kprime, v, e, rank


def check_cost(m, n, u, rank):
    s = sum(n)
    return m * m * s ** 3, m * m * (s * rank * rank + rank ** 3 + s * rank + s * s) + m * s * s


def mm_cost(d, m, K, n, r, a, p, omega, g=None, packed=False):
    """Ordinary MM exponent, or None when the system is underdetermined."""
    sh = shape(d, m, K, n, r, a, p, g)
    if sh is None:
        return None
    u, _, v, e, rank = sh
    if v <= 0 or e < v - 1:
        return None
    setup, check = check_cost(m, n, u, rank)
    work = setup + 2.0 ** (log2(e) + (omega - 1) * log2(v)) + rank * sum(u) * v + check
    return 2 * log2(d) + neg_log2_p(d, n, a, p, r, packed) + log2(work)


def zd_cost(d, m, K, n, r, a, p, j, omega, g=None, packed=False):
    """ZD-MM exponent contracting block j (1-based); None if not applicable."""
    sh = shape(d, m, K, n, r, a, p, g)
    if sh is None:
        return None
    u, kprime, v, e, rank = sh
    jj = j - 1
    a2 = list(a)
    n2 = list(n)
    n2[jj] -= 1  # one more zero inside block j
    sh2 = shape(d, m, K - 1, n2, r, a2, p, g)
    if sh2 is None or e >= v - 1:
        return None
    _, _, v2, e2, _ = sh2
    if min(e, e2) < v2 - 1:
        return None
    if g is None:
        rest = 1
        for i in range(len(u)):
            if i != jj and u[i] > 0:
                rest *= comb(u[i], r[i])
        f = comb(u[jj] - 1, r[jj] - 1) * rest
        gg = comb(u[jj], r[jj] - 1) * rest
    else:
        f = gg = 0
        o = 1 - jj
        for s in range(r[0] - g, r[0] + 1):
            deg = (s, rank - s)
            if deg[jj] >= 1:
                f += comb(u[jj] - 1, deg[jj] - 1) * comb(u[o], deg[o])
                gg += comb(u[jj], deg[jj] - 1) * comb(u[o], deg[o])
    delta = v - e
    q0 = 2 ** d
    ell = r[jj] + 1
    q = (q0 ** ell - 1) // (q0 - 1)
    setup, check = check_cost(m, n, u, rank)
    stages = (
        log2(setup),
        log2(v) + (omega - 1) * log2(e),
        log2(d * ell * f * delta),
        log2(q * gg * delta),
        log2(q * gg) + (omega - 1) * log2(delta),
        log2(q * (rank * sum(u) * delta + check)),
    )
    top = max(stages)
    total = top + log2(sum(2.0 ** (x - top) for x in stages))
    return 2 * log2(d) + neg_log2_p(d, n, a, p, r, packed) + total, (v, e, delta, f, gg, q)


def search_two(d, m, K, n, r, omega, g=None):
    """Cheapest ordinary MM over guesses in two blocks and one puncture."""
    best = None
    for p in ((0, 0), (1, 0), (0, 1)):
        for a1 in range(0, n[0] - r[0] + 1):
            for a2 in range(0, n[1] - r[1] + 1):
                c = mm_cost(d, m, K, n, r, (a1, a2), p, omega, g)
                if c is not None:
                    if best is None or c < best[0]:
                        best = (c, (a1, a2), p)
                    break  # further guesses in block 2 only add r2 bits each
    return best


class Checker:
    def __init__(self, rid):
        self.rid = rid
        self.ok = True

    def near(self, label, got, want, tol=TOL):
        good = got is not None and abs(got - want) <= tol
        self.ok &= good
        shown = "none" if got is None else f"{got:.3f}"
        print(f"  {label}: {shown} (expected {want:.3f}){'' if good else '  MISMATCH'}")

    def true(self, label, cond):
        self.ok &= bool(cond)
        print(f"  {label}: {'yes' if cond else 'NO'}")

    def finish(self, claim):
        if self.ok:
            print(f"ATTACK {self.rid}: {claim}: CONFIRMED")
        else:
            print(f"NOT CONFIRMED {self.rid}: a recomputed value differs")
        return self.ok


# --- BAG-Loong (kem-03-6) ---------------------------------------------------
# n, m, n1, key (t1, t2, g), message (t1, t2, g): specification Table 1,
# physical p. 8; declared MM costs (key, message): Table 5, physical p. 22.
LOONG = {
    128: (42, 47, 10, (5, 5, 4), (5, 5, 3), (158, 139)),
    256: (65, 67, 12, (5, 6, 3), (6, 6, 3), (286, 292)),
    384: (83, 83, 14, (6, 6, 3), (7, 7, 4), (403, 469)),
    512: (104, 97, 15, (7, 7, 4), (7, 7, 4), (626, 534)),
}
# Paper Table 7 "Ordinary MM" (key, message) and Tables 14-15 ZD-MM rows
# (guesses a, punctures p, contraction block j, total).
LOONG_MM = {128: (117.872, 120.520), 256: (217.134, 234.656), 384: (304.862, 348.685), 512: (450.783, 393.517)}
LOONG_ZD = {
    128: (((7, 0), (0, 0), 2, 113.074), ((5, 0), (0, 0), 1, 115.749)),
    256: (((21, 0), (0, 0), 1, 212.315), ((18, 0), (0, 0), 1, 228.984)),
    384: (((29, 0), (0, 0), 2, 299.061), ((29, 0), (1, 0), 1, 342.041)),
    512: (((43, 0), (1, 0), 1, 444.080), ((34, 0), (0, 0), 2, 386.708)),
}
# Theorem 3.4 evaluated directly (this script), omega = 2.807: (key, message).
LOONG_THM = {128: (122.4, 120.7), 256: (221.8, 235.0), 384: (310.7, 349.4), 512: (451.2, 393.9)}


def loong_theorem(m, d1, d2, k, t1, t2, g, omega):
    """Specification Theorem 3.4: min_a 2^(a.t) * m N2 * T2(d - a)^(omega-1)."""
    t = t1 + t2 - g
    n2 = m * comb(d1 + d2 - k - 1, t)
    best = None
    for a1 in range(0, d1 - t1 + 1):
        for a2 in range(0, d2 - t2 + 1):
            t2v = sum(comb(d1 - a1, i) * comb(d2 - a2, t - i) for i in range(t1 - g, t1 + 1))
            if n2 >= t2v - 1:
                c = a1 * t1 + a2 * t2 + log2(n2) + (omega - 1) * log2(t2v)
                best = c if best is None else min(best, c)
                break
    return best


def check_loong():
    ck = Checker("kem-03-6")
    for lvl, (n, m, n1, key, msg, declared) in LOONG.items():
        print(f"BAG-Loong-{lvl}: target {lvl}, declared MM costs {declared} (key, message)")
        thm = [(loong_theorem(m, n, n, n, *key, w), loong_theorem(m, n, n + n1, n, *msg, w)) for w in (2.807, 3.0)]
        ck.near("Theorem 3.4 key, omega 2.807", thm[0][0], LOONG_THM[lvl][0], 0.05)
        ck.near("Theorem 3.4 message, omega 2.807", thm[0][1], LOONG_THM[lvl][1], 0.05)
        print(f"  Theorem 3.4 at omega 3: key {thm[1][0]:.1f}, message {thm[1][1]:.1f}")
        ck.true("Theorem 3.4 minimum below target at omega 2.807 and 3", min(thm[0]) < lvl and min(thm[1]) < lvl)
        ck.true("Theorem 3.4 minimum at least 18 below the declared minimum", min(thm[0]) <= min(declared) - 18)
        # Structured instances: 1 is a known element of Supp(Y) (physical p. 6),
        # which appends one coordinate to the second key block.
        kb = search_two(1, m, n, (n, n + 1), key[:2], 2.8, key[2])
        mb = search_two(1, m, n, (n, n + n1), msg[:2], 2.8, msg[2])
        ck.near("ordinary MM key, omega 2.8", kb[0], LOONG_MM[lvl][0])
        ck.near("ordinary MM message, omega 2.8", mb[0], LOONG_MM[lvl][1])
        (ka, kp, kj, kt), (ma, mp, mj, mt) = LOONG_ZD[lvl]
        ck.near("ZD-MM key", zd_cost(1, m, n, (n, n + 1), key[:2], ka, kp, kj, 2.8, key[2])[0], kt)
        ck.near("ZD-MM message", zd_cost(1, m, n, (n, n + n1), msg[:2], ma, mp, mj, 2.8, msg[2])[0], mt)
    return ck.finish("the specification's Theorem 3.4 and the structured MM estimates are below all four BAG-Loong targets")


# --- BAG-Piglet (kem-04-3) --------------------------------------------------
# Key instance: three ring elements (x, y1, y2) of rank w each under two public
# equations, K = n (specification Equation (1), physical p. 10). Rows are
# (d, m, K, block length, rank) in the paper's representation: the 256- and
# 384-bit sets are packed over F_4 (Table 13).
PIGLET = {
    128: (1, 47, 43, 43, 4, 148),
    256: (2, 79, 39, 39, 6, 269),
    384: (2, 103, 53, 53, 7, 397),
    512: (1, 131, 125, 125, 8, 552),
}
PIGLET_MM = {128: 118.416, 256: 261.798, 384: 402.373, 512: 574.937}
PIGLET_ZD = {
    128: ((0, 0, 5), (0, 0, 38), 1, 114.644),
    256: ((6, 0, 0), (0, 0, 0), 2, 254.033),
    384: ((12, 0, 0), (0, 0, 0), 2, 394.160),
    512: ((0, 0, 30), (0, 0, 1), 3, 567.599),
}


def piglet_search(d, m, K, n0, r, omega):
    """Guesses in block 1, small punctures, or removal of block 3."""
    n = (n0, n0, n0)
    rr = (r, r, r)
    best = None
    for a1 in range(0, n0 - r + 1):
        for p in ((0, 0, 0), (1, 0, 0), (2, 0, 0), (0, 1, 0), (0, 0, 1)):
            c = mm_cost(d, m, K, n, rr, (a1, 0, 0), p, omega, None, d > 1)
            if c is not None and (best is None or c < best[0]):
                best = (c, (a1, 0, 0), p)
        if a1 <= 8:
            for a3 in range(0, min(n0 - r, 40) + 1):
                for p1 in (0, 1):
                    a, p = (a1, 0, a3), (p1, 0, n0 - a3)
                    c = mm_cost(d, m, K, n, rr, a, p, omega, None, d > 1)
                    if c is not None and (best is None or c < best[0]):
                        best = (c, a, p)
    return best


def check_piglet():
    ck = Checker("kem-04-3")
    for lvl, (d, m, K, n0, r, declared) in PIGLET.items():
        print(f"BAG-Piglet-{lvl}: target {lvl}, declared key MM cost {declared}")
        b = piglet_search(d, m, K, n0, r, 2.8)
        ck.near("ordinary MM key, omega 2.8", b[0], PIGLET_MM[lvl])
        print(f"    guesses {b[1]}, punctures {b[2]}")
        a, p, j, total = PIGLET_ZD[lvl]
        ck.near("ZD-MM key", zd_cost(d, m, K, (n0,) * 3, (r,) * 3, a, p, j, 2.8, None, d > 1)[0], total)
        if lvl == 128:
            b3 = piglet_search(d, m, K, n0, r, 3.0)
            print(f"  ordinary MM key at omega 3: {b3[0]:.1f}")
            ck.true("ordinary MM below target at omega 2.8 and 3", b[0] < lvl and b3[0] < lvl)
            ck.true("the cheapest choice removes one block", b[2][2] == n0 - b[1][2] and b[1][2] > 0)
            n3 = m * sum(comb(n0 - 1, k) * comb(n0, 3 * r - k) for k in range(r - 1, 2 * r + 1))
            t3 = comb(n0, r) ** 3
            print(f"  Proposition 3.1 without guessing: {log2(n3) + 1.8 * log2(t3):.1f}")
        elif lvl == 256:
            ck.true("ordinary MM above target; only ZD-MM is below, by about 2 bits",
                    b[0] > lvl and 0 < lvl - total < 2.5)
        else:
            ck.true("both estimates above target", b[0] > lvl and total > lvl)
    return ck.finish("ordinary MM key recovery with one error block removed is below the BAG-Piglet-128 target,"
                     " and ZD-MM key recovery over F_4 is below the BAG-Piglet-256 target")


# --- BRA (kem-06-8) ---------------------------------------------------------
# m, n, (w_x, w_y), modulus exponents: specification Table 2, physical p. 16;
# declared Attack-2n MM cost at omega = 2: Table 6, physical p. 30.
BRA = {
    128: (67, 121, (4, 5), (121, 18, 0), 194),
    256: (83, 161, (5, 5), (161, 18, 0), 304),
    512: (127, 221, (6, 6), (221, 8, 6, 2, 0), 556),
}
BRA_MM = {128: 116.025, 256: 166.503, 512: 321.752}
BRA_ZD = {128: ((3, 3), (0, 0), 2, 112.088), 256: ((0, 13), (0, 0), 1, 161.840), 512: ((0, 32), (0, 0), 2, 316.802)}


def gf2_mod(a, p):
    dp = p.bit_length() - 1
    while a.bit_length() - 1 >= dp:
        a ^= p << (a.bit_length() - 1 - dp)
    return a


def gf2_irreducible(p):
    """Rabin's test over F_2."""
    n = p.bit_length() - 1

    def frob(k):
        y = 2
        for _ in range(k):
            sq = 0
            for i in range(y.bit_length()):
                if (y >> i) & 1:
                    sq |= 1 << (2 * i)
            y = gf2_mod(sq, p)
        return y

    def pgcd(a, b):
        while b:
            a, b = b, gf2_mod(a, b)
        return a

    if frob(n) != 2:
        return False
    primes = [q for q in range(2, n + 1) if n % q == 0 and all(q % k for k in range(2, q))]
    return all(pgcd(frob(n // q) ^ 2, p) == 1 for q in primes)


def check_bra():
    ck = Checker("kem-06-8")
    for lvl, (m, n, w, exps, declared) in BRA.items():
        print(f"BRA-{lvl}: target {lvl}, declared key MM cost {declared} at omega 2")
        modulus = sum(1 << e for e in exps)
        ck.true("P(X) irreducible over F_2 and gcd(m, n) = 1", gf2_irreducible(modulus) and gcd(m, n) == 1)
        orig = search_two(1, m, n, (n, n), w, 2.0)
        ck.near("ordinary MM, two length-n blocks over F_(2^m) (specification's instance)", orig[0], declared, 1.0)
        trans = {om: search_two(1, n, m, (m, m), w, om) for om in (2.0, 2.807, 3.0)}
        ck.near("ordinary MM, two length-m blocks over F_(2^n) (transposed), omega 2", trans[2.0][0], BRA_MM[lvl])
        print(f"    at omega 2.807: {trans[2.807][0]:.1f}; at omega 3: {trans[3.0][0]:.1f}")
        ck.true("transposed cost below target at omega 2", trans[2.0][0] < lvl)
        if lvl == 128:
            ck.true("BRA-128 is above target at omega 2.807", trans[2.807][0] > lvl)
        else:
            ck.true("below target at omega 3 as well", trans[3.0][0] < lvl)
        a, p, j, total = BRA_ZD[lvl]
        ck.near("ZD-MM, transposed, omega 2", zd_cost(1, n, m, (m, m), w, a, p, j, 2.0)[0], total)
    return ck.finish("MM key recovery on the transposed ideal code is below all three BRA targets in the specification's cost model")


CHECKS = {"kem-03-6": check_loong, "kem-04-3": check_piglet, "kem-06-8": check_bra}


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--report-id", action="append", choices=sorted(CHECKS),
                        help="check only this report ID (repeatable)")
    args = parser.parse_args()
    ok = True
    for rid in args.report_id or sorted(CHECKS):
        ok &= CHECKS[rid]()
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
