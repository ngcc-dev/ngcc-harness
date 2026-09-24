#!/usr/bin/env python3
"""SHA3-512 baseline KAT, provenance, and anti-alias checks."""

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
        "target": "sha3_512_reference_c",
        "source": "tools/perf/baselines/xkcp_readable/Keccak-readable-and-compact.c",
        "driver": "tools/perf/baselines/xkcp_readable/benchmark_sha3_512_reference_c.c",
        "upstream": "XKCP-master/Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c",
        "source_sha256": "7d25b518f28b4b9be141495dde205621fdb528e880596ef9507d30ba1b937dc9",
    },
    "compact": {
        "label": "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)",
        "binary": "tools/perf/baselines/xkcp_compactfips202_sha3_512",
        "target": "xkcp_compactfips202_sha3_512",
        "source": "tools/perf/baselines/xkcp_more_compact/Keccak-more-compact.c",
        "driver": "tools/perf/baselines/xkcp_more_compact/benchmark_xkcp_compactfips202_sha3_512.c",
        "upstream": "XKCP-master/Standalone/CompactFIPS202/C/Keccak-more-compact.c",
        "source_sha256": "089b4cbd9e9ec314ce9e00514dde4d90678f861b7d754e05e4726eb38cb702a2",
    },
    "openssl": {
        "label": "SHA3-512 OpenSSL EVP",
        "binary": "tools/perf/baselines/openssl_evp_sha3_512",
        "target": "openssl_evp_sha3_512",
        "source": "tools/baselines/bench_openssl_sha3_512.c",
        "driver": "tools/perf/baselines/benchmark_openssl_evp_sha3_512.c",
        "upstream": "OpenSSL EVP_sha3_512",
    },
}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def run(command: list[str], *, cwd: Path) -> str:
    result = subprocess.run(command, cwd=str(cwd), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(command)}\n{result.stdout}")
    return result.stdout


def parse_digest(output: str, expected_name: str) -> str:
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 4 and fields[0] and fields[1] == expected_name and fields[2] == "OK":
            return fields[3].upper()
        if len(fields) >= 3 and fields[0] and fields[1].startswith("digest-generated-"):
            return fields[2].upper()
    raise RuntimeError(f"could not parse digest from output:\n{output}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--report-out")
    args = parser.parse_args(argv)

    root = Path(args.root).resolve()
    baseline_dir = root / "tools/perf/baselines"
    run(["make", "-C", str(baseline_dir), "clean"], cwd=root)
    run(["make", "-C", str(baseline_dir), "sha3_512_reference_c", "xkcp_compactfips202_sha3_512", "openssl_evp_sha3_512"], cwd=root)

    rows: list[dict[str, str]] = []
    digests_1mib: dict[str, str] = {}
    for key, spec in BASELINES.items():
        binary = root / spec["binary"]
        source = root / spec["source"]
        driver = root / spec["driver"]
        if not source.is_file():
            raise RuntimeError(f"missing source for {spec['label']}: {source}")
        if not driver.is_file():
            raise RuntimeError(f"missing driver for {spec['label']}: {driver}")
        if not binary.is_file():
            raise RuntimeError(f"missing binary for {spec['label']}: {binary}")

        empty = parse_digest(run([str(binary), "--kat-empty"], cwd=root), "empty")
        abc = parse_digest(run([str(binary), "--kat-abc"], cwd=root), "abc")
        if empty != EMPTY:
            raise RuntimeError(f"{spec['label']} empty KAT mismatch")
        if abc != ABC:
            raise RuntimeError(f"{spec['label']} abc KAT mismatch")
        digests_1mib[key] = parse_digest(run([str(binary), "--digest-generated=1048576"], cwd=root), "digest-generated-1048576")

        actual_source_sha = sha256_file(source)
        expected_source_sha = spec.get("source_sha256")
        if expected_source_sha and actual_source_sha != expected_source_sha:
            raise RuntimeError(f"{spec['label']} source SHA mismatch: {actual_source_sha} != {expected_source_sha}")

        rows.append(
            {
                "key": key,
                "label": spec["label"],
                "target": spec["target"],
                "binary": spec["binary"],
                "binary_sha256": sha256_file(binary),
                "source": spec["source"],
                "source_sha256": actual_source_sha,
                "driver": spec["driver"],
                "driver_sha256": sha256_file(driver),
                "upstream": spec["upstream"],
                "empty_kat": "PASS",
                "abc_kat": "PASS",
                "generated_1mib_digest": digests_1mib[key],
            }
        )

    if rows[0]["source_sha256"] == rows[1]["source_sha256"]:
        raise RuntimeError("Reference C and XKCP CompactFIPS202 source SHA-256 values alias")
    if rows[0]["binary_sha256"] == rows[1]["binary_sha256"]:
        raise RuntimeError("Reference C and XKCP CompactFIPS202 binary SHA-256 values alias")
    if len({row["binary_sha256"] for row in rows}) != len(rows):
        raise RuntimeError("not all SHA3 baseline binaries are distinct")
    if len(set(digests_1mib.values())) != 1:
        raise RuntimeError(f"1MiB generated SHA3-512 digest mismatch: {digests_1mib}")

    report_lines = [
        "# Stage S6-15C SHA3-512 KAT and Anti-Alias Report",
        "",
        "| row label | target | source | source sha256 | binary | binary sha256 | empty KAT | abc KAT | 1MiB generated digest | upstream |",
        "|---|---|---|---|---|---|---|---|---|---|",
    ]
    for row in rows:
        report_lines.append(
            "| {label} | `{target}` | `{source}` | `{source_sha256}` | `{binary}` | `{binary_sha256}` | {empty_kat} | {abc_kat} | `{generated_1mib_digest}` | `{upstream}` |".format(
                **row
            )
        )
    report_lines.extend(
        [
            "",
            "Anti-alias status: PASS.",
            "",
            "The two XKCP baselines use distinct official source files and distinct binaries. Close performance values are acceptable only after these provenance and KAT checks pass.",
        ]
    )
    report = "\n".join(report_lines) + "\n"
    if args.report_out:
        out_path = root / args.report_out
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(report, encoding="utf-8")
    print(report, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
