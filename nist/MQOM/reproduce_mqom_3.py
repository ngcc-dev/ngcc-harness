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

# Full-MQ cost rows the reduction loses against (spec Table 24 Crossbred for
# GF(2), Table 23 WXL for GF(16)); loss = full cost - Table 26 lower bound.
FULL_MQ_COST = {
    (2, 1): 144.0, (2, 3): 213.0, (2, 5): 282.0,
    (16, 1): 165.6, (16, 3): 235.1, (16, 5): 302.1,
}
# Cheaper rows that the selected comparison does not use: PXL (Table 23) and
# Dinur's 1.9th algorithm (Table 24, much larger space: 112/164/218 bits).
PXL_COST = {(16, 1): 161.7, (16, 3): 237.7, (16, 5): 313.3}
DINUR_19_COST = {(2, 1): 147.0, (2, 3): 212.0, (2, 5): 278.0}


def main() -> int:
    rows, gaps, losses = [], [], []
    for (field, category), (variables, lower_bound, target) in TABLE_26.items():
        gap = target - lower_bound
        gaps.append(gap)
        losses.append(round(FULL_MQ_COST[(field, category)] - lower_bound, 1))
        rows.append({
            "field": f"GF({field})",
            "category": category,
            "variables": variables,
            "table_26_lower_bound_bits": lower_bound,
            "nist_target_bits": target,
            "gap_to_target_bits": gap,
            "full_mq_cost_bits": FULL_MQ_COST[(field, category)],
            "reduction_loss_vs_full_mq_bits": losses[-1],
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
        "reduction_losses_22_to_79_bits": losses == [22.0, 25.0, 37.0, 44.6, 64.1, 79.1],
        "spec_20_to_65_range_understates_gf16_category_5": max(losses) > 65,
        "selected_rows_not_always_cheapest": (
            round(FULL_MQ_COST[(16, 1)] - PXL_COST[(16, 1)], 1) == 3.9
            and [FULL_MQ_COST[(2, c)] - DINUR_19_COST[(2, c)] for c in (1, 3, 5)] == [-3.0, 1.0, 4.0]
        ),
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
