#!/usr/bin/env python3
"""Build, KAT-check, and measure every staged x86 performance instance.

Results go to an ignored, host-specific directory for review before publication.
No original ZIP, generated library, or full KAT text is required in a clone.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import os
from pathlib import Path
import platform
import re
import socket
import subprocess
import sys

from validate import check as validate_record


ROOT = Path(__file__).resolve().parent.parent
FAMILIES = {
    "mithril": ("kem-22", [f"Mithril-{level}" for level in (128, 256, 512)]),
    "scloud": ("kem-35", ["Scloudplus-128-SHAKE"]),
    "zcdmc": ("hash-31", [f"ZC-DMC-{p}-{h}" for p in (1280, 1536)
                         for h in (512, 768, 1024)]),
    "bit": ("sign-02", [f"BiT-{level}" for level in (128, 256, 512)]),
}


def avx2_available() -> bool:
    try:
        cpuinfo = Path("/proc/cpuinfo").read_text(encoding="utf-8")
    except OSError:
        return False
    return bool(re.search(r"\bavx2\b", cpuinfo))


def instances(families: list[str], variants: list[str]):
    for family in families:
        candidate, names = FAMILIES[family]
        for variant in variants:
            for name in names:
                label = f"{name}-avx2" if variant == "avx2" else (
                    f"{name}-ref" if family == "scloud" else name
                )
                library = Path(candidate) / "lib" / f"lib{label}.so"
                kat_log = Path(candidate) / "results" / f"{label}.log"
                yield family, variant, library, kat_log


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--family", choices=FAMILIES, action="append",
                        help="limit to one family; may be repeated")
    parser.add_argument("--variant", choices=("both", "ref", "avx2"), default="both")
    parser.add_argument("--cpu", type=int, help="CPU in the current affinity set")
    parser.add_argument("--limit-seconds", type=int, default=60,
                        help="per-operation budget, 1..60 (default: 60)")
    parser.add_argument("--output-dir", type=Path,
                        help="new output directory; default: performance/runs/<host>-<UTC>")
    parser.add_argument("--dry-run", action="store_true", help="list work without building")
    args = parser.parse_args()
    if not 1 <= args.limit_seconds <= 60:
        parser.error("--limit-seconds must be 1..60")
    if platform.machine().lower() not in ("x86_64", "amd64"):
        parser.error("this runner is for x86-64; ARM support is not integrated yet")
    variants = ["ref", "avx2"] if args.variant == "both" else [args.variant]
    if "avx2" in variants and not avx2_available():
        parser.error("AVX2 is not available on this host; use --variant ref")
    if args.cpu is not None and args.cpu not in os.sched_getaffinity(0):
        parser.error(f"CPU {args.cpu} is not in this process's affinity set")
    families = list(dict.fromkeys(args.family or FAMILIES))
    selected = list(instances(families, variants))
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    hostname = re.sub(r"[^A-Za-z0-9_.-]", "-", socket.gethostname())
    output_dir = args.output_dir or ROOT / "performance/runs" / f"{hostname}-{stamp}"
    if not output_dir.is_absolute():
        output_dir = ROOT / output_dir
    print(f"output directory: {output_dir}")
    print(f"{len(selected)} instance(s), {args.limit_seconds}s per operation, "
          f"CPU {args.cpu if args.cpu is not None else 'first available'}")
    for family in families:
        for variant in variants:
            print(f"build/KAT: make -C performance build-{family}-{variant}")
    if args.dry_run:
        for _, _, library, _ in selected:
            print(f"measure: {library}")
        return 0
    if output_dir.exists():
        parser.error(f"output directory already exists: {output_dir}")
    output_dir.mkdir(parents=True)
    subprocess.run(["make", "-C", "performance"], cwd=ROOT, check=True)
    failures = []
    for family in families:
        for variant in variants:
            subprocess.run(["make", "-C", "performance", f"build-{family}-{variant}"],
                           cwd=ROOT, check=True)
            for selected_family, selected_variant, library, kat_log in selected:
                if (selected_family, selected_variant) != (family, variant):
                    continue
                output = output_dir / f"{library.parent.parent.name}-{library.stem.removeprefix('lib')}.json"
                command = [sys.executable, "performance/run.py",
                           "--library", str(library), "--kat-log", str(kat_log),
                           "--output", str(output), "--limit-seconds", str(args.limit_seconds)]
                if args.cpu is not None:
                    command += ["--cpu", str(args.cpu)]
                print(f"running {library}", flush=True)
                result = subprocess.run(command, cwd=ROOT, check=False)
                problems = validate_record(output) if output.is_file() else ["no JSON record"]
                for problem in problems:
                    print(problem, file=sys.stderr)
                if result.returncode or problems:
                    failures.append(str(library))
    print(f"finished: {len(selected) - len(failures)}/{len(selected)} instance(s) complete; "
          f"results in {output_dir}")
    if failures:
        print("failed: " + ", ".join(failures), file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
