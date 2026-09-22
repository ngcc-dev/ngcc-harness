#!/usr/bin/env python3
"""Reproduce the public MoFang full-round collision."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent
INSTANCES = {
    "MoFang-256": 32,
    "MoFang-512": 64,
    "MoFang-768": 96,
    "MoFang-1024": 128,
    "MoFang-256-XOF": 32,
    "MoFang-768-XOF": 96,
}


def digest(instance: str, size: int, message: bytes) -> bytes:
    library = ctypes.CDLL(str(ROOT / "lib" / f"lib{instance}.so"))
    function = library.CryptHash
    function.argtypes = (
        ctypes.c_int,
        ctypes.c_void_p,
        ctypes.c_ulonglong,
        ctypes.c_void_p,
    )
    function.restype = ctypes.c_int
    source = (ctypes.c_ubyte * len(message)).from_buffer_copy(message)
    output = (ctypes.c_ubyte * size)()
    if function(8 * size, source, 8 * len(message), output) != 0:
        raise RuntimeError(f"{instance}: CryptHash failed")
    return bytes(output)


def mate(message: bytes, mask: int = 0xFF) -> bytes:
    result = bytearray(message)
    for offset in (*range(40, 48), *range(112, 120)):
        result[offset] ^= mask
    return bytes(result)


def main() -> None:
    # The published zero-message witness, plus a nonzero base-message control
    # showing that the rule constructs a second preimage for an arbitrary
    # 960-bit message rather than exploiting an all-zero special case.
    messages = (bytes(120), bytes(range(120)))
    expected_512 = (
        "ebbc07cddba0f381917c62cf0954fd63eb3e162065f61b14baef1a8d09982e5f"
        "bbd9f6ce5aac1e7d36d979126e11358c0635f4a8d2bda29e037ed733e1a9aca4"
    )
    for instance, size in INSTANCES.items():
        for message in messages:
            other = mate(message)
            assert message != other
            left = digest(instance, size, message)
            right = digest(instance, size, other)
            assert left == right, f"{instance}: collision did not reproduce"
        if instance == "MoFang-512":
            assert digest(instance, size, messages[0]).hex() == expected_512
        print(f"{instance}: COLLISION")

    # A complete 1152-bit block has two independently variable untouched
    # words. The four two-periodic masks for each produce 16 messages with one
    # MoFang-512 digest, confirming the reported multicollision extension.
    multicollision = []
    for first in (0x00, 0xFF, 0xAA, 0x55):
        for second in (0x00, 0xFF, 0xAA, 0x55):
            message = bytearray(range(144))
            for start, mask in ((40, first), (112, first),
                                (56, second), (128, second)):
                for offset in range(start, start + 8):
                    message[offset] ^= mask
            multicollision.append(digest("MoFang-512", 64, bytes(message)))
    assert len(set(multicollision)) == 1
    print("MoFang-512: 16-MESSAGE MULTICOLLISION")


if __name__ == "__main__":
    main()
