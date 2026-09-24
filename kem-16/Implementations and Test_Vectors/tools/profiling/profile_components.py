#!/usr/bin/env python3
"""Aggregate perf-report symbols into HARE implementation components.

Input is one or more text files produced by:

  perf report --stdio --no-children --percent-limit 0 --sort symbol,dso

The script intentionally treats this as a coarse hotspot view. It does not alter
KEM execution and is not a timing side-channel proof. Percentages are perf sample
percentages from the profiled benchmark process.
"""
import argparse
import csv
import re
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

COMPONENTS = [
    "GF2X",
    "Vector/sampling",
    "GF(2^8)",
    "Reed-Muller",
    "Reed-Solomon",
    "Compression",
    "KEM/PKE/FO",
    "Hash/XOF/API_PKC",
    "Benchmark/DRNG",
    "libc/runtime",
    "Other",
]

PATTERNS: List[Tuple[str, List[re.Pattern]]] = [
    ("GF2X", [
        re.compile(r"gf2x", re.I),
        re.compile(r"\b(vect_mul|karatsuba|toom3|clmul|pmull|reduce_arm|divide_by_y_plus_one)\b", re.I),
    ]),
    ("Vector/sampling", [
        re.compile(r"\b(vect_|barrett_reduce|compare_u32)\b", re.I),
        re.compile(r"support", re.I),
        re.compile(r"sample_fixed|generate_random", re.I),
    ]),
    ("GF(2^8)", [
        re.compile(r"\bgf_(mul|square|inverse|generate|reduce|pow)\b", re.I),
        re.compile(r"gf_reduce", re.I),
    ]),
    ("Reed-Muller", [
        re.compile(r"reed_muller", re.I),
        re.compile(r"\b(hadamard|expand_and_sum|find_peak|find_peaks|rm_)\b", re.I),
    ]),
    ("Reed-Solomon", [
        re.compile(r"reed_solomon", re.I),
        re.compile(r"\b(compute_syndromes|erasure_locator|modified_syndromes|error_locator|locator|forney|eval_poly|compute_generator_poly)\b", re.I),
    ]),
    ("Compression", [
        re.compile(r"compression", re.I),
        re.compile(r"\b(code_encode|code_decode|kr_|syndrome|covering)\b", re.I),
    ]),
    ("KEM/PKE/FO", [
        re.compile(r"\b(kem_|pke_|crypto_kem|hqc_pke|hqc_kem)\b", re.I),
        re.compile(r"implicit|decaps|encaps|keygen", re.I),
    ]),
    ("Hash/XOF/API_PKC", [
        re.compile(r"shake|xof|sha3|sha2|sm3|hash|randombytes|api_pkc|Hash", re.I),
    ]),
    ("Benchmark/DRNG", [
        re.compile(r"bench_|derive_seed|splitmix|init_prng|nist_random|drng|random", re.I),
    ]),
    ("libc/runtime", [
        re.compile(r"\b(libc|ld-linux|ld\.so|memcpy|memset|memcmp|malloc|free|calloc|clock_gettime|clock|__GI_|__mem|pthread)\b", re.I),
    ]),
]

LINE_RE = re.compile(r"^\s*(?P<pct>[0-9]+(?:\.[0-9]+)?)%\s+(?P<rest>.*)$")
BRACKET_SYMBOL_RE = re.compile(r"\]\s+(?P<sym>\S.*)$")


def extract_symbol(rest: str) -> str:
    m = BRACKET_SYMBOL_RE.search(rest)
    if m:
        return m.group("sym").strip()
    parts = rest.split()
    return parts[-1] if parts else rest.strip()


def classify(symbol: str) -> str:
    for component, patterns in PATTERNS:
        if any(p.search(symbol) for p in patterns):
            return component
    return "Other"


def parse_report(path: Path) -> Tuple[Dict[str, float], List[Tuple[float, str, str]]]:
    totals = {c: 0.0 for c in COMPONENTS}
    rows: List[Tuple[float, str, str]] = []
    for line in path.read_text(errors="ignore").splitlines():
        m = LINE_RE.match(line)
        if not m:
            continue
        pct = float(m.group("pct"))
        rest = m.group("rest")
        symbol = extract_symbol(rest)
        component = classify(symbol)
        totals[component] += pct
        rows.append((pct, component, symbol))
    return totals, rows


def bar(pct: float, width: int = 24) -> str:
    filled = int(round(min(max(pct, 0.0), 100.0) * width / 100.0))
    return "█" * filled + "░" * (width - filled)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", required=True, help="aggregate CSV output")
    ap.add_argument("--markdown", required=True, help="markdown heatmap output")
    ap.add_argument("--symbols", required=True, help="top-symbol CSV output")
    ap.add_argument("reports", nargs="+", help="perf report text files")
    args = ap.parse_args()

    aggregate_rows = []
    symbol_rows = []
    for report in args.reports:
        path = Path(report)
        label = path.stem.replace("perf_report_", "")
        totals, rows = parse_report(path)
        sample_total = sum(totals.values())
        for component in COMPONENTS:
            aggregate_rows.append({
                "profile": label,
                "component": component,
                "sample_percent": f"{totals[component]:.3f}",
            })
        for pct, component, symbol in sorted(rows, reverse=True)[:200]:
            symbol_rows.append({
                "profile": label,
                "component": component,
                "sample_percent": f"{pct:.3f}",
                "symbol": symbol,
            })
        if sample_total <= 0.0:
            aggregate_rows.append({
                "profile": label,
                "component": "STATUS",
                "sample_percent": "NO_SAMPLES_PARSED",
            })

    csv_path = Path(args.csv)
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["profile", "component", "sample_percent"])
        writer.writeheader()
        writer.writerows(aggregate_rows)

    sym_path = Path(args.symbols)
    with sym_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["profile", "component", "sample_percent", "symbol"])
        writer.writeheader()
        writer.writerows(symbol_rows)

    # Markdown heatmap.
    md_path = Path(args.markdown)
    by_profile: Dict[str, Dict[str, float]] = {}
    for row in aggregate_rows:
        if row["component"] == "STATUS":
            continue
        by_profile.setdefault(row["profile"], {})[row["component"]] = float(row["sample_percent"])
    lines = [
        "# HARE component hotspot heatmap",
        "",
        "Percentages are `perf report --no-children` sample percentages for the profiled benchmark process. They are a coarse profiling aid, not a timing side-channel proof.",
        "",
        "| Profile | Component | Samples % | Heat |",
        "|---|---|---:|---|",
    ]
    for profile in sorted(by_profile):
        comps = by_profile[profile]
        for component, pct in sorted(comps.items(), key=lambda kv: kv[1], reverse=True):
            if pct < 0.05:
                continue
            lines.append(f"| `{profile}` | {component} | {pct:.2f} | `{bar(pct)}` |")
    lines.append("")
    lines.append("See `component_hotspots.csv` and `component_hotspot_symbols.csv` for machine-readable data.")
    md_path.write_text("\n".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
