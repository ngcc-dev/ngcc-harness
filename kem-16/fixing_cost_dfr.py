#!/usr/bin/env python3
"""Parameter and size arithmetic for kem-16/fixing_cost.md.

For a raised weight wr this finds the smallest code length n that keeps HARE's
Model 1 decoding-failure bound (specification Sections 5.1-5.3) at or below a
stated bound, and evaluates the size formulas of Section 4.2 there.

The Model 1 functions are imported from reproduce_model1_dfr.py. The weights
are inputs: they were read from the pinned DS-DOOM estimator of kem-16-2 (see
the note's Evidence) and are not derived here. Standard library only. The
script computes parameters, sizes and word counts; it implements no scheme.

    python3 -B kem-16/fixing_cost_dfr.py            # the costed repair (about 2 minutes)
    python3 -B kem-16/fixing_cost_dfr.py --ceiling  # the 2^80 alternative (several minutes)
"""

import sys

sys.dont_write_bytecode = True

import importlib.util
from fractions import Fraction
from math import comb, log2
from pathlib import Path

_spec = importlib.util.spec_from_file_location(
    "reproduce_model1_dfr", Path(__file__).resolve().parent / "reproduce_model1_dfr.py"
)
m1 = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(m1)

# name: n, w = wr, we, n1, n2, alpha, level; Table 2, physical p. 11.
SUBMITTED = {
    "HARE-2": (20899, 79, 145, 32, 640, 11, 128),
    "HARE-5": (52379, 131, 224, 81, 640, 9, 256),
    "HARE-7": (104869, 193, 361, 91, 1152, 15, 384),
    "HARE-9": (173981, 259, 449, 151, 1152, 13, 512),
}
# name: |ek|, |ct| in bytes; Table 3, physical p. 12.
TABLE3 = {
    "HARE-2": (2629, 4688),
    "HARE-5": (6580, 11790),
    "HARE-7": (13157, 23693),
    "HARE-9": (21812, 39294),
}
KR_N, KR_K = 51, 41  # covering code of Definition 6, physical p. 5

# set, w, wr, log2 bound on the Model 1 DFR, label
REPAIR = (
    ("HARE-7", 194, 194, -384.0, "2^64 limit, w = wr = 194"),
    ("HARE-7", 193, 194, -384.0, "2^64 limit, wr = 194 only"),
)
CEILING = (
    ("HARE-5", 133, 133, None, "2^80, w = wr = 133, bound = submitted Model 1 value"),
    ("HARE-7", 197, 197, -384.0, "2^80, w = wr = 197"),
    ("HARE-9", 261, 261, -519.0, "2^80, w = wr = 261"),
)


def primitive_prime(n):
    """n is prime and 2 is a primitive root of n (Section 2.1)."""
    if n < 3 or n % 2 == 0:
        return False
    d = 3
    while d * d <= n:
        if n % d == 0:
            return False
        d += 2
    rest, q, prime_factors = n - 1, 2, set()
    while q * q <= rest:
        while rest % q == 0:
            prime_factors.add(q)
            rest //= q
        q += 1
    if rest > 1:
        prime_factors.add(rest)
    return all(pow(2, (n - 1) // q, n) != 1 for q in prime_factors)


def model1(n, w, wr, we, n1, n2, alpha, level):
    """log2 of the Model 1 bound; Theorem 1 of Section 5.1 with separate w and wr."""
    numerator = sum(
        comb(n, l) * comb(n - l, w - l) * comb(n - w, wr - l)
        for l in range(1, min(w, wr) + 1, 2)
    )
    p_tilde = Fraction(numerator, comb(n, w) * comb(n, wr))
    p_hqc = (
        2 * p_tilde * (1 - p_tilde) * Fraction(n - we, n)
        + ((1 - p_tilde) ** 2 + p_tilde**2) * Fraction(we, n)
    )
    l2 = (n1 * n2 // KR_N) * KR_N
    p_cover = Fraction(l2 * 2, n * KR_N)
    p = m1.decimal_fraction(p_hqc * (1 - p_cover) + (1 - p_hqc) * p_cover)
    p_error, p_erasure = m1.rm_probabilities(p, n2, alpha)
    return float(m1.log2(m1.rs_failure_probability(p_error, p_erasure, n1, level)))


def sizes(n, n1, n2, level):
    """|ek| and |ct| in bytes, Section 4.2; seed and salt are level/8 bytes."""
    l1 = n1 * n2
    blocks = l1 // KR_N
    payload = (l1 - blocks * KR_N + KR_K * blocks + 7) // 8
    return level // 8 + (n + 7) // 8, (n + 7) // 8 + payload + level // 8


def leaf_work(words):
    """Word products in the schoolbook leaves of the submitted karatsuba_mul.

    gf2x.c:74-122: at most 16 words is schoolbook, words * words; longer
    operands split into products of floor(n/2), ceil(n/2) and ceil(n/2) words.
    """
    if words <= 16:
        return words * words
    low = words >> 1
    return leaf_work(low) + 2 * leaf_work(words - low)


def smallest_n(n0, bound, evaluate):
    """Smallest primitive prime n >= n0 with evaluate(n) <= bound.

    Bisection over n: assumes the bound falls as n grows with all else fixed.
    """
    if evaluate(n0) <= bound:
        return n0
    lo, hi = n0, n0 + max(64, n0 // 200)
    while evaluate(hi) > bound:
        lo, hi = hi, hi + max(64, n0 // 200)
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if evaluate(mid) <= bound:
            hi = mid
        else:
            lo = mid
    while not primitive_prime(hi):
        hi += 1
    return hi


def main():
    for name, (n, w, we, n1, n2, alpha, level) in SUBMITTED.items():
        assert primitive_prime(n) and n >= n1 * n2
        assert sizes(n, n1, n2, level) == TABLE3[name], name
    print("control: Table 3 |ek| and |ct| reproduced for HARE-2/5/7/9; every n is a primitive prime")

    for name, w, wr, bound, label in CEILING if "--ceiling" in sys.argv else REPAIR:
        n0, w0, we, n1, n2, alpha, level = SUBMITTED[name]

        def evaluate(n):
            return model1(n, w, wr, we, n1, n2, alpha, level)

        submitted = model1(n0, w0, w0, we, n1, n2, alpha, level)
        if bound is None:
            bound = submitted
        n = smallest_n(n0, bound, evaluate)
        ek0, ct0 = TABLE3[name]
        ek, ct = sizes(n, n1, n2, level)
        words0, words = (n0 + 63) // 64, (n + 63) // 64
        print(f"{name}: {label}")
        print(f"  submitted n={n0}, w=wr={w0}: log2 DFR = {submitted:.4f}")
        print(f"  w={w}, wr={wr} at n={n0}: log2 DFR = {evaluate(n0):.4f}; bound {bound:.4f}")
        print(f"  smallest primitive prime n={n} (+{100 * (n - n0) / n0:.3f}%): log2 DFR = {evaluate(n):.4f}")
        print(f"  |ek| {ek0} -> {ek} (+{100 * (ek - ek0) / ek0:.2f}%), |ct| {ct0} -> {ct} (+{100 * (ct - ct0) / ct0:.2f}%)")
        print(
            f"  multiplication: {words0} -> {words} words, "
            f"Karatsuba ratio (words'/words)^log2(3) = +{100 * ((words / words0) ** log2(3) - 1):.2f}%"
        )
        leaves0, leaves = leaf_work(words0), leaf_work(words)
        print(
            f"  leaf word products of the submitted recursion: {leaves0} -> {leaves} "
            f"(+{100 * (leaves - leaves0) / leaves0:.2f}%)"
        )


if __name__ == "__main__":
    main()
