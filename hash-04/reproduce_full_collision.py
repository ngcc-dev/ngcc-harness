#!/usr/bin/env python3
"""Check Sunan Wang's short collision and compact long-pair identity.

The unlicensed attack package is not redistributed here. Set
NGCC_CHAMP_COLLISION_PACKAGE to a local copy for an offline check.
"""

import ctypes
import hashlib
import json
import os
import sys
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parent
COMMIT = "cfefda841c614c825f61689cb51bc8a7a7751c9c"
BASE = f"https://raw.githubusercontent.com/snipernan/champ512-collisions/{COMMIT}"
EXPECTED = {
    "short_a.bin": "34bb30a62e04298b5d97cd608840afdb7084b2c0f848a90d4b5ff99a546dd805",
    "short_b.bin": "fd34962528513e9044880943b106b81188ff0526b1c7d7bbbb987505ef42c822",
}
DIGEST = "571fecc332116f68581aa805285a92c52d57a908eca17d83f5ed59d1629c751137a0dc498f73881e2bdb8533f24a63379b2a75d8cd23ab7aff47e37cb89f7c80"
LONG_JSON_SHA256 = "17b8689568c2e2d11f3ad8b0ec6378ed4d0f334e81b4bd4b60645a98a96e7c6c"
P = (1 << 128) - 15449
A = (-2, 1, 4, -3)
B = (-5, 2, -6, 2)


def require(condition, reason):
    if not condition:
        raise SystemExit(f"NOT REPRODUCED hash-04-6: {reason}")


def obtain(path, expected_sha256):
    local = os.environ.get("NGCC_CHAMP_COLLISION_PACKAGE")
    if local:
        source = Path(local) / path
        require(source.is_file(), f"local package lacks {path}")
        data = source.read_bytes()
    else:
        request = urllib.request.Request(f"{BASE}/{path}", headers={"User-Agent": "NGCC-harness/1.0"})
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                data = response.read()
        except Exception as error:
            print(f"SKIP hash-04-6: pinned collision file unavailable: {error}")
            sys.exit(77)
    require(hashlib.sha256(data).hexdigest() == expected_sha256, f"{path} SHA-256 differs")
    return data


def multiply(left, right):
    a, b, c, d = left
    e, f, g, h = right
    return ((a * e + b * g) % P, (a * f + b * h) % P,
            (c * e + d * g) % P, (c * f + d * h) % P)


def power(matrix, exponent):
    result = (1, 0, 0, 1)
    while exponent:
        if exponent & 1:
            result = multiply(result, matrix)
        matrix = multiply(matrix, matrix)
        exponent >>= 1
    return result


def word_product(bits):
    result = (1, 0, 0, 1)
    for bit in bits:
        require(bit in "01", "invalid bit in long-pair certificate")
        result = multiply(result, B if bit == "1" else A)
    return result


def encoded_digest(matrix):
    a, b, c, d = matrix
    return b"".join((pow(x, -1, P) if x else 0).to_bytes(16, "little")
                    for x in (a, c, b, d)).hex()


def check_long_identity():
    raw = obtain("data/long_collision.json", LONG_JSON_SHA256)
    data = json.loads(raw)
    require(int(data["prime"]) == P, "long-pair prime differs")
    require(data["generators"]["0"] == [[A[0], A[1]], [A[2], A[3]]]
            and data["generators"]["1"] == [[B[0], B[1]], [B[2], B[3]]],
            "long-pair generators differ")
    w = data["decoded_word"]
    count = data["append_ones"]
    length = data["shear_length_bits"]
    require(len(w) + 1 + count == length and 3 * length + 2 == data["bit_length"],
            "long-pair lengths differ")
    require(w != "1" + w[:-1], "long-pair message templates are not distinct")
    shear = multiply(multiply(word_product(w), A), power(B, count))
    first = multiply(multiply(multiply(shear, shear), power(B, 2)), shear)
    second = multiply(multiply(multiply(multiply(B, shear), shear), shear), B)
    require(first == second, "long-pair original matrix products differ")
    require(encoded_digest(first) == data["digest_hex"], "long-pair modeled digest differs")
    print("STRUCTURAL hash-04-6: long-pair matrix products and modeled digest agree; 53 MB messages not hashed")


def matrix_product(data):
    a, b, c, d = 1, 0, 0, 1
    for byte in data:
        for shift in range(7, -1, -1):
            e, f, g, h = B if (byte >> shift) & 1 else A
            a, b, c, d = ((a * e + b * g) % P, (a * f + b * h) % P,
                          (c * e + d * g) % P, (c * f + d * h) % P)
    return a, b, c, d


def main():
    library = ROOT / "lib/libCHAMP-512.so"
    if not library.is_file():
        print("SKIP hash-04-6: build the frozen library with make -C hash-04")
        return 77
    a = obtain("messages/short_a.bin", EXPECTED["short_a.bin"])
    b = obtain("messages/short_b.bin", EXPECTED["short_b.bin"])
    require(len(a) == len(b) == 41_800, "short-pair lengths differ")
    require(a != b, "short-pair messages are identical")
    require(matrix_product(a) == matrix_product(b), "short-pair matrix products differ")

    function = ctypes.CDLL(str(library)).CryptHash
    function.argtypes = (ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p)
    function.restype = ctypes.c_int

    def digest(data):
        source = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        output = (ctypes.c_ubyte * 64)()
        require(function(512, source, len(data) * 8, output) == 0,
                "submitted CHAMP-512 library rejected an input")
        return bytes(output).hex()

    require(digest(a) == digest(b) == DIGEST, "short-pair complete digests differ")
    changed = bytes([a[0] ^ 0x80]) + a[1:]
    require(digest(changed) != DIGEST, "changed-bit control still collides")
    check_long_identity()
    print("CONFIRMED hash-04-6: distinct 334400-bit inputs have equal original matrix products and complete CHAMP-512 digests; changed-bit control rejected")
    return 0


if __name__ == "__main__":
    sys.exit(main())
