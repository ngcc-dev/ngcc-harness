#!/usr/bin/env python3
"""Parameter estimate for kem-03/fixing_cost.md (findings kem-03-4 and kem-03-6).

Searches, for each BAG-Loong level, the parameter set with the smallest
growth of the larger of public key and ciphertext that brings the attack
estimates of kem-03-4 and kem-03-6 to the target, inside the specification's
own constraints. Standard library only; the maximal-minors models are imported
from security/rank_mm_costs.py. It computes costs and sizes and implements no
scheme. Nothing here is a recommended parameter set.

    python3 -B kem-03/fixing_cost_params.py
        controls and the sets found (under a second)
    python3 -B kem-03/fixing_cost_params.py --search 256
        redo one search; about 2, 6, 10 and 25 minutes for 128, 256, 384 and 512
    python3 -B kem-03/fixing_cost_params.py --search 256 --theorem-only
        require only Theorem 3.4 and the merged-rank form, without the MM refinements
    python3 -B kem-03/fixing_cost_params.py --search 384 --partial-overlap
        require g < min(t1, t2), as in every submitted rank matrix

Constraints (specification sections 1.2 and 2.2, physical pp. 3-4 and 10-11):
m prime, n' <= m, k = ceil(level / m), eps <= min(n1*n2 - n', n' - k), error
rank r1*t1 + r2*t2 - g'*g <= (n' - k + eps)/2 and the failure-rate formula at
most 2^-level (physical p. 14).
Attack models, each required to reach the level:
  kem-03-4  merged-rank key recovery, closed form
            (m(N-k-1))^w * 2^(r*ceil((k+1)m/N) - m), N = 2n, k = n, w = 2.807;
  kem-03-6  Theorem 3.4 for the key and message instances, w = 2.807;
            ordinary MM (rank_mm_costs.search_two) and ZD-MM
            (rank_mm_costs.zd_cost, guesses (a, 0), at most one puncture in the
            first block, either block contracted), w = 2.8;
  guards    the specification's OJ formula (Theorem 3.2) and its PRR cases
            (section 3.3.2), w = 2.807, so that declared estimates are not reopened.
"""
import importlib.util
import os
import sys
import time
from functools import lru_cache
from math import ceil, log2

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "rank_mm_costs", os.path.join(_HERE, "..", "security", "rank_mm_costs.py"))
R = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(R)

W = 2.807   # report kem-03-4 and kem-03-6, specification formulas
WB = 2.8    # ePrint 2026/2994 refinements

# level: n, m, n', n1, n2, eps, k, A_(X,Y) = (t1, t2, g), A_(R1,E|R2) = (r1, r2, g'); Table 1, physical p. 8
SUBMITTED = {
    128: (42, 47, 46, 10, 10, 33, 3, (5, 5, 4), (5, 5, 3)),
    256: (65, 67, 67, 12, 13, 51, 4, (5, 6, 3), (6, 6, 3)),
    384: (83, 83, 83, 14, 15, 66, 5, (6, 6, 3), (7, 7, 4)),
    512: (104, 97, 97, 15, 15, 73, 6, (7, 7, 4), (7, 7, 4)),
}
TABLE3 = {128: (2500, 3055), 256: (6597, 8384), 384: (12152, 15096), 512: (19043, 21644)}
# Search results, same layout as SUBMITTED. Costing aids, not recommended parameters.
COSTED = {
    128: (40, 47, 47, 11, 10, 36, 3, (5, 4, 2), (4, 6, 2)),
    256: (67, 73, 73, 13, 14, 59, 4, (6, 5, 1), (5, 7, 1)),
    384: (78, 101, 101, 15, 16, 83, 4, (9, 9, 9), (10, 9, 9)),
    512: (91, 127, 127, 17, 16, 98, 5, (10, 10, 10), (11, 10, 10)),
}
THEOREM_ONLY = {
    128: (43, 43, 43, 10, 10, 30, 3, (4, 4, 1), (4, 5, 1)),
    256: (65, 73, 73, 13, 14, 59, 4, (6, 5, 1), (5, 7, 1)),
    384: (75, 101, 101, 15, 16, 83, 4, (9, 9, 9), (10, 10, 10)),
    512: (87, 127, 127, 17, 16, 98, 5, (10, 10, 10), (11, 11, 11)),
}
PARTIAL_OVERLAP = {
    384: (88, 101, 101, 14, 15, 75, 4, (8, 8, 7), (8, 8, 6)),
    512: (118, 109, 109, 16, 16, 88, 5, (7, 7, 3), (7, 8, 3)),
}


def sizes(level, n, m, n1, n2):
    """Specification section 1.5, physical p. 8: pk, ct in bytes."""
    return 2 * level // 8 + -(-(n * n1 * m) // 8), -(-((n * n2 + n1 * n2) * m) // 8)


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


@lru_cache(None)
def merged_prr_key(m, n, r):
    return W * log2(m * (n - 1)) + r * ceil((n + 1) * m / (2 * n)) - m


@lru_cache(None)
def thm_key(m, n, t):
    return R.loong_theorem(m, n, n, n, *t, W)


@lru_cache(None)
def thm_msg(m, n, n1, s):
    return R.loong_theorem(m, n, n + n1, n, *s, W)


@lru_cache(None)
def mm_key(m, n, t):
    b = R.search_two(1, m, n, (n, n + 1), t[:2], WB, t[2])
    return None if b is None else b[0]


@lru_cache(None)
def mm_msg(m, n, n1, s):
    b = R.search_two(1, m, n, (n, n + n1), s[:2], WB, s[2])
    return None if b is None else b[0]


def _zd(m, k, blocks, t):
    best = None
    for p in ((0, 0), (1, 0)):
        for j in (1, 2):
            for a1 in range(0, blocks[0] - t[0] + 1):
                try:
                    z = R.zd_cost(1, m, k, blocks, t[:2], (a1, 0), p, j, WB, t[2])
                except (ValueError, ZeroDivisionError):
                    z = None
                if z is not None and (best is None or z[0] < best):
                    best = z[0]
    return best


@lru_cache(None)
def zd_key(m, n, t):
    return _zd(m, n, (n, n + 1), t)


@lru_cache(None)
def zd_msg(m, n, n1, s):
    return _zd(m, n, (n, n + n1), s)


def _oj(m, d1, t1, t2, g):
    return W * log2(m * (t1 + t2 - g - 1) + (d1 - t1)) + (d1 - t1) * (t1 - 1) + t2


def oj_key(m, n, t):
    return min(_oj(m, n, t[0], t[1], t[2]), _oj(m, n, t[1], t[0], t[2]))


def oj_msg(m, n, n1, s):
    return min(_oj(m, n, s[0], s[1], s[2]), _oj(m, n + n1, s[1], s[0], s[2]))


@lru_cache(None)
def prr_spec(m, d1, d2, t1, t2, g):
    """Specification section 3.3.2 (physical pp. 18-21), code [d1 + d2, d1 + 1]; cheapest applicable case."""
    eqs = d2 - 1
    best = None

    def upd(c):
        nonlocal best
        best = c if best is None else min(best, c)
    if d1 >= m and d2 >= m:
        pre = W * log2(m * eqs)
        for r1 in range(t1, m + 1):
            for r2 in range(t2, m + 1):
                if r1 + r2 - g <= m and d1 * (r1 - g) + d2 * (r2 - g) <= m * eqs:
                    upd(pre + t1 * (m - r1) + t2 * (m - r2) - m)
    if d1 <= m and d2 <= m:
        pre = W * log2(m * m * eqs)
        for r1 in range(t1, d1 + 1):
            for r2 in range(t2, d2 + 1):
                if (r1 - g) + (r2 - g) <= eqs:
                    upd(pre + t1 * (d1 - r1) + t2 * (d2 - r2))
    if d1 <= m < d2:
        pre = W * log2(m * m * eqs)
        for r1 in range(t1, d1 + 1):
            for r2 in range(t2, m + 1):
                if (r1 - g) + (r2 - g) <= eqs and d1 * r1 + d2 * (r2 - g) <= m * eqs:
                    upd(pre + t1 * (d1 - r1) + t2 * (m - r2) - m)
    return best


def error_rank(t, s):
    return s[0] * t[0] + s[1] * t[1] - s[2] * t[2]


def fmt(x):
    return "none" if x is None else f"{x:.1f}"


def describe(label, level, row):
    n, m, npr, n1, n2, eps, k, t, s = row
    pk, ct = sizes(level, n, m, n1, n2)
    pk0, ct0 = TABLE3[level]
    rank = error_rank(t, s)
    print(f"{label}: n={n} m={m} n'={npr} n1={n1} n2={n2} eps={eps} k={k} A_XY={t} A_R={s}")
    print(f"  pk {pk} ({100 * (pk / pk0 - 1):+.1f}%), ct {ct} ({100 * (ct / ct0 - 1):+.1f}%);"
          f" error rank {rank}, delta {(npr - k + eps) // 2}, tail {n1 * n2 - npr},"
          f" log2 DFR {log2_dfr(rank, n1 * n2 - npr, eps):.1f}, k*m {k * m}")
    print(f"  merged-rank key {merged_prr_key(m, n, t[0] + t[1] - t[2]):.1f};"
          f" Theorem 3.4 key {fmt(thm_key(m, n, t))}, message {fmt(thm_msg(m, n, n1, s))};"
          f" ordinary MM {fmt(mm_key(m, n, t))}, {fmt(mm_msg(m, n, n1, s))};"
          f" ZD-MM {fmt(zd_key(m, n, t))}, {fmt(zd_msg(m, n, n1, s))}")
    print(f"  guards: OJ {oj_key(m, n, t):.1f}, {oj_msg(m, n, n1, s):.1f};"
          f" specification PRR {fmt(prr_spec(m, n, n, *t))}, {fmt(prr_spec(m, n, n + n1, *s))}")


def primes(lo, hi):
    return [p for p in range(max(2, lo), hi + 1) if all(p % d for d in range(2, int(p ** 0.5) + 1))]


@lru_cache(None)
def req_tail(level, rank, eps):
    """Smallest tail length with the failure rate at most 2^-level."""
    gap = rank - eps + 1
    if gap <= 0:
        return None
    tail = max(eps, eps - 1 + level // gap - 2)
    while log2_dfr(rank, tail, eps) > -level:
        tail += 1
    while tail > eps and log2_dfr(rank, tail - 1, eps) <= -level:
        tail -= 1
    return tail


def search(level, refinements=True, partial=False, keep=3):
    """Box: n0-30..n0+40, m prime in m0-10..m0+50, n1 and n2 within -3..+6,
    ranks within -2..+6 of the submitted ones, 1 <= g <= min(t1, t2)
    (g < min(t1, t2) with partial), n' in m-7..m."""
    n0, m0, _, n10, n20, _, _, key0, enc0 = SUBMITTED[level]
    pk0, ct0 = TABLE3[level]

    def rank_box(base):
        return [(a, b, g) for a in range(max(2, base[0] - 2), base[0] + 7)
                for b in range(max(2, base[1] - 2), base[1] + 7)
                for g in range(1, min(a, b) + (0 if partial else 1))]
    keys, encs = rank_box(key0), rank_box(enc0)
    n1max, n2max = n10 + 6, n20 + 6

    @lru_cache(None)
    def pairs(m):
        k = ceil(level / m)
        out = []
        for t in keys:
            for s in encs:
                rank = error_rank(t, s)
                best = None
                for npr in range(m, max(k + 1, m - 8), -1):
                    eps = max(1, 2 * rank - npr + k)
                    if eps > npr - k:
                        continue
                    rt = req_tail(level, rank, eps)
                    if rt is None:
                        continue
                    need = npr + max(rt, eps)
                    if best is None or need < best[0]:
                        best = (need, npr, eps)
                if best and best[0] <= n1max * n2max:
                    out.append((best, t, s))
        return sorted(out)

    @lru_cache(None)
    def key_ok(m, n, t):
        if merged_prr_key(m, n, t[0] + t[1] - t[2]) < level or oj_key(m, n, t) < level:
            return False
        v = thm_key(m, n, t)
        return v is not None and v >= level and prr_spec(m, n, n, *t) >= level

    @lru_cache(None)
    def msg_ok(m, n, n1, s):
        if oj_msg(m, n, n1, s) < level:
            return False
        v = thm_msg(m, n, n1, s)
        return v is not None and v >= level and prr_spec(m, n, n + n1, *s) >= level

    @lru_cache(None)
    def key_refined(m, n, t):
        a, z = mm_key(m, n, t), zd_key(m, n, t)
        return (a is None or a >= level) and (z is None or z >= level)

    @lru_cache(None)
    def msg_refined(m, n, n1, s):
        a, z = mm_msg(m, n, n1, s), zd_msg(m, n, n1, s)
        return (a is None or a >= level) and (z is None or z >= level)

    @lru_cache(None)
    def least_need(m, n, n1):
        for (need, npr, eps), t, s in pairs(m):
            if need > n1 * n2max:
                break
            if not (key_ok(m, n, t) and msg_ok(m, n, n1, s)):
                continue
            if refinements and not (key_refined(m, n, t) and msg_refined(m, n, n1, s)):
                continue
            return need, npr, eps, t, s
        return None

    cands = []
    for m in primes(m0 - 10, m0 + 50):
        for n in range(n0 - 30, n0 + 41):
            for n1 in range(max(2, n10 - 3), n1max + 1):
                for n2 in range(max(2, n20 - 3), n2max + 1):
                    pk, ct = sizes(level, n, m, n1, n2)
                    cands.append((max(pk / pk0, ct / ct0), m, n, n1, n2))
    out = []
    for _, m, n, n1, n2 in sorted(cands):
        r = least_need(m, n, n1)
        if r is not None and r[0] <= n1 * n2:
            out.append((n, m, r[1], n1, n2, r[2], ceil(level / m), r[3], r[4]))
            if len(out) >= keep:
                break
    return out


def main():
    if "--search" in sys.argv:
        level = int(sys.argv[sys.argv.index("--search") + 1])
        refinements = "--theorem-only" not in sys.argv
        partial = "--partial-overlap" in sys.argv
        t0 = time.time()
        for row in search(level, refinements, partial):
            describe(f"BAG-Loong-{level} search", level, row)
        print(f"refinements {'on' if refinements else 'off'}; partial overlap {'required' if partial else 'not required'};"
              f" {time.time() - t0:.0f} s")
        return 0
    print("Controls: Table 3 sizes, Table 5 DFR and OJ/PRR columns, report figures")
    for level, row in SUBMITTED.items():
        n, m, npr, n1, n2, eps, k, t, s = row
        assert sizes(level, n, m, n1, n2) == TABLE3[level]
        describe(f"BAG-Loong-{level} submitted", level, row)
    print()
    print("Smallest sets found with Theorem 3.4 and the merged-rank form only")
    for level, row in THEOREM_ONLY.items():
        describe(f"BAG-Loong-{level}", level, row)
    print()
    print("Smallest sets found with ordinary MM and ZD-MM as well (costed)")
    for level, row in COSTED.items():
        describe(f"BAG-Loong-{level}", level, row)
    print()
    print("The same with g < min(t1, t2); at 128 and 256 the costed sets already satisfy it")
    for level, row in PARTIAL_OVERLAP.items():
        describe(f"BAG-Loong-{level}", level, row)
    return 0


if __name__ == "__main__":
    sys.exit(main())
