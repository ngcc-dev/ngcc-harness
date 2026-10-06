#!/usr/bin/env python3
"""Check Tins implementation defects in the current SHA-256-pinned ZIP."""

# Covers sign-29-3, sign-29-4, sign-29-5 and sign-29-6.

import argparse
import hashlib
import io
import re
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path


URL = ("https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20"
       "Algorithms/Round%201%20candidates/Tins.zip")
SHA256 = "84affc1f7cbdeec48a7cb6df3349b5708644672f2ac12140c529e74f6d57ca21"
LEVELS = (128, 256, 512)
TREES = (("Reference_Implementation", "Tins"),
         ("Optimized_Implementation", "Tins", "_opt"),
         ("Additional_Implementation", "Tins", "_add"))


def members(archive: zipfile.ZipFile, tier: int) -> list[tuple[str, str, str]]:
    result = []
    for entry in TREES:
        tree = entry[0]
        name = entry[1] + str(tier) + (entry[2] if len(entry) == 3 else "")
        base = f"tins/Implementations/{tree}/{name}/"
        result.append(tuple(archive.read(base + file).decode("utf-8", "replace")
                            for file in (f"SIG_TINS{tier}.c", "params.h", "bavc_commit.c")))
    return result


def check(archive: zipfile.ZipFile, report_id: str) -> None:
    tiers = LEVELS if report_id == "sign-29-3" else ((128,) if report_id == "sign-29-4" else (512,))
    checked = 0
    for tier in tiers:
        for sign, params, bavc in members(archive, tier):
            if report_id == "sign-29-3":
                assert "node path[T_OPEN]" in sign
                assert re.search(r"path_size\s*=\s*\(sn_len_bytes\s*-\s*NTemp\)\s*/\s*NODE_SIZE", sign)
                assert re.search(r"memcpy\(path,\s*sn\s*\+\s*pos,\s*NODE_SIZE\s*\*\s*path_size\)", sign)
                body = sign.split("path_size = (sn_len_bytes - NTemp)", 1)[1].split("memcpy(path", 1)[0]
                assert "T_OPEN" not in body
            elif report_id == "sign-29-4":
                assert "#define LAMBDA 160" in params
                assert sign.count("sm3hash(256,") >= 2
                assert "hash_t test_h_piop;" in sign
                assert re.search(r"sm3hash\(256,[^;]+,\s*h_piop\);", sign)
                assert "h_piop[i] ^ test_h_piop[i]" in sign
            elif report_id == "sign-29-5":
                assert sign.count("memcpy(sn+pos, stmp,") >= 2
                assert "decompress_aux(stmp, aux, TAU)" in sign
            else:
                assert "#define TAU 47" in params
                assert re.search(r"for\s*\(int i\s*=\s*0;\s*i\s*\*\s*2\s*<\s*TAU", bavc)
                assert "challenge_points[i * 2 + 1]" in bavc
                assert "int challenge_points[TAU]" in bavc
                assert 2 * 23 < 47 and 2 * 23 + 1 == 47
            checked += 1
    assert checked == len(tiers) * 3
    print(f"CONFIRMED {report_id}: source mechanism in {checked} current-ZIP implementation trees")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report-id", required=True,
                        choices=tuple(f"sign-29-{i}" for i in range(3, 7)))
    parser.add_argument("--archive", type=Path, help="offline official Tins.zip")
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
        print(f"SKIP {args.report_id}: official archive unavailable: {exc}")
        return 77
    assert hashlib.sha256(data).hexdigest() == SHA256, "unexpected Tins archive"
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        check(archive, args.report_id)
    return 0


if __name__ == "__main__":
    sys.exit(main())
