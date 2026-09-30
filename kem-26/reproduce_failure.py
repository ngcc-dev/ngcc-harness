#!/usr/bin/env python3
"""Find and replay an honest NSS-HQC-256 decapsulation failure."""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parent
LIBRARY = ROOT / "lib" / "libHQC-256.so"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong


def one_trial(lib, trial: int) -> tuple[bool, int]:
    seed_bytes = hashlib.sha384(f"NSS-HQC-256 failure {trial}".encode()).digest()
    seed = (U8 * len(seed_bytes)).from_buffer_copy(seed_bytes)
    assert lib.ngcc_seed(seed, len(seed_bytes)) == 0
    sizes = [
        int(getattr(lib, f"kem_get_{name}_len_bytes")())
        for name in ("pk", "sk", "ct", "ss")
    ]
    pk, sk, ct, sent, received = (
        (U8 * n)() for n in (sizes[0], sizes[1], sizes[2], sizes[3], sizes[3])
    )
    pkn, skn, ctn = map(ULL, sizes[:3])
    sentn, receivedn = ULL(sizes[3]), ULL(sizes[3])
    assert lib.kem_keygen(pk, ctypes.byref(pkn), sk, ctypes.byref(skn)) == 0
    assert lib.kem_enc(pk, pkn.value, sent, ctypes.byref(sentn), ct, ctypes.byref(ctn)) == 0
    rc = lib.kem_dec(sk, skn.value, ct, ctn.value, received, ctypes.byref(receivedn))
    return bytes(sent) != bytes(received), rc


def load():
    lib = ctypes.CDLL(str(LIBRARY), mode=ctypes.RTLD_LOCAL)
    lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
    for name in ("pk", "sk", "ct", "ss"):
        getattr(lib, f"kem_get_{name}_len_bytes").restype = ULL
    return lib


def main() -> int:
    lib = load()
    # Found by a bounded 30,000-seed scan; replay only the compact witness.
    trial = 4201
    failed, rc = one_trial(lib, trial)
    assert failed
    for control in (4200, 4202):
        control_failed, _ = one_trial(lib, control)
        assert not control_failed
    print(f"failure_seed_index={trial}")
    print(f"kem_dec_return={rc}")
    print("adjacent_seed_controls=AGREE")
    print("fixed_seed_replay=MISMATCH")
    print("NSS_HQC_256_HONEST_FAILURE=CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
