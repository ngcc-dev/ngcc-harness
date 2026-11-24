#!/usr/bin/env python3
"""Sizes and work counts for the TRIKE fixing-cost note (kem-36/fixing_cost.md).

Standard library only. It prints numbers; it implements no part of TRIKE.

The pairs (r', t') below were found with the pinned DS-DOOM estimator of
security/reproduce_code_multi_instance.py (see the note). In each, r' is an
admissible block length, a prime for which 2 is a primitive root
(specification section 1.4.1), with t'/r' <= t/r, and t' is the smallest error
weight at which the estimate reaches the target at that r'. They are costing
aids, not proposed parameters. This script checks the admissibility of each
r' and the ratio, and counts sizes and work.

The inversion of gf2x.c is a fixed chain for the exponent r - 2. It calls
gf2x_mul once for every bit of r - 2 after the first, once more for every set
bit of r - 2 after the first, and once for the last squaring.

Failure-rate bound. Section 3.1 (physical p. 13) models the failure rate as
DFR = Q(sqrt(n) (C(s) - C(s*)) / alpha) with n = 3r and C(s) = 1 - t/n, that is
Q^-1(DFR) = sqrt(n) (p* - t/n) / alpha with p* = 1 - C(s*). The threshold p*
and the slope alpha were fitted and are not published. When t'/n' <= t/n and
p* > t/n (DFR < 1/2), sqrt(n') (p* - t'/n') >= sqrt(n'/n) sqrt(n) (p* - t/n),
so Q^-1(DFR') >= sqrt(n'/n) Q^-1(DFR) whatever p* and alpha are. The script
prints that bound from the Table 8 value (physical p. 15). It holds only if
p* and alpha stay as fitted for the submitted set.
"""

import math
from fractions import Fraction


def is_prime(p):
    if p < 2:
        return False
    i = 2
    while i * i <= p:
        if p % i == 0:
            return False
        i += 1
    return True


def two_is_primitive(p):
    if not is_prime(p):
        return False
    m, x, i, factors = p - 1, p - 1, 2, set()
    while i * i <= x:
        while x % i == 0:
            factors.add(i)
            x //= i
        i += 1
    if x > 1:
        factors.add(x)
    return all(pow(2, m // q, p) != 1 for q in factors)


def next_admissible(x):
    r = -(-x.numerator // x.denominator)
    while not two_is_primitive(r):
        r += 1
    return r


def ceil8(x):
    return -(-x // 8)


def inversion_multiplications(r):
    e = r - 2
    return (e.bit_length() - 1) + (bin(e).count("1") - 1) + 1


# name: r, w, t, l, padded multiplication length (trike_params.h:28),
# stated (sk, pk, ct), (INV_MAX_ITER, nonzero entries of k1) from gf2x.c:8,12
SETS = {
    "TRIKE-2": (15581, 105, 263, 256, 16384, (6328, 1980, 3928), (14, 9)),
    "TRIKE-5": (35363, 165, 429, 256, 65536, (13988, 4453, 8874), (16, 4)),
    "TRIKE-7": (69691, 249, 659, 512, 131072, (27260, 8776, 17488), (17, 5)),
    "TRIKE-9": (114043, 333, 877, 512, 131072, (44228, 14320, 28576), (17, 11)),
}
# (label, name, r', t'), from the estimator
COSTED = {
    "2^64 limit": (
        ("smallest step", "TRIKE-5", 36373, 441),
        ("smallest step with no more inversion multiplications", "TRIKE-5", 36877, 442),
        ("smallest step", "TRIKE-7", 69827, 660),
        ("smallest step", "TRIKE-9", 114419, 879),
    ),
    "2^80 ceiling": (
        ("smallest step", "TRIKE-5", 37379, 453),
        ("smallest step", "TRIKE-7", 70979, 671),
        ("smallest step", "TRIKE-9", 115757, 890),
    ),
}


def sizes(r, w, l):
    return 4 * w + 3 * ceil8(r) + 2 * ceil8(l), ceil8(r) + ceil8(l), 2 * ceil8(r) + ceil8(l)


def pct(new, old):
    return f"{100 * (new - old) / old:+.2f}%"


# log2 DFR, Table 8 (physical p. 15)
TABLE8 = {"TRIKE-5": -256.44, "TRIKE-7": -431.23, "TRIKE-9": -525.71}


def log2_q(z):
    return math.log2(0.5 * math.erfc(z / math.sqrt(2)))


def q_inverse_log2(lg):
    lo, hi = 0.0, 60.0
    for _ in range(200):
        mid = (lo + hi) / 2
        if log2_q(mid) > lg:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2


def fls_bound(name, r2):
    r = SETS[name][0]
    z = q_inverse_log2(TABLE8[name])
    z2 = math.sqrt(r2 / r) * z
    return z, z2, log2_q(z2)


def main():
    print("Control, specification Table 3 (formula sk, pk, ct / stated), and gf2x_mul calls per inversion")
    print("(formula / counted in gf2x.c):")
    for name, (r, w, t, l, pad, stated, (iters, nonzero)) in SETS.items():
        print(
            f"  {name}: {sizes(r, w, l)} / {stated}; 2 primitive mod r: {two_is_primitive(r)}; "
            f"inversion {inversion_multiplications(r)} / {iters - 1 + nonzero + 1}; "
            f"key generation 2 inversions + 4 = {2 * inversion_multiplications(r) + 4}"
        )
    for label, rows in COSTED.items():
        print(f"Costed sets, {label}:")
        for tag, name, r2, t2 in rows:
            r, w, t, l, pad, stated, _ = SETS[name]
            d = w // 3
            bound = Fraction(r * t2, t)
            _, pk, ct = sizes(r, w, l)
            _, pk2, ct2 = sizes(r2, w, l)
            inv, inv2 = inversion_multiplications(r), inversion_multiplications(r2)
            first = f", next admissible prime from there: {next_admissible(bound)}" if tag == "smallest step" else ""
            print(
                f"  {name}, {tag}: r {r} -> {r2} ({pct(r2, r)}), t {t} -> {t2} ({pct(t2, t)}); "
                f"2 primitive mod r': {two_is_primitive(r2)}; r*t'/t = {float(bound):.1f}{first}; "
                f"t'/r' <= t/r: {t2 * r <= t * r2}"
            )
            print(
                f"      pk {pk} -> {pk2} ({pct(pk2, pk)}); ct {ct} -> {ct2} ({pct(ct2, ct)}); "
                f"gf2x_mul per inversion {inv} -> {inv2}; in key generation {2 * inv + 4} -> {2 * inv2 + 4} "
                f"({pct(2 * inv2 + 4, 2 * inv + 4)})"
            )
            print(
                f"      r' <= padded multiplication length {pad}: {r2 <= pad}; "
                f"decoder counters per iteration 3*r*d: {3 * r * d} -> {3 * r2 * d} ({pct(r2, r)}); "
                f"sampler duplicate comparisons t(t-1)/2: {t * (t - 1) // 2} -> {t2 * (t2 - 1) // 2} "
                f"({pct(t2 * (t2 - 1), t * (t - 1))})"
            )
            if name in TABLE8:
                z, z2, lg = fls_bound(name, r2)
                print(
                    f"      failure rate, section 3.1 law with p* and alpha as fitted: Table 8 log2 DFR "
                    f"{TABLE8[name]} (Q^-1 = {z:.3f}); t'/n' <= t/n: {t2 * r <= t * r2}; "
                    f"Q^-1(DFR') >= {z2:.3f}, log2 DFR' <= {lg:.2f}"
                )
    r = SETS["TRIKE-5"][0]
    base = inversion_multiplications(r)
    p, seen = 36373, []
    while True:
        if two_is_primitive(p):
            seen.append((p, inversion_multiplications(p)))
            if seen[-1][1] <= base:
                break
        p += 1
    counts = sorted({m for _, m in seen[:-1]})
    print(
        f"TRIKE-5, admissible primes from 36373: {len(seen) - 1} have {counts[0]} to {counts[-1]} "
        f"gf2x_mul per inversion; the first with at most {base} is {seen[-1][0]}"
    )


if __name__ == "__main__":
    main()
