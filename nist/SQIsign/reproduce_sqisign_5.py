#!/usr/bin/env python3
"""Check the pinned SQIsign proof loss and later theorem's failed premise."""

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
        # SQIsign v3 sets Dmix = next_prime(p*2^(2*lambda)).  Ignoring the
        # negligible next-prime increment, this is below p^2 by this many
        # bits.  Mamah--Jao Theorem 3 assumes Nmix > p^2.
        nmix_gap_to_p_squared = log2_p - 2 * lam
        q = 2 ** lam
        log2_final_min_entropy_term = (
            log2_integer(q + sign_queries + 1)
            + LOG2_SIGN_QUERIES
            - log2_p / 2
            - lam
        )
        # Mamah--Jao, ePrint 2026/1775, Theorem 3:
        #   (q+1) 2 sqrt(s+1)/sqrt(p) + 16(q+s+1)s/p.
        updated_first_at_baseline = (
            log2_integer(sign_queries + 1)
            + 1
            + log2_integer(sign_queries + 1) / 2
            - log2_p / 2
        )
        updated_second_at_baseline = (
            4
            + log2_integer(2 * sign_queries + 1)
            + LOG2_SIGN_QUERIES
            - log2_p
        )
        # Solve the first term >= 1 for the integer q.  The direct integer
        # ceiling avoids loss of precision in the 300-bit instances.
        updated_threshold_approx = log2_p / 2 - 1 - log2_integer(sign_queries + 1) / 2
        updated_first_at_category = (
            log2_integer(q_at_category_work + 1)
            + 1
            + log2_integer(sign_queries + 1) / 2
            - log2_p / 2
        )
        updated_second_at_category = (
            4
            + log2_integer(q_at_category_work + sign_queries + 1)
            + LOG2_SIGN_QUERIES
            - log2_p
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
            "v3_dmix_below_p_squared_bits": nmix_gap_to_p_squared,
            "updated_bound": {
                "applicable_to_v3": False,
                "failed_premise": "Mamah--Jao requires Nmix > p^2; v3 Dmix is approximately p*2^(2*lambda)",
                "first_term_log2_at_q_eq_s_eq_2^64": updated_first_at_baseline,
                "second_term_log2_at_q_eq_s_eq_2^64": updated_second_at_baseline,
                "first_term_threshold_q_log2_approx": updated_threshold_approx,
                "first_term_log2_at_category_work": updated_first_at_category,
                "second_term_log2_at_category_work": updated_second_at_category,
            },
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
        "updated_baseline_values_match": all(
            math.isclose(row["updated_bound"][key], expected, abs_tol=0.02)
            for row, expected, key in [
                (rows[0], -65.79, "first_term_log2_at_q_eq_s_eq_2^64"),
                (rows[1], -155.38, "first_term_log2_at_q_eq_s_eq_2^64"),
                (rows[2], -237.04, "first_term_log2_at_q_eq_s_eq_2^64"),
                (rows[0], -192.58, "second_term_log2_at_q_eq_s_eq_2^64"),
                (rows[1], -371.75, "second_term_log2_at_q_eq_s_eq_2^64"),
                (rows[2], -535.09, "second_term_log2_at_q_eq_s_eq_2^64"),
            ]
        ),
        "hypothetical_updated_thresholds_match": all(
            math.isclose(row["updated_bound"]["first_term_threshold_q_log2_approx"], expected, abs_tol=0.02)
            for row, expected in zip(rows, [129.79, 219.38, 301.04])
        ),
        "hypothetical_updated_gap_only_precedes_level1_target": [
            row["updated_bound"]["first_term_log2_at_category_work"] >= 0
            for row in rows
        ] == [True, False, False],
        "updated_second_term_below_one_at_category_work": all(
            row["updated_bound"]["second_term_log2_at_category_work"] < 0
            for row in rows
        ),
        "mamah_jao_nmix_premise_fails_for_v3": all(
            row["v3_dmix_below_p_squared_bits"] > 0 for row in rows
        ),
        "reviewed_dmix_gaps_match": all(
            math.isclose(row["v3_dmix_below_p_squared_bits"], expected, abs_tol=0.02)
            for row, expected in zip(rows, [69.58, 120.75, 156.09])
        ),
    }
    result = {
        "finding": "SQIsign-5",
        "evidence": "deterministic check of Theorem 8.1.7 and the failed premise of the later Round-2 bound; not an attack",
        "term": "(q+1)(s+1)/2^(lambda+1)",
        "final_min_entropy_term": "(q+s+1)s/(sqrt(p)*2^lambda)",
        "later_round2_terms_not_applicable_to_v3": "(q+1)*2*sqrt(s+1)/sqrt(p) + 16*(q+s+1)*s/p",
        "parameter_sets": rows,
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
