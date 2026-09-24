#!/usr/bin/env python3
"""Lightweight structural checks for long AFS-TrEDM KAT files."""

from __future__ import annotations
import argparse
import hashlib
import re
from pathlib import Path

def count_records(text: str) -> int:
    return len([x for x in text.strip().split("\n\n") if x.strip()])

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--require", action="store_true", help="fail when no long S6 KAT files are present")
    ap.add_argument("files", nargs="*", help="KAT files to check; defaults to flat Test_Vectors/KAT_2_23_*, KAT_2_33_*, and KAT_Loop_* files")
    args = ap.parse_args()

    root = Path(__file__).resolve().parents[1]
    if args.files:
        files = [Path(x) for x in args.files]
    else:
        tv = root / "Test_Vectors"
        files = (
            sorted(tv.glob("KAT_2_23_AFS-TrEDM-*.txt"))
            + sorted(tv.glob("KAT_2_33_AFS-TrEDM-*.txt"))
            + sorted(tv.glob("KAT_Loop_AFS-TrEDM-*.txt"))
        )
    if not files:
        print("No long S6 KAT files found under flat Test_Vectors.")
        return 2 if args.require else 0

    ok = True
    for path in files:
        text = path.read_text()
        recs = count_records(text)
        expected = 3 if "KAT_2_23" in path.name or "KAT_2_33" in path.name else 1
        dst_count = len(re.findall(r"^Dst = [0-9A-F]+$", text, flags=re.MULTILINE))
        sha = hashlib.sha256(path.read_bytes()).hexdigest()
        status = "OK" if recs == expected and dst_count == expected else "FAIL"
        if status != "OK":
            ok = False
        print(f"{status} {path.name}: records={recs}, digests={dst_count}, bytes={path.stat().st_size}, sha256={sha}")
    return 0 if ok else 1

if __name__ == "__main__":
    raise SystemExit(main())
