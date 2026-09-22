#!/usr/bin/env python3
"""Verify the published Facto-DSA-128 public-key forgery locally."""

from __future__ import annotations

import ctypes
import hashlib
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parent
COMMIT = "bd9d4ce38ed2b78daf57093556ef2e866bbc2264"
BASE = (
    "https://raw.githubusercontent.com/MingLLuo/facto_dsa_ngcc_round1/"
    f"{COMMIT}/results/acceptance"
)
ARTIFACTS = {
    "pk.bin": "21d196c8f0c8cf0e35eb7a268ff3743df911c8912db6f04fa045bebc03c12d8e",
    "message.bin": "f88d49cb17e28f900d14f1c9d681cbd3c9622778941f9bec390569043cad8841",
    "signature.bin": "d37f3030c4604f106d3d920f911b7d676b774dbaceb2e01d40cf24653a350f72",
}


def fetch(name: str) -> bytes:
    request = urllib.request.Request(
        f"{BASE}/{name}", headers={"User-Agent": "ngcc-facto-dsa-reproducer/1"}
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        value = response.read()
    assert hashlib.sha256(value).hexdigest() == ARTIFACTS[name]
    return value


def c_buffer(value: bytes):
    return (ctypes.c_ubyte * len(value)).from_buffer_copy(value)


def main() -> None:
    public_key = fetch("pk.bin")
    message = fetch("message.bin")
    signature = fetch("signature.bin")
    assert len(public_key) == 40040 and len(signature) == 40

    library = ctypes.CDLL(str(ROOT / "lib" / "libFacto-DSA-128.so"))
    verify = library.sig_verify
    verify.argtypes = (
        ctypes.c_void_p,
        ctypes.c_ulonglong,
        ctypes.c_void_p,
        ctypes.c_ulonglong,
        ctypes.c_void_p,
        ctypes.c_ulonglong,
    )
    verify.restype = ctypes.c_int

    def check(candidate: bytes) -> int:
        return verify(
            c_buffer(public_key),
            len(public_key),
            c_buffer(candidate),
            len(candidate),
            c_buffer(message),
            len(message),
        )

    accepted = check(signature)
    flipped = bytearray(signature)
    flipped[0] ^= 1
    rejected = check(bytes(flipped))
    assert accepted == 0 and rejected != 0
    print(f"Facto-DSA-128: forged signature rc={accepted} (ACCEPT)")
    print(f"Facto-DSA-128: flipped-bit control rc={rejected} (REJECT)")
    print(f"artifacts: MingLLuo/facto_dsa_ngcc_round1@{COMMIT}")


if __name__ == "__main__":
    main()
