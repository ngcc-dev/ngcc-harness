#!/usr/bin/env python3
"""The specification's other recorded attacks at the kem-06-8 costed BRA sets.

Standard library only. It prints attack-cost exponents; it implements no part
of BRA, and the parameter sets are the costing aids of kem-06/fixing_cost.md,
not recommended parameters.

1. "Attack-3n" of Table 6 (physical p. 30): maximal minors (MM) on the
   three-block ciphertext instance, an [3n, n] ideal code over F_(2^m) with
   block weights (w_r1, w_e, w_r2). The cost formula is mm_cost of
   security/rank_mm_costs.py (Theorem 7.7, physical pp. 27-28), with one
   change: the equation count E is taken for the block order that gives the
   most equations, where rank_mm_costs.direct_counts takes the fewest. The
   specification calls its E "an upper bound" (Theorem 7.6), and only the
   largest order reproduces the equation rates of Table 6 (0.54 and 0.25 at
   BRA-256 and BRA-512). Searched: punctures 0 to 2 in each block, zero
   guesses in one block, or the same number (0 to 20) in all three.
2. AGHT and PRR of Table 7 (physical p. 30) on the specification's two-block
   key instance and on the three-block instance, from Theorems 7.2 and 7.3
   (physical pp. 21-24) at omega = 2. The OJ attack is not modeled.

Run:  python3 -B kem-06/fixing_cost_attack3n.py   (about 3 minutes on 9 cores)
"""
import sys

sys.dont_write_bytecode = True

import importlib.util
import os
from itertools import permutations
from math import ceil, comb, log2
from multiprocessing import get_context

HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "rank_mm_costs", os.path.join(HERE, "..", "security", "rank_mm_costs.py"))
mm = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(mm)

OMEGA = 2.0

# name: m, n, (w_x, w_y), (w_r1, w_e, w_r2), target. Submitted: Table 2
# (physical p. 16). Low and high ends: kem-06/fixing_cost.md.
SETS = (
    ("BRA-128 submitted", 67, 121, (4, 5), (5, 8, 5), 128),
    ("BRA-256 submitted", 83, 161, (5, 5), (6, 8, 6), 256),
    ("BRA-512 submitted", 127, 221, (6, 6), (7, 8, 8), 512),
    ("BRA-128 low end", 80, 103, (4, 5), (4, 8, 5), 128),
    ("BRA-256 low end", 111, 145, (5, 6), (5, 8, 6), 256),
    ("BRA-512 low end", 147, 229, (7, 7), (7, 8, 7), 512),
    ("BRA-128 high end", 71, 132, (4, 6), (5, 8, 5), 128),
    ("BRA-256 high end", 94, 187, (5, 7), (6, 8, 6), 256),
    ("BRA-512 high end", 151, 244, (7, 7), (7, 8, 8), 512),
)
# Table 6 Attack-3n (a1, a2, a3, p1, p2, p3, TC) and Table 7 (AGHT, PRR) for
# Attack-2n and Attack-3n, physical p. 30.
TABLE6_3N = {"BRA-128": ((0, 0, 0), (1, 1, 2), 189), "BRA-256": ((5, 5, 5), (0, 0, 0), 321),
             "BRA-512": ((0, 0, 43), (0, 0, 0), 567)}
TABLE7 = {"BRA-128": (264, 235, 374, 632), "BRA-256": (364, 364, 506, 966),
          "BRA-512": (670, 670, 893, 1585)}


def counts_max(m, u, r, kprime):
    """rank_mm_costs.direct_counts with the block order that maximises E."""
    act = [i for i in range(len(u)) if u[i] > 0]
    v = 1
    for i in act:
        v *= comb(u[i], r[i])
    tot = sum(r[i] for i in act)
    best = 0
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
        best = max(best, c)
    return v, m * best


def mm3(m, n, w, a, p):
    """mm_cost for three length-n blocks of dimension n over F_(2^m), largest E."""
    blk = (n, n, n)
    u = [ni - ai - pi for ni, ai, pi in zip(blk, a, p)]
    kprime = n - sum(a)
    if kprime < 0 or any(x < 0 for x in u):
        return None
    if any(0 < u[i] < w[i] or (u[i] == 0 and blk[i] - a[i] < w[i]) for i in range(3)):
        return None
    v, e = counts_max(m, u, w, kprime)
    if v <= 0 or e < v - 1:
        return None
    rank = sum(w[i] for i in range(3) if u[i] > 0)
    setup, check = mm.check_cost(m, blk, u, rank)
    work = setup + 2.0 ** (log2(e) + (OMEGA - 1) * log2(v)) + rank * sum(u) * v + check
    return mm.neg_log2_p(1, blk, a, p, w, False) + log2(work), e / v


def search3(args):
    name, m, n, w = args
    guesses = {(x, x, x) for x in range(21)}
    for j in range(3):
        for x in range(n - w[j] + 1):
            a = [0, 0, 0]
            a[j] = x
            guesses.add(tuple(a))
    best = None
    for p in [(x, y, z) for x in range(3) for y in range(3) for z in range(3)]:
        for a in sorted(guesses):
            c = mm3(m, n, w, a, p)
            if c is not None and (best is None or c[0] < best[0]):
                best = (c[0], a, p, c[1])
    return name, best


def aght(m, big_n, k, r):
    s = OMEGA * log2((big_n - k - 1) * m) + r * ceil((k + 1) * m / big_n) - m
    c = OMEGA * log2(big_n - k) + 2 * log2(m) + r * k
    return min(s, c)


def prr(m, n, big_n, k, rs):
    """Theorem 7.3 with equal block lengths n: the cheaper of PRR-S and PRR-C."""
    out = []
    for cap, limit, poly in ((min(m, m * (big_n - k - 1) // n), m, OMEGA * log2(m * (big_n - k - 1)) - m),
                             (big_n - k, n, OMEGA * log2(big_n - k) + 2 * log2(m))):
        if sum(rs) > cap:
            continue
        t, left = list(rs), cap - sum(rs)
        for i in sorted(range(len(rs)), key=lambda i: -rs[i]):
            add = min(left, limit - t[i])
            t[i] += add
            left -= add
        out.append(poly + sum(r * (limit - ti) for r, ti in zip(rs, t)))
    return min(out)


def main():
    print("Controls, Table 6 Attack-3n at its stated guesses and punctures (omega = 2):")
    for lvl, (a, p, tc) in TABLE6_3N.items():
        m, n, w = next((s[1], s[2], s[4]) for s in SETS if s[0] == f"{lvl} submitted")
        c = mm3(m, n, w, a, p)
        print(f"  {lvl}: a = {a}, p = {p}: {c[0]:.1f} (E/V = {c[1]:.2f}); Table 6: {tc}")
    print("Controls, Table 7 (AGHT, PRR) for Attack-2n and Attack-3n, omega = 2:")
    for name, m, n, wk, wc, _ in SETS[:3]:
        lvl = name.split()[0]
        got = (aght(m, 2 * n, n, sum(wk)), prr(m, n, 2 * n, n, wk),
               aght(m, 3 * n, n, sum(wc)), prr(m, n, 3 * n, n, wc))
        print(f"  {lvl}: " + ", ".join(f"{x:.1f}" for x in got) + f"; Table 7: {TABLE7[lvl]}")
    print("Attack-3n MM search and Table 7 models at each set (target in brackets):")
    with get_context("fork").Pool(len(SETS)) as pool:
        found = dict(pool.map(search3, [(s[0], s[1], s[2], s[4]) for s in SETS]))
    for name, m, n, wk, wc, lvl in SETS:
        c, a, p, rate = found[name]
        comb2 = min(aght(m, 2 * n, n, sum(wk)), prr(m, n, 2 * n, n, wk))
        comb3 = min(aght(m, 3 * n, n, sum(wc)), prr(m, n, 3 * n, n, wc))
        verdict = "at or above" if min(c, comb2, comb3) >= lvl else "BELOW"
        print(f"  {name} [{lvl}]: m = {m}, n = {n}, (w_r1, w_e, w_r2) = {wc}: Attack-3n MM {c:.1f} "
              f"(a = {a}, p = {p}, E/V = {rate:.2f}); AGHT/PRR Attack-2n {comb2:.1f}, Attack-3n "
              f"{comb3:.1f}: {verdict} the target")


if __name__ == "__main__":
    main()
