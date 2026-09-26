#!/usr/bin/env python3
"""Reproduce uHash collisions caused by malformed partial-byte padding."""

import ctypes
from pathlib import Path


HERE = Path(__file__).resolve().parent


def digest(instance: str, digest_bits: int, byte: int, message_bits: int) -> bytes:
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
    result = library.CryptHash(digest_bits, message, message_bits, output)
    assert result == 0, f"{instance} returned {result}"
    return bytes(output)


def main() -> None:
    for digest_bits in (512, 768, 1024):
        instance = f"uHash-{digest_bits}"

        # 00/1 denotes the one-bit string 0.  20/2 denotes the distinct
        # two-bit string 00: its 0x20 bit lies outside the declared input.
        # The buggy padding maps both encodings to the same padded byte 0x40.
        one_bit = digest(instance, digest_bits, 0x00, 1)
        two_bits_dirty = digest(instance, digest_bits, 0x20, 2)
        assert one_bit == two_bits_dirty

        # Controls: canonical encoding of 00 and the one-bit string 1 do not
        # collide with the attack digest.
        two_bits_canonical = digest(instance, digest_bits, 0x00, 2)
        changed_message = digest(instance, digest_bits, 0x80, 1)
        assert one_bit != two_bits_canonical
        assert one_bit != changed_message

        print(
            f"ATTACK hash-05-3 {instance} CONFIRMED: "
            "H(00/1 bit) = H(20/2 bits); canonical and changed-bit controls differ"
        )


if __name__ == "__main__":
    main()
