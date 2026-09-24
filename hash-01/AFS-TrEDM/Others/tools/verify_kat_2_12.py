#!/usr/bin/env python3
"""Verify KAT_2_12 files against the Reference S6 hash_cli.

Default mode checks all record counts and verifies a deterministic sample of
records. Use --all for full Reference CLI verification; full mode is slow.
"""

from __future__ import annotations
import argparse
import os
import random
import re
import subprocess
import sys
import tempfile
from pathlib import Path

THIS = Path(__file__).resolve()
ROOT = THIS.parents[1]
INSTANCES = ("AFS-TrEDM-512", "AFS-TrEDM-768", "AFS-TrEDM-1024")
S6_INSTANCE_DIR_TO_IMPL = {
    "AFS-TrEDM-S6-512": "AFS-TrEDM-512",
    "AFS-TrEDM-S6-768": "AFS-TrEDM-768",
    "AFS-TrEDM-S6-1024": "AFS-TrEDM-1024",
}

def parse_kat(path: Path):
    text = path.read_text()
    entries = text.strip().split("\n\n")
    for entry in entries:
        m_len = re.search(r"Msg_Len = (\d+)", entry)
        m_msg = re.search(r"Msg = ([0-9A-F]*)", entry)
        m_dst_len = re.search(r"Dst_Len = (\d+)", entry)
        m_dst = re.search(r"Dst = ([0-9A-F]+)", entry)
        if m_len and m_msg and m_dst_len and m_dst:
            yield int(m_len.group(1)), m_msg.group(1), int(m_dst_len.group(1)), m_dst.group(1)

def selected_indices(count: int, full: bool) -> set[int]:
    if full:
        return set(range(count))
    fixed_lengths = [0, 1, 2, 7, 8, 9, 15, 16, 63, 64, 65, 127, 128, 129,
                     255, 256, 257, 511, 512, 513, 1023, 1024, 1025,
                     2047, 2048, 2049, 4095, 4096]
    idx = {x for x in fixed_lengths if 0 <= x < count}
    idx.update(range(min(32, count)))
    idx.update(range(max(0, count - 32), count))
    rng = random.Random(0xA5712D12)
    while len(idx) < min(count, 160):
        idx.add(rng.randrange(count))
    return idx


def instance_from_path(path: Path) -> str | None:
    text = "/".join(path.parts)
    for s6_dir, instance in S6_INSTANCE_DIR_TO_IMPL.items():
        if s6_dir in text:
            return instance
    for instance in INSTANCES:
        if instance in text:
            return instance
    return None


def digest_len_from_instance(instance: str) -> int:
    return int(instance.rsplit("-", 1)[1])


def discover_default_files() -> list[Path]:
    return sorted((ROOT / "Test_Vectors").glob("KAT_2_12_AFS-TrEDM-*.txt"))


def build_hash_cli(instance: str, build_dir: Path) -> Path:
    inst_dir = ROOT / "Implementations" / "Reference_Implementation" / instance
    out = build_dir / f"{instance}_hash_cli"
    cc = os.environ.get("CC", "cc")
    cflags = os.environ.get("CFLAGS", "-std=c99 -O2 -Wall -Wextra -pedantic").split()
    cmd = [
        cc,
        *cflags,
        "-o",
        str(out),
        "hash_cli.c",
        "CryptHash_AlgorithmInstance.c",
        "afs_sbox64.c",
        "afs_lmds1600_s6.c",
        "afs_p1600.c",
        "afs_tredm.c",
    ]
    subprocess.run(cmd, cwd=inst_dir, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return out


def digest_hex(hash_cli: Path, msg_hex: str, msg_len_bits: int) -> str:
    msg_arg = msg_hex if msg_hex else "-"
    proc = subprocess.run(
        [str(hash_cli), str(msg_len_bits), msg_arg],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return proc.stdout.strip().upper()

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true", help="verify every record with the Reference CLI; slow")
    ap.add_argument("files", nargs="*")
    args = ap.parse_args()

    if args.files:
        files = [Path(x) for x in args.files]
    else:
        files = discover_default_files()
    if not files:
        print("No S6 KAT_2_12 files found under Test_Vectors/KAT_2_12_AFS-TrEDM-*.txt.", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="afs_s6_kat_hash_cli_") as tmp:
        build_dir = Path(tmp)
        hash_cli_by_instance: dict[str, Path] = {}
        for path in files:
            instance = instance_from_path(path)
            if instance is None:
                print(f"FAIL {path}: cannot infer AFS-TrEDM instance from path", file=sys.stderr)
                return 1
            hash_cli = hash_cli_by_instance.get(instance)
            if hash_cli is None:
                hash_cli = build_hash_cli(instance, build_dir)
                hash_cli_by_instance[instance] = hash_cli
            expected_dst_len = digest_len_from_instance(instance)
            records = list(parse_kat(path))
            if len(records) != 4097:
                print(f"FAIL {path.name}: expected 4097 entries, got {len(records)}", file=sys.stderr)
                return 1
            indices = selected_indices(len(records), args.all)
            for i in sorted(indices):
                msg_len, msg_hex, dst_len, dst = records[i]
                if dst_len != expected_dst_len:
                    print(f"FAIL {path.name} record={i}: expected Dst_Len={expected_dst_len}, got {dst_len}", file=sys.stderr)
                    return 1
                exp = digest_hex(hash_cli, msg_hex, msg_len)
                if exp != dst.upper():
                    print(f"FAIL {path.name} record={i} len={msg_len}", file=sys.stderr)
                    print(f"msg={msg_hex}", file=sys.stderr)
                    print(f"kat={dst}", file=sys.stderr)
                    print(f"ref={exp}", file=sys.stderr)
                    return 1
            mode = "full" if args.all else "sampled"
            print(f"OK {path}: {len(records)} entries counted, {len(indices)} {mode} Reference CLI checks passed")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
