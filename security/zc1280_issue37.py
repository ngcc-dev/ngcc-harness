#!/usr/bin/env python3
"""Replay the hash-pinned ZC-1280 certificates and compare native full hashes.

The published four-round collisions are reduced-round witnesses. The reported
2^144 full-round cost assumes an unverified nine-round tail probability.
"""

import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile


ROOT = Path(__file__).resolve().parent.parent
URL = "https://github.com/user-attachments/files/34259097/zc1280_programs.zip"
SHA256 = "0e40a8fee81018d0db9b50d84f4811f7e70651260485a731e2f63cfb5e4aa88d"
MODES = (("DM", "hash-30"), ("DMC", "hash-31"), ("EDMC", "hash-32"))


def main():
    local = os.environ.get("NGCC_ZC1280_ARCHIVE")
    try:
        archive = Path(local).read_bytes() if local else urllib.request.urlopen(URL, timeout=30).read()
    except (OSError, urllib.error.URLError) as exc:
        print(f"SKIP: ZC-1280 attachment unavailable: {exc}", file=sys.stderr)
        raise SystemExit(77) from exc
    if hashlib.sha256(archive).hexdigest() != SHA256:
        raise RuntimeError("ZC-1280 attachment SHA-256 mismatch")

    with tempfile.TemporaryDirectory(prefix="ngcc-zc1280-") as temp:
        path = Path(temp)
        bundle = path / "attachment.zip"
        bundle.write_bytes(archive)
        with zipfile.ZipFile(bundle) as z:
            for info in z.infolist():
                dest = Path(info.filename)
                if dest.is_absolute() or ".." in dest.parts or info.is_dir() and info.filename == "":
                    raise RuntimeError("Unsafe ZC-1280 archive member")
                if (info.external_attr >> 16) & 0o170000 == 0o120000:
                    raise RuntimeError("ZC-1280 archive contains a symlink")
            z.extractall(path)
        source = path / "zc1280_programs"
        witness = source / "reproduce.py"
        if not witness.is_file():
            raise RuntimeError("ZC-1280 reproducer missing")
        result = subprocess.run(
            [sys.executable, "-B", str(witness), "--upstream", str(ROOT)],
            check=True, capture_output=True, text=True, timeout=180,
            cwd=path,
        )
        if "ALL EXACT CERTIFICATE CHECKS PASS" not in result.stdout:
            raise RuntimeError("ZC-1280 certificate marker missing")
        if result.stdout.count("actual 4-round collision PASS") != 3:
            raise RuntimeError("ZC-1280 reduced-collision marker missing")
        print(result.stdout, end="")

        records = {d["mode"]: d for d in json.loads((source / "reduced_collisions.json").read_text())}
        for mode, candidate in MODES:
            package = ROOT / candidate / ("ZC-" + mode) / "Implementations"
            ref = package / "Implementations" / "Reference_Implementation" / ("ZC-" + mode + "-1280-512") / "CryptHash_AlgorithmInstance.c"
            primitive = package / "lib" / "low" / "ZuD-1280" / "plain" / "ZuD1280-plain.c"
            library = path / (mode + ".so")
            subprocess.run(
                ["cc", "-O2", "-shared", "-fPIC", "-I" + str(package / "lib" / "common"),
                 "-o", str(library), str(ref), str(primitive)],
                check=True, capture_output=True, text=True, timeout=60,
            )
            f = ctypes.CDLL(str(library)).CryptHash
            f.argtypes = (ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p)
            f.restype = ctypes.c_int
            record = records[mode]
            for message_key, digest_key in (("message_hex", "full12_digest_hex"),
                                            ("message_prime_hex", "full12_digest_prime_hex")):
                message = bytes.fromhex(record[message_key])
                digest = ctypes.create_string_buffer(64)
                if f(512, ctypes.c_char_p(message), len(message) * 8, digest) != 0:
                    raise RuntimeError(f"{mode}: native CryptHash failed")
                if digest.raw.hex() != record[digest_key]:
                    raise RuntimeError(f"{mode}: native full12 digest mismatch")
            print(f"{mode}-1280-512: submitted native full12 digest agreement PASS")
    print("ZC-1280 exact and native checks PASS; full12 collision and tail model unproved")


if __name__ == "__main__":
    main()
