#!/usr/bin/env python3
"""Show that FEILIAN C hashes bits outside the declared bitstring."""

import ctypes
from pathlib import Path


HERE = Path(__file__).resolve().parent


def digest(instance: str, digest_bits: int, byte: int) -> bytes:
    library = ctypes.CDLL(str(HERE / "lib" / f"lib{instance}.so"))
    library.CryptHash.argtypes = [
        ctypes.c_int,
        ctypes.c_void_p,
        ctypes.c_ulonglong,
        ctypes.c_void_p,
    ]
    library.CryptHash.restype = ctypes.c_int
    message = (ctypes.c_ubyte * 1)(byte)
    output = (ctypes.c_ubyte * (digest_bits // 8))()
    result = library.CryptHash(digest_bits, message, 1, output)
    assert result == 0, f"{instance} returned {result}"
    return bytes(output)


def main() -> None:
    for digest_bits in (512, 768, 1024):
        instance = f"FEILIAN{digest_bits}"
        canonical = digest(instance, digest_bits, 0x80)
        same_bitstring = digest(instance, digest_bits, 0x81)
        changed_bit = digest(instance, digest_bits, 0x00)
        assert canonical != same_bitstring
        assert canonical != changed_bit
        assert canonical == digest(instance, digest_bits, 0x80)
        print(
            f"ATTACK hash-10-4 {instance} CONFIRMED: "
            "the same declared one-bit message hashes differently when an unused bit changes"
        )


if __name__ == "__main__":
    main()
