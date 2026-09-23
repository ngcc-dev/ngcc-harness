#!/usr/bin/env python3
"""Distinguish an honest NIIKE raw shared key from uniform field elements.

Run with Sage's Python after `make -C kex-08`:
    sage -python kex-08/reproduce_raw_key_distinguisher.py
"""

from __future__ import annotations

import ctypes
import secrets
from pathlib import Path

from sage.all import EllipticCurve_from_j, GF, PolynomialRing


ROOT = Path(__file__).resolve().parent
P128 = int("53d1c0debdc0c1ba3edd760148d21a078008f642a9f26f65995c4d86fc6efff7", 16)
COORD_BYTES = 32


def honest_key() -> bytes:
    library = ROOT / "lib" / "libNIIKE-lv128.so"
    if not library.exists():
        raise SystemExit("build first: make -C kex-08")
    lib = ctypes.CDLL(str(library))
    lib.ngcc_seed.argtypes = (ctypes.c_void_p, ctypes.c_ulonglong)
    lib.ngcc_seed.restype = ctypes.c_int
    seed = (ctypes.c_ubyte * 64)(*range(64))
    if lib.ngcc_seed(seed, len(seed)) != 0:
        raise RuntimeError("NIIKE DRNG seed failed")
    for name in ("pk", "sk", "sta", "ss"):
        getter = getattr(lib, f"kex_get_{name}_len_bytes")
        getter.restype = ctypes.c_ulonglong
    pk_len = lib.kex_get_pk_len_bytes()
    sk_len = lib.kex_get_sk_len_bytes()
    sta_len = lib.kex_get_sta_len_bytes()
    ss_len = lib.kex_get_ss_len_bytes()
    assert ss_len == 2 * COORD_BYTES

    a_pk = (ctypes.c_ubyte * pk_len)()
    a_sk = (ctypes.c_ubyte * sk_len)()
    b_pk = (ctypes.c_ubyte * pk_len)()
    b_sk = (ctypes.c_ubyte * sk_len)()
    sta = (ctypes.c_ubyte * max(sta_len, 1))()
    a_ss = (ctypes.c_ubyte * ss_len)()
    b_ss = (ctypes.c_ubyte * ss_len)()

    for fn_name, pk, sk in (("kex_init_a", a_pk, a_sk), ("kex_init_b", b_pk, b_sk)):
        fn = getattr(lib, fn_name)
        lens = [ctypes.c_ulonglong(n) for n in (pk_len, sk_len, sta_len)]
        rc = fn(pk, ctypes.byref(lens[0]), sk, ctypes.byref(lens[1]), sta, ctypes.byref(lens[2]))
        if rc != 0:
            raise RuntimeError(f"{fn_name} failed: {rc}")

    for fn_name, sk, pk, out in (
        ("kex_derive_ss_a", a_sk, b_pk, a_ss),
        ("kex_derive_ss_b", b_sk, a_pk, b_ss),
    ):
        out_len = ctypes.c_ulonglong(ss_len)
        rc = getattr(lib, fn_name)(sk, sk_len, pk, pk_len, None, 0, sta, sta_len, out, ctypes.byref(out_len))
        if rc != 0 or out_len.value != ss_len:
            raise RuntimeError(f"{fn_name} failed: rc={rc}, len={out_len.value}")
    if bytes(a_ss) != bytes(b_ss):
        raise AssertionError("honest A/B agreement failed")
    return bytes(a_ss)


def main() -> None:
    p = P128
    base = GF(p)
    poly = PolynomialRing(base, "x")
    x = poly.gen()
    field = base.extension(x * x + 1, "i")
    i = field.gen()

    def decode(raw: bytes):
        re = int.from_bytes(raw[:COORD_BYTES], "little")
        im = int.from_bytes(raw[COORD_BYTES:], "little")
        if not (re < p and im < p):
            return None
        return field(re) + field(im) * i

    real = honest_key()
    j_real = decode(real)
    if j_real is None:
        raise AssertionError("honest key is not canonically encoded")
    if not EllipticCurve_from_j(j_real).is_supersingular():
        raise AssertionError("honest shared key is not a supersingular j-invariant")

    controls = 16
    ordinary = 0
    for _ in range(controls):
        j_control = field(secrets.randbelow(p)) + field(secrets.randbelow(p)) * i
        ordinary += not EllipticCurve_from_j(j_control).is_supersingular()
    if ordinary != controls:
        raise AssertionError("unexpected supersingular random control")
    print(f"ATTACK kex-08-1 NIIKE-lv128 CONFIRMED honest A/B key agrees and is supersingular; "
          f"{ordinary}/{controls} uniform Fp2 controls are ordinary")


if __name__ == "__main__":
    main()
