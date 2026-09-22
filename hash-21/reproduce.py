#!/usr/bin/env python3
"""Reproduce both public Neulaser full-round collision constructions."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SIZES = {512: 64, 768: 96, 1024: 128}


def digest(bits: int, message: bytes) -> bytes:
    size = SIZES[bits]
    library = ctypes.CDLL(str(ROOT / "lib" / f"libNeulaser-{bits}.so"))
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
    if function(bits, source, 8 * len(message), output) != 0:
        raise RuntimeError(f"Neulaser-{bits}: CryptHash failed")
    return bytes(output)


def check(bits: int, left: bytes, right: bytes, expected: str) -> None:
    assert left != right
    left_digest = digest(bits, left)
    right_digest = digest(bits, right)
    assert left_digest == right_digest, f"Neulaser-{bits}: collision failed"
    assert left_digest.hex() == expected


def iscas_collision(bits: int, prefix: int) -> tuple[bytes, bytes]:
    a = bytes(prefix) + bytes.fromhex("00002743") + bytes(60)
    b = bytes(prefix) + bytes.fromhex("0002ad84") + bytes(60)
    return a, b


def reduction_collision(
    length: int, offset: int, a: str, b: str
) -> tuple[bytes, bytes]:
    left = bytearray(length)
    right = bytearray(length)
    left[offset : offset + 4] = bytes.fromhex(a)
    right[offset : offset + 4] = bytes.fromhex(b)
    return bytes(left), bytes(right)


def main() -> None:
    iscas = {
        512: (
            56,
            "f899bf50663e6e04fa5374baf28886153ea332bf25443817eb2abbca88072458"
            "dbd7a077712aae99fb112b7672c85a705700e027b348516cc45f8fff537713d3",
        ),
        768: (
            88,
            "ce762a05b18ab4fd659e6e601e4236d0908a59ef0feae7dfc71b9580af2f7a5e"
            "3cd9e61caa7f2a59dec94cd7b45dc6f9b700a5260145a7f4531e6a207ffc8930"
            "1f19c342a8a244c17df9140b9e37d9393aa63dd242bc5f9c47a1c8d7edd5094e",
        ),
        1024: (
            120,
            "9004dccd0bc7fb735b191dba252363bd283b6e8faa065725e74520c980ed34e22"
            "a13973901e7eb9bca06e47ced3a9f9dd50130787f4fe09e5014c5a8bb571e074"
            "174a9a260a81ad126e56fc19e1bd5d8a85c8635c5cc77f97da4290157f45d6d0"
            "bbace9bb06e88e991d1222e811e0e7167a9d3c58512eaa67118ab2740aad8d2",
        ),
    }
    for bits, (prefix, expected) in iscas.items():
        check(bits, *iscas_collision(bits, prefix), expected)
        print(f"Neulaser-{bits}: LOCAL-FEEDBACK COLLISION")

    reduction = {
        512: (
            111,
            24,
            "45d27672",
            "ba2d8989",
            "a4cfaefc283b5d788a4e0afc7be001cc9975bfaac13c2445c0b85e3c7c56d341"
            "3420424a0aa131b34f875e3ae5dcf475f074b44fa64a6a651feb2395e56a51af",
        ),
        768: (
            143,
            56,
            "7830985b",
            "87cf67a0",
            "08f2b56b0013420a9c97d5078e97bf13feab1c40b33d1da69580b1b6308c748"
            "25d595a14cfd88024a5848cd03e25e7b4d70053c5c65f14d14b1c85319805d38"
            "bfa8c6cda589ea8dea8af78d6b60c4762192ad3a8b0f78d84288a9a5061f7981f",
        ),
        1024: (
            175,
            24,
            "6a1ab451",
            "95e54baa",
            "2e97257246089526ed253dcd347d792ca87e06ab57d8220a81dfb60da87a9ba72"
            "65e96b21a99d377e36819f8c02d45364bc66099f4a4dcdae5773362b8aee7c5c"
            "654e0f32c5581dba1b1c193461fa4ea0199861beaf0fc3d9e14ab3a87d597217"
            "9999f2f5298571ecc6eb02a431960b3b06d0a93ed5a655b1504908300065970",
        ),
    }
    for bits, (length, offset, a, b, expected) in reduction.items():
        check(bits, *reduction_collision(length, offset, a, b), expected)
        print(f"Neulaser-{bits}: MODULAR-REDUCTION COLLISION")


if __name__ == "__main__":
    main()
