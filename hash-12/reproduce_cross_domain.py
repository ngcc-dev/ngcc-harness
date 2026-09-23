#!/usr/bin/env python3
"""Construct an archived Iphe-512/1024 cross-profile digest relation.

Run after `make -C hash-12`. The temporary helper compiles the submission's
own permutation so that the compensating block can be computed; both final
hashes are obtained through the submitted CryptHash libraries.
"""

import ctypes
import subprocess
import tempfile
from pathlib import Path


HERE = Path(__file__).resolve().parent
SOURCE = HERE / "Iphe/Implementations/Reference_Implementation/Iphe-512/CryptHash_AlgorithmInstance.c"
U64 = ctypes.c_uint64
STATE = U64 * 32


def external_bytes(words):
    """Convert internal LSB-first words to the API's MSB-first message bytes."""
    return bytes(int(f"{b:08b}"[::-1], 2) for w in words for b in w.to_bytes(8, "little"))


def hash_bits(lib, bits, n, digest_bits):
    out = (ctypes.c_ubyte * (digest_bits // 8))()
    inp = (ctypes.c_ubyte * len(bits)).from_buffer_copy(bits)
    rc = lib.CryptHash(digest_bits, inp, n, out)
    assert rc == 0, f"CryptHash returned {rc}"
    return bytes(out)


def main():
    with tempfile.TemporaryDirectory(prefix="iphe-cross-domain-") as tmp:
        helper_path = Path(tmp) / "permutation.so"
        subprocess.run(
            ["cc", "-O2", "-fPIC", "-shared", "-o", str(helper_path), str(SOURCE)],
            check=True,
        )
        helper = ctypes.CDLL(str(helper_path))
        helper.CryptHash_AlgorithmInstance_permutation.argtypes = [ctypes.POINTER(U64)]
        hash512 = ctypes.CDLL(str(HERE / "lib/libIphe-512.so"))
        hash1024 = ctypes.CDLL(str(HERE / "lib/libIphe-1024.so"))
        for lib in (hash512, hash1024):
            lib.CryptHash.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p]
            lib.CryptHash.restype = ctypes.c_int

        # The larger rate has an eight-word (512-bit) gap. Find a first-block
        # image whose last two gap bits can serve as Iphe-512's final pad bits.
        for counter in range(1024):
            first = [counter] + [0] * 14
            state = STATE(*(first + [0] * 17))
            helper.CryptHash_AlgorithmInstance_permutation(state)
            gap = list(state)[15:23]
            if gap[-1] >> 62 == 3:
                break
        else:
            raise AssertionError("no padding-compatible first block")

        second = [0] * 15
        last = [0] * 14 + [3 << 62]
        msg1024 = external_bytes(first + second + last)
        msg512 = external_bytes((first + [0] * 8) + (second + [0] * 8) + (last + gap))
        n1024 = 3 * 960 - 2
        n512 = 3 * 1472 - 2
        out1024 = hash_bits(hash1024, msg1024, n1024, 1024)
        out512 = hash_bits(hash512, msg512, n512, 512)
        assert out1024[64:] == out512, "cross-profile relation failed"

        changed = bytearray(msg512)
        changed[0] ^= 0x80  # A meaningful message bit, not padding.
        control = hash_bits(hash512, changed, n512, 512)
        assert control != out512, "bit-flip control did not change digest"
        print(f"Iphe-1024 input: {n1024} bits; Iphe-512 input: {n512} bits")
        print(f"First-block search counter: {counter}")
        print(f"Equal 512-bit output: {out512.hex()}")
        print("CONFIRMED hash-12-1: cross-profile suffix relation; bit-flip control differs")


if __name__ == "__main__":
    main()
