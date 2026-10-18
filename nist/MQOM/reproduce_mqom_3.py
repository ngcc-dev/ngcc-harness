#!/usr/bin/env python3
"""Recompute MQOM Table 26's correlated-tree proof margins."""

import csv
import hashlib
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC_SHA256 = "489942de99a8001b77f8abc46bf93524aeb4719c46f23f8f6cb139d9eaeaac45"

# (field order, NIST category): (MQ variables, Table 26 lower bound, target).
TABLE_26 = {
    (2, 1): (160, 122, 143),
    (2, 3): (240, 188, 207),
    (2, 5): (320, 245, 272),
    (16, 1): (64, 121, 143),
    (16, 3): (96, 171, 207),
    (16, 5): (128, 223, 272),
}


def main() -> int:
    rows, gaps = [], []
    for (field, category), (variables, lower_bound, target) in TABLE_26.items():
        gap = target - lower_bound
        gaps.append(gap)
        rows.append({
            "field": f"GF({field})",
            "category": category,
            "variables": variables,
            "table_26_lower_bound_bits": lower_bound,
            "nist_target_bits": target,
            "gap_to_target_bits": gap,
        })

    with (HERE / "parameters.csv").open(newline="") as source:
        reference = [row for row in csv.DictReader(source) if row["Implementation"] == "reference"]
    correlated = [row for row in reference if row["ParameterSet"].endswith("_ct")]
    one_tree = [row for row in reference if row["ParameterSet"].endswith("_ot")]
    covered = []
    for row in correlated:
        field = 2 if "_gf2_" in row["ParameterSet"] else 16
        key = (field, int(row["NISTCategory"]))
        covered.append({
            "parameter": row["ParameterSet"],
            "table_row": f"GF({field})/category-{key[1]}",
            "lower_bound_bits": TABLE_26[key][1],
        })

    checks = {
        "pinned_specification": hashlib.sha256(
            (HERE / "MQOM-spec.pdf").read_bytes()
        ).hexdigest() == SPEC_SHA256,
        "table_26_gaps_are_19_to_49_bits": min(gaps) == 19 and max(gaps) == 49,
        "table_26_exact_gaps": gaps == [21, 19, 27, 22, 36, 49],
        "all_nine_correlated_tree_sets_covered": len(correlated) == 9 and len(covered) == 9,
        "nine_one_tree_sets_excluded": len(one_tree) == 9,
    }
    result = {
        "finding": "MQOM-3",
        "evidence": "Table 26 arithmetic and submitted-parameter scope check; not an attack",
        "model": (
            "PGOW-to-standard-MQ lower bounds for the correlated-tree EUF-CMA assumption; "
            "one-tree sets rely directly on standard MQ and are excluded"
        ),
        "table_26": rows,
        "covered_parameter_sets": covered,
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
