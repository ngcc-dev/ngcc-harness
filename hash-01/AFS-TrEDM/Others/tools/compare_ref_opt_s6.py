#!/usr/bin/env python3
"""Compare Reference and Optimized AFS-TrEDM-S6 hash_cli outputs."""

from __future__ import annotations

import argparse
import json
import os
import random
import subprocess
import tempfile
import time
from pathlib import Path


DEFAULT_LENGTHS = "0,1,7,8,9,24,512,1024,8192,65536,1048576"
RATE_BITS = {
    "512": 1024,
    "768": 768,
    "1024": 512,
}


def parse_lengths(value: str) -> list[int]:
    out: list[int] = []
    for item in value.split(","):
        item = item.strip()
        if not item:
            continue
        n = int(item, 10)
        if n < 0:
            raise ValueError("lengths must be non-negative")
        out.append(n)
    return out


def unique_preserve_order(values: list[int]) -> list[int]:
    out: list[int] = []
    seen: set[int] = set()
    for value in values:
        if value not in seen:
            seen.add(value)
            out.append(value)
    return out


def random_message_hex(rng: random.Random, bit_len: int) -> str:
    byte_len = (bit_len + 7) // 8
    if byte_len == 0:
        return "-"
    data = bytearray(rng.getrandbits(8) for _ in range(byte_len))
    unused_bits = byte_len * 8 - bit_len
    if unused_bits:
        data[-1] &= (0xFF << unused_bits) & 0xFF
    return data.hex()


def run_hash(bin_path: Path, bit_len: int, msg_hex: str) -> str:
    arg_hex = msg_hex
    temp_path: str | None = None
    if len(msg_hex) > 12000:
        with tempfile.NamedTemporaryFile("w", delete=False, prefix="afs_s6_msg_", suffix=".hex") as fp:
            fp.write(msg_hex)
            temp_path = fp.name
        arg_hex = "@" + temp_path

    try:
        proc = subprocess.run(
            [str(bin_path), str(bit_len), arg_hex],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=180,
        )
    finally:
        if temp_path is not None:
            try:
                os.unlink(temp_path)
            except OSError:
                pass

    if proc.returncode != 0:
        raise RuntimeError(
            f"{bin_path} failed for {bit_len} bits: rc={proc.returncode}, stderr={proc.stderr.strip()}"
        )
    return proc.stdout.strip().upper()


def instance_dirs(root: Path, instance: str, mode: str) -> tuple[Path, Path]:
    name = f"AFS-TrEDM-{instance}"
    ref = root / "Implementations" / "Reference_Implementation" / name / "hash_cli"
    opt_name = {
        "opt64": "hash_cli",
        "avx2": "hash_cli_avx2",
        "dispatch": "hash_cli_dispatch",
    }[mode]
    opt = root / "Implementations" / "Optimized_Implementation" / name / opt_name
    if not ref.exists():
        raise FileNotFoundError(ref)
    if not opt.exists():
        raise FileNotFoundError(opt)
    return ref, opt


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--instance", required=True, choices=("512", "768", "1024"))
    parser.add_argument("--mode", default="opt64", choices=("opt64", "avx2", "dispatch"))
    parser.add_argument("--lengths", default=DEFAULT_LENGTHS)
    parser.add_argument(
        "--include-rate-boundaries",
        action="store_true",
        help="add r-1, r, and r+1 bit lengths for the selected S6 instance",
    )
    parser.add_argument("--random", type=int, default=0)
    parser.add_argument("--seed", type=int, default=0xA65E6)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    root = Path(args.root).resolve()
    out_path = (root / args.out).resolve() if not Path(args.out).is_absolute() else Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    ref_bin, opt_bin = instance_dirs(root, args.instance, args.mode)
    lengths = parse_lengths(args.lengths)
    rate_bits = RATE_BITS[args.instance]
    if args.include_rate_boundaries:
        lengths = unique_preserve_order(lengths + [rate_bits - 1, rate_bits, rate_bits + 1])
    max_len = max(lengths) if lengths else 0
    rng = random.Random(args.seed + int(args.instance))

    cases: list[tuple[str, int, str]] = []
    for bit_len in lengths:
        cases.append(("listed", bit_len, random_message_hex(random.Random(args.seed ^ bit_len), bit_len)))
    for _ in range(args.random):
        bit_len = rng.randrange(max_len + 1) if max_len > 0 else 0
        cases.append(("random", bit_len, random_message_hex(rng, bit_len)))

    started = time.time()
    mismatches = []
    for index, (kind, bit_len, msg_hex) in enumerate(cases):
        ref_digest = run_hash(ref_bin, bit_len, msg_hex)
        opt_digest = run_hash(opt_bin, bit_len, msg_hex)
        if ref_digest != opt_digest:
            mismatches.append(
                {
                    "case_index": index,
                    "kind": kind,
                    "bit_len": bit_len,
                    "msg_hex": msg_hex,
                    "reference": ref_digest,
                    "optimized": opt_digest,
                }
            )
            break

    elapsed = time.time() - started
    result = {
        "instance": args.instance,
        "mode": args.mode,
        "reference": str(ref_bin.relative_to(root)),
        "optimized": str(opt_bin.relative_to(root)),
        "lengths": lengths,
        "rate_bits": rate_bits,
        "included_rate_boundaries": bool(args.include_rate_boundaries),
        "random_cases": args.random,
        "seed": args.seed,
        "cases_run": len(cases) if not mismatches else mismatches[0]["case_index"] + 1,
        "mismatch_count": len(mismatches),
        "elapsed_seconds": round(elapsed, 3),
        "status": "PASS" if not mismatches else "FAIL",
        "mismatches": mismatches,
    }
    out_path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    md_path = out_path.with_suffix(".md")
    md_lines = [
        f"# Reference vs Optimized S6 compare: {args.instance} {args.mode}",
        "",
        f"- status: {result['status']}",
        f"- cases_run: {result['cases_run']}",
        f"- listed_lengths_bits: {','.join(str(x) for x in lengths)}",
        f"- random_cases: {args.random}",
        f"- seed: {args.seed}",
        f"- elapsed_seconds: {result['elapsed_seconds']}",
    ]
    if mismatches:
        mm = mismatches[0]
        md_lines.extend(
            [
                "",
                "## First mismatch",
                "",
                f"- case_index: {mm['case_index']}",
                f"- kind: {mm['kind']}",
                f"- bit_len: {mm['bit_len']}",
                f"- msg_hex: {mm['msg_hex']}",
                f"- reference: {mm['reference']}",
                f"- optimized: {mm['optimized']}",
            ]
        )
    md_path.write_text("\n".join(md_lines) + "\n", encoding="utf-8")

    print(f"{args.instance}: {result['status']} cases={result['cases_run']} elapsed={result['elapsed_seconds']}s")
    return 0 if not mismatches else 1


if __name__ == "__main__":
    raise SystemExit(main())
