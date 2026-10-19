#!/usr/bin/env python3
"""Recompute the wedge-attack estimates reported for VDOO.

This is an exact arithmetic certificate for ePrint 2026/2528, Section 4 and
Table 1.  It does not validate the semi-regularity or generic-rank heuristics
on which the attack model relies.
"""

from math import comb, log2


REPORT_ID = "sign-33-9"


def truncated_ring_coefficient(variables: int, width: int, degree: int) -> int:
    """Coefficient of z^degree in (1 + ... + z^(width-1))^variables."""
    if degree < 0 or degree > variables * (width - 1):
        return 0
    return sum(
        (-1) ** j
        * comb(variables, j)
        * comb(variables + degree - j * width - 1, variables - 1)
        for j in range(degree // width + 1)
    )


def recovery_degree(n: int, o: int, m: int, ell: int, p: int = 2) -> int:
    """First D satisfying the truncated-Hilbert-series recovery criterion."""
    width = p**ell
    quotient: list[int] = []
    for degree in range(o * (width - 1) + 1):
        value = truncated_ring_coefficient(n, width, degree)
        for j in range(1, degree // 2 + 1):
            denominator = truncated_ring_coefficient(m, width, j)
            if denominator:
                value -= denominator * quotient[degree - 2 * j]
        quotient.append(value)
        if degree >= 2 and value <= truncated_ring_coefficient(o, width, degree):
            return degree
    raise AssertionError("recovery criterion was not reached")


def gate_exponent(
    n: int, o: int, m: int, q: int, k: int, expected_degree: int, ell: int
) -> float:
    """Return log2 of the paper's wedge cost after restriction by k variables."""
    n -= k
    o -= k
    degree = recovery_degree(n, o, m, ell)
    assert degree == expected_degree, (degree, expected_degree)
    width = 2**ell
    matrix_gap = (
        truncated_ring_coefficient(n, width, degree)
        - truncated_ring_coefficient(o, width, degree)
        + 1
    )
    field_operations = (
        log2(3) + log2(comb(n + 1, 2)) + 2 * log2(matrix_gap)
    )
    gates_per_field_operation = 2 * log2(q) ** 2 + log2(q)
    return field_operations + log2(gates_per_field_operation)


def main() -> None:
    cases = (
        # name, n, o, m, q, k, D, ell, expected exponent
        ("VDOO-128 reconciliation", 137, 40, 70, 16, 19, 21, 1, 172.20),
        ("VDOO-256 reconciliation", 300, 68, 95, 256, 2, 198, 2, 947.65),
        ("VDOO-512 reconciliation", 455, 175, 195, 256, 123, 52, 1, 432.10),
        ("VDOO-512 intersection", 455, 70, 583, 256, 33, 37, 1, 379.18),
    )
    results: dict[str, float] = {}
    for name, n, o, m, q, k, degree, ell, expected in cases:
        result = gate_exponent(n, o, m, q, k, degree, ell)
        assert abs(result - expected) < 0.01, (name, result, expected)
        results[name] = result
        print(f"{name}: 2^{result:.2f} gates")

    assert results["VDOO-128 reconciliation"] > 128
    assert results["VDOO-256 reconciliation"] > 256
    assert results["VDOO-512 reconciliation"] < 512
    assert results["VDOO-512 intersection"] < 512
    print(f"LEAD {REPORT_ID}: published wedge arithmetic reproduced; attack not run")


if __name__ == "__main__":
    main()
