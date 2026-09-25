#!/usr/bin/env python3
"""Check OAEP-NTRU +q ciphertext aliases and noncongruent controls."""

import argparse
import ctypes
import hashlib
from pathlib import Path

U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
ROOT = Path(__file__).resolve().parent
SETS = {"648": (648, 7129, 13), "1296": (1296, 17497, 15), "2592": (2592, 28513, 15)}


def array(data):
    return (U8 * len(data)).from_buffer_copy(data)


class Kem:
    def __init__(self, level):
        self.lib = lib = ctypes.CDLL(str(ROOT / "lib" / f"libOAEP-NTRU-{level}.so"))
        for name in ("pk", "sk", "ct", "ss"):
            getattr(lib, f"kem_get_{name}_len_bytes").restype = ULL
        self.size = {name: int(getattr(lib, f"kem_get_{name}_len_bytes")())
                     for name in ("pk", "sk", "ct", "ss")}
        lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
        seed = hashlib.sha512(f"OAEP-NTRU alias {level}".encode()).digest()
        if lib.ngcc_seed(array(seed), len(seed)) != 0:
            raise RuntimeError("seed failed")

    def keygen(self):
        s = self.size
        pk, sk = (U8 * s["pk"])(), (U8 * s["sk"])()
        pk_len, sk_len = ULL(s["pk"]), ULL(s["sk"])
        rc = self.lib.kem_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len))
        if rc != 0 or pk_len.value != s["pk"] or sk_len.value != s["sk"]:
            raise RuntimeError("keygen failed")
        return bytes(pk), bytes(sk)

    def enc(self, pk):
        s = self.size
        ct, ss = (U8 * s["ct"])(), (U8 * s["ss"])()
        ct_len, ss_len = ULL(s["ct"]), ULL(s["ss"])
        rc = self.lib.kem_enc(array(pk), len(pk), ss, ctypes.byref(ss_len), ct, ctypes.byref(ct_len))
        if rc != 0 or ct_len.value != s["ct"] or ss_len.value != s["ss"]:
            raise RuntimeError("encapsulation failed")
        return bytes(ct), bytes(ss)

    def dec(self, sk, ct):
        out, out_len = (U8 * self.size["ss"])(), ULL(self.size["ss"])
        rc = self.lib.kem_dec(array(sk), len(sk), array(ct), len(ct), out, ctypes.byref(out_len))
        return rc, bytes(out)[:out_len.value]


def variants(ct, n, q, bits):
    poly_len = n * bits // 8
    packed = int.from_bytes(ct[:poly_len], "little")
    mask = (1 << bits) - 1
    for i in range(n):
        shift = bits * i
        coeff = (packed >> shift) & mask
        if coeff + q + 1 <= mask:
            cleared = packed & ~(mask << shift)
            def replace(value):
                poly = (cleared | (value << shift)).to_bytes(poly_len, "little")
                return poly + ct[poly_len:]
            return replace(coeff + q), replace(coeff + q + 1)
    raise AssertionError("no coefficient has a liftable encoding")


def run(level, trials):
    n, q, bits = SETS[level]
    kem = Kem(level)
    pk, sk = kem.keygen()
    for trial in range(trials):
        ct, key = kem.enc(pk)
        if kem.dec(sk, ct) != (0, key):
            raise AssertionError(f"{level} trial {trial}: honest decapsulation failed")
        alias, control = variants(ct, n, q, bits)
        if alias == ct or kem.dec(sk, alias) != (0, key):
            raise AssertionError(f"{level} trial {trial}: +q alias did not return the challenge key")
        _, control_key = kem.dec(sk, control)
        if control == ct or control_key == key:
            raise AssertionError(f"{level} trial {trial}: noncongruent control returned the challenge key")
    print(f"ATTACK kem-28-1 OAEP-NTRU-{level} CONFIRMED: {trials}/{trials} +q aliases returned the honest key; "
          f"0/{trials} noncongruent controls did")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trials", type=int, default=5)
    args = parser.parse_args()
    if args.trials < 1:
        parser.error("--trials must be positive")
    for level in SETS:
        run(level, args.trials)


if __name__ == "__main__":
    main()
