#!/usr/bin/env python3
"""Parameter estimate for kem-04/fixing_cost.md (finding kem-04-3).

Searches the smallest BAG-Piglet-128 parameter change that brings the
maximal-minors (MM) key-recovery estimates of kem-04-3 to 2^128, inside the
specification's own constraints. Standard library only; the attack models are
imported from security/rank_mm_costs.py. It computes costs and sizes and
implements no scheme. Nothing here is a recommended parameter set.

    python3 -B kem-04/fixing_cost_params.py            controls and the costed sets
    python3 -B kem-04/fixing_cost_params.py --search   full search (about 2 minutes)

Constraints (specification section 2.2, physical p. 10): m prime, n' <= m,
k*m >= 128, eps <= min(2n - n', n' - k), error rank
w_y1*w_R21 + w_y2*w_R22 + w_e + w_x*w_r1 <= (n' - k + eps)/2, and the
failure-rate formula of physical p. 4 at most 2^-128 (physical p. 14).
Attack models, all at omega = 2.8 and required to be >= 128:
  - ordinary MM with guessed zeros, punctures and block removal, as searched by
    rank_mm_costs.piglet_search, over the assignments of ranks to blocks;
  - ZD-MM (rank_mm_costs.zd_cost) over the same guesses, either block contracted;
  - the specification's Proposition 3.1 (physical p. 19) and its (3,1) OJ
    formula (physical p. 17), so that the declared estimates are not reopened.
The encryption weights (2, 3, 3, 3) and n1 = 2 are kept as submitted.
"""
import importlib.util
import os
import sys
from functools import lru_cache
from itertools import permutations
from math import ceil, comb, log2

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "rank_mm_costs", os.path.join(_HERE, "..", "security", "rank_mm_costs.py"))
R = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(R)

LAM = 128
W = 2.8
B = (2, 3, 3, 3)  # (w_r1, w_R21, w_R22, w_e), Table 1, physical p. 8


def sizes(n, m, n1=2):
    """Specification section 1.5, physical p. 8: pk, ct in bytes."""
    return -(-(2 * m * n) // 8) + LAM // 8, -(-(2 * m * n * n1) // 8)


@lru_cache(None)
def log2_dfr(rank, tail, eps):
    """Specification failure rate (physical p. 4): P(rank of a rank x tail binary matrix < eps)."""
    total = 0
    for i in range(eps):
        num = den = 1
        for j in range(i):
            num *= (2 ** rank - 2 ** j) * (2 ** tail - 2 ** j)
            den *= 2 ** i - 2 ** j
        total += num // den
    bits = total.bit_length()
    lg = log2(total) if bits < 1000 else (bits - 60) + log2(total >> (bits - 60))
    return lg - rank * tail


def _patterns(n, rr):
    for a1 in range(0, n - rr[0] + 1):
        for p in ((0, 0, 0), (1, 0, 0), (2, 0, 0), (0, 1, 0), (0, 0, 1)):
            yield (a1, 0, 0), p
    for a1 in range(0, 9):
        for a3 in range(0, min(n - rr[2], 40) + 1):
            for p1 in (0, 1):
                yield (a1, 0, a3), (p1, 0, n - a3)


@lru_cache(None)
def mm(m, n, ranks):
    """Ordinary MM: the guesses of rank_mm_costs.piglet_search, any block order."""
    best = None
    for rr in set(permutations(ranks)):
        for a, p in _patterns(n, rr):
            c = R.mm_cost(1, m, n, (n, n, n), rr, a, p, W, None, False)
            if c is not None and (best is None or c < best[0]):
                best = (c, rr, a, p)
    return best


@lru_cache(None)
def zd(m, n, ranks):
    """ZD-MM over the same guesses plus zeros in the contracted block; None if no row applies."""
    best = None
    for rr in set(permutations(ranks)):
        cands = list(_patterns(n, rr))
        cands += [((0, 0, a3), p) for a3 in range(0, n - rr[2] + 1) for p in ((0, 0, 0), (0, 0, 1))]
        for a, p in cands:
            for j in (1, 2, 3):
                try:
                    z = R.zd_cost(1, m, n, (n, n, n), rr, a, p, j, W, None, False)
                except (ValueError, ZeroDivisionError):
                    z = None
                if z is not None and (best is None or z[0] < best[0]):
                    best = (z[0], rr, a, p, j)
    return best


def oj31(m, n, ranks):
    t1, _, t3 = sorted(ranks)
    return W * log2(m * (sum(ranks) - 1) + (n - t1)) + (n - t1) * (t1 - 1) + t3


@lru_cache(None)
def prop31(m, n, ranks):
    t1, t2, t3 = sorted(ranks)
    t = t1 + t2 + t3
    n3 = m * sum(comb(n - 1, j) * comb(n, t - j) for j in range(t2 - 1, t1 + t2 + 1))
    best = None
    for a1 in range(0, n - t1 + 1):
        for a2 in range(0, n - t2 + 1):
            if best is not None and a1 * t1 + a2 * t2 >= best:
                break
            for a3 in range(0, n - t3 + 1):
                big_t = comb(n - a1, t1) * comb(n - a2, t2) * comb(n - a3, t3)
                if n3 >= big_t - 1:
                    c = a1 * t1 + a2 * t2 + a3 * t3 + log2(n3) + (W - 1) * log2(big_t)
                    best = c if best is None else min(best, c)
                    break
    return best


def decoding(n, m, ranks):
    """Largest n' with the failure rate within 2^-128: (n', eps, error rank, log2 DFR, k) or None."""
    wx, wy1, wy2 = ranks
    rank = wy1 * B[1] + wy2 * B[2] + B[3] + wx * B[0]
    k = ceil(LAM / m)
    for npr in range(min(m, 2 * n - 1), k, -1):
        eps = max(1, 2 * rank - npr + k)
        tail = 2 * n - npr
        if eps > min(tail, npr - k):
            continue
        d = log2_dfr(rank, tail, eps)
        if d <= -LAM:
            return npr, eps, rank, d, k
    return None


def admissible(n, m, ranks, with_zd):
    dec = decoding(n, m, ranks)
    if dec is None:
        return None
    key = tuple(sorted(ranks))
    if oj31(m, n, key) < LAM or prop31(m, n, key) < LAM:
        return None
    a = mm(m, n, key)
    if a is not None and a[0] < LAM:
        return None
    z = zd(m, n, key) if with_zd else None
    if z is not None and z[0] < LAM:
        return None
    return dec


def describe(label, n, m, ranks):
    key = tuple(sorted(ranks))
    pk, ct = sizes(n, m)
    pk0, ct0 = sizes(43, 47)
    dec = decoding(n, m, ranks)
    a, z = mm(m, n, key), zd(m, n, key)
    print(f"{label}: n={n} m={m} w(x,y)={ranks}")
    print(f"  pk {pk} ({100 * (pk / pk0 - 1):+.1f}%), ct {ct} ({100 * (ct / ct0 - 1):+.1f}%)")
    if dec is None:
        print("  no n' gives a failure rate within 2^-128")
    else:
        print(f"  n'={dec[0]} k={dec[4]} eps={dec[1]} error rank={dec[2]} log2 DFR={dec[3]:.1f}")
    print(f"  ordinary MM {a[0]:.1f} (ranks {a[1]}, guesses {a[2]}, punctures {a[3]});"
          f" ZD-MM {'none' if z is None else format(z[0], '.1f')};"
          f" Proposition 3.1 {prop31(m, n, key):.1f}; OJ {oj31(m, n, key):.1f}")


def primes(lo, hi):
    return [p for p in range(lo, hi + 1) if all(p % d for d in range(2, int(p ** 0.5) + 1))]


def search(with_zd, keep=3):
    pk0, ct0 = sizes(43, 47)
    cands = sorted((max(sizes(n, m)[0] / pk0, sizes(n, m)[1] / ct0), n, m)
                   for m in primes(37, 89) for n in range(36, 81))
    ranksets = [(a, b, c) for a in range(3, 7) for b in range(3, 7) for c in range(b, 7)]
    out = []
    for _, n, m in cands:
        for ranks in ranksets:
            if admissible(n, m, ranks, with_zd):
                out.append((n, m, ranks))
                break
        if len(out) >= keep:
            break
    return out


def main():
    print("Controls")
    print(f"  sizes at (n, m) = (43, 47) and (47, 43): {sizes(43, 47)}, {sizes(47, 43)}  (Table 3: 522, 1011)")
    print(f"  log2 DFR, delta=35, eps=30, n'=43: n=47 {log2_dfr(35, 51, 30):.1f} (Table 5: -130);"
          f" n=43 {log2_dfr(35, 43, 30):.1f}")
    for lvl, (n, npr, eps, delta) in {256: (78, 78, 54, 64), 384: (106, 103, 77, 88), 512: (125, 130, 92, 109)}.items():
        print(f"  log2 DFR BAG-Piglet-{lvl}: {log2_dfr(delta, 2 * n - npr, eps):.1f}")
    print(f"  Proposition 3.1 at (n, m) = (47, 43): {prop31(43, 47, (4, 4, 4)):.1f} (Table 5: 148);"
          f" OJ {oj31(43, 47, (4, 4, 4)):.1f} (Table 5: 158)")
    a, z = mm(47, 43, (4, 4, 4)), zd(47, 43, (4, 4, 4))
    print(f"  kem-04-3 at (n, m) = (43, 47): ordinary MM {a[0]:.3f}, ZD-MM {z[0]:.3f}"
          f"  (report: 118.4, 114.6; paper {R.PIGLET_MM[128]}, {R.PIGLET_ZD[128][3]})")
    z5 = zd(131, 125, (8, 8, 8))
    print(f"  ZD-MM search at BAG-Piglet-512: {z5[0]:.3f}  (paper {R.PIGLET_ZD[512][3]})")
    print()
    describe("Submitted, as coded and in Table 2", 43, 47, (4, 4, 4))
    describe("Submitted, as listed in Table 1", 47, 43, (4, 4, 4))
    describe("Costed set, ordinary MM and ZD-MM at 2^128", 55, 43, (5, 4, 4))
    describe("One step smaller", 54, 43, (5, 4, 4))
    describe("Ordinary MM at 2^128, ZD-MM within 2 bits of it", 54, 43, (4, 4, 4))
    describe("Ordinary MM alone at 2^128", 51, 43, (4, 4, 4))
    if "--search" in sys.argv:
        for with_zd in (False, True):
            print()
            print("Search, n in 36..80, m prime in 37..89, key weights in 3..6,",
                  "with ZD-MM" if with_zd else "ordinary MM only")
            for n, m, ranks in search(with_zd):
                describe("  smallest sizes", n, m, ranks)
    return 0


if __name__ == "__main__":
    sys.exit(main())
