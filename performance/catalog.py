#!/usr/bin/env python3
"""Inventory software source from verified official submission archives.

Run after downloading the ZIPs and before discarding the ignored archive cache:

    python3 performance/catalog.py --write

The CSV describes archived source, not successful builds or measurements.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
from pathlib import Path, PurePosixPath
import subprocess
import tempfile
import zipfile

from import_sources import ROOT, SOURCE_SUFFIXES, expected_sha256


FIELDS = ("id", "archive_sha256", "source_files", "source_bytes",
          "reference_path_files", "optimized_path_files", "additional_path_files",
          "avx_path_files", "arm_path_files", "nested_rar")


def source_paths_from_rar(data: bytes, scratch: Path, index: int):
    rar = scratch / f"{index}.rar"
    rar.write_bytes(data)
    names = subprocess.check_output(["bsdtar", "-tf", str(rar)], text=True).splitlines()
    for name in names:
        path = PurePosixPath(name)
        if path.suffix.lower() in SOURCE_SUFFIXES:
            payload = subprocess.check_output(["bsdtar", "-xOf", str(rar), name])
            yield path, len(payload)


def inventory(candidate: str) -> dict[str, str | int]:
    archive = ROOT / "orig" / candidate / "orig.zip"
    with archive.open("rb") as source:
        digest = hashlib.file_digest(source, "sha256").hexdigest()
    if digest != expected_sha256(candidate):
        raise ValueError(f"{candidate}: ZIP digest differs from SOURCE_ARCHIVES.md")
    record: dict[str, str | int] = {field: 0 for field in FIELDS}
    record.update(id=candidate, archive_sha256=digest)

    def add(path: PurePosixPath, size: int) -> None:
        lower = path.as_posix().lower()
        record["source_files"] += 1
        record["source_bytes"] += size
        for field, words in (
            ("reference_path_files", ("reference",)),
            ("optimized_path_files", ("optimiz", "avx", "neon")),
            ("additional_path_files", ("additional",)),
            ("avx_path_files", ("avx",)),
            ("arm_path_files", ("arm", "aarch64", "neon", "cortex")),
        ):
            if any(word in lower for word in words):
                record[field] += 1

    with zipfile.ZipFile(archive) as submitted, tempfile.TemporaryDirectory(prefix="ngcc-catalog-") as temp:
        scratch = Path(temp)
        for member in submitted.infolist():
            path = PurePosixPath(member.filename)
            if member.is_dir():
                continue
            if path.suffix.lower() in SOURCE_SUFFIXES:
                add(path, member.file_size)
            elif candidate == "sign-09" and path.suffix.lower() == ".rar":
                record["nested_rar"] += 1
                for nested_path, size in source_paths_from_rar(submitted.read(member), scratch,
                                                               record["nested_rar"]):
                    add(PurePosixPath("Implementations") / nested_path, size)
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="update the checked-in CSV")
    args = parser.parse_args()
    with (ROOT / "downloads.csv").open(newline="", encoding="utf-8") as source:
        ids = [row["ID"] for row in csv.DictReader(source, delimiter=";")]
    records = [inventory(candidate) for candidate in ids]
    output = io.StringIO()
    writer = csv.DictWriter(output, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(records)
    target = ROOT / "performance/source_catalog.csv"
    if args.write:
        target.write_text(output.getvalue(), encoding="utf-8")
    elif not target.is_file() or target.read_text(encoding="utf-8") != output.getvalue():
        raise ValueError("source_catalog.csv differs; run with --write")
    print(f"{len(records)} official archives, {sum(r['source_files'] for r in records):,} "
          f"software source files, {sum(r['source_bytes'] for r in records):,} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
