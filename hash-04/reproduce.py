#!/usr/bin/env python3
"""Reproduce CHAMP's known-length preimage search and API length defect."""

from __future__ import annotations

import argparse
import ctypes
import gc
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent
A = (-2, 1, 4, -3)
B = (-5, 2, -6, 2)
IDENTITY = (1, 0, 0, 1)
PRIMES = {512: (1 << 128) - 15449, 1024: (1 << 256) - 36113}
WITNESS = "0011110111011110111010011000111001101011"


def multiply(left: tuple[int, ...], right: tuple[int, ...], prime: int):
    a, b, c, d = left
    e, f, g, h = right
    return (
        (a * e + b * g) % prime,
        (a * f + b * h) % prime,
        (c * e + d * g) % prime,
        (c * f + d * h) % prime,
    )


def inverse(matrix: tuple[int, ...], prime: int):
    a, b, c, d = matrix
    scale = pow((a * d - b * c) % prime, -1, prime)
    return tuple(value * scale % prime for value in (d, -b, -c, a))


def products(length: int, generators: tuple[tuple[int, ...], ...],
             prime: int, initial=IDENTITY):
    stack = [(0, initial, 0)]
    while stack:
        depth, state, word = stack.pop()
        if depth == length:
            yield word, state
            continue
        stack.append((depth + 1, multiply(state, generators[1], prime),
                      2 * word + 1))
        stack.append((depth + 1, multiply(state, generators[0], prime),
                      2 * word))


def find_preimage(target: tuple[int, ...], length: int, prime: int) -> str | None:
    left = length // 2
    right = length - left
    table = {matrix: word for word, matrix in products(left, (A, B), prime)}
    inverse_generators = (inverse(A, prime), inverse(B, prime))
    for reversed_word, needed in products(right, inverse_generators, prime, target):
        prefix = table.get(needed)
        if prefix is not None:
            suffix = f"{reversed_word:0{right}b}"[::-1]
            return f"{prefix:0{left}b}" + suffix
    return None


class Champ:
    def __init__(self, bits: int):
        self.bits = bits
        self.size = bits // 8
        self.function = ctypes.CDLL(
            str(ROOT / "lib" / f"libCHAMP-{bits}.so")
        ).CryptHash
        self.function.argtypes = (
            ctypes.c_int,
            ctypes.c_void_p,
            ctypes.c_ulonglong,
            ctypes.c_void_p,
        )
        self.function.restype = ctypes.c_int

    @staticmethod
    def pack(bit_string: str):
        padded = bit_string.ljust((len(bit_string) + 7) // 8 * 8, "0")
        data = int(padded or "0", 2).to_bytes(len(padded) // 8, "big")
        return (ctypes.c_ubyte * len(data)).from_buffer_copy(data)

    def hash(self, bit_string: str, requested: int | None = None) -> tuple[int, bytes]:
        message = self.pack(bit_string)
        output = (ctypes.c_ubyte * self.size)()
        rc = self.function(requested or self.bits, message, len(bit_string), output)
        return rc, bytes(output)

    def guarded_wrong_length(self, bit_string: str) -> None:
        message = self.pack(bit_string)
        requested = self.bits // 2
        storage = (ctypes.c_ubyte * (self.size + 16))(
            *([0xA5] * (self.size + 16))
        )
        rc = self.function(requested, message, len(bit_string), storage)
        rc_full, expected = self.hash(bit_string)
        assert rc == rc_full == 0
        assert bytes(storage[: self.size]) == expected
        assert bytes(storage[requested // 8 : self.size]) != bytes(
            [0xA5] * (self.size - requested // 8)
        )
        assert bytes(storage[self.size :]) == bytes([0xA5] * 16)
        print(
            f"CHAMP-{self.bits}: requested {requested}, returned 0, "
            f"wrote {self.bits} bits; trailing guard intact"
        )


def decode(digest: bytes, bits: int) -> tuple[int, ...]:
    prime = PRIMES[bits]
    size = bits // 32
    encoded = [
        int.from_bytes(digest[offset : offset + size], "little")
        for offset in range(0, len(digest), size)
    ]
    assert len(encoded) == 4 and all(value < prime for value in encoded)
    values = [pow(value, -1, prime) if value else 0 for value in encoded]
    return values[0], values[2], values[1], values[3]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mitm-bits", type=int, default=32, choices=range(2, 41))
    arguments = parser.parse_args()
    message = WITNESS[: arguments.mitm_bits]

    for bits in (512, 1024):
        champ = Champ(bits)
        champ.guarded_wrong_length("101")
        rc, target_digest = champ.hash(message)
        assert rc == 0
        start = time.perf_counter()
        recovered = find_preimage(
            decode(target_digest, bits), arguments.mitm_bits, PRIMES[bits]
        )
        elapsed = time.perf_counter() - start
        assert recovered is not None and len(recovered) == arguments.mitm_bits
        assert champ.hash(recovered) == (0, target_digest)
        changed = str(1 - int(recovered[0])) + recovered[1:]
        assert champ.hash(changed)[1] != target_digest
        print(
            f"CHAMP-{bits}: {arguments.mitm_bits}-bit target recovered and "
            f"verified in {elapsed:.3f} seconds"
        )
        del champ
        gc.collect()


if __name__ == "__main__":
    main()
