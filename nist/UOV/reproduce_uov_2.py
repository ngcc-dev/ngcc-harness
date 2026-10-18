#!/usr/bin/env python3
"""Recompute the Polynomial-XL uov-Ip cost and dense-memory bounds."""

from math import comb, log2


def series(a, b, dmax=100):
    """Coefficients of (1-z)^a (1+z)^b, allowing negative a."""
    if a >= 0:
        left = [(-1) ** i * comb(a, i) if i <= a else 0
                for i in range(dmax + 1)]
    else:
        left = [comb(-a - 1 + i, i) for i in range(dmax + 1)]
    right = [comb(b, i) if i <= b else 0 for i in range(dmax + 1)]
    return [sum(left[i] * right[d - i] for i in range(d + 1))
            for d in range(dmax + 1)]


def solving_degree(variables, equations):
    coefficients = series(equations - variables - 1, equations)
    return next(d for d in range(2, len(coefficients))
                if coefficients[d] <= 1)


def alpha(variables, equations, degree):
    coefficients = series(equations - variables, equations)
    return sum(max(coefficients[d], 0) for d in range(degree + 1))


def pxl(q, variables, equations, omega):
    best = None
    for guessed in range(1, variables):
        remaining = variables - guessed
        degree = solving_degree(remaining, equations)
        dimension = alpha(remaining, equations, degree)
        linearize_one = (guessed * guessed * dimension
                         * comb(remaining + degree, degree)
                         * comb(variables + degree, degree))
        fix = q ** guessed * dimension * dimension * comb(guessed + degree,
                                                           degree)
        linearize_two = q ** guessed * dimension ** omega
        cost = log2(linearize_one + fix + linearize_two)
        candidate = {
            "field_operations": cost,
            "guessed": guessed,
            "degree": degree,
            "alpha": log2(dimension),
            "memory_bits": (log2(comb(guessed + degree, degree))
                            + 2 * log2(comb(remaining + degree, degree))
                            + log2(log2(q))),
        }
        if best is None or cost < best["field_operations"]:
            best = candidate
    return best


def main():
    # Thomae-Wolf reduces uov-Ip to 44 equations in 44 variables over F_256.
    results = {}
    conversion = log2(2 * 8 * 8 + 8)
    for omega in (2.81, 3.0):
        result = pxl(256, 44, 44, omega)
        result["gate_cost"] = result["field_operations"] + conversion
        results[str(omega)] = result

    fast = results["2.81"]
    cubic = results["3.0"]
    checks = {
        "omega_2.81_parameters": (fast["guessed"], fast["degree"]) == (5, 17),
        "omega_2.81_gate_cost": abs(fast["gate_cost"] - 140.0) < 0.1,
        "omega_2.81_dense_memory": abs(fast["memory_bits"] - 110.6) < 0.1,
        "omega_3_gate_cost": abs(cubic["gate_cost"] - 144.3) < 0.1,
    }
    for omega, result in results.items():
        print(f"omega={omega}: field operations=2^{result['field_operations']:.1f}; "
              f"gates=2^{result['gate_cost']:.1f}; dense memory=2^{result['memory_bits']:.1f} bits; "
              f"k={result['guessed']}; D={result['degree']}; alpha=2^{result['alpha']:.1f}")
    print("PASS UOV-2" if all(checks.values()) else f"FAIL UOV-2: {checks}")
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
