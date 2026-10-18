#!/usr/bin/env python3
"""Recompute the Theorem 8.1.7 query loss reported in SQIsign-5."""

import hashlib
import json
import math
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
SPEC_SHA256 = "843912e5eb3dee20288af9fc9f01c0e1f1055834d3ce87c4687795c13adfd73c"
LOG2_SIGN_QUERIES = 64
SETS = {
    "p324_3": {"c": 3, "f": 324, "lambda": 128, "target": 143},
    "p500_27": {"c": 27, "f": 500, "lambda": 192, "target": 207},
    "p664_17": {"c": 17, "f": 664, "lambda": 256, "target": 272},
}


def log2_integer(value: int) -> float:
    return math.log2(value)


def main() -> int:
    sign_queries = 2 ** LOG2_SIGN_QUERIES
    rows = []
    for name, values in SETS.items():
        lam = values["lambda"]
        denominator = 2 ** (lam + 1)
        # Smallest integer q for which (q+1)(s+1)/2^(lambda+1) >= 1.
        first_vacuous_q = (denominator + sign_queries) // (sign_queries + 1) - 1
        displayed_q = 2 ** (lam - 63)
        log2_term_at_displayed_q = (
            log2_integer(displayed_q + 1)
            + log2_integer(sign_queries + 1)
            - (lam + 1)
        )
        q_at_category_work = 2 ** values["target"]
        log2_term_at_category_work = (
            log2_integer(q_at_category_work + 1)
            + log2_integer(sign_queries + 1)
            - (lam + 1)
        )
        prime = values["c"] * 2 ** values["f"] - 1
        log2_p = math.log2(prime)
        q = 2 ** lam
        log2_final_min_entropy_term = (
            log2_integer(q + sign_queries + 1)
            + LOG2_SIGN_QUERIES
            - log2_p / 2
            - lam
        )
        rows.append({
            "parameter": name,
            "lambda": lam,
            "target_gate_exponent": values["target"],
            "sign_queries_log2": LOG2_SIGN_QUERIES,
            "first_integer_q_making_statistical_term_at_least_one": first_vacuous_q,
            "first_integer_q_log2": log2_integer(first_vacuous_q),
            "displayed_threshold_q_log2": lam - 63,
            "log2_statistical_term_at_displayed_threshold": log2_term_at_displayed_q,
            "log2_statistical_term_at_category_work": log2_term_at_category_work,
            "log2_final_min_entropy_term_at_q_2lambda": log2_final_min_entropy_term,
        })

    rules = (ROOT / "doc/NIST-rules.md").read_text()
    checks = {
        "pinned_specification": hashlib.sha256(
            (HERE / "SQIsign-spec.pdf").read_bytes()
        ).hexdigest() == SPEC_SHA256,
        "nist_2^64_baseline_recorded": (
            "2^64 chosen-message signing queries" in " ".join(rules.split())
        ),
        "displayed_thresholds_make_term_at_least_one": all(
            row["log2_statistical_term_at_displayed_threshold"] >= 0
            for row in rows
        ),
        "thresholds_below_category_gate_counts": all(
            row["displayed_threshold_q_log2"] < row["target_gate_exponent"]
            for row in rows
        ),
        "category_work_makes_bound_vacuous": all(
            row["log2_statistical_term_at_category_work"] > 0
            for row in rows
        ),
        "round3_min_entropy_term_remains_below_one": all(
            row["log2_final_min_entropy_term_at_q_2lambda"] < 0
            for row in rows
        ),
        "reviewed_min_entropy_values_match": all(
            math.isclose(
                row["log2_final_min_entropy_term_at_q_2lambda"], expected,
                abs_tol=0.01,
            )
            for row, expected in zip(rows, [-98.7925, -188.3774, -270.0437])
        ),
        "reviewed_thresholds_match": [
            row["displayed_threshold_q_log2"] for row in rows
        ] == [65, 129, 193],
    }
    result = {
        "finding": "SQIsign-5",
        "evidence": "deterministic cost/model check of Theorem 8.1.7; not an attack",
        "term": "(q+1)(s+1)/2^(lambda+1)",
        "final_min_entropy_term": "(q+s+1)s/(sqrt(p)*2^lambda)",
        "parameter_sets": rows,
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
