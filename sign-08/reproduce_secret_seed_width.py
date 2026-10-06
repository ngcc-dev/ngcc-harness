#!/usr/bin/env python3
"""Certify the DARTS-512 AVX2 secret-sampling seed width."""

import argparse
import hashlib
import io
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path


URL = ("https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20"
       "Algorithms/Round%201%20candidates/DARTS.zip")
SHA256 = "1846cfe63f0cef83e2e0ca21f5dcadce3c4b16da33713957be6d156e2a9e6e95"
OPT = "Implementations/Optimized_Implementation/DARTS_AVX2/DARTS512/"
REF = "Implementations/Reference_Implementation/DARTS512/"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, help="offline copy of the official DARTS.zip")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    required = [(OPT, name) for name in ("params.h", "sign.c", "poly.c", "polyvec.c", "symmetric.h")]
    required.append((REF, "poly.c"))
    if args.archive or not all((root / prefix / name).is_file() for prefix, name in required):
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
            print(f"SKIP sign-08-4: official archive unavailable: {exc}")
            return 77
        assert hashlib.sha256(data).hexdigest() == SHA256, "unexpected DARTS archive"
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            sources = {
                (prefix, name): archive.read(prefix + name).decode("utf-8", "replace")
                for prefix, name in required
            }
    else:
        sources = {
            (prefix, name): (root / prefix / name).read_text()
            for prefix, name in required
        }

    params = sources[OPT, "params.h"]
    sign = sources[OPT, "sign.c"]
    poly = sources[OPT, "poly.c"]
    vec = sources[OPT, "polyvec.c"]
    symmetric = sources[OPT, "symmetric.h"]
    reference = sources[REF, "poly.c"]

    assert "#define CRHBYTES 64" in params
    assert "#define SEEDBYTES 64" in params
    assert "seed_s  = seed_A1 + SEEDBYTES" in sign
    assert "polyveckl_ternary_p(&s0, &s1, &e, seed_s, counter)" in sign
    assert "pack_pk(pk, &A0, seed_A1)" in sign
    assert "polyveckl_ternary_p" in vec and "poly_ternary_p" in vec
    assert "void poly_ternary_p(poly *a, const uint8_t seed[CRHBYTES]" in poly
    assert "stream256_init(&state, seed, nonce)" in poly
    assert "sm3_xof_stream_init(state, seed, 32, nonce)" in symmetric
    assert "sm3_xof_stream_init(&state, seed, CRHBYTES, nonce)" in reference

    print("ATTACK sign-08-4 CONFIRMED: AVX2 DARTS-512 secret sampler ignores 32 of 64 seed bytes")
    print("BOUND sign-08-4: at most 2^256 seed-prefix candidates; full search not run")
    return 0


if __name__ == "__main__":
    sys.exit(main())
