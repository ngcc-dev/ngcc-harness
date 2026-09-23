#!/usr/bin/env python3
"""Concrete ZC-DMC-1536-512/768 first-squeeze-block distinguisher.

Run ``make -C hash-31 exploit`` from the repository root.  Only the two
archived, official CryptHash implementations are used for the verdict.
"""

from __future__ import annotations

import ctypes
from pathlib import Path


HERE = Path(__file__).resolve().parent
MIDDLE = bytes.fromhex(
    "39cef84831618f0b62cb103e413db2e1"
    "1603c992c7a0247a7acbcb367166e527"
)


def crypt_hash(bits: int, message: bytes, message_bits: int) -> bytes:
    assert len(message) == (message_bits + 7) // 8
    library = ctypes.CDLL(str(HERE / "lib" / f"libZC-DMC-1536-{bits}.so"))
    fn = library.CryptHash
    fn.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p]
    fn.restype = ctypes.c_int
    output = ctypes.create_string_buffer(bits // 8)
    source = ctypes.create_string_buffer(message)
    rc = fn(bits, source, message_bits, output)
    if rc != 0:
        raise RuntimeError(f"ZC-DMC-1536-{bits} CryptHash returned {rc}")
    return output.raw


def main() -> None:
    # MIDDLE = P(LE32(13) || 188 zero bytes)[88:120], where P is the archived
    # 12-round ZC-1536 permutation.  Its final low nibble is 0111, permitting
    # the required 01 domain suffix and pad10*1 terminator in the 512 profile.
    assert len(MIDDLE) == 32 and MIDDLE[-1] & 0x0F == 0x07
    first_768 = (13).to_bytes(4, "little") + bytes(84)
    second_768 = bytes(88)
    final_768 = bytes(87) + b"\x07"

    # The final four bits of each padded block are 0111.  The 768 message
    # ends after bit 2108 and the 512 message ends after bit 2876.
    message_768 = first_768 + second_768 + final_768[:87] + b"\x00"
    message_512 = (
        first_768
        + bytes(32)
        + second_768
        + bytes(32)
        + final_768
        + MIDDLE[:31]
        + bytes([MIDDLE[-1] & 0xF0])
    )
    digest_768 = crypt_hash(768, message_768, 2108)
    digest_512 = crypt_hash(512, message_512, 2876)

    # Changing one message bit destroys the constructed relation.
    control_512 = bytes([message_512[0] ^ 1]) + message_512[1:]
    control_digest = crypt_hash(512, control_512, 2876)
    print(f"512 first squeeze block: {digest_512[:8].hex()}")
    print(f"768 first squeeze block: {digest_768[:8].hex()}")
    print(f"one-bit control block:  {control_digest[:8].hex()}")
    assert digest_512[:8] == digest_768[:8] == bytes.fromhex("7a07b396b47dfec4")
    assert control_digest[:8] != digest_768[:8]
    assert digest_512 != digest_768[:64]  # Not a full-digest collision.
    print("CONFIRMED hash-31-1: cross-profile first-64-bit distinguisher")


if __name__ == "__main__":
    main()
