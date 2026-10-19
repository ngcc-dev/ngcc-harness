#!/usr/bin/env python3
"""Recompute UOV wedge margins and the conditional sparse-row UOV-V lead."""

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


def partial_tower_dimension(variables, equations, degree, level):
    """Conjectural quotient dimension after selected divided-power levels.

    Level zero uses only the ordinary polar forms and has series
    (1+t)^variables/(1+t^2)^equations. Level one also includes their divided
    squares and uses (1+t)^variables*(1-t^2)^equations/
    (1-t^8)^equations. The positive-part convention leaves the planted oil
    Pluecker vector as a one-dimensional kernel.
    """
    base = [comb(variables, d) for d in range(degree + 1)]
    if level == 0:
        factor = [0] * (degree + 1)
        for j in range(degree // 2 + 1):
            factor[2 * j] = (-1) ** j * comb(equations + j - 1, j)
        coefficients = [sum(base[i] * factor[d - i] for i in range(d + 1))
                        for d in range(degree + 1)]
    elif level == 1:
        numerator = [0] * (degree + 1)
        for j in range(degree // 2 + 1):
            numerator[2 * j] = (-1) ** j * comb(equations, j)
        first = [sum(base[i] * numerator[d - i] for i in range(d + 1))
                 for d in range(degree + 1)]
        coefficients = []
        for d in range(degree + 1):
            coefficients.append(sum(
                comb(equations + j - 1, j) * first[d - 8 * j]
                for j in range(d // 8 + 1)
            ))
    else:
        raise ValueError("only the plain and divided-square levels are used")
    return 1 if any(value <= 0 for value in coefficients) else max(coefficients[degree], 1)


def selected_divided_square(q, n, m, restrict_hyperplane=False):
    """Conditional sparse-first exterior attack after valid intersection.

    In characteristic two the intersection step needs even ambient dimension.
    For an odd-dimensional instance a generic public hyperplane reduces both
    n and the planted oil dimension by one before intersection.
    """
    vinegar, oil = n - m, m
    if restrict_hyperplane:
        n -= 1
        oil -= 1
    if n % 2:
        raise ValueError("characteristic-two intersection requires even n")
    iv, io, im = 2 * vinegar - oil, 2 * oil - vinegar, 3 * m - 2
    r = log2(q)
    candidates = []
    for degree in range(2, io + 1):
        if (degree - 1) * im < 2 * iv:
            continue
        projected_n = iv + degree
        columns = comb(projected_n, degree)
        plain_nullity = partial_tower_dimension(projected_n, im, degree, 0)
        square_nullity = partial_tower_dimension(projected_n, im, degree, 1)
        if square_nullity != 1:
            continue
        plain_weight = comb(iv + 2, 2)
        square_weight = comb(iv + 4, 4)
        # N-1 independent rows plus one redundant cheap row make a square
        # rank-(N-1) matrix. Existence follows if the conjectured ranks hold;
        # finding such a subset at full scale is an explicit open assumption.
        nonzeros = ((columns - plain_nullity + 1) * plain_weight
                    + (plain_nullity - 1) * square_weight)
        selected_cost = log2(3 * nonzeros * columns * (2 * r * r + r))

        plain_rows = im * comb(projected_n, degree - 2)
        square_rows = im * comb(projected_n, degree - 4) if degree >= 4 else 0
        average_weight = ((plain_rows * plain_weight + square_rows * square_weight)
                          / (plain_rows + square_rows))
        random_partial_cost = log2(
            3 * average_weight * columns * columns * (2 * r * r + r)
        )
        candidates.append({
            "cost": selected_cost,
            "random_partial_cost": random_partial_cost,
            "degree": degree,
            "columns": columns,
            "plain_nullity": plain_nullity,
            "nonzeros": nonzeros,
            "intersection": (iv, io, im),
        })
    return min(candidates, key=lambda item: item["cost"]) if candidates else None


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

    print("conditional sparse-row divided-square review:")
    selected = {}
    for name, q, n, m, target, _ in parameters:
        result = selected_divided_square(q, n, m, restrict_hyperplane=bool(n % 2))
        selected[name] = result
        if result is None:
            print(f"uov-{name}: no admissible plain+divided-square estimate")
            continue
        iv, io, im = result["intersection"]
        print(
            f"uov-{name}: 2^{result['cost']:.2f} selected-row gates; "
            f"random-partial 2^{result['random_partial_cost']:.2f}; "
            f"margin {result['cost']-target:+.2f}; a={result['degree']}; "
            f"intersection=({iv},{io},{im}); log2(N)={log2(result['columns']):.2f}; "
            f"log2(h0)={log2(result['plain_nullity']):.2f}"
        )
    passed &= selected["Ip"] is None
    passed &= abs(selected["III"]["cost"] - 209.2649) < 0.01
    passed &= abs(selected["V"]["cost"] - 267.5751) < 0.01
    passed &= abs(selected["V"]["random_partial_cost"] - 271.3167) < 0.01
    print("PASS UOV-4" if passed else "FAIL UOV-4")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
