#!/usr/bin/env python3

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_REFERENCE_ROOT = Path("/home/baoxueqian/下载/Optimized_Implementation(3)/Optimized_Implementation")

PROFILE_MAP = {
    "128": {
        "reference_dir": DEFAULT_REFERENCE_ROOT / "ZEN_128",
        "local_dir": REPO_ROOT / "optimized_implementation_128v3" / "software" / "ZEN_swift_128",
        "reference_output": "KAT_KEM_ZEN_128.txt",
        "local_output": "KAT_KEM_ZEN_SWIFT_128.txt",
    },
    "256": {
        "reference_dir": DEFAULT_REFERENCE_ROOT / "ZEN_256",
        "local_dir": REPO_ROOT / "optimized_implementation_256" / "software" / "ZEN_swift_256",
        "reference_output": "KAT_KEM_ZEN_256.txt",
        "local_output": "KAT_KEM_ZEN_SWIFT_256.txt",
    },
    "512": {
        "reference_dir": DEFAULT_REFERENCE_ROOT / "ZEN_512",
        "local_dir": REPO_ROOT / "optimized_implementation_512" / "software" / "ZEN_swift_512",
        "reference_output": "KAT_KEM_ZEN_512.txt",
        "local_output": "KAT_KEM_ZEN_SWIFT_512.txt",
    },
}

COMPARE_FIELDS = ("Seed", "PK", "SK", "CT", "SS")


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_kat_file(path: Path):
    rows = []
    current = {}
    for raw_line in path.read_text(encoding="utf-8-sig").splitlines():
        line = raw_line.strip()
        if not line:
            if current:
                rows.append(current)
                current = {}
            continue
        if " = " in line:
            key, value = line.split(" = ", 1)
            current[key] = value
    if current:
        rows.append(current)
    return rows


def rebuild_local_kat(local_dir: Path) -> None:
    subprocess.run(
        ["make", "clean", "KAT_KEM", "CC=gcc"],
        cwd=local_dir,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    subprocess.run(
        ["./KAT_KEM"],
        cwd=local_dir,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def compare_profile(profile: str, reference_root: Path, rebuild_local: bool) -> int:
    cfg = PROFILE_MAP[profile].copy()
    cfg["reference_dir"] = reference_root / f"ZEN_{profile}"
    reference_dir = cfg["reference_dir"]
    local_dir = cfg["local_dir"]
    ref_file = reference_dir / "output" / cfg["reference_output"]
    local_file = local_dir / "output" / cfg["local_output"]

    if rebuild_local:
        rebuild_local_kat(local_dir)

    ref_rows = parse_kat_file(ref_file)
    local_rows = parse_kat_file(local_file)

    if len(ref_rows) != len(local_rows):
        print(f"[FAIL] ZEN-{profile}: vector count mismatch ref={len(ref_rows)} local={len(local_rows)}")
        return 1

    for idx, (ref_row, local_row) in enumerate(zip(ref_rows, local_rows)):
        for field in COMPARE_FIELDS:
            if ref_row.get(field) != local_row.get(field):
                print(f"[FAIL] ZEN-{profile}: count={idx} field={field} mismatch")
                return 1

    ref_hash = sha256_file(ref_file)
    local_hash = sha256_file(local_file)
    status = "PASS" if ref_hash == local_hash else "WARN"
    print(
        f"[{status}] ZEN-{profile}: counts={len(ref_rows)} "
        f"sha256(ref)={ref_hash} sha256(local)={local_hash}"
    )
    return 0 if ref_hash == local_hash else 1


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare local KEM KAT outputs against the reference implementation.")
    parser.add_argument("--reference-root", type=Path, default=DEFAULT_REFERENCE_ROOT)
    parser.add_argument("--profile", choices=("128", "256", "512", "all"), default="all")
    parser.add_argument("--rebuild-local", action="store_true", help="Rebuild and rerun local KAT_KEM before comparing.")
    args = parser.parse_args()

    profiles = ("128", "256", "512") if args.profile == "all" else (args.profile,)
    rc = 0
    for profile in profiles:
        rc |= compare_profile(profile, args.reference_root, args.rebuild_local)
    return rc


if __name__ == "__main__":
    sys.exit(main())
