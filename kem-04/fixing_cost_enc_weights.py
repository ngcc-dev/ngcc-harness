#!/usr/bin/env python3
"""Second parameter estimate for kem-04/fixing_cost.md (finding kem-04-3).

fixing_cost_params.py keeps the submitted encryption weights. This script also
lets the encryption weights vary, and checks every set against the
specification's own message-instance and quantum formulas. Standard library
only; the maximal-minors models are imported from security/rank_mm_costs.py.
It computes costs and sizes and implements no scheme. Nothing here is a
recommended parameter set.

    python3 -B kem-04/fixing_cost_enc_weights.py            controls and the sets quoted (about 20 s)
    python3 -B kem-04/fixing_cost_enc_weights.py --search   the searches

The --search mode was started for the 2026-11-23 assessment and stopped before
it finished. Its running time and its results are not known. The sets printed
by the default mode were found by hand and are not shown to be the smallest.

Constraints (specification section 2.2, physical p. 10): m prime, n' <= m,
k*m >= 128, eps <= min(2n - n', n' - k), error rank
w_y1*w_R21 + w_y2*w_R22 + w_e + w_x*w_r1 <= (n' - k + eps)/2, and the
failure-rate formula of physical p. 4 at most 2^-128 (physical p. 14).
Key instance, all at omega = 2.8 and required to be >= 128:
  - ordinary MM with guessed zeros, punctures and block removal, as searched by
    rank_mm_costs.piglet_search, over the assignments of ranks to blocks;
  - ZD-MM (rank_mm_costs.zd_cost) over the same guesses, any block contracted;
  - the specification's Proposition 3.1 (physical p. 19) and its (3,1) OJ
    formula (physical p. 17).
Message instance, with the specification's own formulas, required to be >= 128:
  - the (4,2) OJ formula of Theorem 3.2 (physical p. 16);
  - Proposition 3.2 (physical p. 20).
Quantum guard: both OJ formulas with the exponent halved, required to be >= 80.
This form reproduces the "OJ quantum" column of Table 5 (physical p. 21).
Not evaluated: the specification's PRR formulas, and the block-removing MM of
kem-04-3 applied to the message instance.
"""
import importlib.util
import os
import sys
from functools import lru_cache
from itertools import combinations_with_replacement, permutations
from math import ceil, comb, log2

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "rank_mm_costs", os.path.join(_HERE, "..", "security", "rank_mm_costs.py"))
R = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(R)

LAM = 128
QLAM = 80   # quantum target of the 128-bit level
W = 2.8
B = (2, 3, 3, 3)  # submitted (w_r1, w_R21, w_R22, w_e), Table 1, physical p. 8


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


def oj31(m, n, ranks, quantum=False):
    """(3,1) OJ formula, physical p. 17; quantum: exponent halved, as in Table 5."""
    t1, _, t3 = sorted(ranks)
    e = (n - t1) * (t1 - 1) + t3
    return W * log2(m * (sum(ranks) - 1) + (n - t1)) + (e / 2 if quantum else e)


def oj42(m, n, enc, quantum=False):
    """(4,2) OJ formula of Theorem 3.2, physical p. 16; quantum: exponent halved, as in Table 5."""
    t1, t2, _, t4 = sorted(enc)
    e = (n - t1) * (t1 - 1) + (n - t2) * (t2 - 1) + t4
    return W * log2(m * (sum(enc) - 1) + (n - t1) + (n - t2)) + (e / 2 if quantum else e)


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


@lru_cache(None)
def prop32(m, n, enc):
    """Proposition 3.2, physical p. 20: MM on the (4,2) instance, guessed zeros a."""
    t1, t2, t3, t4 = sorted(enc)
    t = t1 + t2 + t3 + t4
    n4 = m * sum(comb(n - 1, j) * comb(n, t - j) for j in range(t3 - 1, t1 + t2 + t3 + 1))
    best = None
    for a1 in range(0, n - t1 + 1):
        if best is not None and a1 * t1 >= best:
            break
        for a2 in range(0, n - t2 + 1):
            if best is not None and a1 * t1 + a2 * t2 >= best:
                break
            for a3 in range(0, n - t3 + 1):
                if best is not None and a1 * t1 + a2 * t2 + a3 * t3 >= best:
                    break
                part = comb(n - a1, t1) * comb(n - a2, t2) * comb(n - a3, t3)
                for a4 in range(0, n - t4 + 1):
                    big_t = part * comb(n - a4, t4)
                    if n4 >= big_t - 1:
                        c = a1 * t1 + a2 * t2 + a3 * t3 + a4 * t4 + log2(n4) + (W - 1) * log2(big_t)
                        best = c if best is None else min(best, c)
                        break
    return best


def error_rank(key, enc):
    """key = (w_x, w_y1, w_y2), enc = (w_r1, w_R21, w_R22, w_e); specification physical p. 6."""
    return key[1] * enc[1] + key[2] * enc[2] + enc[3] + key[0] * enc[0]


@lru_cache(None)
def assign(key, enc):
    """Assignment of the two weight multisets to the blocks with the smallest error rank."""
    return min((error_rank(k, e), k, e) for k in set(permutations(key)) for e in set(permutations(enc)))


@lru_cache(None)
def decoding(n, m, rank):
    """Largest n' with the failure rate within 2^-128: (n', eps, log2 DFR, k) or None."""
    k = ceil(LAM / m)
    for npr in range(min(m, 2 * n - 1), k, -1):
        eps = max(1, 2 * rank - npr + k)
        tail = 2 * n - npr
        if eps > min(tail, npr - k):
            continue
        d = log2_dfr(rank, tail, eps)
        if d <= -LAM:
            return npr, eps, d, k
    return None


def key_guards(m, n, key, quantum):
    key = tuple(sorted(key))
    if oj31(m, n, key) < LAM or (quantum and oj31(m, n, key, True) < QLAM):
        return False
    return prop31(m, n, key) >= LAM


def msg_guards(m, n, enc, quantum):
    enc = tuple(sorted(enc))
    if oj42(m, n, enc) < LAM or (quantum and oj42(m, n, enc, True) < QLAM):
        return False
    return prop32(m, n, enc) >= LAM


def key_attacks(m, n, key, with_zd):
    key = tuple(sorted(key))
    a = mm(m, n, key)
    if a is not None and a[0] < LAM:
        return False
    z = zd(m, n, key) if with_zd else None
    return z is None or z[0] >= LAM


def describe(label, n, m, key, enc=B):
    """key and enc are given in block order."""
    skey, senc = tuple(sorted(key)), tuple(sorted(enc))
    pk, ct = sizes(n, m)
    pk0, ct0 = sizes(43, 47)
    rank = error_rank(key, enc)
    dec = decoding(n, m, rank)
    a, z = mm(m, n, skey), zd(m, n, skey)
    print(f"{label}: n={n} m={m} w(x,y)={key} w(r1,R2,e)={enc}")
    print(f"  pk {pk} ({100 * (pk / pk0 - 1):+.1f}%), ct {ct} ({100 * (ct / ct0 - 1):+.1f}%); error rank {rank}")
    if dec is None:
        print("  no n' gives a failure rate within 2^-128")
    else:
        print(f"  n'={dec[0]} k={dec[3]} eps={dec[1]} log2 DFR={dec[2]:.1f}")
    print(f"  key: ordinary MM {a[0]:.1f} (ranks {a[1]}, guesses {a[2]}, punctures {a[3]});"
          f" ZD-MM {'none' if z is None else format(z[0], '.1f')};"
          f" Proposition 3.1 {prop31(m, n, skey):.1f}; OJ {oj31(m, n, skey):.1f}, quantum {oj31(m, n, skey, True):.1f}")
    print(f"  message: OJ {oj42(m, n, senc):.1f}, quantum {oj42(m, n, senc, True):.1f};"
          f" Proposition 3.2 {prop32(m, n, senc):.1f}")
    print(f"  ring products: n^2 ratio to the code (n=43) {n * n / 43 ** 2:.3f},"
          f" times m/47 {n * n * m / (43 ** 2 * 47):.3f}")


def primes(lo, hi):
    return [p for p in range(lo, hi + 1) if all(p % d for d in range(2, int(p ** 0.5) + 1))]


def search(with_zd=True, free_enc=True, quantum=True, keep=3):
    """Box: n in 36..80, m prime in 29..89, key weights in 3..6, encryption
    weights in 1..5 (or the submitted ones). Sets are tried in the order of
    the growth of the larger of public key and ciphertext."""
    pk0, ct0 = sizes(43, 47)
    cands = sorted((max(sizes(n, m)[0] / pk0, sizes(n, m)[1] / ct0), n, m)
                   for m in primes(29, 89) for n in range(36, 81))
    keys = list(combinations_with_replacement(range(3, 7), 3))
    encs = list(combinations_with_replacement(range(1, 6), 4)) if free_enc else [tuple(sorted(B))]
    out = []
    for _, n, m in cands:
        good_enc = [e for e in encs if msg_guards(m, n, e, quantum)]
        if not good_enc:
            continue
        found = None
        for key in keys:
            if not key_guards(m, n, key, quantum):
                continue
            fits = sorted((assign(key, e) for e in good_enc))
            fits = [f for f in fits if decoding(n, m, f[0]) is not None]
            if not fits or not key_attacks(m, n, key, with_zd):
                continue
            if found is None or fits[0] < found:
                found = fits[0]
        if found is not None:
            out.append((n, m, found[1], found[2]))
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
    print(f"  key instance at (n, m) = (47, 43): Proposition 3.1 {prop31(43, 47, (4, 4, 4)):.1f} (Table 5: 148);"
          f" OJ {oj31(43, 47, (4, 4, 4)):.1f} (158), quantum {oj31(43, 47, (4, 4, 4), True):.1f} (91)")
    print(f"  message instance at (n, m) = (47, 43): Proposition 3.2 {prop32(43, 47, B):.1f} (Table 5: 167);"
          f" OJ {oj42(43, 47, B):.1f} (162), quantum {oj42(43, 47, B, True):.1f} (94)")
    print(f"  BAG-Piglet-256 OJ: {oj31(79, 78, (6, 6, 6)):.1f}, {oj42(79, 78, (3, 3, 4, 4)):.1f} (Table 5: 395, 332);"
          f" quantum {oj31(79, 78, (6, 6, 6), True):.1f}, {oj42(79, 78, (3, 3, 4, 4), True):.1f} (212, 180)")
    a, z = mm(47, 43, (4, 4, 4)), zd(47, 43, (4, 4, 4))
    print(f"  kem-04-3 at (n, m) = (43, 47): ordinary MM {a[0]:.3f}, ZD-MM {z[0]:.3f}"
          f"  (report: 118.4, 114.6; paper {R.PIGLET_MM[128]}, {R.PIGLET_ZD[128][3]})")
    z5 = zd(131, 125, (8, 8, 8))
    print(f"  ZD-MM search at BAG-Piglet-512: {z5[0]:.3f}  (paper {R.PIGLET_ZD[512][3]})")
    print()
    describe("Submitted, as coded and in Table 2", 43, 47, (4, 4, 4))
    describe("Submitted, as listed in Table 1", 47, 43, (4, 4, 4))
    describe("Encryption weights lowered, smaller field", 56, 37, (4, 4, 4), (2, 2, 3, 3))
    describe("One step smaller", 55, 37, (4, 4, 4), (2, 2, 3, 3))
    describe("Submitted encryption weights kept", 55, 43, (5, 4, 4))
    describe("The same, one step smaller", 54, 43, (5, 4, 4))
    if "--search" in sys.argv:
        for label, kw in (("all guards", {}),
                          ("submitted encryption weights kept", {"free_enc": False}),
                          ("ZD-MM left out", {"with_zd": False}),
                          ("no quantum guard", {"quantum": False})):
            print()
            print(f"Search, n in 36..80, m prime in 29..89, key weights in 3..6, encryption weights in 1..5: {label}")
            for n, m, key, enc in search(**kw):
                describe("  smallest sizes", n, m, key, enc)
    return 0


if __name__ == "__main__":
    sys.exit(main())
