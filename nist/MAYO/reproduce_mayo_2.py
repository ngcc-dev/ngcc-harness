#!/usr/bin/env python3
"""Recompute MAYO Theorem 1's signing-query limitation."""

import hashlib
import json
import math
from fractions import Fraction
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC_SHA256 = "1fc7561dc8ffdbe36c101b985266e4d3c8aa88fa580e18ead215a80ac7bb371f"

# name: (n, m, o, k, q), from the On-Ramp Round 3 specification and submitted constants.
PARAMETERS = {
    "MAYO1": (88, 80, 8, 10, 16),
    "MAYO2": (86, 64, 13, 5, 16),
    "MAYO3": (118, 108, 10, 11, 16),
    "MAYO5": (154, 142, 12, 12, 16),
}


def integer_power(base: int, exponent: int) -> Fraction:
    if exponent >= 0:
        return Fraction(base ** exponent)
    return Fraction(1, base ** (-exponent))


def failure_bound(n: int, m: int, o: int, k: int, q: int) -> Fraction:
    return (
        integer_power(q, k - (n - o))
        + integer_power(q, m - k * o)
    ) / (q - 1)


def main() -> int:
    rows = {}
    checks = {
        "pinned_specification": hashlib.sha256(
            (HERE / "MAYO-spec.pdf").read_bytes()
        ).hexdigest() == SPEC_SHA256,
    }
    expected_restart_bounds = {
        "MAYO1": Fraction(1, 15),
        "MAYO2": Fraction(1, 240),
        "MAYO3": Fraction(1, 3840),
        "MAYO5": Fraction(1, 3840),
    }
    for name, parameters in PARAMETERS.items():
        bound = failure_bound(*parameters)
        theorem_limit = math.log2(bound.denominator / bound.numerator)
        constant_loss_limit = theorem_limit - 1
        rows[name] = {
            "B_exact": f"{bound.numerator}/{bound.denominator}",
            "log2_B": math.log2(bound.numerator / bound.denominator),
            "log2_queries_before_Qs_B_reaches_1": theorem_limit,
            "log2_queries_for_loss_below_2": constant_loss_limit,
            "Qs_B_at_2^64_log2": 64 + math.log2(bound.numerator / bound.denominator),
        }
        # The first term in B is tiny but positive, so the exact bound is just
        # above the simple values quoted in the specification's discussion.
        checks[f"{name}_matches_quoted_bound"] = (
            bound > expected_restart_bounds[name]
            and float(bound - expected_restart_bounds[name]) < 2.0 ** -100
        )
        checks[f"{name}_theorem_fails_at_2^64"] = Fraction(2 ** 64) * bound >= 1

    expected_constant_limits = {
        "MAYO1": 2.9069,
        "MAYO2": 6.9069,
        "MAYO3": 10.9069,
        "MAYO5": 10.9069,
    }
    checks["constant_factor_limits_match"] = all(
        abs(rows[name]["log2_queries_for_loss_below_2"] - expected) < 0.0001
        for name, expected in expected_constant_limits.items()
    )
    result = {
        "finding": "MAYO-2",
        "evidence": "exact model check of Theorem 1's multiplicative proof loss; not an attack",
        "formula": "B=(q^(k-(n-o)) + q^(m-k*o))/(q-1); theorem requires Qs*B<1",
        "parameters": rows,
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
