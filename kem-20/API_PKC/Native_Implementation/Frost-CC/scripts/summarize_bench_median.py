#!/usr/bin/env python3
"""Summarize repeated MAMBA-Frost benchmark CSVs with medians and spreads."""

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from statistics import median

OPS = ["keygen_cycles", "encaps_cycles", "decaps_cycles", "total_cycles"]
MEDIAN_FIELDS = [
    "scheme", "level", "mode", "backend",
    "keygen_cycles", "encaps_cycles", "decaps_cycles", "total_cycles",
    "pk_bytes", "ct_bytes", "sk_bytes", "ss_bytes",
]
SPREAD_FIELDS = [
    "scheme", "level", "mode", "backend", "operation",
    "min_cycles", "median_cycles", "max_cycles", "relative_spread",
]


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", help="Input benchmark CSV files")
    parser.add_argument("--median-out", required=True, help="Output final_median.csv path")
    parser.add_argument("--spread-out", required=True, help="Output final_spread.csv path")
    parser.add_argument("--table-out", required=True, help="Output LaTeX table fragment path")
    return parser.parse_args()


def row_key(row):
    return (row["scheme"], int(row["level"]), row["mode"], row["backend"])


def read_rows(paths):
    grouped = defaultdict(list)
    for path in paths:
        with open(path, newline="") as fh:
            reader = csv.DictReader(fh)
            for row in reader:
                if row.get("status") != "ok":
                    raise ValueError(f"{path}: non-ok row: {row}")
                for op in OPS:
                    if not row.get(op):
                        raise ValueError(f"{path}: empty {op}: {row}")
                    row[op] = int(row[op])
                for size in ["pk_bytes", "ct_bytes", "sk_bytes", "ss_bytes"]:
                    row[size] = int(row[size])
                grouped[row_key(row)].append(row)
    return grouped


def ensure_complete(grouped, expected_runs):
    expected_levels = {128, 192, 256, 384, 512}
    seen_levels = {key[1] for key in grouped}
    if seen_levels != expected_levels:
        raise ValueError(f"expected levels {sorted(expected_levels)}, got {sorted(seen_levels)}")
    for level in expected_levels:
        modes = {key[2] for key in grouped if key[1] == level}
        if modes != {"REFERENCE", "FAST"}:
            raise ValueError(f"level {level}: expected REFERENCE and FAST, got {sorted(modes)}")
    for key, rows in grouped.items():
        if len(rows) != expected_runs:
            raise ValueError(f"{key}: expected {expected_runs} rows, got {len(rows)}")
        size_tuple = {(r["pk_bytes"], r["ct_bytes"], r["sk_bytes"], r["ss_bytes"]) for r in rows}
        if len(size_tuple) != 1:
            raise ValueError(f"{key}: size fields differ across runs: {size_tuple}")


def summarize(grouped):
    median_rows = []
    spread_rows = []
    for key in sorted(grouped, key=lambda k: (k[1], {"REFERENCE": 0, "FAST": 1}.get(k[2], 2), k[3])):
        scheme, level, mode, backend = key
        rows = grouped[key]
        out = {
            "scheme": scheme,
            "level": str(level),
            "mode": mode,
            "backend": backend,
        }
        for size in ["pk_bytes", "ct_bytes", "sk_bytes", "ss_bytes"]:
            out[size] = str(rows[0][size])
        for op in OPS:
            values = [r[op] for r in rows]
            med = int(median(values))
            out[op] = str(med)
            mn = min(values)
            mx = max(values)
            spread = (mx - mn) / med if med else 0.0
            spread_rows.append({
                "scheme": scheme,
                "level": str(level),
                "mode": mode,
                "backend": backend,
                "operation": op,
                "min_cycles": str(mn),
                "median_cycles": str(med),
                "max_cycles": str(mx),
                "relative_spread": f"{spread:.6f}",
            })
        median_rows.append(out)
    return median_rows, spread_rows


def write_csv(path, fields, rows):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def fmt_cycles(value):
    return f"{int(value):,}"


def write_table(path, median_rows):
    by_level = defaultdict(dict)
    latex_break = "\\\\"
    for row in median_rows:
        by_level[int(row["level"])][row["mode"]] = row
    lines = [
        "% Generated from Frost/benchmarks/final_median.csv.",
        "% Reference profiles: u16 path; FAST profiles: avx2_u16 for 128/192/256/384/512.",
        "\\begin{tabular}{rrrrrrrrrrrrr}",
        "\\toprule",
        "Level & Ref KeyGen & Ref Encaps & Ref Decaps & Opt KeyGen & Opt Encaps & Opt Decaps & pk & ct & sk & ss & Ref backend & Opt backend " + latex_break,
        "\\midrule",
    ]
    for level in sorted(by_level):
        ref = by_level[level]["REFERENCE"]
        opt = by_level[level]["FAST"]
        lines.append(
            f"{level} & {fmt_cycles(ref['keygen_cycles'])} & {fmt_cycles(ref['encaps_cycles'])} & {fmt_cycles(ref['decaps_cycles'])} & "
            f"{fmt_cycles(opt['keygen_cycles'])} & {fmt_cycles(opt['encaps_cycles'])} & {fmt_cycles(opt['decaps_cycles'])} & "
            f"{ref['pk_bytes']} & {ref['ct_bytes']} & {ref['sk_bytes']} & {ref['ss_bytes']} & {ref['backend']} & {opt['backend']} " + latex_break
        )
    lines.extend(["\\bottomrule", "\\end{tabular}", ""])
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text("\n".join(lines))

def main():
    args = parse_args()
    grouped = read_rows(args.inputs)
    ensure_complete(grouped, len(args.inputs))
    median_rows, spread_rows = summarize(grouped)
    write_csv(args.median_out, MEDIAN_FIELDS, median_rows)
    write_csv(args.spread_out, SPREAD_FIELDS, spread_rows)
    write_table(args.table_out, median_rows)


if __name__ == "__main__":
    main()
