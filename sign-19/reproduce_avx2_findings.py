#!/usr/bin/env python3
"""Source certificates for Phoenix AVX2 length and public-seed defects."""

import argparse
import hashlib
import io
import re
import sys
import urllib.request
import zipfile
from pathlib import Path


URL = ("https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20"
       "Algorithms/Round%201%20candidates/Phoenix.zip")
SHA256 = "ec801791976a10baffa852c479afd12362af3b5fb6420104254fb48814d99918"
ROOT = Path(__file__).resolve().parent / "Implementations and Test_Vectors/Implementations"
OPT = "Optimized_Implementation/avx2/"
REF = "Reference_Implementation/"


def get_reader(archive_path: Path | None):
    probe = ROOT / OPT / "Phoenix-SM3-128f-avx2/hash_sm3.c"
    if archive_path is None and probe.is_file():
        return lambda name: (ROOT / name).read_text()
    try:
        data = archive_path.read_bytes() if archive_path else urllib.request.urlopen(URL, timeout=30).read()
    except (OSError, TimeoutError) as exc:
        print(f"SKIP sign-19 AVX2 findings: official archive unavailable: {exc}")
        return None
    assert hashlib.sha256(data).hexdigest() == SHA256, "unexpected Phoenix archive"
    archive = zipfile.ZipFile(io.BytesIO(data))
    names = archive.namelist()

    def read(name: str) -> str:
        suffix = "Implementations/" + name
        hits = [member for member in names if member.endswith(suffix)]
        assert len(hits) == 1, (name, hits)
        return archive.read(hits[0]).decode("utf-8", "replace")

    return read


def length(read) -> None:
    name = "Phoenix-SHAKE-384f"
    optimized = read(OPT + name + "-avx2/params/params-phoenix-shake-384f.h")
    reference = read(REF + name + "/params/params-phoenix-shake-384f.h")
    signer = read(OPT + name + "-avx2/SIG_AlgorithmInstance.c")
    api = read(OPT + name + "-avx2/api.h")
    assert "#define SPX_N 48" in optimized
    assert "#define SPX_TFORS_SIG_MAX 23040" in optimized
    assert 23040 % 48 == 0
    opt_line = next(line for line in optimized.splitlines() if line.startswith("#define SPX_BYTES"))
    ref_line = next(line for line in reference.splitlines() if line.startswith("#define SPX_BYTES"))
    assert "COUNTER_SIZE + SPX_TFORS_SIG_MAX" in opt_line
    assert "COUNTER_SIZE + 2 + SPX_TFORS_SIG_MAX" in ref_line
    assert "ull_to_bytes(sig, 2, tfors_siglen)" in signer
    assert "SPX_N + COUNTER_SIZE + 2 +" in signer
    assert "#define CRYPTO_BYTES SPX_BYTES" in api
    print("CONFIRMED sign-19-2: reachable maximum signature exceeds declared buffer by 2 bytes")


def seed(read) -> None:
    count = 0
    for level in (128, 192, 256, 384, 512):
        for speed in ("f", "s"):
            stem = f"Phoenix-SM3-{level}{speed}"
            opt = OPT + stem + "-avx2/"
            ref = REF + stem + "/"
            thash = read(opt + "thash_sm3_simple.c")
            prf = read(opt + "hash_sm3.c")
            ref_thash = read(ref + "thash_sm3_simple.c")
            ref_prf = read(ref + "hash_sm3.c")
            thash_body = thash.split("void thash(", 1)[1].split("void thash_init_bitmask(", 1)[0]
            prf_body = prf.split("void prf_addr(", 1)[1].split("void gen_message_random(", 1)[0]
            assert "ctx->state_seeded_sm3" in thash_body
            assert "sm3_xof(out, SPX_N, buf," in thash_body
            assert "sm3_inc_finalize" not in thash_body
            assert "sm3_xof(out, SPX_N, buf," in prf_body
            assert "ctx->state_seeded_sm3" not in prf_body
            assert "ctx->state_seeded_sm3" in ref_thash
            assert "ctx->state_seeded_sm3" in ref_prf
            count += 1
    assert count == 10
    print("CONFIRMED sign-19-3: scalar AVX2 SM3 thash/PRF omit the seeded state in all 10 sets")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report-id", required=True, choices=("sign-19-2", "sign-19-3"))
    parser.add_argument("--archive", type=Path, help="offline official Phoenix.zip")
    args = parser.parse_args()
    read = get_reader(args.archive)
    if read is None:
        return 77
    (length if args.report_id == "sign-19-2" else seed)(read)
    return 0


if __name__ == "__main__":
    sys.exit(main())
