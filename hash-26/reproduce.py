#!/usr/bin/env python3
"""Validate the public CHIME invariant-subspace witness and bounds."""

from __future__ import annotations

import ctypes
from fractions import Fraction
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

    first = bytes.fromhex(
        "5194027f143fc830" * 4 + "0700ed5eeeffc081" * 3 + "0700ed5eeeffc0"
    )
    second = bytes.fromhex(
        "b0da6a248be280ad" * 4 + "0700ed5eeeffc081" * 3 + "0700ed5eeeffc0"
    )
    assert len(first) == len(second) == 63 and first != second
    first_digest, second_digest = digest(first), digest(second)
    assert first_digest.hex().startswith("5c145b0796a8f1db" * 4)
    assert first_digest[:32] == second_digest[:32]
    assert first_digest[32:] != second_digest[32:]
    assert digest(bytes([first[0] ^ 1]) + first[1:])[:32] != second_digest[:32]

    # Recompute the two image-size arguments. These are proof parameters, not
    # a claim that this script has searched 2^64 or 2^160 candidates.
    assert 3 * 64 + 48 == 240
    assert 2 * 64 == 128
    assert 128 // 2 == 64
    assert 5 * 64 == 320  # A1[0], B0, B1, C0, C1 on the restricted subspace
    assert 6 * 64 > 320  # six controlled blocks avoid input-space saturation
    assert 320 // 2 == 160
    samples = 1 << 160
    assert Fraction(samples * (samples - 1), 2 * (1 << 320)) > Fraction(99, 200)
    assert Fraction(samples * (samples - 1), 2 * (1 << 384)) < Fraction(1, 1 << 64)
    print("CHIME-512: INVARIANT WITNESS; COLLISION BOUND <= 2^64")
    print("CHIME-512: 256-BIT PARTIAL COLLISION CONFIRMED (FULL COLLISION NOT RUN)")
    print("CHIME-1024: CAPACITY COUNT; COLLISION BOUND ~ 2^160 (SEARCH NOT RUN)")


if __name__ == "__main__":
    main()
