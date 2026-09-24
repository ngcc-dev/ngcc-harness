#!/usr/bin/env python3
"""Summarize context-qualified Frost breakdown profiling CSVs.

Input raw CSV format:
  scheme,level,mode,implementation_backend,matrix_backend,message_codec,component,cycles,iterations,status,notes

The raw component names are context-qualified (for example KeyGen.Expand_A
and Encaps.Expand_A). The summary intentionally avoids merging components
across KeyGen, Encaps, Decaps, and Decaps re-encryption contexts.
"""
from __future__ import annotations

import csv
import sys
from pathlib import Path

SUMMARY_FIELDS = [
    "scheme", "level", "mode", "implementation_backend", "matrix_backend", "message_codec",
    "KeyGen.Expand_A", "KeyGen.A_times_S", "KeyGen.Dither_PK", "KeyGen.Quant_PK", "KeyGen.Pack_PK", "KeyGen.PK_Hash",
    "Encaps.Expand_A", "Encaps.AT_times_R", "Encaps.BhatT_times_R", "Encaps.Dither_UV", "Encaps.Quant_UV", "Encaps.Pack_CT", "Encaps.Hash_KDF",
    "Decaps.ST_times_U", "Decaps.ReEnc_Expand_A", "Decaps.ReEnc_AT_times_R", "Decaps.ReEnc_BhatT_times_R", "Decaps.ReEnc_Dither_UV", "Decaps.ReEnc_Quant_UV", "Decaps.Hash_KDF",
    "Codec.Message_Encode", "Codec.Message_Decode",
    "notes",
]

TABLE_FIELDS = [
    ("Level", None),
    ("Impl.", None),
    ("Matrix A", None),
    ("KG Expand A", "KeyGen.Expand_A"),
    ("KG A*S", "KeyGen.A_times_S"),
    ("KG Quant", "KeyGen.Quant_PK"),
    ("Enc Expand A", "Encaps.Expand_A"),
    ("Enc A^T*R", "Encaps.AT_times_R"),
    ("Enc B^T*R", "Encaps.BhatT_times_R"),
    ("Enc Quant", "Encaps.Quant_UV"),
    ("Dec S^T*U", "Decaps.ST_times_U"),
    ("Dec ReEnc A^T*R", "Decaps.ReEnc_AT_times_R"),
    ("Dec ReEnc B^T*R", "Decaps.ReEnc_BhatT_times_R"),
    ("Dec Quant", "Decaps.ReEnc_Quant_UV"),
    ("Hash/KDF", "All.Hash_KDF"),
    ("Msg Enc", "Codec.Message_Encode"),
    ("Msg Dec", "Codec.Message_Decode"),
]

CAPTION = (
    "The component timings are collected under the profiling build and are "
    "reported as context-qualified profiling medians. They are intended to "
    "identify implementation bottlenecks. They are not additive decompositions "
    "of the normal full-KEM timings. Full KEM KeyGen, Encaps, and Decaps "
    "timings are reported separately using the normal benchmark harness."
)


def fmt_num(value: float | None) -> str:
    if value is None:
        return "NA"
    return str(int(round(value)))


def fmt_kcycles(value: float | None) -> str:
    if value is None:
        return "NA"
    return f"{value / 1000.0:.1f}"


def sum_components(comp: dict[str, float], names: list[str]) -> float | None:
    values = [comp[name] for name in names if name in comp]
    if not values:
        return None
    return sum(values)


def get_component(comp: dict[str, float], name: str) -> float | None:
    return comp.get(name)


def main(argv: list[str]) -> int:
    if len(argv) != 4:
        print("usage: summarize_profile_breakdown.py RAW.csv SUMMARY.csv TABLE.tex", file=sys.stderr)
        return 2

    raw_path, summary_path, table_path = map(Path, argv[1:])
    rows: dict[tuple[str, str, str, str, str, str], dict[str, float]] = {}
    with raw_path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row.get("status") != "ok":
                continue
            impl_backend = row.get("implementation_backend") or row.get("backend", "")
            matrix_backend = row.get("matrix_backend", "unknown")
            message_codec = row.get("message_codec") or ("e8" if "e8" in row.get("notes", "") else "scalar")
            key = (row["scheme"], row["level"], row["mode"], impl_backend, matrix_backend, message_codec)
            component = row["component"]
            # The raw CSV is already median-reduced per context-qualified component.
            # Keep each context-qualified component separate and never merge names like
            # KeyGen.Expand_A, Encaps.Expand_A, and Decaps.ReEnc_Expand_A.
            rows.setdefault(key, {})[component] = float(row["cycles"])

    summary_path.parent.mkdir(parents=True, exist_ok=True)
    table_path.parent.mkdir(parents=True, exist_ok=True)

    summary_rows: list[tuple[tuple[str, str, str, str, str, str], dict[str, float | None]]] = []
    mode_order = {"REFERENCE": 0, "FAST": 1}
    for key in sorted(rows, key=lambda k: (int(k[1]), mode_order.get(k[2], 99), k[3], k[4], k[5])):
        comp = rows[key]
        vals: dict[str, float | None] = {
            "KeyGen.Expand_A": get_component(comp, "KeyGen.Expand_A"),
            "KeyGen.A_times_S": get_component(comp, "KeyGen.A_times_S"),
            "KeyGen.Dither_PK": get_component(comp, "KeyGen.Dither_PK"),
            "KeyGen.Quant_PK": get_component(comp, "KeyGen.Quant_PK"),
            "KeyGen.Pack_PK": get_component(comp, "KeyGen.Pack_PK"),
            "KeyGen.PK_Hash": get_component(comp, "KeyGen.PK_Hash"),
            "Encaps.Expand_A": get_component(comp, "Encaps.Expand_A"),
            "Encaps.AT_times_R": get_component(comp, "Encaps.AT_times_R"),
            "Encaps.BhatT_times_R": get_component(comp, "Encaps.BhatT_times_R"),
            "Encaps.Dither_UV": sum_components(comp, ["Encaps.Dither_U", "Encaps.Dither_V"]),
            "Encaps.Quant_UV": sum_components(comp, ["Encaps.Quant_U", "Encaps.Quant_V"]),
            "Encaps.Pack_CT": get_component(comp, "Encaps.Pack_CT"),
            "Encaps.Hash_KDF": sum_components(comp, ["Encaps.PK_Hash", "Encaps.FO_Hash", "Encaps.KDF"]),
            "Decaps.ST_times_U": get_component(comp, "Decaps.ST_times_U"),
            "Decaps.ReEnc_Expand_A": get_component(comp, "Decaps.ReEnc_Expand_A"),
            "Decaps.ReEnc_AT_times_R": get_component(comp, "Decaps.ReEnc_AT_times_R"),
            "Decaps.ReEnc_BhatT_times_R": get_component(comp, "Decaps.ReEnc_BhatT_times_R"),
            "Decaps.ReEnc_Dither_UV": sum_components(comp, ["Decaps.ReEnc_Dither_U", "Decaps.ReEnc_Dither_V"]),
            "Decaps.ReEnc_Quant_UV": sum_components(comp, ["Decaps.ReEnc_Quant_U", "Decaps.ReEnc_Quant_V"]),
            "Decaps.Hash_KDF": sum_components(comp, ["Decaps.FO_Hash", "Decaps.KDF"]),
            "Codec.Message_Encode": get_component(comp, "Codec.Message_Encode"),
            "Codec.Message_Decode": get_component(comp, "Codec.Message_Decode"),
        }
        vals["All.Hash_KDF"] = sum_components(comp, ["Encaps.PK_Hash", "Encaps.FO_Hash", "Encaps.KDF", "Decaps.FO_Hash", "Decaps.KDF", "KeyGen.PK_Hash"])
        summary_rows.append((key, vals))

    notes = "profile-build component medians only; normal full-KEM timings come from bench_levels_ref_avx2.sh"
    with summary_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=SUMMARY_FIELDS, lineterminator="\n")
        writer.writeheader()
        for (scheme, level, mode, impl_backend, matrix_backend, message_codec), vals in summary_rows:
            out = {"scheme": scheme, "level": level, "mode": mode, "implementation_backend": impl_backend, "matrix_backend": matrix_backend, "message_codec": message_codec, "notes": notes}
            for field in SUMMARY_FIELDS[6:-1]:
                out[field] = fmt_num(vals.get(field))
            writer.writerow(out)

    with table_path.open("w") as f:
        f.write("% Generated from Frost/benchmarks/breakdown_profile.csv. Values are kCycles.\n")
        f.write("\\begin{table}[t]\n")
        f.write("\\centering\n")
        f.write(f"\\caption{{{CAPTION}}}\n")
        f.write("\\begin{tabular}{rrrrrrrrrrrrrrrrr}\n")
        f.write("\\toprule\n")
        f.write(" & ".join(title for title, _ in TABLE_FIELDS) + " " + (chr(92) * 2) + "\n")
        f.write("\\midrule\n")
        for (scheme, level, mode, impl_backend, matrix_backend, message_codec), vals in summary_rows:
            impl = f"{mode}/{impl_backend}"
            cells = [level, impl, matrix_backend]
            for _, field in TABLE_FIELDS[3:]:
                cells.append(fmt_kcycles(vals.get(field)))
            f.write(" & ".join(cells) + " " + (chr(92) * 2) + "\n")
        f.write("\\bottomrule\n")
        f.write("\\end{tabular}\n")
        f.write("\\end{table}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
