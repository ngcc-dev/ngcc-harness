#!/usr/bin/env python3
"""Compare pinned PolarLAC reference and optimized KAT records."""

import argparse
import hashlib
import io
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path


URL = ("https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20"
       "Algorithms/Round%201%20candidates/PolarLAC.zip")
SHA256 = "906be26ce8d27de335b691ff8490b2344b46d7519d88c799cba0242324e4882c"
LEVELS = ("Light", "128", "256", "512", "512-Star")


def record(text: str) -> dict[str, str]:
    fields = {}
    for line in text.splitlines():
        if line.startswith("Count = 1") and fields:
            break
        if " = " in line:
            key, value = line.split(" = ", 1)
            fields[key.strip().lower()] = value.strip()
    return fields


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, help="offline official PolarLAC.zip")
    args = parser.parse_args()
    try:
        if args.archive:
            data = args.archive.read_bytes()
        else:
            try:
                data = urllib.request.urlopen(URL, timeout=30).read()
            except OSError:
                data = subprocess.run(["curl", "-fsSL", "--max-time", "40", URL],
                                      check=True, capture_output=True).stdout
    except (OSError, TimeoutError, subprocess.CalledProcessError) as exc:
        print(f"SKIP kem-30-3: official archive unavailable: {exc}")
        return 77
    assert hashlib.sha256(data).hexdigest() == SHA256, "unexpected PolarLAC archive"
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        names = archive.namelist()
        for level in LEVELS:
            found = []
            for kind in ("Reference_Implementation", "Optimized_Implementation"):
                matches = [name for name in names
                           if kind in name and "x86" in name
                           and name.lower().endswith(f"kat_kem_polarlac-{level}.txt".lower())]
                assert len(matches) == 1, (level, kind, matches)
                found.append(record(archive.read(matches[0]).decode("ascii")))
            ref, opt = found
            for field in ("seed", "pk", "ss"):
                assert field in ref and field in opt, (level, field)
            assert ref["seed"] == opt["seed"], level
            assert ref["pk"] != opt["pk"] and ref["ss"] != opt["ss"], level
            arm_matches = [name for name in names
                           if "Optimized_Implementation/ARM/" in name
                           and name.lower().endswith(f"kat_kem_polarlac-{level}.txt".lower())]
            assert len(arm_matches) == 1, (level, "ARM", arm_matches)
            arm = record(archive.read(arm_matches[0]).decode("ascii"))
            assert arm["seed"] == ref["seed"], level
            assert arm["pk"] == ref["pk"] and arm["ss"] == ref["ss"], level
    print("CONFIRMED kem-30-3: x86 optimized KAT differs; ARM matches reference in all five sets")
    return 0


if __name__ == "__main__":
    sys.exit(main())
