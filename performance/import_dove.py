#!/usr/bin/env python3
"""Import DOVE source from the two RARs inside its verified NICCS ZIP.

Only source/include members are copied. The nested RARs and their submitted
build artifacts are not retained; bsdtar is needed only for this import step.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import subprocess
import tempfile
import zipfile

from import_sources import ROOT, SOURCE_SUFFIXES, expected_sha256


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify without copying")
    args = parser.parse_args()
    archive = ROOT / "orig/sign-09/orig.zip"
    with archive.open("rb") as source:
        digest = hashlib.file_digest(source, "sha256").hexdigest()
    if digest != expected_sha256("sign-09"):
        parser.error("DOVE submission ZIP SHA-256 mismatch")
    selected = 0
    added = 0
    missing = 0
    with zipfile.ZipFile(archive) as outer, tempfile.TemporaryDirectory(prefix="ngcc-dove-") as scratch:
        rar_members = [name for name in outer.namelist()
                       if name.startswith("Implementations/") and name.endswith(".rar")]
        if len(rar_members) != 2:
            parser.error(f"expected two submitted RARs, found {len(rar_members)}")
        for index, rar_member in enumerate(sorted(rar_members)):
            rar = Path(scratch) / f"{index}.rar"
            rar.write_bytes(outer.read(rar_member))
            names = subprocess.check_output(["bsdtar", "-tf", str(rar)], text=True).splitlines()
            for name in names:
                member = PurePosixPath(name)
                if member.suffix.lower() not in SOURCE_SUFFIXES:
                    continue
                if member.is_absolute() or ".." in member.parts or len(member.parts) < 2:
                    parser.error(f"unsafe RAR member: {name}")
                source_bytes = subprocess.check_output(["bsdtar", "-xOf", str(rar), name])
                if len(source_bytes) > 100_000_000:
                    parser.error(f"oversized RAR member: {name}")
                destination = ROOT / "sign-09/Implementations" / Path(*member.parts)
                selected += 1
                if destination.exists():
                    if destination.read_bytes() != source_bytes:
                        parser.error(f"existing source differs: {destination}")
                else:
                    missing += 1
                    if not args.check:
                        destination.parent.mkdir(parents=True, exist_ok=True)
                        destination.write_bytes(source_bytes)
                        added += 1
    print(f"sign-09: {selected} nested-RAR source files, {missing} previously missing, "
          f"{added} added; ZIP SHA-256 {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
