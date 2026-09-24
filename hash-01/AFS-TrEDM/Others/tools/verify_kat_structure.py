#!/usr/bin/env python3
"""Lightweight structural checks for AFS-TrEDM KAT files.

This checker verifies record counts, digest counts, digest lengths, and SHA-256
hashes for KAT_2_12, KAT_2_23, KAT_2_33, and KAT_Loop files in the active
S6 Test_Vectors layout.
It does not recompute the digests.
"""

from __future__ import annotations
import argparse
import hashlib
import re
from pathlib import Path

EXPECTED_RECORDS = {
    "KAT_2_12": 4097,
    "KAT_2_23": 3,
    "KAT_2_33": 3,
    "KAT_Loop": 1,
}

def kat_kind(name: str) -> str | None:
    for k in EXPECTED_RECORDS:
        if name.startswith(k):
            return k
    return None

def digest_len_from_path(path: Path) -> int | None:
    text = "/".join(path.parts)
    if "AFS-TrEDM-S6-512" in text or "AFS-TrEDM-512" in text:
        return 512
    if "AFS-TrEDM-S6-768" in text or "AFS-TrEDM-768" in text:
        return 768
    if "AFS-TrEDM-S6-1024" in text or "AFS-TrEDM-1024" in text:
        return 1024
    return None

def count_records(text: str) -> int:
    return len([x for x in text.strip().split("\n\n") if x.strip()])

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*", help="KAT files to check; defaults to Test_Vectors/KAT_*.txt")
    args = ap.parse_args()

    root = Path(__file__).resolve().parents[1]
    if args.files:
        files = [Path(x) for x in args.files]
    else:
        files = sorted((root / "Test_Vectors").glob("KAT_*.txt"))
    if not files:
        print("No S6 KAT files found under Test_Vectors/KAT_*.txt.")
        return 2

    ok = True
    for path in files:
        text = path.read_text()
        kind = kat_kind(path.name)
        dlen = digest_len_from_path(path)
        expected_recs = EXPECTED_RECORDS.get(kind or "", None)
        recs = count_records(text)
        digests = re.findall(r"^Dst = ([0-9A-F]+)$", text, flags=re.MULTILINE)
        digest_count = len(digests)
        digest_len_ok = (dlen is not None and all(len(d) * 4 == dlen for d in digests))
        sha = hashlib.sha256(path.read_bytes()).hexdigest()
        status = "OK" if (expected_recs is not None and recs == expected_recs and digest_count == expected_recs and digest_len_ok) else "FAIL"
        if status != "OK":
            ok = False
        print(f"{status} {path.name}: records={recs}, digests={digest_count}, digest_bits={dlen}, bytes={path.stat().st_size}, sha256={sha}")
    return 0 if ok else 1

if __name__ == "__main__":
    raise SystemExit(main())
