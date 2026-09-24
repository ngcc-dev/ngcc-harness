#!/usr/bin/env python3
"""Fail if Git tracks, or would normally add, bulky submission artifacts."""

from __future__ import annotations

import os
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent
ARCHIVE_SUFFIXES = (
    ".zip", ".rar", ".7z", ".tar", ".tar.gz", ".tgz",
    ".tar.bz2", ".tar.xz", ".gz", ".bz2", ".xz",
)
DOCUMENT_SUFFIXES = {".doc", ".docx", ".ppt", ".pptx", ".xls", ".xlsx"}
VECTOR_SUFFIXES = {".txt", ".rsp", ".dat", ".bin"}
BULK_DATA_SUFFIXES = VECTOR_SUFFIXES | {".json", ".csv"}
BULK_DATA_LIMIT = 1_000_000


def paths(*flags: str) -> set[str]:
    result = subprocess.check_output(
        ["git", "-C", str(ROOT), "ls-files", "-z", *flags]
    )
    return {os.fsdecode(path) for path in result.split(b"\0") if path}


def reason(path: str) -> str | None:
    lower_path = path.lower()
    suffix = Path(path).suffix.lower()
    if lower_path.endswith(ARCHIVE_SUFFIXES):
        return "submission archive"
    if suffix in DOCUMENT_SUFFIXES:
        return "office document"
    if re.search(r"(?:^|[/_. -])(?:ip|ipr|intellectual.?property|patent)[_. -]?(?:statement|declaration)",
                 lower_path):
        return "IP statement"
    if suffix == ".pdf":
        candidate = path.split("/", 1)[0]
        if path == f"{candidate}/{candidate}-spec.pdf" and re.fullmatch(
            r"(?:sign|kem|kex|hash)-\d{2}", candidate
        ):
            return None
        if re.fullmatch(r"doc/perf-(?:x86|arm|asic|fpga)\.pdf", path):
            return None
        return "non-specification PDF"
    if suffix in VECTOR_SUFFIXES and (
        re.search(r"(?:^|[_. -])kat[_. -]", Path(lower_path).name)
        or re.search(r"(?:^|/)test[_ -]?vectors?(?:/|$)", lower_path)
        or re.search(r"test[_ -]?vectors?", Path(lower_path).name)
    ):
        return "full KAT/test-vector data"
    full_path = ROOT / path
    if (suffix in BULK_DATA_SUFFIXES and not path.startswith("performance/data/")
            and full_path.is_file() and full_path.stat().st_size > BULK_DATA_LIMIT):
        return "bulk data over 1 MB"
    return None


def main() -> int:
    tracked = paths("--cached")
    addable = paths("--others", "--exclude-standard")
    failures = [(path, reason(path)) for path in sorted(tracked | addable)]
    failures = [(path, why) for path, why in failures if why is not None]
    if failures:
        for path, why in failures:
            state = "tracked" if path in tracked else "addable"
            print(f"{state}: {why}: {path}", file=sys.stderr)
        print(f"{len(failures)} file(s) must be removed from Git or ignored", file=sys.stderr)
        return 1
    print(f"publishable-file check: PASS ({len(tracked)} tracked, "
          f"{len(addable)} addable; no archives, IP statements or full KAT data)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
