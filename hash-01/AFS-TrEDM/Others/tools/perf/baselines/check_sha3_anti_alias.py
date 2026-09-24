#!/usr/bin/env python3
"""Stage16 SHA3-512 baseline anti-alias validation."""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path


EMPTY = "A69F73CCA23A9AC5C8B567DC185A756E97C982164FE25859E0D1DCC1475C80A615B2123AF1F5F94C11E3E9402C3AC558F500199D95B6D3E301758586281DCD26"
ABC = "B751850B1A57168A5693CD924B6B096E08F621827444F70D884F5D0240D2712E10E116E9192AF3C91A7EC57647E3934057340B4CF408D5A56592F8274EEC53F0"

BASELINES = {
    "reference": {
        "label": "SHA3-512 Reference C (XKCP readable)",
        "binary": "tools/perf/baselines/sha3_512_reference_c",
        "source": "tools/perf/baselines/xkcp_readable/Keccak-readable-and-compact.c",
        "driver": "tools/perf/baselines/xkcp_readable/benchmark_sha3_512_reference_c.c",
        "upstream": "official XKCP Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c",
        "expected_source_sha256": "7d25b518f28b4b9be141495dde205621fdb528e880596ef9507d30ba1b937dc9",
    },
    "compact": {
        "label": "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)",
        "binary": "tools/perf/baselines/xkcp_compactfips202_sha3_512",
        "source": "tools/perf/baselines/xkcp_more_compact/Keccak-more-compact.c",
        "driver": "tools/perf/baselines/xkcp_more_compact/benchmark_xkcp_compactfips202_sha3_512.c",
        "upstream": "official XKCP Standalone/CompactFIPS202/C/Keccak-more-compact.c",
        "expected_source_sha256": "089b4cbd9e9ec314ce9e00514dde4d90678f861b7d754e05e4726eb38cb702a2",
    },
    "openssl": {
        "label": "SHA3-512 OpenSSL EVP",
        "binary": "tools/perf/baselines/openssl_evp_sha3_512",
        "source": "tools/baselines/bench_openssl_sha3_512.c",
        "driver": "tools/perf/baselines/benchmark_openssl_evp_sha3_512.c",
        "upstream": "OpenSSL EVP_sha3_512",
        "expected_source_sha256": None,
    },
}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def run(command: list[str], cwd: Path) -> str:
    result = subprocess.run(command, cwd=str(cwd), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(command)}\n{result.stdout}")
    return result.stdout


def parse_kat(output: str, kat_name: str) -> str:
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 4 and fields[1] == kat_name and fields[2] == "OK":
            return fields[3].upper()
    raise RuntimeError(f"missing {kat_name} KAT output:\n{output}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--report-out", default="docs/sha3_baselines/SHA3_ANTI_ALIAS_REPORT.md")
    args = parser.parse_args(argv)

    root = Path(args.root).resolve()
    baseline_dir = root / "tools/perf/baselines"
    run(["make", "-C", str(baseline_dir), "sha3_512_reference_c", "xkcp_compactfips202_sha3_512", "openssl_evp_sha3_512"], root)

    rows: list[dict[str, str]] = []
    for key, spec in BASELINES.items():
        binary = root / spec["binary"]
        source = root / spec["source"]
        driver = root / spec["driver"]
        if not binary.is_file():
            raise RuntimeError(f"missing binary for {spec['label']}: {binary}")
        if not source.is_file():
            raise RuntimeError(f"missing source for {spec['label']}: {source}")
        if not driver.is_file():
            raise RuntimeError(f"missing driver for {spec['label']}: {driver}")

        empty = parse_kat(run([str(binary), "--kat-empty"], root), "empty")
        abc = parse_kat(run([str(binary), "--kat-abc"], root), "abc")
        if empty != EMPTY:
            raise RuntimeError(f"{spec['label']} empty KAT mismatch")
        if abc != ABC:
            raise RuntimeError(f"{spec['label']} abc KAT mismatch")

        source_sha = sha256_file(source)
        expected = spec["expected_source_sha256"]
        if expected is not None and source_sha != expected:
            raise RuntimeError(f"{spec['label']} source sha256 mismatch: {source_sha} != {expected}")
        rows.append(
            {
                "key": key,
                "label": spec["label"],
                "source": spec["source"],
                "source_sha256": source_sha,
                "driver": spec["driver"],
                "driver_sha256": sha256_file(driver),
                "binary": spec["binary"],
                "binary_sha256": sha256_file(binary),
                "upstream": spec["upstream"],
                "empty": "PASS",
                "abc": "PASS",
            }
        )

    ref = next(row for row in rows if row["key"] == "reference")
    compact = next(row for row in rows if row["key"] == "compact")
    if ref["source"] == compact["source"]:
        raise RuntimeError("Reference C and XKCP CompactFIPS202 use the same source path")
    if ref["source_sha256"] == compact["source_sha256"]:
        raise RuntimeError("Reference C and XKCP CompactFIPS202 use the same source SHA-256")
    if ref["binary_sha256"] == compact["binary_sha256"]:
        raise RuntimeError("Reference C and XKCP CompactFIPS202 produced the same binary")
    if len({row["binary_sha256"] for row in rows}) != len(rows):
        raise RuntimeError("SHA3 baseline binaries are not all distinct")
    openssl_source = (root / BASELINES["openssl"]["source"]).read_text(encoding="utf-8", errors="ignore")
    if "EVP_sha3_512" not in openssl_source:
        raise RuntimeError("OpenSSL EVP baseline source does not call EVP_sha3_512")

    report = [
        "# Stage S6-16B SHA3-512 Anti-Alias Report",
        "",
        "Status: PASS.",
        "",
        "| Baseline | Source | Source SHA-256 | Binary | Binary SHA-256 | Empty KAT | abc KAT | Upstream |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for row in rows:
        report.append(
            f"| {row['label']} | `{row['source']}` | `{row['source_sha256']}` | `{row['binary']}` | `{row['binary_sha256']}` | {row['empty']} | {row['abc']} | `{row['upstream']}` |"
        )
    report.extend(
        [
            "",
            "Anti-alias checks:",
            "",
            "- Reference C and XKCP CompactFIPS202 source paths differ: PASS.",
            "- Reference C and XKCP CompactFIPS202 source SHA-256 values differ: PASS.",
            "- Reference C and XKCP CompactFIPS202 binary SHA-256 values differ: PASS.",
            "- All three SHA3-512 baseline binaries are distinct: PASS.",
            "- OpenSSL EVP source calls `EVP_sha3_512`: PASS.",
        ]
    )
    out = root / args.report_out
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(report) + "\n", encoding="utf-8")
    print(f"PASS: Stage16 SHA3 anti-alias report written to {out.relative_to(root)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
