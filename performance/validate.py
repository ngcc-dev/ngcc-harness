#!/usr/bin/env python3
"""Check published performance JSON arithmetic, provenance and limitations."""

from __future__ import annotations

import csv
import json
import math
from pathlib import Path
import statistics
import sys

from import_sources import expected_sha256


ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "performance/data"


def check_source_catalog() -> list[str]:
    problems = []
    with (ROOT / "downloads.csv").open(newline="", encoding="utf-8") as source:
        expected_ids = [row["ID"] for row in csv.DictReader(source, delimiter=";")]
    with (ROOT / "performance/source_catalog.csv").open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    if [row["id"] for row in rows] != expected_ids:
        problems.append("source catalog does not cover downloads.csv in submission order")
    for row in rows:
        if row["archive_sha256"] != expected_sha256(row["id"]):
            problems.append(f"{row['id']}: source catalog archive digest differs from provenance")
        if int(row["source_files"]) < 1 or int(row["source_bytes"]) < 1:
            problems.append(f"{row['id']}: no software source recorded")
    return problems


def check(path: Path) -> list[str]:
    problems = []
    raw = path.read_text(encoding="utf-8")
    record = json.loads(raw)
    if record.get("schema") != 1 or not record.get("rows"):
        return [f"{path.name}: missing schema or rows"]
    candidate = record.get("candidate", "")
    if record.get("source_archive_sha256") != expected_sha256(candidate):
        problems.append(f"{path.name}: archive digest differs from SOURCE_ARCHIVES.md")
    if record.get("guide_complete") and record.get("guide_gaps"):
        problems.append(f"{path.name}: complete despite listed guide gaps")
    if "/home/" in raw or "ngcc1" in raw:
        problems.append(f"{path.name}: local/private path leaked")
    for row in record["rows"]:
        trials = row.get("trials", [])
        if len(trials) != 5 or row.get("status") != "complete":
            problems.append(f"{path.name} {row.get('operation')}: incomplete trials")
            continue
        counts = [trial["measurements"] for trial in trials]
        seconds = [trial["elapsed_seconds"] for trial in trials]
        total = sum(counts)
        if total != row.get("measurements") or total < 100:
            problems.append(f"{path.name} {row['operation']}: measurement count mismatch or <100")
        mean = sum(seconds) / total
        middle = statistics.median(sec / n for sec, n in zip(seconds, counts))
        if not math.isclose(mean, row.get("mean_seconds_per_operation", 0), rel_tol=1e-8):
            problems.append(f"{path.name} {row['operation']}: arithmetic mean mismatch")
        if not math.isclose(middle, row.get("middle_trial_seconds_per_operation", 0), rel_tol=1e-8):
            problems.append(f"{path.name} {row['operation']}: middle trial mismatch")
        if row["metadata"].get("cpu_cycles_available") == "no" and row.get("mean_cpu_cycles") is not None:
            problems.append(f"{path.name} {row['operation']}: cycles given without a counter")
        if row.get("limit_seconds", 0) > 60:
            problems.append(f"{path.name} {row['operation']}: pilot budget exceeds 60 seconds")
    return problems


def main() -> int:
    files = sorted(DATA.glob("*.json"))
    problems = check_source_catalog()
    problems.extend(problem for path in files for problem in check(path))
    for problem in problems:
        print(problem)
    print(f"performance source catalog: 119 submissions; data: {len(files)} record(s), "
          f"{sum(len(json.loads(p.read_text())['rows']) for p in files)} operation(s), "
          f"{len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
