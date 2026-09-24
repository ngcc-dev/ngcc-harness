#!/usr/bin/env python3
"""Import source or legal notices from a SHA-256-verified official ZIP.

The archive is downloaded with IDS=<id> ./download.sh and is never executed.
Select an implementation tree explicitly, for example:

    python3 performance/import_sources.py kem-22 Implementations/Optimized_Implementation

Use --all-source for the software-source inventory and --notices for package
license/copyright/notice files. Existing files are never overwritten. The
checkout retains original archive-relative paths for provenance checks.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import re
import stat
import sys
import zipfile


ROOT = Path(__file__).resolve().parent.parent
SOURCE_SUFFIXES = {
    ".c", ".cc", ".cpp", ".cu", ".cuh", ".h", ".hh", ".hpp", ".hxx",
    ".s", ".asm", ".i", ".inc", ".inl", ".ipp", ".tcc", ".macros",
    ".py", ".sage", ".rs", ".jl", ".m", ".cl", ".f90",
    ".ld",
}


def expected_sha256(candidate: str) -> str:
    text = (ROOT / "SOURCE_ARCHIVES.md").read_text(encoding="utf-8")
    match = re.search(
        rf"^\|\s*{re.escape(candidate)}\s*\|[^|]*\|\s*`([0-9a-f]{{64}})`",
        text,
        re.MULTILINE,
    )
    if match is None:
        raise ValueError(f"{candidate}: no SHA-256 in SOURCE_ARCHIVES.md")
    return match.group(1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("candidate", help="candidate ID, e.g. kem-22")
    parser.add_argument("prefix", nargs="?", help="archive-relative implementation tree")
    parser.add_argument("--all-source", action="store_true",
                        help="import every source/include file from the verified submission ZIP")
    parser.add_argument("--notices", action="store_true",
                        help="import package license, copyright and notice files")
    parser.add_argument("--check", action="store_true", help="list without importing")
    parser.add_argument("--quiet", action="store_true", help="print only the summary")
    args = parser.parse_args()
    if not re.fullmatch(r"(?:sign|kem|kex|hash)-\d{2}", args.candidate):
        parser.error("invalid candidate ID")
    if sum((bool(args.prefix), args.all_source, args.notices)) != 1:
        parser.error("choose exactly one of PREFIX, --all-source and --notices")
    prefix = PurePosixPath(args.prefix.rstrip("/")) if args.prefix else None
    if prefix and (prefix.is_absolute() or ".." in prefix.parts or
                   not prefix.parts or not prefix.parts[0].startswith("Implementation")):
        parser.error("prefix must be an Implementation.../ path without ..")
    archive = ROOT / "orig" / args.candidate / "orig.zip"
    if not archive.is_file():
        parser.error(f"download {args.candidate} first: IDS={args.candidate} ./download.sh")
    with archive.open("rb") as source_file:
        digest = hashlib.file_digest(source_file, "sha256").hexdigest()
    if digest != expected_sha256(args.candidate):
        parser.error(f"archive SHA-256 mismatch: {digest}")
    selected = []
    with zipfile.ZipFile(archive) as source:
        for member in source.infolist():
            path = PurePosixPath(member.filename)
            is_notice = path.name.lower().startswith(
                ("license", "licence", "copying", "notice", "copyright"))
            if (member.is_dir() or path.is_absolute() or ".." in path.parts or
                    (prefix is not None and path.parts[:len(prefix.parts)] != prefix.parts) or
                    (not is_notice if args.notices else path.suffix.lower() not in SOURCE_SUFFIXES)):
                continue
            if stat.S_ISLNK(member.external_attr >> 16):
                raise ValueError(f"archive symlink refused: {member.filename}")
            if member.file_size > 100_000_000:
                raise ValueError(f"source file exceeds 100 MB: {member.filename}")
            destination = ROOT / args.candidate / Path(*path.parts)
            payload = source.read(member)
            if args.notices and b"\0" in payload:
                raise ValueError(f"notice appears binary: {member.filename}")
            if destination.exists():
                if destination.read_bytes() != payload:
                    raise ValueError(f"existing file differs; refusing overwrite: {destination}")
                action = "same"
            elif args.check:
                action = "new"
            else:
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(payload)
                action = "added"
            selected.append((action, path, len(payload)))
    if not selected:
        raise ValueError(f"no source files under {prefix or 'archive'} in {archive}")
    if not args.quiet:
        for action, path, _ in selected:
            print(f"{action:5} {args.candidate}/{path}")
    kind = "notices" if args.notices else "source files"
    print(f"{args.candidate}: {len(selected)} {kind}, {sum(size for _, _, size in selected)} bytes; "
          f"{sum(action == 'same' for action, _, _ in selected)} already present; "
          f"archive SHA-256 {digest}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        sys.exit(f"import_sources: {error}")
