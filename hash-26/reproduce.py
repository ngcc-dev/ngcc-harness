#!/usr/bin/env python3
"""Validate the public CHIME invariant-subspace witness and bounds."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def digest(message: bytes) -> bytes:
    library = ctypes.CDLL(str(ROOT / "lib" / "libCHIME-512.so"))
    function = library.CryptHash
    function.argtypes = (
        ctypes.c_int,
        ctypes.c_void_p,
        ctypes.c_ulonglong,
        ctypes.c_void_p,
    )
    function.restype = ctypes.c_int
    source = (ctypes.c_ubyte * len(message)).from_buffer_copy(message)
    output = (ctypes.c_ubyte * 64)()
    if function(512, source, 8 * len(message), output) != 0:
        raise RuntimeError("CHIME-512: CryptHash failed")
    return bytes(output)


def main() -> None:
    message = bytes.fromhex(
        "0000000000000001" * 4
        + "0000000000000002" * 4
        + "0000000000000003" * 4
        + "0000000000008001" * 3
        + "000000000000"
    )
    expected = (
        "ef3be6a284ddbfa8" * 4 + "36f4af87fc2dc4fa" * 4
    )
    result = digest(message)
    assert len(message) == 126
    assert result.hex() == expected
    words = [result[index : index + 8] for index in range(0, 64, 8)]
    assert len(set(words[:4])) == len(set(words[4:])) == 1

    # Recompute the two image-size arguments. These are proof parameters, not
    # a claim that this script has searched 2^64 or 2^224 candidates.
    assert 3 * 64 + 48 == 240
    assert 2 * 64 == 128
    assert 128 // 2 == 64
    assert 8 * 64 == 512
    assert 7 * 64 == 448
    assert 448 // 2 == 224
    print("CHIME-512: INVARIANT WITNESS; COLLISION BOUND <= 2^64")
    print("CHIME-1024: DIMENSION CHECK; COLLISION BOUND <= 2^224")


if __name__ == "__main__":
    main()
