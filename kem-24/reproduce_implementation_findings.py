#!/usr/bin/env python3
"""Source certificate for kem-24-2."""
import argparse
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations"


def oversized_ciphertext() -> None:
    checked = 0
    for tree in ("Reference_Implementation", "Optimized_Implementation"):
        for level in (128, 256, 512):
            base = ROOT / tree / f"scabbard{level}"
            wrapper = (base / f"KEM_scabbard{level}.c").read_text()
            verify = (base / "verify.c").read_text()
            body = wrapper.split("int kem_dec(", 1)[1].split("\n}", 1)[0]
            assert re.search(r"uint8_t\s+cmp\[SCABBARD_CIPHERTEXTBYTES\]", body)
            assert "verify(ct, cmp, ct_len_bytes)" in body
            assert "diff |= a[i] ^ b[i]" in verify
            assert re.search(r"for\s*\(i\s*=\s*0;\s*i\s*<\s*len_bytes;\s*i\+\+\)", verify)
            assert "return (-(uint64_t)diff) >> 63;" in verify
            assert "ct_len_bytes == SCABBARD_CIPHERTEXTBYTES" not in body
            checked += 1
    assert checked == 6
    print("CONFIRMED kem-24-2: all six decapsulators use the supplied length, bypassing comparison at zero and overrunning cmp when oversized")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--report-id", choices=("kem-24-2",), required=True)
    args = parser.parse_args()
    oversized_ciphertext()
