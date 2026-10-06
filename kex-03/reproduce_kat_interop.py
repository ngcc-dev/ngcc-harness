#!/usr/bin/env python3
"""Verify kex-03-6 against the SHA-256-pinned CreTAKE KAT archive."""
import argparse
import hashlib
import io
import subprocess
import sys
import urllib.error
import urllib.request
import zipfile
from pathlib import Path


URL = "https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/CreTAKE.zip"
SHA256 = "0b356074741bc20fa82132719e1678e001083b9746f5c4c582425b727b511741"
PREFIX = "Test_Vectors/"


def first_field(data: bytes, label: str) -> str:
    for line in data.decode("ascii").splitlines():
        if line.startswith(label + " = "):
            return line.partition(" = ")[2]
    raise AssertionError(f"missing {label} field")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, help="offline path to the official CreTAKE.zip")
    args = parser.parse_args()
    try:
        if args.archive:
            data = args.archive.read_bytes()
        else:
            try:
                data = urllib.request.urlopen(URL, timeout=60).read()
            except OSError:
                data = subprocess.run(["curl", "-fsSL", "--max-time", "70", URL],
                                      check=True, capture_output=True).stdout
    except (OSError, TimeoutError, subprocess.CalledProcessError) as exc:
        print(f"SKIP kex-03-6: official archive unavailable: {exc}")
        return 77
    if hashlib.sha256(data).hexdigest() != SHA256:
        raise AssertionError("official archive SHA-256 mismatch")
    plac = zen = 0
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        names = set(archive.namelist())
        for name in sorted(names):
            if not name.startswith(PREFIX + "Reference_Test_Vector/KAT_KEX_CreTAKE-"):
                continue
            other = name.replace("Reference_Test_Vector/", "Optimized_Test_Vector/", 1)
            assert other in names, f"missing optimized pair for {name}"
            ref = archive.read(name)
            opt = archive.read(other)
            assert first_field(ref, "Seed") == first_field(opt, "Seed"), name
            same = first_field(ref, "SS") == first_field(opt, "SS")
            if "PLAC" in name:
                assert not same, name
                plac += 1
            else:
                assert "ZEN" in name and same, name
                zen += 1
    assert (plac, zen) == (13, 12), (plac, zen)
    print("CONFIRMED kex-03-6: 13/13 POLARLAC KAT pairs disagree; 12/12 ZEN controls agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
