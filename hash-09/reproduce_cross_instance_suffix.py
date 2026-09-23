#!/usr/bin/env python3
"""Check Eijen's one-block cross-instance relation through the submitted APIs."""

import ctypes
from pathlib import Path


HERE = Path(__file__).resolve().parent


def digest(bits: int, message: bytes) -> bytes:
    library = ctypes.CDLL(str(HERE / "lib" / f"libEijen-{bits}.so"))
    library.CryptHash.argtypes = [
        ctypes.c_int,
        ctypes.c_void_p,
        ctypes.c_ulonglong,
        ctypes.c_void_p,
    ]
    library.CryptHash.restype = ctypes.c_int
    input_buffer = (ctypes.c_ubyte * len(message)).from_buffer_copy(message)
    output_buffer = (ctypes.c_ubyte * (bits // 8))()
    result = library.CryptHash(bits, input_buffer, 8 * len(message), output_buffer)
    assert result == 0, f"Eijen-{bits} returned {result}"
    return bytes(output_buffer)


def main() -> None:
    # 119 bytes is the largest whole-byte message fitting the 960-bit rate.
    for message in (b"", b"abc", bytes(range(119))):
        d512 = digest(512, message)
        d768 = digest(768, message)
        d1024 = digest(1024, message)
        assert d512 == d768[-64:] == d1024[-64:]
        changed = message + b"\x01" if len(message) < 119 else bytes([message[0] ^ 1]) + message[1:]
        assert digest(512, changed) != d512
    print("CONFIRMED hash-09-2: Eijen-512 is a suffix of both larger digests on three short messages; changed-message controls differ")


if __name__ == "__main__":
    main()
