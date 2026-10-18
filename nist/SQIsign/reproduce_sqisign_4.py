#!/usr/bin/env python3
"""Recompute the model-dependent SQIsign concrete margins in SQIsign-4."""

from decimal import Decimal, getcontext
import hashlib
import json
import math
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
SPEC_SHA256 = "843912e5eb3dee20288af9fc9f01c0e1f1055834d3ce87c4687795c13adfd73c"
NIST_CALL_SHA256 = "d66c2427fee94568791c1626c38c0c8dcd1825c4991482a0225599efc9b3ad5b"
SETS = {
    "p324_3": {"c": 3, "f": 324, "target": 143, "cap": 80, "per_j": 27.3},
    "p500_27": {"c": 27, "f": 500, "target": 207, "cap": 120, "per_j": 29.8},
    "p664_17": {"c": 17, "f": 664, "target": 272, "cap": 160, "per_j": 31.4},
}
PAPER_EXAMPLES = {
    # Section 4.1: (printed Fp2-work exponent, printed memory exponent).
    256: (106.5, 92.5),
    384: (157.5, 138.6),
    512: (204.2, 181.3),
}


def log2_sum(a: float, b: float) -> float:
    high, low = max(a, b), min(a, b)
    return high + math.log2(1 + 2 ** (low - high))


def build_dickman_log_table(step_count: int = 400, maximum: int = 40):
    """Numerically tabulate log2 Dickman's rho using its integral recurrence."""
    getcontext().prec = 100
    values = [Decimal(1)] * (step_count + 1)
    for i in range(step_count + 1, maximum * step_count + 2):
        u = Decimal(i) / step_count
        window = sum(values[i - step_count + 1:i]) + values[i - step_count] / 2
        value = (window / step_count) / (
            u - Decimal(1) / (2 * step_count)
        )
        values.append(value)
    log_two = Decimal(2).ln()
    return [float(value.ln() / log_two) for value in values], step_count


def interpolate_log_rho(table, step_count: int, u: float) -> float:
    if u <= 1:
        return 0.0
    position = u * step_count
    index = int(position)
    fraction = position - index
    return table[index] * (1 - fraction) + table[index + 1] * fraction


def wesolowski_model(log_p: float, table, step_count: int):
    """Reproduce the rough Section 4.1 smoothness model used in review."""
    best = None
    for tenth in range(100, 4000):
        log_b = tenth / 10
        log_x = log_b / 2 + (log_p - 1) / 6
        smoothness_u = log_x / log_b
        if smoothness_u > 39:
            continue
        log_psi = log_x + interpolate_log_rho(
            table, step_count, smoothness_u
        )
        log_memory = log_psi + log_x
        success_u = (log_p - 1) / (3 * log_b)
        log_success = (
            -success_u * math.log2(success_u) if success_u > 1 else 0
        )
        candidate = (log_memory - log_success, log_memory, log_b)
        if best is None or candidate[0] < best[0]:
            best = candidate
    return best


def main() -> int:
    table, steps = build_dickman_log_table()
    calibration = []
    for log_p, (paper_time, paper_memory) in PAPER_EXAMPLES.items():
        model_time, model_memory, log_b = wesolowski_model(log_p, table, steps)
        calibration.append({
            "log2_p": log_p,
            "paper_fp2_work_log2": paper_time,
            "model_fp2_work_log2": model_time,
            "work_delta": model_time - paper_time,
            "paper_memory_entries_log2": paper_memory,
            "model_memory_entries_log2": model_memory,
            "memory_delta": model_memory - paper_memory,
            "model_log2_B": log_b,
        })
    rows = []
    for name, values in SETS.items():
        prime = values["c"] * 2 ** values["f"] - 1
        log_p = math.log2(prime)
        capped = (log_p - values["cap"]) / 2 + values["per_j"]
        free_memory = log_p / 3 + values["per_j"]
        log_memory = log_p / 3
        charged = (log_p - log_memory) / 2 + log2_sum(
            values["per_j"], log_memory / 3
        )
        memory_for_target = (
            log_p + 2 * values["per_j"] - 2 * values["target"]
        )
        wes_time, wes_memory, log_b = wesolowski_model(log_p, table, steps)
        wes_logp_squared_bit_cost = wes_time + 2 * math.log2(log_p)
        rows.append({
            "parameter": name,
            "log2_p": log_p,
            "target_gate_exponent": values["target"],
            "spec_memory_cap_entries_log2": values["cap"],
            "spec_capped_work_log2": capped,
            "free_memory_entries_log2": log_memory,
            "free_memory_work_log2": free_memory,
            "free_memory_margin": free_memory - values["target"],
            "memory_needed_for_target_log2": memory_for_target,
            "charged_w_to_one_third_work_log2": charged,
            "charged_margin": charged - values["target"],
            "wesolowski_model_fp2_work_log2": wes_time,
            "wesolowski_model_memory_entries_log2": wes_memory,
            "wesolowski_model_log2_B": log_b,
            "wesolowski_logp_squared_bit_work_log2": (
                wes_logp_squared_bit_cost
            ),
            "wesolowski_logp_squared_bit_work_margin": (
                wes_logp_squared_bit_cost - values["target"]
            ),
        })

    checks = {
        "pinned_specification": hashlib.sha256(
            (HERE / "SQIsign-spec.pdf").read_bytes()
        ).hexdigest() == SPEC_SHA256,
        "pinned_nist_call_with_memory_cost_rule": hashlib.sha256(
            (ROOT / "doc/NIST-call-for-proposals.pdf").read_bytes()
        ).hexdigest() == NIST_CALL_SHA256,
        "spec_capped_model_above_targets": all(
            row["spec_capped_work_log2"] > row["target_gate_exponent"]
            for row in rows
        ),
        "free_memory_extrapolation_below_targets": all(
            row["free_memory_work_log2"] < row["target_gate_exponent"]
            for row in rows
        ),
        "charged_memory_model_above_targets": all(
            row["charged_w_to_one_third_work_log2"]
            > row["target_gate_exponent"] for row in rows
        ),
        "wesolowski_converted_model_above_targets": all(
            row["wesolowski_logp_squared_bit_work_log2"]
            > row["target_gate_exponent"] for row in rows
        ),
        "wesolowski_model_calibrates_to_paper_examples": all(
            abs(row["work_delta"]) < 2.3
            and abs(row["memory_delta"]) < 1.2
            for row in calibration
        ),
        "reviewed_numeric_ranges_match": (
            135.8 < rows[0]["free_memory_work_log2"] < 135.9
            and 198.0 < rows[1]["free_memory_work_log2"] < 198.1
            and 254.0 < rows[2]["free_memory_work_log2"] < 254.2
            and 152.2 < rows[0]["wesolowski_logp_squared_bit_work_log2"] < 152.4
            and 221.3 < rows[1]["wesolowski_logp_squared_bit_work_log2"] < 221.5
            and 282.8 < rows[2]["wesolowski_logp_squared_bit_work_log2"] < 283.0
        ),
    }
    result = {
        "finding": "SQIsign-4",
        "evidence": "deterministic cost-model comparison; not an attack",
        "wesolowski_section_4_1_calibration": calibration,
        "parameter_sets": rows,
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
