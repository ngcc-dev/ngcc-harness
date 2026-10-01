#!/usr/bin/env python3
"""Check the published performance evidence and the reports generated from it.

    python3 performance/validate.py          (also: make -C performance check)

For the source catalog: every submission is covered in order, with the archive
digest recorded in SOURCE_ARCHIVES.md. For every published campaign dataset
performance/data/<system>/ (see performance/systems.csv):

  - the system is registered and matches campaign.json;
  - no file contains a local absolute path;
  - every KAT log exists, matches the digest in build.json and ends with the
    recorded result;
  - every timing record belongs to a built instance and its statistics
    (measurement count, mean time and cycles, median trial, throughput) are
    recomputed from the raw trials;
  - every hash profile's shares are recomputed from its call records;
  - every measured instance has one explicit comparison-target row;
  - performance/report.py --check regenerates every report of that system and
    finds them byte-for-byte identical to the committed pages.
"""

from __future__ import annotations

import csv
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import sys

from import_sources import expected_sha256


ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "performance/data"
ABSOLUTE = re.compile(r"(?<![\w.])/(home|tmp|root|Users)/")


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


def systems() -> dict:
    with (ROOT / "performance/systems.csv").open(encoding="utf-8") as f:
        return {r["ID"]: r for r in csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";")}


def check_security_targets(datasets: list[Path]) -> list[str]:
    path = ROOT / "performance/security_targets.csv"
    problems = []
    with path.open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source, delimiter=";")
        if tuple(reader.fieldnames or ()) != ("ID", "Instance", "TargetBits"):
            return ["performance/security_targets.csv: invalid header"]
        rows = list(reader)
    found = set()
    for line_number, row in enumerate(rows, 2):
        key = row["ID"], row["Instance"]
        if key in found:
            problems.append(f"performance/security_targets.csv:{line_number}: duplicate {key[0]}/{key[1]}")
        found.add(key)
        if row["TargetBits"] not in ("", "128", "256", "512"):
            problems.append(
                f"performance/security_targets.csv:{line_number}: invalid target {row['TargetBits']!r}")
    expected = set()
    for dataset in datasets:
        build = json.loads((dataset / "build.json").read_text(encoding="utf-8"))["instances"]
        for entry in build.values():
            if entry["candidate"] != "iccs":
                expected.add((entry["candidate"], entry["label"]))
    for candidate, label in sorted(expected - found):
        problems.append(f"performance/security_targets.csv: missing {candidate}/{label}")
    for candidate, label in sorted(found - expected):
        problems.append(f"performance/security_targets.csv: stale {candidate}/{label}")
    return problems


def close(a, b) -> bool:
    return (a is None and b is None) or (a is not None and b is not None and math.isclose(a, b, rel_tol=1e-9))


def check_record(name: str, r: dict, build: dict) -> list[str]:
    problems = []
    key = f"{r.get('candidate')}/{r.get('label')}"
    entry = build.get(key)
    if not entry:
        return [f"{name}: {key} is not in build.json"]
    if r.get("library") != entry.get("library") or r.get("kat_log") != entry.get("kat_log"):
        problems.append(f"{name}: library or KAT log differs from build.json")
    if r.get("status") != "complete":
        return problems
    trials = r.get("trials", [])
    if len(trials) != r["plan"]["trials"]:
        problems.append(f"{name}: {len(trials)} trials, plan says {r['plan']['trials']}")
    n = sum(t["measurements"] for t in trials)
    secs = sum(t["seconds"] for t in trials)
    per = [t["seconds"] / t["measurements"] for t in trials]
    cycles_ok = bool(trials) and all(t["cycles"] > 0 for t in trials)
    expect = {
        "measurements": n,
        "mean_seconds": secs / n if n else None,
        "median_trial_seconds": statistics.median(per) if per else None,
        "mean_cycles": sum(t["cycles"] for t in trials) / n if cycles_ok and n else None,
        "operations_per_second": n / secs if secs else None,
    }
    for field, value in expect.items():
        if field == "measurements":
            if r.get(field) != value:
                problems.append(f"{name}: measurements {r.get(field)} != sum of trials {value}")
        elif not close(r.get(field), value):
            problems.append(f"{name}: {field} {r.get(field)} does not follow from the trials ({value})")
    if r.get("guide_100_measurements_met") != (n >= 100):
        problems.append(f"{name}: guide_100_measurements_met inconsistent with {n} measurements")
    if r.get("input_bytes") and not close(r.get("throughput_MB_per_second"), r["input_bytes"] * n / secs / 1e6):
        problems.append(f"{name}: throughput does not follow from the trials")
    if not r.get("environment"):
        problems.append(f"{name}: no environment recorded")
    return problems


def check_profile(name: str, p: dict) -> list[str]:
    if p.get("status") != "complete" or not p.get("total"):
        return []
    total = p["total"]["ticks"]
    by_fn = {}
    for c in p.get("calls", []):
        by_fn[c["fn"]] = by_fn.get(c["fn"], 0) + c["ticks"]
    problems = []
    for fn, ticks in by_fn.items():
        if not close(p.get("share", {}).get(fn), ticks / total if total else None):
            problems.append(f"{name}: share of {fn} does not follow from its calls")
    hash_share = sum(t for fn, t in by_fn.items() if fn != "drng") / total if total else None
    if by_fn and not close(p.get("hash_share"), hash_share):
        problems.append(f"{name}: hash_share does not follow from its calls")
    return problems


def check_dataset(ds: Path, known: dict) -> tuple[list[str], dict]:
    problems, counts = [], {"records": 0, "profiles": 0, "kat_logs": 0}
    sid = ds.name
    campaign = json.loads((ds / "campaign.json").read_text(encoding="utf-8"))
    if sid not in known:
        problems.append(f"{sid}: not registered in performance/systems.csv")
    if campaign.get("system_id") != sid:
        problems.append(f"{sid}: campaign.json system_id is {campaign.get('system_id')!r}")
    for f in sorted(ds.rglob("*")):
        if f.is_file() and ABSOLUTE.search(f.read_text(encoding="utf-8", errors="replace")):
            problems.append(f"{f.relative_to(ROOT)}: contains a local absolute path")
    build = json.loads((ds / "build.json").read_text(encoding="utf-8"))["instances"]
    for key, e in sorted(build.items()):
        if not e.get("kat_log"):
            continue
        log = ds / e["kat_log"]
        if not log.is_file():
            problems.append(f"{sid}: {key} KAT log {e['kat_log']} missing")
            continue
        counts["kat_logs"] += 1
        if hashlib.sha256(log.read_bytes()).hexdigest() != e.get("kat_log_sha256"):
            problems.append(f"{sid}: {e['kat_log']} does not match its digest in build.json")
        results = re.findall(r"^RESULT \S+ \S+ (\S+)", log.read_text(encoding="utf-8", errors="replace"), re.M)
        if not results or results[-1] != e.get("kat"):
            problems.append(f"{sid}: {e['kat_log']} result {results[-1:] or '-'} != build.json {e.get('kat')}")
    for f in sorted((ds / "records").rglob("*.json")):
        counts["records"] += 1
        problems += check_record(f.relative_to(ds).as_posix(), json.loads(f.read_text(encoding="utf-8")), build)
    for f in sorted((ds / "profile").rglob("*.json")):
        counts["profiles"] += 1
        problems += check_profile(f.relative_to(ds).as_posix(), json.loads(f.read_text(encoding="utf-8")))
    check = subprocess.run([sys.executable, str(ROOT / "performance/report.py"), str(ds), "--system", sid, "--check"],
                           capture_output=True, text=True)
    if check.returncode:
        problems += [line for line in check.stdout.splitlines() if line.startswith("differs")] or \
                    [f"{sid}: report check failed: {check.stderr.strip()[-300:]}"]
    counts["report_check"] = check.stdout.strip().splitlines()[-1] if check.stdout.strip() else "failed"
    return problems, counts


def main() -> int:
    problems = check_source_catalog()
    print(f"performance source catalog: 119 submissions, {len(problems)} problem(s)")
    known = systems()
    datasets = sorted(d for d in DATA.iterdir() if (d / "campaign.json").is_file()) if DATA.is_dir() else []
    target_problems = check_security_targets(datasets)
    for problem in target_problems:
        print(problem)
    print(f"performance security targets: {len(target_problems)} problem(s)")
    problems += target_problems
    for ds in datasets:
        found, counts = check_dataset(ds, known)
        for problem in found:
            print(problem)
        print(f"performance data {ds.name}: {counts['records']} timing record(s), {counts['profiles']} "
              f"profile(s), {counts['kat_logs']} KAT log(s), {len(found)} problem(s); {counts['report_check']}")
        problems += found
    stray = sorted(p.name for p in DATA.glob("*.json")) if DATA.is_dir() else []
    if stray:
        problems.append(f"performance/data has files outside a system dataset: {', '.join(stray)}")
        print(problems[-1])
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
