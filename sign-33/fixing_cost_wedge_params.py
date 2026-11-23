#!/usr/bin/env python3
"""Fixing-cost aid for sign-33-9: smallest VDOO-512 parameter change that
brings the report's wedge estimate to 2^512, with no margin.

The cost model is the one certified by reproduce_wedge_estimate.py (imported
from this directory): the truncated-Hilbert-series recovery degree and the
gate conversion of ePrint 2026/2528 as recorded in report.md.  This script
adds only a search over the restriction k and the ring width 2^ell, and the
mapping from (v, d, o1, o2) to the two instances that the report quotes:

  reconciliation  UOV(n, o2, m)             n = v+d+o1+o2, m = d+o1+o2
  intersection    UOV(n, 3*o2 - n, 3*m - 2) when 3*o2 > n

The mapping is inferred from the report's figures (455, 175, 195) and
(455, 70, 583) and from the condition n < 3*o2 in specification section 5.5
(physical p. 18).  The paper itself was not read.  Nothing here validates the
semi-regularity or generic-rank heuristics, and no other attack is modelled.
The printed parameter sets are costing aids, not recommended parameters.
"""

from functools import lru_cache
from math import comb, log2
import os
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import reproduce_wedge_estimate as wedge  # noqa: E402

Q = 256
TARGET = 512.0
BOUND = 520.0  # the search reports nothing above 2^BOUND


@lru_cache(maxsize=None)
def coeff(variables, width, degree):
    return wedge.truncated_ring_coefficient(variables, width, degree)


def gates(n, o, q, width, degree):
    gap = coeff(n, width, degree) - coeff(o, width, degree) + 1
    if gap <= 0:
        return None
    return (log2(3) + log2(comb(n + 1, 2)) + 2 * log2(gap)
            + log2(2 * log2(q) ** 2 + log2(q)))


def exponent(n, o, m, q, k, ell, bound):
    """(log2 gates, D) at restriction k, or None when the cost passes `bound`
    before a recovery degree is reached (the cost grows with the degree below
    the middle of the ring) or when no recovery degree exists."""
    n -= k
    o -= k
    if o < 1:
        return None
    width = 2 ** ell
    quotient = []
    for degree in range(o * (width - 1) + 1):
        value = coeff(n, width, degree)
        for j in range(1, degree // 2 + 1):
            den = coeff(m, width, j)
            if den:
                value -= den * quotient[degree - 2 * j]
        quotient.append(value)
        if degree >= 2:
            if value <= coeff(o, width, degree):
                return gates(n, o, q, width, degree), degree
            cost = gates(n, o, q, width, degree)
            if degree < n * (width - 1) // 2 and cost is not None and cost > bound:
                return None
    return None


def cheapest(n, o, m, q, bound=BOUND):
    """Cheapest (log2 gates, k, D, ell) below `bound` over all k and ell."""
    best = None
    for ell in range(1, int(log2(q)) + 1):
        for k in range(o):
            r = exponent(n, o, m, q, k, ell, bound)
            if r and r[0] is not None and (best is None or r[0] < best[0]):
                best = (r[0], k, r[1], ell)
    return best


def show(r):
    if r is None:
        return f"none below 2^{BOUND:.0f}"
    return f"2^{r[0]:.2f} at (k, D, ell) = ({r[1]}, {r[2]}, {r[3]})"


def vdoo(v, d, o1, o2):
    n, m = v + d + o1 + o2, d + o1 + o2
    rec = cheapest(n, o2, m, Q)
    oi = 3 * o2 - n
    inter = cheapest(n, oi, 3 * m - 2, Q) if oi >= 1 else None
    return n, m, rec, oi, inter


def sizes(n, m):
    """Specification section 4.7 and eq. (6): q = 256, one byte per element."""
    return m * n * (n + 1) // 2, n + 16


def keygen_work(n, m):
    """Byte multiply-adds of generate_public_key (vdoo_keypair.c:296-355):
    apply_T_to_F is two batched products, m n^3 / 2 and m n^3; apply_S_to_FT
    is m^2 n(n+1)/2."""
    return 1.5 * m * n**3 + m * m * n * (n + 1) / 2


def main():
    print("Control: the search returns the published points")
    for name, n, o, m, q, pub in (
        ("VDOO-128 reconciliation", 137, 40, 70, 16, (19, 21, 1, 172.20)),
        ("VDOO-256 reconciliation", 300, 68, 95, 256, (2, 198, 2, 947.65)),
        ("VDOO-512 reconciliation", 455, 175, 195, 256, (123, 52, 1, 432.10)),
        ("VDOO-512 intersection", 455, 70, 583, 256, (33, 37, 1, 379.18)),
    ):
        r = cheapest(n, o, m, q, bound=pub[3] + 0.5)
        assert r is not None and r[1:] == pub[:3] and abs(r[0] - pub[3]) < 0.01, (name, r)
        print(f"  {name}: {show(r)}")

    pk0, sig0 = sizes(455, 195)
    assert (pk0, sig0) == (20229300, 471)  # Table 1: 19755.2 KB and 471 B
    print(f"Baseline VDOO-512 (260, 10, 10, 175): pk {pk0} B, signature {sig0} B")

    print("Move t variables from the second oil layer to the first (n, m fixed)")
    for t in (0, 24, 32, 33, 34, 35):
        n, m, rec, oi, inter = vdoo(260, 10, 10 + t, 175 - t)
        tail = f"intersection (o = {oi}) {show(inter)}" if oi >= 1 else f"3*o2 - n = {oi}, no intersection"
        print(f"  t = {t:2d}  (260, 10, {10 + t}, {175 - t})  reconciliation {show(rec)}; {tail}")

    print("Add dv vinegar variables (layers fixed)")
    for dv in (0, 32, 33, 34, 35):
        n, m, rec, oi, inter = vdoo(260 + dv, 10, 10, 175)
        pk, sig = sizes(n, m)
        tail = f"intersection (o = {oi}) {show(inter)}" if oi >= 1 else "no intersection"
        print(f"  dv = {dv:2d}  ({260 + dv}, 10, 10, 175)  n = {n}  reconciliation {show(rec)}; {tail};"
              f" pk {pk} B ({100 * (pk / pk0 - 1):+.2f}%), signature {sig} B ({100 * (sig / sig0 - 1):+.2f}%)")

    print("Mixed: move t and add dv (the estimate follows t + dv)")
    for t, dv in ((17, 16), (17, 17), (10, 24)):
        n, m, rec, oi, inter = vdoo(260 + dv, 10, 10 + t, 175 - t)
        pk, sig = sizes(n, m)
        tail = f"intersection (o = {oi}) {show(inter)}" if oi >= 1 else "no intersection"
        print(f"  t = {t}, dv = {dv}  ({260 + dv}, 10, {10 + t}, {175 - t})  n = {n}  reconciliation {show(rec)}; {tail};"
              f" pk {100 * (pk / pk0 - 1):+.2f}%, signature {100 * (sig / sig0 - 1):+.2f}%,"
              f" key generation {100 * (keygen_work(n, m) / keygen_work(455, 195) - 1):+.1f}%")

    for label, params in (("rebalanced", (260, 10, 44, 141)), ("more vinegar", (294, 10, 10, 175))):
        n, m, rec, oi, inter = vdoo(*params)
        ok = rec is None or rec[0] >= TARGET
        ok = ok and (oi < 1 or inter is None or inter[0] >= TARGET)
        pk, sig = sizes(n, m)
        print(f"Smallest {label} set {params}: wedge estimate at or above 2^512: {ok};"
              f" pk {pk} B ({100 * (pk / pk0 - 1):+.2f}%), signature {sig} B ({100 * (sig / sig0 - 1):+.2f}%)")
        print(f"  work ratios to the baseline: key generation {keygen_work(n, m) / keygen_work(455, 195):.3f},"
              f" verification (m n(n+1)/2) {pk / pk0:.3f}")


if __name__ == "__main__":
    main()
