#!/usr/bin/env python3
"""Recompute the On-Ramp Round 3 UOV truncated-ring/wedge RAM-gate margins."""

from functools import lru_cache
from math import comb, log2


def bounded_monomials(variables, characteristic_power, degree):
    if degree < 0:
        return 0
    if variables == 0:
        return int(degree == 0)
    return sum(
        (-1) ** i * comb(variables, i)
        * comb(degree - i * characteristic_power + variables - 1,
               variables - 1)
        for i in range(min(variables, degree // characteristic_power) + 1)
    )


@lru_cache(maxsize=None)
def hilbert_coefficients(variables, equations, characteristic_power, maximum):
    numerator = [bounded_monomials(variables, characteristic_power, d)
                 for d in range(maximum + 1)]
    first = [0] * (maximum + 1)
    for j in range(maximum // 2 + 1):
        first[2 * j] = (-1) ** j * comb(equations, j) if j <= equations else 0
    second = [0] * (maximum + 1)
    for j in range(maximum // (2 * characteristic_power) + 1):
        second[2 * characteristic_power * j] = comb(equations + j - 1, j)
    denominator = [sum(first[i] * second[d - i] for i in range(d + 1))
                   for d in range(maximum + 1)]
    return tuple(sum(numerator[i] * denominator[d - i]
                     for i in range(d + 1))
                 for d in range(maximum + 1))


def truncated_wedge(q, variables, oil, equations, maximum_guess):
    r = log2(q)
    best = None
    for ell in range(1, int(r) + 1):
        characteristic_power = 2 ** ell
        for guessed in range(maximum_guess + 1):
            remaining = variables - guessed
            remaining_oil = oil - guessed
            if remaining_oil <= 0:
                continue
            maximum = min(remaining_oil * (characteristic_power - 1), 160)
            coefficients = hilbert_coefficients(remaining, equations,
                                                 characteristic_power, maximum)
            degree = next((d for d in range(2, maximum + 1)
                           if coefficients[d] <= bounded_monomials(
                               remaining_oil, characteristic_power, d)), None)
            if degree is None:
                continue
            columns = (bounded_monomials(remaining, characteristic_power, degree)
                       - bounded_monomials(remaining_oil, characteristic_power,
                                           degree) + 1)
            cost = log2(3 * comb(remaining + 1, 2) * columns * columns
                        * (2 * r * r + r))
            candidate = (cost, ell, guessed, degree)
            if best is None or candidate < best:
                best = candidate
    return best


def wedge(q, n, m):
    basic = truncated_wedge(q, n, m, m, m - 1)
    intersection = None
    if n < 3 * m:
        intersection = truncated_wedge(q, n, 3 * m - n, 3 * m - 2,
                                       3 * m - n - 1)
    return min(item for item in (basic, intersection) if item is not None)


def main():
    parameters = [
        ("Ip", 256, 119, 45, 143, 144.75),
        ("Is", 16, 160, 64, 143, 158.55),
        ("III", 256, 193, 72, 207, 213.12),
        ("V", 256, 259, 96, 272, 272.05),
    ]
    passed = True
    for name, q, n, m, target, expected in parameters:
        cost, ell, guessed, degree = wedge(q, n, m)
        passed &= abs(cost - expected) < 0.06
        print(f"uov-{name}: 2^{cost:.2f} gates; margin {cost-target:+.2f}; "
              f"ell={ell}; k={guessed}; D={degree}")
    print("PASS UOV-4" if passed else "FAIL UOV-4")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
