#!/usr/bin/env python3
"""Check the verified MAYO2 structural-forgery cost certificate.

This is a self-contained check of the selected schedules obtained by the
independent exhaustive reimplementation of Ostuzzi's On-Ramp Round 3 model.  It does
not repeat that optimizer (whose Gröbner-basis subcosts require
CryptographicEstimators), validate the paper's heuristics, or execute an
attack.
"""

import hashlib
import json
import math
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC_SHA256 = "1fc7561dc8ffdbe36c101b985266e4d3c8aa88fa580e18ead215a80ac7bb371f"
PAPER_SHA256 = "193b99524174f4a16963bb18667268adbcdf0f14e5dde4b49ac33a6be3a986d2"

# On-Ramp Round 3 MAYO2 dimensions and the independently selected geometric-forgery
# schedule.  The relations are Theorems 1 and 3 and Section 7.2 of ePrint
# 2026/2247: u=2*kappa+a, r=kappa*a-a(a-1)/2, and
# w=min(m,u+r+kappa+a).
N = 86
M = 64
KAPPA = 5
A = 5


def logsum2(exponents: list[float]) -> float:
    maximum = max(exponents)
    return maximum + math.log2(sum(2.0 ** (value - maximum) for value in exponents))


def quadratic_operations(size: int) -> int:
    """Operation count used by the paper author's generic specialization model."""
    return (
        size * (size + 1) // 2
        + size
        + size * (size - 1) // 2
        + size
        - 1
    )


def specialization_operations(domain: int, fixed: int) -> int:
    """Field operations for one specialization in the author's generic model."""
    remaining = domain - fixed
    mixed = remaining * fixed + remaining * (fixed - 1)
    return quadratic_operations(fixed) + mixed + 2 * domain - 1


def main() -> int:
    u = 2 * KAPPA + A
    r = KAPPA * A - A * (A - 1) // 2
    w = min(M, u + r + KAPPA + A)

    # Rounded log2 stage costs emitted by the independent exhaustive model.
    # Their log-sums retain the reported precision while keeping this witness
    # independent of optional external estimator packages.
    selected_costs = {
        "paper_16_bitops_per_F16_operation": [139.00, 48.72, 137.00, 140.13],
        "spec_36_bitops_per_F16_operation": [140.17, 49.89, 138.17, 141.68],
        "spec_36_bitops_omega_3": [140.17, 49.89, 138.17, 142.64],
    }
    totals = {name: logsum2(stages) for name, stages in selected_costs.items()}

    # Applying the paper author's generic per-guess specialization formula to
    # the structural attack changes the selected schedule to two blocks of 4.
    # domain=NH-u=145, and the two blocks fix 31 and then 35 coordinates.
    domain = 145
    specialization_ops = sum(
        specialization_operations(domain, fixed) for fixed in (31, 35)
    )
    guesses_log2 = 31 * math.log2(16)
    specialization_stages = {
        "symmetric_16_bitops_per_F16_operation": (
            guesses_log2 + math.log2(specialization_ops) + math.log2(16)
        ),
        "symmetric_36_bitops_per_F16_operation": (
            guesses_log2 + math.log2(specialization_ops) + math.log2(36)
        ),
    }
    totals["symmetric_16_bitops_per_F16_operation"] = logsum2(
        [143.00, 43.67, 144.42, 140.13,
         specialization_stages["symmetric_16_bitops_per_F16_operation"]]
    )
    totals["symmetric_36_bitops_per_F16_operation"] = logsum2(
        [144.17, 44.84, 145.59, 141.68,
         specialization_stages["symmetric_36_bitops_per_F16_operation"]]
    )

    expected = {
        "paper_16_bitops_per_F16_operation": 140.7818160166943,
        "spec_36_bitops_per_F16_operation": 142.20495532649264,
        "spec_36_bitops_omega_3": 142.93350460775937,
        "symmetric_16_bitops_per_F16_operation": 145.11929220837726,
        "symmetric_36_bitops_per_F16_operation": 146.30290556387087,
    }
    checks = {
        "pinned_specification": hashlib.sha256(
            (HERE / "MAYO-spec.pdf").read_bytes()
        ).hexdigest() == SPEC_SHA256,
        "round3_dimensions": (u, r, w) == (15, 15, 40),
        "specialization_operation_count": specialization_ops == 17438,
        "cost_certificate": all(
            # The retained stage certificates are rounded to two decimals,
            # while the specialization stage is recomputed from its exact
            # integer operation count.
            abs(totals[name] - value) < 1e-9 for name, value in expected.items()
        ),
        "paper_convention_below_143": totals[
            "paper_16_bitops_per_F16_operation"
        ] < 143,
        "spec_convention_below_143": totals[
            "spec_36_bitops_per_F16_operation"
        ] < 143,
        "symmetric_accounting_above_143": min(
            totals["symmetric_16_bitops_per_F16_operation"],
            totals["symmetric_36_bitops_per_F16_operation"],
        ) > 143,
    }
    result = {
        "finding": "MAYO-3",
        "evidence": (
            "selected-cost certificate for a published heuristic model; "
            "not an optimizer rerun, experiment, or attack execution"
        ),
        "source": {
            "title": "Geometric Forgeries: Structural Cryptanalysis of MAYO",
            "url": "https://eprint.iacr.org/2026/2247",
            "verified_pdf_sha256": PAPER_SHA256,
        },
        "MAYO2_dimensions": {"n": N, "m": M, "kappa": KAPPA,
                             "a": A, "u": u, "r": r, "w": w},
        "log2_bit_operation_estimates": totals,
        "symmetric_specialization": {
            "domain": domain,
            "fixed_dimensions": [31, 35],
            "field_operations_per_guess": specialization_ops,
            "stage_log2_costs": specialization_stages,
        },
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
