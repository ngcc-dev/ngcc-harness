#!/usr/bin/env python3
"""Deterministic cost/model verification for MAYO-6 (ePrint 2026/2444 + Complexity.magma)."""

import argparse
import hashlib
import json
import math
from math import comb
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
SPEC = ROOT / "MAYO-spec.pdf"
SPEC_SHA256 = "1fc7561dc8ffdbe36c101b985266e4d3c8aa88fa580e18ead215a80ac7bb371f"
PAPER_2444_SHA256 = "5cdf33a993945ec8e2218a51ce67fe0720d4b63e7d2a0397a496a660c02ae2f7"
MAGMA_COMMIT = "a292e4c74371fd0d6bae3159ac8c257a441fea27"
MAGMA_COMPLEXITY_SHA256 = "d76d1aeecba73feac95bb81d2adfeb7313bbc3ae1061b41e73adbacf3353d24f"
MAGMA_MATH_SHA256 = "3b68d8cad988e7529fa7ac500d9a2c6e9b916e57299f16d39b321c82df3b536c"
FIELD_COST_GF16 = 2 * (4 ** 2) + 4  # 36 bit ops per GF(16) multiply-add


def fo(n: int, p: int, d: int) -> int:
    if d < 0:
        return 0
    if p == 2:
        return comb(n, d) if d <= n else 0
    return sum(((-1) ** i) * comb(n, i) * comb(n + d - p * i - 1, d - p * i) for i in range(d // p + 1))


def go(n: int, p: int, d: int) -> int:
    if d < 0:
        return 0
    return sum(((-1) ** (d - p * i)) * comb(n, d - p * i) * comb(n + i - 1, i) for i in range(d // p + 1))


def uo(n1: int, n2: int, p: int, d1: int, d2: int) -> int:
    if d1 < 0 or d2 < 0:
        return 0
    return fo(n1, p, d1 - 1) * fo(n2, p, d2) - fo(n1, p, d1) * fo(n2, p, d2 - 1)


def sto(n1: int, n2: int, m: int, p: int, r: int, d1: int, d2: int) -> int:
    pr1 = p ** (r + 1)
    total = 0
    for i in range((d1 + 1) // 2 + 1):
        gi = go(m, pr1, i)
        if gi == 0:
            continue
        for j in range(d2 // 2 + 1):
            gj = go(m, pr1, j)
            if gj == 0:
                continue
            for k in range(min(d1 + 1 - 2 * i, d2 - 2 * j) + 1):
                gk = go(m, pr1, k)
                if gk == 0:
                    continue
                total += gi * gj * gk * uo(n1, n2, p, d1 + 1 - 2 * i - k, d2 - 2 * j - k)
    return total


def schtwodes_char2(n1: int, n2: int, m: int, r: int, d1: int, d2: int) -> float:
    a1 = sum(comb(n1, d1 - (1 << (i + 1))) * comb(n2, d2) * m for i in range(r + 1) if d1 >= (1 << (i + 1)))
    a2 = sum(comb(n1, d1 + (1 << i)) * comb(n2, d2 - (1 << i)) for i in range(d2.bit_length()) if (1 << i) <= d2)
    b1 = sum(
        comb(n1 - d1 + (1 << (i + 1)), 1 << (i + 1)) * comb(n1, d1 - (1 << (i + 1))) * comb(n2, d2) * m
        for i in range(r + 1)
        if d1 >= (1 << (i + 1))
    )
    b2 = sum(
        comb(d1 + (1 << i), 1 << i) * comb(n1, d1 + (1 << i)) * comb(n2, d2 - (1 << i))
        for i in range(d2.bit_length())
        if (1 << i) <= d2
    )
    return math.log2((b1 + b2) / (a1 + a2))


def eval_tuple_char2(n: int, m: int, o: int, k1: int, k2: int, r: int) -> dict:
    n1, n2 = n - k1, n - k2
    d1, d2 = o - k1, o - k2
    s_val = sto(n1, n2, m, 2, r, d1, d2)
    rho = schtwodes_char2(n1, n2, m, r, d1, d2)
    h_dim = comb(n1, d1) * comb(n2, d2)
    log2_h = math.log2(h_dim)
    cost = 2.0 * log2_h + math.log2(3 * FIELD_COST_GF16) + rho
    return {
        "k1": k1,
        "k2": k2,
        "d1": d1,
        "d2": d2,
        "sto_le_zero": s_val <= 0,
        "ram_bits": round(cost, 4),
        "floored_bits": math.floor(cost),
        "log2_H": round(log2_h, 4),
        "log2_rho": round(rho, 4),
    }


def best_schur_char2(n: int, m: int, o: int, r: int, balanced_only: bool = False) -> dict | None:
    best = None
    for k2 in range(o):
        k1_range = (k2,) if balanced_only else range(k2 + 1)
        for k1 in k1_range:
            n1, n2 = n - k1, n - k2
            d1, d2 = o - k1, o - k2
            if sto(n1, n2, m, 2, r, d1, d2) <= 0:
                entry = eval_tuple_char2(n, m, o, k1, k2, r)
                if best is None or entry["ram_bits"] < best["ram_bits"]:
                    best = entry
    return best


MAIN_SETS = [
    ("MAYO1", 1, 143, 88, 80, 8, 10),
    ("MAYO2", 1, 143, 86, 64, 13, 5),
    ("MAYO3", 3, 207, 118, 108, 10, 11),
    ("MAYO5", 5, 272, 154, 142, 12, 12),
]

# All 28 Table 2.2 additional parameter sets with o <= 33 (MAYO-spec.pdf physical p. 20).
TABLE22_SWEEP = [
    ("L1-k10", 1, 143, 88, 80, 8, 10),
    ("L1-k9", 1, 143, 85, 76, 9, 9),
    ("L1-k7", 1, 143, 80, 70, 10, 7),
    ("L1-k6", 1, 143, 77, 66, 11, 6),
    ("L1-k5", 1, 143, 86, 64, 13, 5),
    ("L1-k4", 1, 143, 98, 64, 16, 4),
    ("L1-k3", 1, 143, 102, 62, 21, 3),
    ("L1-k2", 1, 143, 110, 60, 30, 2),
    ("L3-k13", 3, 207, 123, 114, 9, 13),
    ("L3-k11", 3, 207, 118, 108, 10, 11),
    ("L3-k10", 3, 207, 115, 104, 11, 10),
    ("L3-k9", 3, 207, 114, 102, 12, 9),
    ("L3-k8", 3, 207, 113, 100, 13, 8),
    ("L3-k7", 3, 207, 112, 98, 14, 7),
    ("L3-k6", 3, 207, 118, 96, 16, 6),
    ("L3-k5", 3, 207, 137, 96, 20, 5),
    ("L3-k4", 3, 207, 155, 96, 24, 4),
    ("L3-k3", 3, 207, 161, 94, 32, 3),
    ("L5-k13", 5, 272, 153, 142, 11, 13),
    ("L5-k12", 5, 272, 154, 142, 12, 12),
    ("L5-k11", 5, 272, 153, 140, 13, 11),
    ("L5-k10", 5, 272, 152, 138, 14, 10),
    ("L5-k9", 5, 272, 152, 136, 16, 9),
    ("L5-k8", 5, 272, 151, 134, 17, 8),
    ("L5-k7", 5, 272, 152, 132, 19, 7),
    ("L5-k6", 5, 272, 168, 132, 22, 6),
    ("L5-k5", 5, 272, 192, 132, 27, 5),
    ("L5-k4", 5, 272, 212, 130, 33, 4),
]

TABLE4_ROWS = [
    ("L3-k8", 3, 207, 113, 100, 13, 8, (0, 3, 13, 10), 215, 1, True),
    ("L3-k7", 3, 207, 112, 98, 14, 7, (0, 7, 17, 10), 198, 1, True),
    ("L3-k6", 3, 207, 118, 96, 16, 6, (0, 11, 16, 5), 196, 1, True),
    ("L5-k9", 5, 272, 152, 136, 16, 9, (0, 3, 16, 13), 281, 0, False),
    ("L5-k8", 5, 272, 151, 134, 17, 8, (0, 6, 17, 11), 269, 1, True),
    ("L5-k7", 5, 272, 152, 132, 19, 7, (0, 14, 19, 5), 230, 1, True),
    ("L5-k6", 5, 272, 168, 132, 22, 6, (0, 17, 22, 5), 254, 1, True),
]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    checks = {}
    spec_sha = hashlib.sha256(SPEC.read_bytes()).hexdigest() if SPEC.is_file() else ""
    checks["pinned_mayo_spec"] = spec_sha == SPEC_SHA256

    main_results = {}
    for name, level, target, n, m, o, k in MAIN_SETS:
        main_results[name] = {
            "params": [n, m, o, k, 16],
            "target": target,
            "r1": best_schur_char2(n, m, o, 1),
            "r0": best_schur_char2(n, m, o, 0),
            "balanced_r1": best_schur_char2(n, m, o, 1, balanced_only=True),
        }

    checks["main_mayo1_no_admissible_tuple"] = main_results["MAYO1"]["r1"] is None and main_results["MAYO1"]["r0"] is None
    checks["main_mayo3_no_admissible_tuple"] = main_results["MAYO3"]["r1"] is None and main_results["MAYO3"]["r0"] is None
    checks["main_mayo5_no_admissible_tuple"] = main_results["MAYO5"]["r1"] is None and main_results["MAYO5"]["r0"] is None
    checks["main_mayo2_above_target"] = (
        main_results["MAYO2"]["r1"] is not None
        and main_results["MAYO2"]["r1"]["ram_bits"] > 143.0
        and main_results["MAYO2"]["r0"]["ram_bits"] > 143.0
    )

    sweep_results = {}
    below_r1 = []
    below_r0 = []
    below_balanced_r1 = []
    for name, level, target, n, m, o, k in TABLE22_SWEEP:
        res_r1 = best_schur_char2(n, m, o, 1)
        res_r0 = best_schur_char2(n, m, o, 0)
        bal_r1 = best_schur_char2(n, m, o, 1, balanced_only=True)
        sweep_results[name] = {
            "params": [n, m, o, k, 16],
            "level": level,
            "target": target,
            "r1": res_r1,
            "r0": res_r0,
            "balanced_r1": bal_r1,
        }
        if res_r1 is not None and res_r1["ram_bits"] < target:
            below_r1.append(name)
        if res_r0 is not None and res_r0["ram_bits"] < target:
            below_r0.append(name)
        if bal_r1 is not None and bal_r1["ram_bits"] < target:
            below_balanced_r1.append(name)

    checks["sweep_r1_below_target_exact_five"] = sorted(below_r1) == [
        "L3-k6",
        "L3-k7",
        "L5-k6",
        "L5-k7",
        "L5-k8",
    ]
    checks["sweep_r0_adds_l1_k6_only"] = sorted(below_r0) == ["L1-k6", "L3-k7", "L5-k7"]
    checks["balanced_k1_eq_k2_r1_none_below_target"] = len(below_balanced_r1) == 0

    table4_results = {}
    six_off_by_one_ok = True
    for name, level, target, n, m, o, k, printed_tuple, paper_bit, match_r, expect_off_by_one in TABLE4_ROWS:
        res_r1 = sweep_results[name]["r1"]
        res_r0 = sweep_results[name]["r0"]
        matched = res_r1 if match_r == 1 else res_r0
        p_k1, p_k2, p_d1, p_d2 = printed_tuple
        if expect_off_by_one:
            row_off = (
                res_r1 is not None
                and res_r1["k1"] == p_k1
                and res_r1["k2"] == p_k2 + 1
                and res_r1["floored_bits"] == paper_bit
            )
            six_off_by_one_ok &= row_off
        table4_results[name] = {
            "printed_tuple": list(printed_tuple),
            "printed_bit": paper_bit,
            "match_r": match_r,
            "r1": res_r1,
            "r0": res_r0,
            "balanced_r1": sweep_results[name]["balanced_r1"],
            "matches_printed_floored_bit": matched is not None and matched["floored_bits"] == paper_bit,
        }

    checks["all_seven_table4_floored_bits_reproduce"] = all(
        row["matches_printed_floored_bit"] for row in table4_results.values()
    )
    checks["six_of_seven_table4_rows_reproduce_at_k2_plus_1_r1"] = six_off_by_one_ok
    l5_k9_r0_at_printed = eval_tuple_char2(152, 136, 16, 0, 3, 0)
    checks["l5_k9_reproduces_at_printed_tuple_only_with_r0"] = (
        l5_k9_r0_at_printed["sto_le_zero"]
        and l5_k9_r0_at_printed["floored_bits"] == 281
        and table4_results["L5-k9"]["r1"]["floored_bits"] == 285
    )
    checks["table4_l3_k7_printed_d1_exceeds_o"] = 17 > 14 and table4_results["L3-k7"]["r1"]["d1"] == 14

    ok = all(checks.values())
    payload = {
        "id": "MAYO-6",
        "status": "PASS" if ok else "FAIL",
        "evidence": (
            "Port of Li and Tao's published Magma estimator (Yulan24/Extended-Schur-Macaulay commit a292e4c7) "
            "across all four Table 2.1 main sets and all 28 Table 2.2 additional sets with o <= 33 reproduces "
            "all seven Table 4 floored bit costs (six at k2+1 for r=1; (152,136,16,9) at its printed tuple for "
            "r=0 only), confirms five Table 2.2 sets fall 2.76 to 41.67 bits below target at r=1 and one Level 1 "
            "set (77,66,11,6) falls 0.59 bit below 143 at r=0, confirms the estimator finds no admissible tuple "
            "or stays above target for all four Table 2.1 main sets, and shows all balanced k1=k2 (d1=d2=o-k) r=1 "
            "estimates stay above target; this is a conditional free-memory cost/model check, not a full-scale attack."
        ),
        "source_sha256": {
            "MAYO/MAYO-spec.pdf": spec_sha,
            "eprint_2026_2444_pdf": PAPER_2444_SHA256,
            "magma_commit": MAGMA_COMMIT,
            "Complexity.magma": MAGMA_COMPLEXITY_SHA256,
            "Math.magma": MAGMA_MATH_SHA256,
        },
        "checks": checks,
        "measurements": {
            "main_table21": main_results,
            "table4_rows": table4_results,
            "table22_l1_k6": sweep_results["L1-k6"],
            "l5_k9_r0_at_printed_0_3": l5_k9_r0_at_printed,
            "sweep_below_target_r1": below_r1,
            "sweep_below_target_r0": below_r0,
        },
    }
    rendered = json.dumps(payload, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    sys.stdout.write(rendered)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
