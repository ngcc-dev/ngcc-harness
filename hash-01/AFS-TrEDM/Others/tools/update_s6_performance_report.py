#!/usr/bin/env python3
"""Render the S6 performance report from the latest live benchmark rows."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


SIZES = ("64B", "1MB")
INSTANCES = ("512", "768", "1024")
AFS_IMPLEMENTATIONS = (
    ("Reference", "Reference"),
    ("Optimized opt64", "Optimized opt64"),
    ("Optimized AVX2 hybrid", "Optimized AVX2 hybrid"),
    ("Batch16 AVX512 true", "Batch16 AVX512 norm"),
)
SHA3_ROWS = (
    ("SHA3-512 Reference C (XKCP readable)", "SHA3-512 Reference C (XKCP readable)"),
    ("SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)", "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)"),
    ("SHA3-512 OpenSSL EVP", "SHA3-512 OpenSSL EVP"),
)
CSV_COLUMNS = (
    "row",
    "instance",
    "backend_identity",
    "metric_class",
    "msg64_cycles_per_byte",
    "msg64_MiB_per_second",
    "msg64_cycles_per_hash",
    "msg1MiB_cycles_per_byte",
    "msg1MiB_MiB_per_second",
    "msg1MiB_cycles_per_hash",
    "hot_path_messages",
    "fallback_messages",
    "hot_path_blocks",
    "fallback_blocks",
    "source_log",
    "historical_or_current",
    "notes",
)


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def fnum(value: str, digits: int = 6) -> str:
    if value in ("", "NA"):
        return "NA"
    return f"{float(value):.{digits}f}"


def relpath(root: Path, value: str) -> str:
    if not value or value == "NA":
        return "NA"
    path = Path(value)
    if path.is_absolute():
        try:
            return str(path.relative_to(root))
        except ValueError:
            return str(path)
    return str(path)


def find_row(rows: list[dict[str, str]], algorithm: str, implementation: str, size: str) -> dict[str, str]:
    matches = [
        row
        for row in rows
        if row.get("algorithm") == algorithm
        and row.get("implementation") == implementation
        and row.get("message_label") == size
    ]
    if len(matches) != 1:
        raise SystemExit(
            f"FAIL: expected one live row for algorithm={algorithm!r} implementation={implementation!r} size={size!r}, got {len(matches)}"
        )
    return matches[0]


def report_label(algorithm: str, display_impl: str) -> str:
    if algorithm.startswith("AFS-TrEDM-S6-"):
        return f"{algorithm} {display_impl}"
    return display_impl


def retained_live_source(row: dict[str, str]) -> str:
    return "docs/perf/live/latest/live_perf_raw.csv"


def build_report_rows(root: Path, raw_rows: list[dict[str, str]]) -> list[dict[str, str]]:
    out: list[dict[str, str]] = []
    for inst in INSTANCES:
        algorithm = f"AFS-TrEDM-S6-{inst}"
        for raw_impl, display_impl in AFS_IMPLEMENTATIONS:
            r64 = find_row(raw_rows, algorithm, raw_impl, "64B")
            r1m = find_row(raw_rows, algorithm, raw_impl, "1MB")
            is_batch = display_impl == "Batch16 AVX512 norm"
            out.append(
                {
                    "row": report_label(algorithm, display_impl),
                    "instance": inst,
                    "backend_identity": r1m["backend"],
                    "metric_class": r1m["metric_class"],
                    "msg64_cycles_per_byte": fnum(r64["cycles_per_byte"]),
                    "msg64_MiB_per_second": fnum(r64["throughput_MiBps"]),
                    "msg64_cycles_per_hash": fnum(r64["cycles_per_hash"]),
                    "msg1MiB_cycles_per_byte": fnum(r1m["cycles_per_byte"]),
                    "msg1MiB_MiB_per_second": fnum(r1m["throughput_MiBps"]),
                    "msg1MiB_cycles_per_hash": fnum(r1m["cycles_per_hash"]),
                    "hot_path_messages": r1m.get("hot_path_messages", "NA") or "NA",
                    "fallback_messages": r1m.get("fallback_messages", "NA") or "NA",
                    "hot_path_blocks": r1m.get("hot_path_blocks", "NA") or "NA",
                    "fallback_blocks": r1m.get("fallback_blocks", "NA") or "NA",
                    "source_log": retained_live_source(r1m),
                    "historical_or_current": "current",
                    "notes": (
                        "fresh Stage16 live benchmark row; cpb is normalized multi-buffer throughput, not single-message latency"
                        if is_batch
                        else "fresh Stage16 live benchmark row"
                    ),
                }
            )
    for raw_impl, display_impl in SHA3_ROWS:
        r64 = find_row(raw_rows, "SHA3-512", raw_impl, "64B")
        r1m = find_row(raw_rows, "SHA3-512", raw_impl, "1MB")
        out.append(
            {
                "row": display_impl,
                "instance": "SHA3-512",
                "backend_identity": r1m["backend"],
                "metric_class": r1m["metric_class"],
                "msg64_cycles_per_byte": fnum(r64["cycles_per_byte"]),
                "msg64_MiB_per_second": fnum(r64["throughput_MiBps"]),
                "msg64_cycles_per_hash": fnum(r64["cycles_per_hash"]),
                "msg1MiB_cycles_per_byte": fnum(r1m["cycles_per_byte"]),
                "msg1MiB_MiB_per_second": fnum(r1m["throughput_MiBps"]),
                "msg1MiB_cycles_per_hash": fnum(r1m["cycles_per_hash"]),
                "hot_path_messages": "NA",
                "fallback_messages": "NA",
                "hot_path_blocks": "NA",
                "fallback_blocks": "NA",
                "source_log": retained_live_source(r1m),
                "historical_or_current": "current",
                "notes": "fresh Stage16 live SHA3-512 single-message baseline",
            }
        )
    return out


def write_report_csv(path: Path, rows: list[dict[str, str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_COLUMNS)
        writer.writeheader()
        writer.writerows(rows)


def row_by_name(rows: list[dict[str, str]], name: str) -> dict[str, str]:
    for row in rows:
        if row["row"] == name:
            return row
    raise SystemExit(f"FAIL: missing report row {name!r}")


def md_table(headers: list[str], rows: list[list[str]]) -> str:
    lines = ["| " + " | ".join(headers) + " |", "| " + " | ".join(["---"] * len(headers)) + " |"]
    for row in rows:
        lines.append("| " + " | ".join(row) + " |")
    return "\n".join(lines)


def render_markdown(
    *,
    report_rows: list[dict[str, str]],
    metadata: dict[str, object],
    summary_csv: str,
    terminal_output: str,
) -> str:
    run_id = str(metadata.get("run_id", "NA"))
    timestamp = str(metadata.get("timestamp", "NA"))
    mode = str(metadata.get("mode", "NA"))
    compiler = str(metadata.get("compiler", "NA"))
    openssl = str(metadata.get("openssl_version", "NA"))
    cpu = str(metadata.get("cpu_model", "NA"))
    governor = str(metadata.get("governor", "NA"))

    comparison_rows = [
        [
            row["row"],
            row["instance"],
            f"`{row['backend_identity']}`",
            row["metric_class"],
            row["msg64_cycles_per_byte"],
            row["msg64_MiB_per_second"],
            row["msg64_cycles_per_hash"],
            row["msg1MiB_cycles_per_byte"],
            row["msg1MiB_MiB_per_second"],
            row["msg1MiB_cycles_per_hash"],
            f"`{row['source_log']}`",
        ]
        for row in report_rows
    ]

    r_batch = row_by_name(report_rows, "AFS-TrEDM-S6-512 Batch16 AVX512 norm")
    r_avx2 = row_by_name(report_rows, "AFS-TrEDM-S6-512 Optimized AVX2 hybrid")
    r_refc = row_by_name(report_rows, "SHA3-512 Reference C (XKCP readable)")
    r_xkcp = row_by_name(report_rows, "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)")
    r_ossl = row_by_name(report_rows, "SHA3-512 OpenSSL EVP")
    batch_cpb = float(r_batch["msg1MiB_cycles_per_byte"])
    avx2_cpb = float(r_avx2["msg1MiB_cycles_per_byte"])
    xkcp_cpb = float(r_xkcp["msg1MiB_cycles_per_byte"])
    ossl_cpb = float(r_ossl["msg1MiB_cycles_per_byte"])

    if batch_cpb < ossl_cpb:
        batch_claim = (
            "AFS-TrEDM-S6-512 Batch16 AVX512 norm is faster than SHA3-512 OpenSSL EVP "
            f"for 1MiB normalized throughput: {batch_cpb:.2f} cpb vs {ossl_cpb:.2f} cpb."
        )
    else:
        batch_claim = (
            "AFS-TrEDM-S6-512 Batch16 AVX512 norm is not faster than SHA3-512 OpenSSL EVP "
            f"in this 1MiB run: {batch_cpb:.2f} cpb vs {ossl_cpb:.2f} cpb."
        )
    if avx2_cpb > ossl_cpb:
        avx2_claim = (
            "AFS-TrEDM-S6-512 Optimized AVX2 hybrid single-message remains slower than "
            f"SHA3-512 OpenSSL EVP at 1MiB: {avx2_cpb:.2f} cpb vs {ossl_cpb:.2f} cpb."
        )
    else:
        avx2_claim = (
            "AFS-TrEDM-S6-512 Optimized AVX2 hybrid single-message is faster than "
            f"SHA3-512 OpenSSL EVP in this 1MiB run: {avx2_cpb:.2f} cpb vs {ossl_cpb:.2f} cpb."
        )
    if avx2_cpb < xkcp_cpb:
        xkcp_claim = (
            "AFS-TrEDM-S6-512 Optimized AVX2 hybrid is faster than SHA3-512 XKCP CompactFIPS202 (XKCP more-compact) "
            f"at 1MiB in this run: {avx2_cpb:.2f} cpb vs {xkcp_cpb:.2f} cpb."
        )
    else:
        xkcp_claim = (
            "AFS-TrEDM-S6-512 Optimized AVX2 hybrid is not faster than SHA3-512 XKCP CompactFIPS202 (XKCP more-compact) "
            f"at 1MiB in this run: {avx2_cpb:.2f} cpb vs {xkcp_cpb:.2f} cpb."
        )

    return "\n".join(
        [
            "# AFS-TrEDM-S6 Performance Report",
            "",
            "This report is generated from the latest Stage16 live benchmark evidence.",
            "",
            "## Live Run",
            "",
            f"- run_id: `{run_id}`",
            f"- timestamp: `{timestamp}`",
            f"- mode: `{mode}`",
            f"- compiler: `{compiler}`",
            f"- OpenSSL: `{openssl}`",
            f"- CPU: `{cpu}`",
            f"- governor: `{governor}`",
            f"- raw CSV: `docs/perf/live/latest/live_perf_raw.csv`",
            f"- summary CSV: `{summary_csv}`",
            f"- terminal output: `{terminal_output}`",
            "",
            "## Measurement Scope",
            "",
            "- AFS-TrEDM-S6 Batch16 AVX512 norm is a multi-buffer normalized throughput backend. Its latency, cycles/hash, and cpb are normalized per message across a 16-message batch and must not be interpreted as single-message latency.",
            "- AFS-TrEDM-S6 Optimized AVX2 hybrid is a single-message backend using an AVX2 AFS-64 nonlinear layer plus generated opt64 S6 linear layer.",
            "",
            "## SHA3-512 baseline taxonomy",
            "",
            "- SHA3-512 Reference C (XKCP readable): official XKCP readable-and-compact C implementation, reference-style baseline.",
            "- SHA3-512 XKCP CompactFIPS202 (XKCP more-compact): official XKCP more-compact CompactFIPS202 implementation, compact baseline.",
            "- SHA3-512 OpenSSL EVP is a single-message deployment baseline.",
            "- SHA3-512 Reference C and XKCP CompactFIPS202 are external baselines, not AFS-TrEDM implementations.",
            "- None of these SHA3 baselines is part of AFS-TrEDM's Reference_Implementation or Optimized_Implementation.",
            "- Close SHA3 baseline performance is acceptable only because source provenance and anti-alias checks are recorded.",
            "- No true SHA3-512 batch baseline is claimed in this package unless a separate SHA3 batch backend is implemented and labeled.",
            "",
            "## Main Comparison",
            "",
            md_table(
                [
                    "Backend",
                    "Instance",
                    "Backend identity",
                    "Metric class",
                    "64B cpb",
                    "64B MiB/s",
                    "64B cycles/hash",
                    "1MiB cpb",
                    "1MiB MiB/s",
                    "1MiB cycles/hash",
                    "Source log",
                ],
                comparison_rows,
            ),
            "",
            "## Required Conclusions",
            "",
            f"- {batch_claim}",
            f"- {avx2_claim}",
            f"- {xkcp_claim}",
            "- Batch16 AVX512 norm values are normalized over a 16-message batch and must not be described as single-message latency.",
            f"- SHA3-512 Reference C (XKCP readable) was freshly executed in this run: {float(r_refc['msg1MiB_cycles_per_byte']):.2f} cpb at 1MiB.",
            "- SHA3-512 Reference C (XKCP readable), SHA3-512 XKCP CompactFIPS202 (XKCP more-compact), and SHA3-512 OpenSSL EVP rows were freshly executed in the current live run.",
            "",
            "## Detailed Tables",
            "",
            "The full detailed Throughput, Latency, Cycles per Hash, and Cycles per Byte tables for each requested instance are in:",
            "",
            "- `docs/perf/live/latest/AFS-TrEDM-S6_Live_Performance_Tables.md`",
            "- `docs/perf/live/latest/AFS-TrEDM-S6_Internal_Performance_Tables.md`",
            "- `docs/perf/live/latest/live_perf_summary.csv`",
            "- `docs/perf/live/latest/live_perf_raw.csv`",
            "",
            "The terminal output intentionally uses the four Stage16 main table titles and avoids legacy numbered table labels.",
            "",
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    latest = root / "docs/perf/live/latest"
    raw_path = latest / "live_perf_raw.csv"
    metadata_path = latest / "run_metadata.json"
    if not raw_path.is_file():
        raise SystemExit(f"FAIL: missing {raw_path}")
    if not metadata_path.is_file():
        raise SystemExit(f"FAIL: missing {metadata_path}")
    raw_rows = read_csv(raw_path)
    modes = {row.get("mode", "") for row in raw_rows}
    if len(modes) != 1 or "" in modes:
        raise SystemExit(f"FAIL: performance report requires a single populated latest live mode, got {sorted(modes)}")
    if any(row.get("source_kind") != "fresh" for row in raw_rows):
        raise SystemExit("FAIL: performance report raw rows must all be fresh")

    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    report_rows = build_report_rows(root, raw_rows)
    report_csv = root / "docs/perf/AFS-TrEDM-S6_Performance_Report.csv"
    report_md = root / "docs/perf/AFS-TrEDM-S6_Performance_Report.md"
    write_report_csv(report_csv, report_rows)
    report_md.write_text(
        render_markdown(
            report_rows=report_rows,
            metadata=metadata,
            summary_csv="docs/perf/live/latest/live_perf_summary.csv",
            terminal_output="docs/perf/live/latest/terminal_output.txt",
        ),
        encoding="utf-8",
    )
    print(f"updated {report_csv.relative_to(root)}")
    print(f"updated {report_md.relative_to(root)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
