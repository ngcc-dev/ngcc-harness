#!/usr/bin/env python3
"""Reproduce the source-level evidence for MQOM-1 without timing claims."""

import argparse
import hashlib
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
EXPECTED_TABLE_SHA256 = "57334c49e51ab5eb2544907c35769124890455744f4905360ddf05ff1cdf0ca5"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    impl = HERE / "submission/submission_package_v3/Reference_Implementation"
    tables = sorted(impl.glob("*/rijndael/rijndael_table.c"))
    seeds = sorted(impl.glob("*/blc/seed_commit_impl.h"))
    makefile = (HERE / "Makefile").read_text()
    checks = {
        "18_parameter_directories": len(tables) == 18 and len(seeds) == 18,
        "table_sources_pinned": bool(tables) and all(
            hashlib.sha256(path.read_bytes()).hexdigest() == EXPECTED_TABLE_SHA256
            for path in tables
        ),
        "secret_indexed_round_tables": bool(tables) and all(
            "Te0[ ( (Y0)       ) & 0xFF ]" in path.read_text()
            and "sbox[ ( Y0   ) & 0xFF ]" in path.read_text()
            for path in tables
        ),
        "secret_seed_reaches_cipher_plaintext": bool(seeds) and all(
            "sc_encrypt_x2_x2(&ctx->enc_ctx_x2, seed, seed" in path.read_text()
            for path in seeds
        ),
        "harness_selects_table_backend": makefile.count("-DRIJNDAEL_TABLE") == 18,
        "submitted_ct64_alternative_present": len(list(impl.glob("*/rijndael/rijndael_ct64.c"))) == 18,
    }
    result = {
        "finding": "MQOM-1",
        "evidence": "source-level root-cause witness; no timing measurement or key recovery",
        "table_source_sha256": EXPECTED_TABLE_SHA256,
        "table_source_copies": len(tables),
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded)
    print(encoded, end="")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
