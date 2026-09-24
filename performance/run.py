#!/usr/bin/env python3
"""Run bounded five-trial API timings and write raw, platform-tagged JSON.

Example (after the optimized library's KAT has passed):

    python3 performance/run.py --library kem-22/lib/libMithril-256-avx2.so \
        --kat-log kem-22/results/Mithril-256-avx2.log \
        --output performance/data/mithril256-avx2-vm.json

The guide's arithmetic mean is primary; the median of five trial means is
retained for noisy hosts. The runner never calls submitted build scripts.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent
HASH_SIZES = (32, 128, 512, 1024, 4096, 8192, 16384, 65536)
OPERATIONS = {
    "kem": ("keygen", "enc", "dec"),
    "sign": ("keygen", "sign", "verify"),
    "hash": ("hash",),
}


def sha256(path: Path) -> str:
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def archive_digest(candidate: str) -> str | None:
    provenance = (ROOT / "SOURCE_ARCHIVES.md").read_text(encoding="utf-8")
    match = re.search(rf"^\|\s*{re.escape(candidate)}\s*\|[^|]*\|\s*`([0-9a-f]{{64}})`",
                      provenance, re.MULTILINE)
    return match.group(1) if match else None


def command_version(command: list[str]) -> str | None:
    try:
        return subprocess.check_output(command, text=True, stderr=subprocess.DEVNULL).splitlines()[0]
    except (OSError, subprocess.CalledProcessError, IndexError):
        return None


def elf_load_bytes(path: Path) -> int | None:
    try:
        listing = subprocess.check_output(["readelf", "-lW", str(path)], text=True)
        return sum(int(parts[5], 16) for line in listing.splitlines()
                   if (parts := line.split()) and parts[0] == "LOAD")
    except (OSError, subprocess.CalledProcessError, ValueError):
        return None


def environment(cpu: int) -> dict:
    cpuinfo = Path("/proc/cpuinfo").read_text(encoding="utf-8")
    model = next((line.split(":", 1)[1].strip() for line in cpuinfo.splitlines()
                  if line.startswith("model name") or line.startswith("Hardware")), "unknown")
    reported_mhz = next((float(line.split(":", 1)[1]) for line in cpuinfo.splitlines()
                         if line.startswith("cpu MHz")), None)
    meminfo = Path("/proc/meminfo").read_text(encoding="utf-8")
    memory_kib = next((int(line.split()[1]) for line in meminfo.splitlines()
                       if line.startswith("MemTotal:")), None)
    os_release = Path("/etc/os-release").read_text(encoding="utf-8")
    os_name = next((line.split("=", 1)[1].strip('"') for line in os_release.splitlines()
                    if line.startswith("PRETTY_NAME=")), platform.platform())
    return {
        "cpu_model": model,
        "cpu_mhz_reported": reported_mhz,
        "cpu_number": cpu,
        "cpu_affinity": [cpu],
        "architecture": platform.machine(),
        "virtualized": "hypervisor" in cpuinfo,
        "logical_cpus_visible": os.cpu_count(),
        "memory_total_kib": memory_kib,
        "os": os_name,
        "kernel": platform.release(),
        "compiler": command_version(["gcc", "--version"]),
        "cmake": command_version(["cmake", "--version"]),
        "perf_event_paranoid": Path("/proc/sys/kernel/perf_event_paranoid").read_text().strip(),
    }


def parse_native(output: str) -> tuple[dict, list[dict], str]:
    metadata: dict[str, str] = {}
    trials: list[dict] = []
    status = "missing"
    for line in output.splitlines():
        parts = line.split("\t")
        if parts[0] == "META" and len(parts) == 3:
            metadata[parts[1]] = parts[2]
        elif parts[0] == "TRIAL" and len(parts) == 5:
            trials.append({"number": int(parts[1]), "measurements": int(parts[2]),
                           "elapsed_seconds": float(parts[3]), "cpu_cycles": int(parts[4])})
        elif parts[0] == "STATUS" and len(parts) == 2:
            status = parts[1]
    return metadata, trials, status


def measure(library: Path, operation: str, input_bytes: int, cpu: int, limit: int) -> dict:
    command = ["taskset", "-c", str(cpu), str(ROOT / "performance/ngcc_perf"),
               str(library), operation, str(input_bytes), "5", str(limit)]
    command_for_report = ["taskset", "-c", str(cpu), "performance/ngcc_perf",
                          library.relative_to(ROOT).as_posix(), operation,
                          str(input_bytes), "5", str(limit)]
    try:
        run = subprocess.run(command, text=True, capture_output=True,
                             timeout=limit + 5, check=False)
    except subprocess.TimeoutExpired as error:
        return {"operation": operation, "input_bytes": input_bytes,
                "status": "hard_timeout", "limit_seconds": limit,
                "error": f"wall-clock timeout after {limit + 5} seconds",
                "command": command_for_report}
    metadata, trials, native_status = parse_native(run.stdout)
    record = {"operation": operation, "input_bytes": input_bytes,
              "status": native_status, "exit_code": run.returncode,
              "limit_seconds": limit, "command": command_for_report,
              "metadata": metadata, "trials": trials,
              "stderr": run.stderr.strip()}
    if run.returncode != 0 or native_status != "complete" or len(trials) != 5:
        record["status"] = "failed_or_partial"
        return record
    samples = sum(trial["measurements"] for trial in trials)
    seconds = sum(trial["elapsed_seconds"] for trial in trials)
    trial_means = [trial["elapsed_seconds"] / trial["measurements"] for trial in trials]
    cycles_available = metadata.get("cpu_cycles_available") == "yes"
    record.update({
        "measurements": samples,
        "guide_100_measurements_met": samples >= 100,
        "mean_seconds_per_operation": seconds / samples,
        "middle_trial_seconds_per_operation": statistics.median(trial_means),
        "operations_per_second": samples / seconds,
        "mean_cpu_cycles": (sum(trial["cpu_cycles"] for trial in trials) / samples
                            if cycles_available else None),
        "throughput_MB_per_second": (input_bytes * samples / seconds / 1_000_000
                                      if operation == "hash" else None),
    })
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", required=True, type=Path)
    parser.add_argument("--kat-log", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--limit-seconds", type=int, default=60)
    parser.add_argument("--cpu", type=int)
    args = parser.parse_args()
    library = args.library.resolve()
    kat_log = args.kat_log.resolve()
    if not library.is_file() or not kat_log.is_file():
        parser.error("library and fresh KAT log must both exist")
    if kat_log.stat().st_mtime < library.stat().st_mtime:
        parser.error("KAT log predates the library; rerun the KAT")
    kind = library.parent.parent.name.split("-", 1)[0]
    if kind not in OPERATIONS:
        parser.error("only KEM, signature and hash APIs have timing drivers yet")
    label = library.stem.removeprefix("lib")
    candidate = library.parent.parent.name
    kat_text = kat_log.read_text(encoding="utf-8")
    instance_match = re.search(r"^instance\s*=\s*(\S+)\s*$", kat_text, re.MULTILINE)
    if not instance_match or f"library     = lib/lib{label}.so" not in kat_text:
        parser.error("KAT log does not describe this library")
    expected = f"RESULT {candidate} {instance_match.group(1)} PASS"
    if expected not in kat_text:
        parser.error(f"KAT log does not contain {expected!r}")
    if not (0 < args.limit_seconds <= 600):
        parser.error("limit must be 1..600 seconds")
    available = os.sched_getaffinity(0)
    cpu = args.cpu if args.cpu is not None else min(available)
    if cpu not in available:
        parser.error(f"CPU {cpu} is not available in this process affinity")
    if not (ROOT / "performance/ngcc_perf").is_file():
        parser.error("build first: make -C performance")
    if args.output.exists():
        parser.error(f"refusing to overwrite {args.output}")

    start = dt.datetime.now(dt.UTC)
    load_start = os.getloadavg()
    records = []
    for operation in OPERATIONS[kind]:
        for size in HASH_SIZES if operation == "hash" else (0,):
            print(f"measuring {candidate} {label} {operation} {size or ''}", flush=True)
            records.append(measure(library, operation, size, cpu, args.limit_seconds))
    end = dt.datetime.now(dt.UTC)
    load_end = os.getloadavg()
    cycle_gaps = any(row.get("mean_cpu_cycles") is None for row in records)
    guide_gaps = (["hardware CPU cycles unavailable on this host"] if cycle_gaps else [])
    guide_gaps.extend([
        "baseline/peak process RSS and ELF image size are proxies, not an isolated static-memory measurement",
        "the KAT manifest proves output equality but the complete functional vectors are not embedded in this report",
        "-fPIC is added for the harness shared library and is not one of the guide's listed build flags",
    ])
    result = {
        "schema": 1,
        "status": "preliminary",
        "guide_complete": not guide_gaps and all(row.get("guide_100_measurements_met")
                                               and row.get("status") == "complete" for row in records),
        "guide_gaps": guide_gaps,
        "started_utc": start.isoformat(), "ended_utc": end.isoformat(),
        "candidate": candidate, "library": library.relative_to(ROOT).as_posix(),
        "library_sha256": sha256(library),
        "source_archive_sha256": archive_digest(candidate),
        "kat_log": kat_log.relative_to(ROOT).as_posix(),
        "kat_log_sha256": sha256(kat_log),
        "guide": "doc/perf-x86.pdf, NICCS June 2026, sections 3.3-3.5",
        "environment": {**environment(cpu), "load_average_start": load_start,
                        "load_average_end": load_end},
        "elf_load_mem_bytes": elf_load_bytes(library),
        "input_rule": "ngcc_seed uses bytes 00..2f via the ICCS DRNG; 64-byte signing messages and S1..S8 hash inputs use byte[i]=(131*i+length) mod 256",
        "rows": records,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as destination:
        json.dump(result, destination, indent=2)
        destination.write("\n")
    print(f"wrote {args.output}: {len(records)} operation(s), guide_complete={result['guide_complete']}")
    return 0 if all(row.get("status") == "complete" for row in records) else 1


if __name__ == "__main__":
    sys.exit(main())
