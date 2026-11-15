#!/usr/bin/env python3
"""Recompute the concrete QROM reduction shortfall reported in FAEST-2."""

import hashlib
import json
import math
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
SPEC_SHA256 = "a880ae59c1f35e0a54a83316de85e2b70bf3ca80f1b4b593340008bc4ef9107b"
LOG2_QSIG = 64


def ceil_log2(value: int) -> int:
    return (value - 1).bit_length()


def evaluate(row: dict) -> dict:
    """Evaluate Corollary 10.54's first square-root PRP term.

    The retained substitution gives each one-query keyed-primitive advantage
    the standard key-guess value 2^-lambda and drops all other nonnegative
    terms inside the radical.
    """
    lam = row["lambda_bits"]
    tree_factor = ceil_log2(row["L"]) + 1
    log2_term_one_guess = (
        math.log2(6)
        + LOG2_QSIG
        + (math.log2(tree_factor) - lam) / 2
    )
    # With AdvPRP(t) = t^2/2^lambda, the additive term reaches one
    # when this log2 quantum-work exponent is attained.
    grover_threshold = (
        lam / 2
        - LOG2_QSIG
        - math.log2(6)
        - math.log2(tree_factor) / 2
    )
    # lambda=128 statistical summand alone: 6*Qsig*sqrt(3*sqrt(Q1)*2^-lambda)
    # at Q1 = 1 (Corollary 10.54, physical p. 139).
    log2_statistical_term_q1_1 = (
        math.log2(6) + LOG2_QSIG + (math.log2(3) - lam) / 2
    )
    # EUF-KO part of Corollary 10.54 with no adversarial hash queries:
    # 2 * (1 - dom/cod)^-1 * 10 * Q^2 * w * 2^-lambda, Q = (tau+4)Qsig+3tau+dzk+19,
    # w = tau + dzk (FAEST) or tau (FAEST-EM); (1-dom/cod)^-1 = 4 except FAEST-192.
    tau, dzk = row["tau"], row["dzk"]
    is_em = "_em_" in row["parameter"]
    weight = tau if is_em else tau + dzk
    domain_factor = 4.0 if (is_em or lam != 192) else 1.0 / (1 - 0.75 * 2.0**-64)
    log2_q = math.log2((tau + 4) * 2.0**LOG2_QSIG + 3 * tau + dzk + 19)
    log2_eufko_term = (
        1 + math.log2(domain_factor) + math.log2(10) + 2 * log2_q
        + math.log2(weight) - lam
    )
    log2_eufko_qsig_limit = LOG2_QSIG - log2_eufko_term / 2
    # Classical-ROM Theorem 10.42 (physical p. 115): 3 + (tau+1)*ceil(log2 L)
    # single-key terms, each t/2^lambda, times 2*Qsig.
    rom_units = 3 + (tau + 1) * ceil_log2(row["L"])
    log2_rom_t_threshold = lam - 1 - LOG2_QSIG - math.log2(rom_units)
    return {
        "log2_statistical_term_at_q1_1": log2_statistical_term_q1_1,
        "log2_eufko_term_at_qsig_2^64": log2_eufko_term,
        "log2_qsig_where_eufko_term_reaches_one": log2_eufko_qsig_limit,
        "rom_single_key_terms": rom_units,
        "log2_rom_classical_work_when_bound_reaches_one": log2_rom_t_threshold,
        "parameter": row["parameter"],
        "lambda": lam,
        "L": row["L"],
        "ceil_log2_L": ceil_log2(row["L"]),
        "log2_additive_term_at_one_key_guess": log2_term_one_guess,
        "log2_grover_work_when_term_reaches_one": grover_threshold,
        "ideal_grover_key_search_exponent": lam / 2,
    }


def main() -> int:
    parameters = json.loads((HERE / "internal-parameters.json").read_text())[
        "parameter_sets"
    ]
    rows = [evaluate(row) for row in parameters]
    by_lambda = {
        str(lam): [row for row in rows if row["lambda"] == lam]
        for lam in (128, 192, 256)
    }
    rules = (ROOT / "doc/NIST-rules.md").read_text()
    checks = {
        "pinned_specification": hashlib.sha256(
            (HERE / "FAEST-spec.pdf").read_bytes()
        ).hexdigest() == SPEC_SHA256,
        "all_twelve_parameter_sets": len(rows) == 12,
        "nist_2^64_baseline_recorded": (
            "2^64 chosen-message signing queries" in " ".join(rules.split())
        ),
        "lambda_128_term_already_vacuous": all(
            row["log2_additive_term_at_one_key_guess"] > 0
            for row in by_lambda["128"]
        ),
        "lambda_128_values_match": all(
            4.43 < row["log2_additive_term_at_one_key_guess"] < 4.59
            for row in by_lambda["128"]
        ),
        "lambda_192_grover_threshold_matches": all(
            27.36 < row["log2_grover_work_when_term_reaches_one"] < 27.52
            for row in by_lambda["192"]
        ),
        "lambda_256_grover_threshold_matches": all(
            59.36 < row["log2_grover_work_when_term_reaches_one"] < 59.52
            for row in by_lambda["256"]
        ),
        "lambda_128_statistical_term_alone_exceeds_one": all(
            3.37 < row["log2_statistical_term_at_q1_1"] < 3.39
            for row in by_lambda["128"]
        ),
        "lambda_128_eufko_term_values_match": all(
            17.55 < row["log2_eufko_term_at_qsig_2^64"] < 19.75
            for row in by_lambda["128"]
        ) and all(
            54.1 < row["log2_qsig_where_eufko_term_reaches_one"] < 55.25
            for row in by_lambda["128"]
        ),
        "rom_thresholds_match": all(
            {128: (55.2, 55.5), 192: (118.55, 118.95), 256: (182.15, 182.5)}
            [row["lambda"]][0] < row["log2_rom_classical_work_when_bound_reaches_one"]
            < {128: (55.2, 55.5), 192: (118.55, 118.95), 256: (182.15, 182.5)}
            [row["lambda"]][1]
            for row in rows
        ),
        "all_thresholds_below_ideal_key_search": all(
            row["log2_grover_work_when_term_reaches_one"]
            < row["ideal_grover_key_search_exponent"]
            for row in rows
        ),
    }
    result = {
        "finding": "FAEST-2",
        "evidence": "cost/model check of Corollary 10.54 and Theorem 10.42; not an attack",
        "formula": (
            "6*Qsig*sqrt((ceil(log2(L))+1)*2^-lambda), "
            "with Qsig=2^64"
        ),
        "primitive_model": (
            "one-key guess 2^-lambda; Grover advantage t^2/2^lambda"
        ),
        "parameter_sets": rows,
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
