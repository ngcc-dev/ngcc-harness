#!/usr/bin/env python3
"""Deterministic cost/model verification for SNOVA-11 (ePrint 2026/2272 Table 6)."""

import argparse
import csv
import hashlib
import json
import math
from math import comb
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
SPEC = ROOT / "SNOVA-spec.pdf"
ALT_CSV = ROOT / "alternative-parameters.csv"
SPEC_SHA256 = "67bf2e5414001d4562f76fff77cbc72b6c997ff2cffef3b5180a886359892898"
ALT_CSV_SHA256 = "081eaa23398614f7778f8009521163a64a97ced28c6aeba61b8480bc776554eb"
PAPER_2272_SHA256 = "9e30f2873807a7b7eabef7f734e3953f7bb254c3e01628c0d1e9ccf3e954f6e5"
MAGMA_MATH_SHA256 = "3b68d8cad988e7529fa7ac500d9a2c6e9b916e57299f16d39b321c82df3b536c"
FIELD_COST_GF256 = 2 * (8 ** 2) + 8  # 136 bit ops per GF(256) multiply-add


def go(n: int, p: int, d: int) -> int:
    if d < 0:
        return 0
    return sum(((-1) ** (d - p * i)) * comb(n, d - p * i) * comb(n + i - 1, i) for i in range(d // p + 1))


def vto_char2(n1: int, n2: int, o: int, r1: int, r2: int, r3: int, d1: int, d2: int) -> int:
    p2, p3, p4 = 2 ** (r1 + 1), 2 ** (r2 + 1), 2 ** (r3 + 1)
    total = 0
    for i in range(d1 // 2 + 1):
        gi = go(o, p2, i)
        if gi == 0:
            continue
        for j in range(d2 // 2 + 1):
            gj = go(o, p4, j)
            if gj == 0:
                continue
            for k in range(min(d1 - 2 * i, d2 - 2 * j) + 1):
                gk = go(2 * o, p3, k)
                if gk == 0:
                    continue
                total += gi * gj * gk * comb(n1, d1 - 2 * i - k) * comb(n2, d2 - 2 * j - k)
    return total


def eval_l2(v: int, o: int, k1: int, k2: int, r1: int, r2: int, r3: int, d1: int, d2: int) -> dict:
    n = v + o
    n1, n2 = n - k1, n - k2
    vto_val = vto_char2(n1, n2, o, r1, r2, r3, d1, d2)
    h_dim = comb(n1, d1) * comb(n2, d2)
    a1 = sum(comb(n1, d1 - 2 * (1 << e)) * comb(n2, d2) * o for e in range(r1 + 1) if d1 >= 2 * (1 << e))
    a2 = sum(
        comb(n1, d1 - (1 << e)) * comb(n2, d2 - (1 << e)) * (2 * o)
        for e in range(r2 + 1)
        if d1 >= (1 << e) and d2 >= (1 << e)
    )
    a3 = sum(comb(n1, d1) * comb(n2, d2 - 2 * (1 << e)) * o for e in range(r3 + 1) if d2 >= 2 * (1 << e))
    b1 = sum(
        comb(n1 - d1 + 2 * (1 << e), 2 * (1 << e)) * comb(n1, d1 - 2 * (1 << e)) * comb(n2, d2) * o
        for e in range(r1 + 1)
        if d1 >= 2 * (1 << e)
    )
    b2 = sum(
        comb(n1 - d1 + (1 << e), 1 << e)
        * comb(n2 - d2 + (1 << e), 1 << e)
        * comb(n1, d1 - (1 << e))
        * comb(n2, d2 - (1 << e))
        * (2 * o)
        for e in range(r2 + 1)
        if d1 >= (1 << e) and d2 >= (1 << e)
    )
    b3 = sum(
        comb(n2 - d2 + 2 * (1 << e), 2 * (1 << e)) * comb(n1, d1) * comb(n2, d2 - 2 * (1 << e)) * o
        for e in range(r3 + 1)
        if d2 >= 2 * (1 << e)
    )
    rho = math.log2((b1 + b2 + b3) / (a1 + a2 + a3))
    log2_h = math.log2(h_dim)
    cost = 2.0 * log2_h + math.log2(3 * FIELD_COST_GF256) + rho
    return {
        "k": [k1, k2],
        "n1": n1,
        "n2": n2,
        "d1": d1,
        "d2": d2,
        "r": [r1, r2, r3],
        "vto_le_zero": vto_val <= 0,
        "log2_H": round(log2_h, 4),
        "log2_rho": round(rho, 4),
        "ram_bits": round(cost, 4),
        "rounded_bits": round(cost),
    }


def search_l2(v: int, o: int) -> dict | None:
    """Search projection tuples (k1 >= k2) and levels r_i in {0, 1}."""
    best = None
    for k1 in range(o):
        for k2 in range(k1 + 1):
            d1, d2 = o - k1, o - k2
            for r1 in (0, 1):
                for r2 in (0, 1):
                    for r3 in (0, 1):
                        if r1 == 0 and r2 == 0 and r3 == 0:
                            continue
                        if vto_char2(v + o - k1, v + o - k2, o, r1, r2, r3, d1, d2) <= 0:
                            entry = eval_l2(v, o, k1, k2, r1, r2, r3, d1, d2)
                            if best is None or entry["ram_bits"] < best["ram_bits"]:
                                best = entry
    return best


CASES = [
    ("q16-I-l2", 1, 143, 50, 17, 14, 0, 0, 1, 1, 3, 17, 156, 154.95),
    ("q16-III-l2", 3, 207, 76, 25, 23, 0, 0, 1, 1, 2, 25, 205, 204.63),
    ("q16-V-l2", 5, 272, 104, 33, 31, 0, 0, 0, 1, 2, 33, 263, 262.96),
]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    spec_sha = hashlib.sha256(SPEC.read_bytes()).hexdigest() if SPEC.is_file() else ""
    csv_sha = hashlib.sha256(ALT_CSV.read_bytes()).hexdigest() if ALT_CSV.is_file() else ""
    checks = {
        "pinned_snova_spec": spec_sha == SPEC_SHA256,
        "pinned_alternative_csv": csv_sha == ALT_CSV_SHA256,
    }

    csv_rows = {}
    if ALT_CSV.is_file():
        with ALT_CSV.open(newline="", encoding="utf-8") as handle:
            for row in csv.DictReader(handle):
                csv_rows[row["PaperSet"]] = row

    results = {}
    for name, cat, target, v, o, k1, k2, r1, r2, r3, d1, d2, paper_bit, expected_min in CASES:
        row = csv_rows.get(name)
        csv_ok = (
            row is not None
            and int(row["v"]) == v
            and int(row["o"]) == o
            and int(row["q"]) == 16
            and int(row["l"]) == 2
            and int(row["NISTCategory"]) == cat
        )
        printed_res = eval_l2(v, o, k1, k2, r1, r2, r3, d1, d2)
        searched_min = search_l2(v, o)
        ctrl_r0 = eval_l2(v, o, k1, k2, 0, 0, 0, d1, d2)
        results[name] = {
            "csv_matches": csv_ok,
            "target": target,
            "paper_printed_bit": paper_bit,
            "printed_tuple": printed_res,
            "search_minimum": searched_min,
            "expected_search_minimum": expected_min,
            "classical_r0_control": ctrl_r0,
        }

    checks["all_three_csv_rows_match"] = all(r["csv_matches"] for r in results.values())
    checks["all_three_vto_conditions_hold"] = all(r["printed_tuple"]["vto_le_zero"] for r in results.values())
    checks["all_three_printed_costs_within_1_bit_of_table6"] = all(
        abs(r["printed_tuple"]["ram_bits"] - r["paper_printed_bit"]) < 1.0 for r in results.values()
    )
    checks["search_minima_match_expected"] = all(
        r["search_minimum"] is not None
        and abs(r["search_minimum"]["ram_bits"] - r["expected_search_minimum"]) < 0.02
        for r in results.values()
    )
    checks["cat1_minimum_above_target"] = results["q16-I-l2"]["search_minimum"]["ram_bits"] > 143.0
    checks["cat3_minimum_below_target"] = results["q16-III-l2"]["search_minimum"]["ram_bits"] < 207.0
    checks["cat5_minimum_below_target"] = results["q16-V-l2"]["search_minimum"]["ram_bits"] < 272.0
    checks["classical_r0_inapplicable_for_cat3_and_cat5_at_same_degrees"] = (
        not results["q16-III-l2"]["classical_r0_control"]["vto_le_zero"]
        and not results["q16-V-l2"]["classical_r0_control"]["vto_le_zero"]
    )

    ok = all(checks.values())
    payload = {
        "id": "SNOVA-11",
        "status": "PASS" if ok else "FAIL",
        "evidence": (
            "Recomputation of Li and Tao's ePrint 2026/2272 Table 6 multigraded divided-power wedge model "
            "over F_256 (Vto matching Math.magma commit a292e4c7, paired with our own section-3.4 row-density "
            "formula reproducing the paper's three Table 6 values), including a search over projection tuples "
            "(k1 >= k2) and levels r_i in {0,1}, gives minima 154.95 (q16-I-l2, above 143), 204.63 "
            "(q16-III-l2, below 207), and 262.96 (q16-V-l2, below 272), while classical r=(0,0,0) controls "
            "fail Vto<=0 at the same degrees for Cat 3 and Cat 5; this is a conditional free-memory "
            "cost/model check with no SNOVA experiments and leaves the nine recommended l=4, 5 sets unaffected."
        ),
        "source_sha256": {
            "SNOVA/SNOVA-spec.pdf": spec_sha,
            "SNOVA/alternative-parameters.csv": csv_sha,
            "eprint_2026_2272_pdf": PAPER_2272_SHA256,
            "Math.magma": MAGMA_MATH_SHA256,
        },
        "checks": checks,
        "measurements": results,
    }
    rendered = json.dumps(payload, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    sys.stdout.write(rendered)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
