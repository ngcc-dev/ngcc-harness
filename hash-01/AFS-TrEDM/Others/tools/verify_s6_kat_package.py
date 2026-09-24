#!/usr/bin/env python3
"""Verify the submitted flat AFS-TrEDM S6 Test_Vectors package."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


INSTANCES = ("512", "768", "1024")
REQUIRED_KINDS_BY_MODE = {
    "quick": ("KAT_2_12",),
    "medium": ("KAT_2_12", "KAT_2_23"),
    "full": ("KAT_2_12", "KAT_2_23", "KAT_2_33", "KAT_Loop"),
}
EXPECTED_RECORDS = {
    "KAT_2_12": 4097,
    "KAT_2_23": 3,
    "KAT_2_33": 3,
    "KAT_Loop": 1,
}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fp:
        for chunk in iter(lambda: fp.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def count_records(text: str) -> int:
    return len([part for part in text.strip().split("\n\n") if part.strip()])


def verify_kat_file(path: Path, digest_bits: int, expected_records: int) -> dict[str, object]:
    text = path.read_text(encoding="utf-8")
    digests = re.findall(r"^Dst = ([0-9A-F]+)$", text, flags=re.MULTILINE)
    record_count = count_records(text)
    digest_count = len(digests)
    digest_len_ok = all(len(digest) * 4 == digest_bits for digest in digests)
    return {
        "path": str(path),
        "exists": True,
        "records": record_count,
        "expected_records": expected_records,
        "digest_count": digest_count,
        "digest_bits": digest_bits,
        "digest_len_ok": digest_len_ok,
        "sha256": sha256_file(path),
        "status": "PASS" if record_count == expected_records and digest_count == expected_records and digest_len_ok else "FAIL",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--mode", choices=("quick", "medium", "full"), default="full")
    parser.add_argument("--json-out")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    tv_root = root / "Test_Vectors"
    findings: list[dict[str, str]] = []
    instances: dict[str, object] = {}

    if not tv_root.is_dir():
        findings.append({"path": "Test_Vectors", "reason": "missing Test_Vectors directory"})
    else:
        required_kinds = REQUIRED_KINDS_BY_MODE[args.mode]
        for inst in INSTANCES:
            inst_name = f"AFS-TrEDM-{inst}"
            file_results: list[dict[str, object]] = []
            for kind in required_kinds:
                path = tv_root / f"{kind}_{inst_name}.txt"
                if not path.is_file():
                    findings.append({"path": str(path.relative_to(root)), "reason": "missing required KAT file"})
                    file_results.append({"path": str(path), "exists": False, "status": "FAIL"})
                    continue
                result = verify_kat_file(path, int(inst), EXPECTED_RECORDS[kind])
                file_results.append(result)
                if result["status"] != "PASS":
                    findings.append({"path": str(path.relative_to(root)), "reason": "KAT structure or digest-length check failed"})
            instances[inst_name] = {"files": file_results}

        # Flag stale old-layout directories, while allowing the flat submission files.
        stale_dirs = sorted(
            p.name for p in tv_root.iterdir()
            if p.is_dir() and (p.name.startswith("AFS-TrEDM-") or p.name.startswith("AFS-TrEDM-S6-"))
        )
        if stale_dirs:
            findings.append({"path": "Test_Vectors", "reason": "old directory-based KAT layout remains: " + ",".join(stale_dirs)})

    result = {
        "status": "PASS" if not findings else "FAIL",
        "mode": args.mode,
        "layout": "flat-submission",
        "finding_count": len(findings),
        "findings": findings,
        "instances": instances,
    }
    text = json.dumps(result, indent=2, sort_keys=True)
    print(text)
    if args.json_out:
        Path(args.json_out).write_text(text + "\n", encoding="utf-8")
    return 0 if not findings else 1


if __name__ == "__main__":
    raise SystemExit(main())
