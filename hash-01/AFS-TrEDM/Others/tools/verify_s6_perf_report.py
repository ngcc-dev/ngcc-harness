#!/usr/bin/env python3
"""Verify the generated S6 performance report artifacts."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


REQUIRED_CURRENT_ROWS = (
    ("AFS-TrEDM-S6-512 Reference", "512"),
    ("AFS-TrEDM-S6-512 Optimized opt64", "512"),
    ("AFS-TrEDM-S6-512 Optimized AVX2 hybrid", "512"),
    ("AFS-TrEDM-S6-512 Batch16 AVX512 norm", "512"),
    ("AFS-TrEDM-S6-768 Reference", "768"),
    ("AFS-TrEDM-S6-768 Optimized opt64", "768"),
    ("AFS-TrEDM-S6-768 Optimized AVX2 hybrid", "768"),
    ("AFS-TrEDM-S6-768 Batch16 AVX512 norm", "768"),
    ("AFS-TrEDM-S6-1024 Reference", "1024"),
    ("AFS-TrEDM-S6-1024 Optimized opt64", "1024"),
    ("AFS-TrEDM-S6-1024 Optimized AVX2 hybrid", "1024"),
    ("AFS-TrEDM-S6-1024 Batch16 AVX512 norm", "1024"),
    ("SHA3-512 Reference C (XKCP readable)", "SHA3-512"),
    ("SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)", "SHA3-512"),
    ("SHA3-512 OpenSSL EVP", "SHA3-512"),
)

REQUIRED_CONCLUSIONS = (
    "AFS-TrEDM-S6-512 Batch16 AVX512 norm is faster than SHA3-512 OpenSSL EVP for 1MiB normalized throughput",
    "AFS-TrEDM-S6-512 Optimized AVX2 hybrid single-message remains slower than SHA3-512 OpenSSL EVP at 1MiB",
    "Batch16 AVX512 norm values are normalized over a 16-message batch and must not be described as single-message latency.",
    "SHA3-512 Reference C (XKCP readable): official XKCP readable-and-compact C implementation, reference-style baseline.",
    "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact): official XKCP more-compact CompactFIPS202 implementation, compact baseline.",
    "SHA3-512 OpenSSL EVP is a single-message deployment baseline.",
    "None of these SHA3 baselines is part of AFS-TrEDM's Reference_Implementation or Optimized_Implementation.",
)

FORBIDDEN_ACTIVE_LABELS = tuple(
    "".join(parts)
    for parts in (
        ("S6 BatchMany", " scalar fb"),
        ("Batch16 AVX512", "-label"),
        ("S6 Batch16", " AVX512 true"),
        ("AVX512", "-label"),
        ("AVX512", "-labelled"),
        ("Batch16", "/OpenSSL"),
        ("AVX2", "/OpenSSL"),
        ("Table ", "4."),
        ("Table ", "5."),
        ("Table ", "H1."),
    )
)


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def read_csv_rows(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def as_float(row: dict[str, str], key: str) -> float:
    try:
        return float(row[key])
    except (KeyError, ValueError) as exc:
        raise SystemExit(f"FAIL: invalid numeric field {key!r} in {row.get('row')} {row.get('instance')}") from exc


def row(rows: list[dict[str, str]], name: str, instance: str = "512") -> dict[str, str]:
    for item in rows:
        if item.get("row") == name and item.get("instance") == instance:
            return item
    fail(f"missing row: {name} {instance}")


def require_numeric(item: dict[str, str], key: str) -> float:
    value = item.get(key, "NA")
    if value in ("", "NA"):
        fail(f"required numeric field is NA: {item.get('row')} {item.get('instance')} {key}")
    return as_float(item, key)


def require_source_exists(root: Path, source: str, row_name: str) -> None:
    if not source or source == "NA":
        fail(f"missing source log for {row_name}")
    path = Path(source)
    if not path.is_absolute():
        path = root / path
    if not path.is_file():
        fail(f"source log does not exist for {row_name}: {source}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--csv", default="docs/perf/AFS-TrEDM-S6_Performance_Report.csv")
    parser.add_argument("--markdown", default="docs/perf/AFS-TrEDM-S6_Performance_Report.md")
    parser.add_argument("--terminal", default="docs/perf/live/latest/terminal_output.txt")
    parser.add_argument("--live-raw", default="docs/perf/live/latest/live_perf_raw.csv")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    csv_path = root / args.csv
    md_path = root / args.markdown
    terminal_path = root / args.terminal
    live_raw_path = root / args.live_raw

    for path in (csv_path, md_path, terminal_path, live_raw_path):
        if not path.exists():
            fail(f"missing artifact: {path.relative_to(root)}")

    rows = read_csv_rows(csv_path)
    md = md_path.read_text(encoding="utf-8")
    terminal = terminal_path.read_text(encoding="utf-8")
    report_csv_text = csv_path.read_text(encoding="utf-8")

    for name, instance in REQUIRED_CURRENT_ROWS:
        item = row(rows, name, instance)
        if item["historical_or_current"] != "current":
            fail(f"current row is not marked current: {name} {instance}")
        notes = item.get("notes", "")
        if "fresh Stage16 live" not in notes and not ("fresh Stage" in notes and "live" in notes):
            fail(f"current row is not marked as fresh Stage16 live evidence: {name} {instance}")
        for key in (
            "msg64_cycles_per_byte",
            "msg64_MiB_per_second",
            "msg64_cycles_per_hash",
            "msg1MiB_cycles_per_byte",
            "msg1MiB_MiB_per_second",
            "msg1MiB_cycles_per_hash",
        ):
            require_numeric(item, key)
        require_source_exists(root, item["source_log"], f"{name} {instance}")

    if any(item.get("historical_or_current") == "historical" for item in rows):
        fail("performance report CSV must remain current-only")

    for item in rows:
        name = item["row"]
        source = item["source_log"]
        if name not in md:
            fail(f"Markdown missing CSV row label: {name}")
        if item.get("historical_or_current") == "current" and source not in md:
            fail(f"Markdown missing CSV source log: {source}")

    for text in REQUIRED_CONCLUSIONS:
        if text not in md:
            fail(f"missing required conclusion: {text}")

    for text in FORBIDDEN_ACTIVE_LABELS:
        if text in md or text in terminal or text in report_csv_text:
            fail(f"forbidden active label remains: {text}")

    terminal_titles = [
        line
        for line in terminal.splitlines()
        if line
        in (
            "Cycles per Byte Comparison (cpb)",
            "Cycles per Hash Comparison",
            "Throughput Comparison (MB/s)",
            "Latency Comparison (milliseconds)",
        )
    ]
    if terminal_titles != [
        "Cycles per Byte Comparison (cpb)",
        "Cycles per Hash Comparison",
        "Throughput Comparison (MB/s)",
        "Latency Comparison (milliseconds)",
    ]:
        fail(f"terminal does not have the Stage16 four-table title order: {terminal_titles}")

    for inst in ("512", "768", "1024"):
        item = row(rows, f"AFS-TrEDM-S6-{inst} Batch16 AVX512 norm", inst)
        if item["backend_identity"] != "avx512_batch16_true":
            fail(f"Batch16 backend identity is wrong for {inst}")
        if item["metric_class"] != "multi-buffer normalized throughput":
            fail(f"Batch16 metric class is wrong for {inst}")
        if int(item["hot_path_messages"]) <= 0 or int(item["hot_path_blocks"]) <= 0:
            fail(f"Batch16 hot counters are not positive for {inst}")
        if int(item["fallback_messages"]) != 0 or int(item["fallback_blocks"]) != 0:
            fail(f"Batch16 fallback counters are nonzero for {inst}")

    for name in ("SHA3-512 Reference C (XKCP readable)", "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)", "SHA3-512 OpenSSL EVP"):
        item = row(rows, name, "SHA3-512")
        if item["metric_class"] != "single-message latency/throughput":
            fail(f"SHA3 baseline metric class is wrong: {name}")

    batch16_512 = as_float(row(rows, "AFS-TrEDM-S6-512 Batch16 AVX512 norm"), "msg1MiB_cycles_per_byte")
    avx2_512 = as_float(row(rows, "AFS-TrEDM-S6-512 Optimized AVX2 hybrid"), "msg1MiB_cycles_per_byte")
    openssl = as_float(row(rows, "SHA3-512 OpenSSL EVP", "SHA3-512"), "msg1MiB_cycles_per_byte")
    if not batch16_512 < openssl:
        fail("Batch16 norm 512 does not beat SHA3-512 OpenSSL EVP in the CSV")
    if not avx2_512 > openssl:
        fail("AVX2 hybrid 512 is not slower than SHA3-512 OpenSSL EVP in the CSV")

    live_rows = read_csv_rows(live_raw_path)
    if not live_rows:
        fail("live_perf_raw.csv is empty")
    if {item.get("source_kind") for item in live_rows} != {"fresh"}:
        fail("live_perf_raw.csv contains non-fresh rows")
    live_backends = {item.get("row") for item in live_rows}
    for required in ("ref", "opt64", "avx2", "batch16", "ref-sha3", "compact-sha3", "openssl-sha3"):
        if required not in live_backends:
            fail(f"live_perf_raw.csv missing fresh backend: {required}")

    if "Historical " in md:
        fail("Markdown contains a historical comparison section")

    print("PASS: Stage16 S6 performance report artifacts are internally consistent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
