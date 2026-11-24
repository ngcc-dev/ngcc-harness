#!/usr/bin/env python3
"""BAG-Piglet-256 parameter estimate for kem-04/fixing_cost.md (finding kem-04-3).

kem-04-3 puts the zero-direction maximal-minors (ZD-MM) key-recovery estimate
of BAG-Piglet-256 at 2^254.0, after the instance is packed over F_4 (ePrint
2026/2994, Section 4.4 and Table 13). This script searches the smallest
change of n, m, n', eps, k and the key weight that brings every model below
to 2^256 with no margin. The encryption weights (3, 3, 4, 4) and n1 = 2 are
kept as submitted (Table 1, physical p. 8). Standard library only: the attack
models come from security/rank_mm_costs.py, the specification's formulas
from kem-04/fixing_cost_enc_weights.py, and the decapsulation counts from
kem-04/fixing_cost_decaps_count.py. It computes costs and sizes and
implements no scheme. Nothing here is a recommended parameter set.

    python3 -B kem-04/fixing_cost_piglet256.py

Constraints (specification section 2.2, physical p. 10): m prime, n' <= m,
k*m >= 256, eps <= min(2n - n', n' - k), error rank at most
(n' - k + eps)/2, and the failure-rate formula of physical p. 4 at most
2^-256 (physical p. 14). The submitted n' = 78, k = 4, eps = 54 are kept
when they meet these; otherwise, as in fixing_cost_params.py, n' is the
largest value that does.

Models at omega = 2.8, each required to be at least 256:
  - ordinary MM (rank_mm_costs.piglet_search) and ZD-MM (rank_mm_costs.zd_cost,
    guesses in the first block or in the contracted block, every block
    contracted) on the key instance, unpacked and packed over F_(2^d) for each
    d in 2..4 that divides n;
  - Proposition 3.1 and the (3,1) OJ formula on the key instance, and
    Proposition 3.2 and the (4,2) OJ formula on the message instance.

Searched: n even in 78..110, m prime in 79..113, equal key weights 6..8.
Odd n is not searched, because the packed models need d | n and the effect
of an odd length on packing is not modelled. Unequal key weights and other
encryption weights are not searched.
"""
import importlib.util
import itertools
import os
import sys
from functools import lru_cache
from math import ceil

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


R = _load("rank_mm_costs", os.path.join(_HERE, "..", "security", "rank_mm_costs.py"))
F = _load("fixing_cost_enc_weights", os.path.join(_HERE, "fixing_cost_enc_weights.py"))
C = _load("fixing_cost_decaps_count", os.path.join(_HERE, "fixing_cost_decaps_count.py"))

LAM = 256
W = 2.8
ENC = (3, 3, 4, 4)  # (w_r1, w_R21, w_R22, w_e), Table 1, physical p. 8
BASE = (78, 79, 6)  # (n, m, key weight) as submitted


def sizes(n, m, n1=2):
    """Specification section 1.5, physical p. 8: pk, ct in bytes."""
    return -(-(2 * m * n) // 8) + LAM // 8, -(-(2 * m * n * n1) // 8)


@lru_cache(None)
def decoding(n, m, w):
    """(n', eps, error rank, log2 DFR, k) with the failure rate within 2^-256, or None."""
    rank = F.error_rank((w, w, w), ENC)
    npr, eps, k = 78, 54, 4  # submitted, Table 1
    if npr <= m and k * m >= LAM and 2 * rank <= npr - k + eps and eps <= min(2 * n - npr, npr - k):
        d = F.log2_dfr(rank, 2 * n - npr, eps)
        if d <= -LAM:
            return npr, eps, rank, d, k
    k = ceil(LAM / m)
    for npr in range(min(m, 2 * n - 1), k, -1):
        eps = max(1, 2 * rank - npr + k)
        tail = 2 * n - npr
        if eps > min(tail, npr - k):
            continue
        d = F.log2_dfr(rank, tail, eps)
        if d <= -LAM:
            return npr, eps, rank, d, k
    return None


@lru_cache(None)
def key_models(n, m, w):
    """{(method, d): (log2 cost, detail)} over the packings d that divide n."""
    out = {}
    for d in (1, 2, 3, 4):
        if n % d:
            continue
        n0 = n // d
        b = R.piglet_search(d, m, n0, n0, w, W)
        if b is not None:
            out[("ordinary MM", d)] = (b[0], f"guesses {b[1]}, punctures {b[2]}")
        best = None
        guesses = [((a1, 0, 0), p) for a1 in range(0, n0 - w + 1)
                   for p in ((0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1))]
        guesses += [((0, 0, a3), p) for a3 in range(0, n0 - w + 1) for p in ((0, 0, 0), (0, 0, 1))]
        for (a, p), j in itertools.product(guesses, (1, 2, 3)):
            try:
                z = R.zd_cost(d, m, n0, (n0,) * 3, (w,) * 3, a, p, j, W, None, d > 1)
            except (ValueError, ZeroDivisionError):
                z = None
            if z is not None and (best is None or z[0] < best[0]):
                best = (z[0], f"guesses {a}, punctures {p}, block {j} contracted")
        if best is not None:
            out[("ZD-MM", d)] = best
    return out


@lru_cache(None)
def spec_models(n, m, w):
    return {
        "Proposition 3.1 (key)": F.prop31(m, n, (w, w, w)),
        "(3,1) OJ (key)": F.oj31(m, n, (w, w, w)),
        "Proposition 3.2 (message)": F.prop32(m, n, ENC),
        "(4,2) OJ (message)": F.oj42(m, n, ENC),
    }


def admissible(n, m, w):
    if decoding(n, m, w) is None:
        return False
    if min(spec_models(n, m, w).values()) < LAM:
        return False
    return min(c for c, _ in key_models(n, m, w).values()) >= LAM


def work(n, m, w):
    """Counted decapsulation field work and ring-product multiplications."""
    npr, eps, _, _, k = decoding(n, m, w)
    ring, enc, parts, _ = C.operation_counts(n, m, npr, k, eps)
    return ring.mul, C.total([ring, enc, C.total(parts.values())])


def describe(label, n, m, w):
    pk, ct = sizes(n, m)
    pk0, ct0 = sizes(*BASE[:2])
    print(f"{label}: n = {n}, m = {m}, key weights ({w}, {w}, {w})")
    print(f"  pk {pk} B ({100 * (pk / pk0 - 1):+.1f}%), ct {ct} B ({100 * (ct / ct0 - 1):+.1f}%)")
    dec = decoding(n, m, w)
    if dec is None:
        print("  no n' gives a failure rate within 2^-256")
        return
    print(f"  n' = {dec[0]}, k = {dec[4]}, eps = {dec[1]}, error rank {dec[2]}, log2 DFR {dec[3]:.1f}")
    for (meth, d), (c, detail) in sorted(key_models(n, m, w).items(), key=lambda x: x[1][0]):
        print(f"  {meth}, d = {d}: {c:.3f} ({detail})")
    print("  " + "; ".join(f"{k} {v:.1f}" for k, v in spec_models(n, m, w).items()))
    if (n, m, w) != BASE:
        ring0, all0 = work(*BASE)
        ring, alln = work(n, m, w)
        print(f"  ring-product multiplications x{ring / ring0:.3f} (x{ring / ring0 * m / BASE[1]:.3f} per m)")
        ratios = [C.cost(alln, m, BASE[1], per_m, sq) / C.cost(all0, BASE[1], BASE[1], per_m, sq)
                  for per_m, sq in itertools.product((False, True), (0, 1))]
        print(f"  decapsulation counted work x{min(ratios):.3f} to x{max(ratios):.3f}"
              " (multiplication charged 1 or m/79; squaring 0 or 1)")


def primes(lo, hi):
    return [p for p in range(lo, hi + 1) if all(p % d for d in range(2, int(p ** 0.5) + 1))]


def search(keep=3):
    pk0, ct0 = sizes(*BASE[:2])
    cands = sorted((max(sizes(n, m)[0] / pk0, sizes(n, m)[1] / ct0), n, m)
                   for m in primes(79, 113) for n in range(78, 111, 2))
    out = []
    for _, n, m in cands:
        for w in (6, 7, 8):
            if admissible(n, m, w):
                out.append((n, m, w))
                break
        if len(out) >= keep:
            break
    return out


def main():
    print("Controls")
    print(f"  sizes at (n, m) = (78, 79): {sizes(78, 79)}  (Table 3: 1573, 3081)")
    km = key_models(*BASE)
    print(f"  packed over F_4: ordinary MM {km[('ordinary MM', 2)][0]:.3f}, ZD-MM {km[('ZD-MM', 2)][0]:.3f}"
          f" (paper {R.PIGLET_MM[256]}, {R.PIGLET_ZD[256][3]})")
    print(f"  log2 DFR at n' = 78, eps = 54: {decoding(*BASE)[3]:.1f} (Table 5: -273)")
    print(f"  OJ (3,1) and (4,2): {F.oj31(79, 78, (6, 6, 6)):.1f}, {F.oj42(79, 78, ENC):.1f} (Table 5: 395, 332)")
    print()
    describe("Submitted BAG-Piglet-256", *BASE)
    describe("n = 80, the next even length", 80, 79, 6)
    print()
    print("Search, n even in 78..110, m prime in 79..113, key weights 6..8, smallest sizes first")
    for n, m, w in search():
        describe("  candidate", n, m, w)
    return 0


if __name__ == "__main__":
    sys.exit(main())
