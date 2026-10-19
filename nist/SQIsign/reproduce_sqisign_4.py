#!/usr/bin/env python3
"""Check the validated finite-size SQIsign p^(1/3) resource model."""

import hashlib
import json
import math
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
DATA = HERE / "analysis/p13"
SPEC_SHA256 = "843912e5eb3dee20288af9fc9f01c0e1f1055834d3ce87c4687795c13adfd73c"
NIST_CALL_SHA256 = "d66c2427fee94568791c1626c38c0c8dcd1825c4991482a0225599efc9b3ad5b"
ARTIFACTS = {
    "model-data/fp2_gates.json": "12daaa298453c180232b6f089752e77e332d5f3e36ee8766955f3ff25730aaae",
    "model-data/pair_grid.jsonl": "501be3ef6a892288c2575887bad1734aa8234b46875979ac47f654f7cf022853",
    "validation-results/cost_model2.json": "23ed06261482981388985113b009b634bcea2f04528e244b6685088c1dba296a",
    "validation-results/an_p324_3.json": "6a081c958e6fb1883a704e22aeeaee241f2b4f3f872f1d80e2fd53c895f3f240",
    "validation-results/an_p500_27.json": "190633f0d97f9ccde3ac0f71430b2d0d17662ad7d7692695d00899d3e1bf03bd",
    "validation-results/an_p664_17.json": "8a4df5d48944716f0276b5796bf8c871b45fd4a589bdb9947c0e4679e5fb207c",
    "validation-results/val_p324_3.jsonl": "ce54e93ea94c42289dec527048f9c83381758f3eaf8688e2f68f9201a1413a60",
    "validation-results/val_p500_27.jsonl": "44101ac48fae42f42270f2e8b8e147aa80763eefb445b9a3f8c98c43b04114af",
}
TARGETS = {"p324_3": 143, "p500_27": 207, "p664_17": 272}
EXPECTED = {
    "p324_3": {
        "spec_cap": 150.6938, "spec_free": 137.4985,
        "gate_cap": (143.2773, 143.4691),
        "logp2_cap": (140.1997, 140.4777),
        "cbrt": (145.7190, 146.7580), "memory": 109.9007,
    },
    "p500_27": {
        "spec_cap": 222.8968, "spec_free": 199.9850,
        "gate_cap": (214.1015, 214.2619),
        "logp2_cap": (211.2751, 211.4822),
        "cbrt": (225.3959, 226.5246), "memory": 169.8809,
    },
    "p664_17": {
        "spec_cap": 286.3213, "spec_free": 255.8419,
        "gate_cap": (276.4496, 276.7959),
        "logp2_cap": (273.7841, 274.1700),
        "cbrt": (298.1304, 299.0729), "memory": 223.8041,
    },
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def select(rows, convention: str, memory: str, precomputation: bool):
    matches = [row for row in rows
               if row["conv"] == convention and row["mem"] == memory
               and row["precomp"] is precomputation]
    if len(matches) != 1:
        raise ValueError(
            f"expected one {convention}/{memory}/{precomputation} row"
        )
    return matches[0]


def close(actual: float, expected: float, tolerance: float = 0.015) -> bool:
    return abs(actual - expected) <= tolerance


def main() -> int:
    artifact_hashes = {
        relative: digest(DATA / relative) for relative in ARTIFACTS
    }
    model = json.loads(
        (DATA / "validation-results/cost_model2.json").read_text()
    )
    rows = []
    numeric_checks = []
    for name, expected in EXPECTED.items():
        item = model[name]
        target = TARGETS[name]
        spec_cap = select(item["rows"], "spec", "cap", True)
        spec_free = select(item["rows"], "spec", "free", False)
        gate_cap = [select(item["rows"], "gate1", "cap", flag)["cost"]
                    for flag in (False, True)]
        logp2_cap = [select(item["rows"], "logp2", "cap", flag)["cost"]
                     for flag in (False, True)]
        cbrt = [select(item["rows"], "spec", "cbrt", flag)["cost"]
                for flag in (False, True)]
        numeric_checks.extend([
            close(spec_cap["cost"], expected["spec_cap"]),
            close(spec_free["cost"], expected["spec_free"]),
            close(spec_free["log2mem"], expected["memory"]),
            all(close(value, wanted) for value, wanted
                in zip(gate_cap, expected["gate_cap"])),
            all(close(value, wanted) for value, wanted
                in zip(logp2_cap, expected["logp2_cap"])),
            all(close(value, wanted) for value, wanted
                in zip(cbrt, expected["cbrt"])),
        ])
        rows.append({
            "parameter": name,
            "target": target,
            "spec_cap_cost": spec_cap["cost"],
            "spec_cap_margin": spec_cap["cost"] - target,
            "spec_free_cost": spec_free["cost"],
            "spec_free_margin": spec_free["cost"] - target,
            "spec_free_memory_entries_log2": spec_free["log2mem"],
            "one_fp2_multiplication_cap_range": gate_cap,
            "log2_p_squared_cap_range": logp2_cap,
            "w_to_one_third_charged_range": cbrt,
        })

    lattice = [json.loads((DATA / f"validation-results/an_{name}.json").read_text())
               for name in TARGETS]
    validation = []
    for name in ("p324_3", "p500_27"):
        validation.extend(
            json.loads(line) for line in
            (DATA / f"validation-results/val_{name}.jsonl")
            .read_text().splitlines()
        )
    operating_points = [
        row for row in validation
        if row["empirical_success"] > 0
        and row["empirical_success"] < 0.95
    ]
    success_deltas = [
        abs(math.log2(row["empirical_success"])
            - math.log2(row["model_1_minus_exp_lam"]))
        for row in operating_points
    ]
    checks = {
        "pinned_specification": digest(HERE / "SQIsign-spec.pdf") == SPEC_SHA256,
        "pinned_nist_call": digest(ROOT / "doc/NIST-call-for-proposals.pdf") == NIST_CALL_SHA256,
        "retained_artifact_hashes": artifact_hashes == ARTIFACTS,
        "reviewed_numeric_values": all(numeric_checks),
        "specification_cap_above_targets": all(
            row["spec_cap_margin"] > 0 for row in rows
        ),
        "free_memory_below_targets": all(
            row["spec_free_margin"] < 0 for row in rows
        ),
        "charged_memory_above_targets": all(
            min(row["w_to_one_third_charged_range"]) > row["target"]
            for row in rows
        ),
        "lattice_determinants_p_over_4": all(
            row["det3_is_p_over_4"] for row in lattice
        ),
        "large_radius_counts_within_five_percent": all(
            abs(point["ratio"] - 1) < 0.05
            for row in lattice for point in row["counts"][-3:]
        ),
        "success_model_nonzero_points_within_0.55_bit": (
            success_deltas and max(success_deltas) < 0.55
        ),
        "level_v_end_to_end_not_claimed": not (
            DATA / "validation-results/val_p664_17.jsonl"
        ).exists(),
    }
    result = {
        "finding": "SQIsign-4",
        "evidence": "validated finite-size cost model; not a full-scale attack",
        "parameter_sets": rows,
        "success_model_max_delta_bits": max(success_deltas),
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
