#!/usr/bin/env python3
"""KAT-gated, short operation smoke test; writes no timing data."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import subprocess

from run import ROOT, parse_native


OPERATIONS = {
    "kem": (("keygen", 0), ("enc", 0), ("dec", 0)),
    "sign": (("keygen", 0), ("sign", 0), ("verify", 0)),
    "hash": (("hash", 32), ("hash", 65536)),
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--kat-log", type=Path, required=True)
    parser.add_argument("--cpu", type=int)
    parser.add_argument("--limit-seconds", type=int, default=2)
    args = parser.parse_args()
    library, kat_log = args.library.resolve(), args.kat_log.resolve()
    if not library.is_file() or not kat_log.is_file():
        parser.error("library and KAT log must exist")
    if kat_log.stat().st_mtime < library.stat().st_mtime:
        parser.error("KAT log predates library; rerun the KAT")
    candidate = library.parent.parent.name
    category = candidate.split("-", 1)[0]
    if category not in OPERATIONS:
        parser.error("this smoke driver currently supports KEM, signature and hash APIs")
    label = library.stem.removeprefix("lib")
    log = kat_log.read_text(encoding="utf-8")
    instance = re.search(r"^instance\s*=\s*(\S+)\s*$", log, re.MULTILINE)
    if (not instance or f"library     = lib/lib{label}.so" not in log or
            f"RESULT {candidate} {instance.group(1)} PASS" not in log):
        parser.error("KAT log does not show PASS for this exact library")
    if not 1 <= args.limit_seconds <= 60:
        parser.error("smoke limit must be 1..60 seconds per operation")
    available = os.sched_getaffinity(0)
    cpu = args.cpu if args.cpu is not None else min(available)
    if cpu not in available:
        parser.error(f"CPU {cpu} is unavailable")
    driver = ROOT / "performance/ngcc_perf"
    if not driver.is_file():
        parser.error("build the driver: make -C performance")
    for operation, size in OPERATIONS[category]:
        label_operation = f"{operation}({size}B)" if size else operation
        command = ["taskset", "-c", str(cpu), str(driver), str(library), operation,
                   str(size), "5", str(args.limit_seconds)]
        try:
            run = subprocess.run(command, capture_output=True, text=True,
                                 timeout=args.limit_seconds + 5, check=False)
        except subprocess.TimeoutExpired:
            print(f"SMOKE {candidate} {label} {label_operation} TIMEOUT")
            return 1
        metadata, trials, status = parse_native(run.stdout)
        if run.returncode or status != "complete" or len(trials) != 5:
            print(f"SMOKE {candidate} {label} {label_operation} FAIL: {run.stderr.strip() or status}")
            return 1
        if metadata.get("warmups") != "3":
            print(f"SMOKE {candidate} {label} {label_operation} FAIL: warm-ups incomplete")
            return 1
        print(f"SMOKE {candidate} {label} {label_operation} PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
