#!/usr/bin/env python3
"""Recompute HARE's basic (independent-coordinate) DFR bound.

This is a direct implementation of specification Sections 5.1--5.3.  It uses
only exact integers/Fractions and Python's Decimal module.  In the p_cover
formula, the denominator n_2 is the length 51 of the KR covering-code block,
not the similarly named inner Reed--Muller length in Table 2.
"""

from decimal import Decimal, getcontext
from fractions import Fraction
from math import comb


getcontext().prec = 180
TWO = Decimal(2)
ONE = Decimal(1)


PARAMETERS = (
    # name, n, w=wr, we, RS n1, RM n2, alpha, message bits, expected log2 DFR
    ("HARE-128", 20899, 79, 145, 32, 640, 11, 128, -121.4485),
    ("HARE-256", 52379, 131, 224, 81, 640, 9, 256, -239.2122),
    ("HARE-384", 104869, 193, 361, 91, 1152, 15, 384, -384.9303),
    ("HARE-512", 173981, 259, 449, 151, 1152, 13, 512, -519.4515),
)


def decimal_fraction(value):
    return Decimal(value.numerator) / Decimal(value.denominator)


def coordinate_error_probability(n, w, we, rs_n, rm_n):
    denominator = comb(n, w) ** 2
    numerator = sum(
        comb(n, overlap)
        * comb(n - overlap, w - overlap)
        * comb(n - w, w - overlap)
        for overlap in range(1, w + 1, 2)
    )
    p_tilde = Fraction(numerator, denominator)
    p_hqc = (
        2 * p_tilde * (1 - p_tilde) * Fraction(n - we, n)
        + ((1 - p_tilde) ** 2 + p_tilde**2) * Fraction(we, n)
    )

    l1 = rs_n * rm_n
    l2 = (l1 // 51) * 51
    p_cover = Fraction(l2 * 2, n * 51)  # KR [51,41] covering radius R_2=2
    return decimal_fraction(p_hqc * (1 - p_cover) + (1 - p_hqc) * p_cover)


def rm_probabilities(p, length, alpha):
    distance = length // 2
    p_error = Decimal(0)
    p_erasure = Decimal(0)

    for weight in range(length + 1):
        lo = max(0, weight - (length - distance))
        hi = min(distance, weight)
        error_count = 0
        erasure_count = 0
        for intersection in range(lo, hi + 1):
            count = (
                255
                * comb(distance, intersection)
                * comb(length - distance, weight - intersection)
            )
            if 2 * intersection - distance > alpha:
                error_count += count
            if -alpha <= distance - 2 * intersection <= alpha:
                erasure_count += count

        all_words = comb(length, weight)
        probability_per_word = p**weight * (ONE - p) ** (length - weight)
        p_error += min(error_count, all_words) * probability_per_word
        p_erasure += min(erasure_count, all_words) * probability_per_word

    return p_error, p_erasure


def rs_failure_probability(p_error, p_erasure, length, message_bits):
    dimension = message_bits // 8
    p_correct = ONE - p_error - p_erasure
    failure = Decimal(0)

    for errors in range(length + 1):
        for erasures in range(length - errors + 1):
            if 2 * errors + erasures <= length - dimension:
                continue
            arrangements = comb(length, errors) * comb(length - errors, erasures)
            failure += (
                arrangements
                * p_error**errors
                * p_erasure**erasures
                * p_correct ** (length - errors - erasures)
            )
    return failure


def log2(value):
    return value.ln() / TWO.ln()


def main():
    results = []
    for name, n, w, we, rs_n, rm_n, alpha, bits, expected in PARAMETERS:
        p = coordinate_error_probability(n, w, we, rs_n, rm_n)
        p_error, p_erasure = rm_probabilities(p, rm_n, alpha)
        result = float(log2(rs_failure_probability(p_error, p_erasure, rs_n, bits)))
        if abs(result - expected) > 0.001:
            raise SystemExit(f"{name}: unexpected log2(DFR) {result:.6f}")
        results.append((name, result))

    for name, result in results:
        print(f"{name}: Model-1 log2(DFR) = {result:.4f}")
    print("ATTACK model1-dfr HARE CONFIRMED "
          "128/256 conservative bounds miss nominal targets")


if __name__ == "__main__":
    main()
