#!/usr/bin/env python3
"""Validate Stage16 live performance metadata."""

from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter
from pathlib import Path


REQUIRED_SIZES = {"64", "192", "1024", "1536", "65536", "1048576"}
REQUIRED_FRESH_FIELDS = [
    "run_id",
    "source_kind",
    "algorithm",
    "implementation",
    "msg_bytes",
    "message_bytes",
    "command",
    "binary_path",
    "binary_sha256",
    "source_path",
    "started_at_utc",
    "ended_at_utc",
    "host",
    "cpu_model",
]
SHA3_ROWS = {"ref-sha3", "compact-sha3", "openssl-sha3"}


def fail(message: str) -> int:
    print(f"FAIL: {message}", file=sys.stderr)
    return 1


def missing(value: object) -> bool:
    return value is None or value == "" or value == "NA"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--live-dir", default="docs/perf/live/latest")
    args = parser.parse_args(argv)

    root = Path(args.root).resolve()
    live_dir = root / args.live_dir
    raw_path = live_dir / "live_perf_raw.csv"
    metadata_path = live_dir / "run_metadata.json"
    historical_path = live_dir / "live_perf_historical.csv"
    if not raw_path.is_file():
        return fail(f"missing raw CSV: {raw_path}")
    if not metadata_path.is_file():
        return fail(f"missing run metadata: {metadata_path}")

    rows = list(csv.DictReader(raw_path.open(encoding="utf-8", newline="")))
    if not rows:
        return fail("raw CSV has no rows")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    if metadata.get("fresh_run_policy", "").find("built and executed benchmark programs") < 0:
        return fail("metadata does not state fresh-run policy")
    if metadata.get("terminal_output_mode") != "stage16-four-main-tables":
        return fail(f"unexpected terminal output mode: {metadata.get('terminal_output_mode')}")
    if missing(metadata.get("cpu_model")):
        return fail("metadata missing cpu_model")

    counts = Counter(row.get("source_kind") for row in rows)
    if counts != Counter({"fresh": len(rows)}):
        return fail(f"fresh raw CSV contains non-fresh rows: {dict(counts)}")

    batch16_count = 0
    for row in rows:
        for field in REQUIRED_FRESH_FIELDS:
            if missing(row.get(field)):
                return fail(f"fresh row missing {field}: {row.get('row')} {row.get('msg_bytes')}")
        if row.get("msg_bytes") not in REQUIRED_SIZES:
            return fail(f"unexpected message size in raw CSV: {row.get('msg_bytes')}")
        if row.get("message_bytes") != row.get("msg_bytes"):
            return fail(f"message_bytes/msg_bytes mismatch: {row.get('row')} {row.get('message_bytes')} != {row.get('msg_bytes')}")
        if row.get("source_kind") == "fresh" and missing(row.get("binary_sha256")):
            return fail(f"fresh row missing binary SHA-256: {row.get('row')}")
        if row.get("row") in SHA3_ROWS:
            for field in ("source_origin", "upstream_source", "source_file", "source_sha256", "kat_status", "kat_log"):
                if missing(row.get(field)):
                    return fail(f"SHA3 row missing {field}: {row.get('row')} {row.get('msg_bytes')}")
            if row.get("kat_status") != "pass":
                return fail(f"SHA3 row KAT did not pass: {row.get('row')}")
        if row.get("row") == "batch16":
            batch16_count += 1
            if row.get("multi_buffer_normalized") != "true":
                return fail("Batch16 row lacks multi_buffer_normalized=true")
            if row.get("metric_class") != "multi-buffer normalized throughput":
                return fail("Batch16 row metric_class is not multi-buffer normalized throughput")
            if row.get("fallback_messages") not in {"0", "0.0"}:
                return fail(f"Batch16 row has fallback messages: {row.get('fallback_messages')}")
    if batch16_count == 0:
        return fail("no Batch16 rows found")

    backend_rows = metadata.get("main_row_metadata")
    if not isinstance(backend_rows, list) or len(backend_rows) < 15:
        return fail("run_metadata main_row_metadata missing expected backend entries")
    for item in backend_rows:
        if item.get("row") == "batch16" and item.get("multi_buffer_normalized") != "true":
            return fail("Batch16 backend metadata lacks multi_buffer_normalized=true")

    if historical_path.exists():
        historical_rows = list(csv.DictReader(historical_path.open(encoding="utf-8", newline="")))
        for row in historical_rows:
            if row.get("source_kind") == "fresh":
                return fail("historical CSV contains source_kind=fresh")
            if row.get("source_kind") == "historical" and "historical" not in row.get("source", "").lower():
                return fail(f"historical row lacks historical label: {row}")

    print(f"PASS: Stage16 live metadata validated in {live_dir.relative_to(root)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
