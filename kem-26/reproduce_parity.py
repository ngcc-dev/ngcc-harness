#!/usr/bin/env python3
"""Exercise the kem-26-1 NSS-HQC-256 one-bit ciphertext distinguisher."""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parent
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
N = 54_493
N_BYTES = (N + 7) // 8


def parity(data: bytes) -> int:
    return sum(byte.bit_count() for byte in data) & 1


def main() -> int:
    lib = ctypes.CDLL(str(ROOT / "lib" / "libHQC-256.so"), mode=ctypes.RTLD_LOCAL)
    lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
    for name in ("pk", "sk", "ct", "ss"):
        getattr(lib, f"kem_get_{name}_len_bytes").restype = ULL
    sizes = {
        name: int(getattr(lib, f"kem_get_{name}_len_bytes")())
        for name in ("pk", "sk", "ct", "ss")
    }
    seed_bytes = hashlib.sha384(b"NSS-HQC-256 parity witness").digest()
    seed = (U8 * len(seed_bytes)).from_buffer_copy(seed_bytes)
    assert lib.ngcc_seed(seed, len(seed_bytes)) == 0

    pk, sk = (U8 * sizes["pk"])(), (U8 * sizes["sk"])()
    pkn, skn = ULL(sizes["pk"]), ULL(sizes["sk"])
    assert lib.kem_keygen(pk, ctypes.byref(pkn), sk, ctypes.byref(skn)) == 0

    observed = []
    controls = []
    for index in range(64):
        ct, ss = (U8 * sizes["ct"])(), (U8 * sizes["ss"])()
        ctn, ssn = ULL(sizes["ct"]), ULL(sizes["ss"])
        assert lib.kem_enc(pk, pkn.value, ss, ctypes.byref(ssn), ct, ctypes.byref(ctn)) == 0
        observed.append(parity(bytes(ct[:N_BYTES])))

        random_u = bytearray(hashlib.shake_256(f"uniform {index}".encode()).digest(N_BYTES))
        random_u[-1] &= (1 << (N & 7)) - 1
        controls.append(parity(random_u))

    assert observed == [0] * 64
    assert 0 in controls and 1 in controls
    print("submitted_ciphertext_u: even=64 odd=0")
    print(f"uniform_control: even={controls.count(0)} odd={controls.count(1)}")
    print("NSS_HQC_256_PARITY_DISTINGUISHER=CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
