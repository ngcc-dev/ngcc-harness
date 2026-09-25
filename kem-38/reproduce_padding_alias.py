#!/usr/bin/env python3
"""Show that UVW-KEM ciphertexts with changed unused c1 bits decapsulate to the honest key."""

import argparse
import os
import pickle
import sys
import ctypes, hashlib

U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong


def array(data, extra=0):
    buf = (U8 * (len(data) + extra))()
    ctypes.memmove(buf, data, len(data))
    return buf


class Kem:
    def __init__(self, path, seed):
        self.lib = lib = ctypes.CDLL(path)
        for field in ("pk", "sk", "ct", "ss"):
            getattr(lib, f"kem_get_{field}_len_bytes").restype = ULL
        self.sizes = {f: int(getattr(lib, f"kem_get_{f}_len_bytes")())
                      for f in ("pk", "sk", "ct", "ss")}
        lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
        digest = hashlib.sha512(seed).digest()
        assert lib.ngcc_seed(array(digest), len(digest)) == 0

    def keygen(self):
        s = self.sizes
        pk, sk = (U8 * s["pk"])(), (U8 * s["sk"])()
        pk_len, sk_len = ULL(s["pk"]), ULL(s["sk"])
        assert self.lib.kem_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len)) == 0
        return bytes(pk)[:pk_len.value], bytes(sk)[:sk_len.value]

    def enc(self, pk):
        s = self.sizes
        ct, ss = (U8 * s["ct"])(), (U8 * s["ss"])()
        ct_len, ss_len = ULL(s["ct"]), ULL(s["ss"])
        rc = self.lib.kem_enc(array(pk), len(pk), ss, ctypes.byref(ss_len), ct, ctypes.byref(ct_len))
        return rc, bytes(ct)[:ct_len.value], bytes(ss)[:ss_len.value]

    def dec(self, sk, ct):
        s = self.sizes
        out, out_len = (U8 * s["ss"])(), ULL(s["ss"])
        rc = self.lib.kem_dec(array(sk), len(sk), array(ct), len(ct), out, ctypes.byref(out_len))
        return rc, bytes(out)[:out_len.value]


PARAMS = {"128": (860, 9, 256), "512": (3412, 11, 512)}   # n, bits per coefficient, m


def check(level):
    n, qbits, m = PARAMS[level]
    kem = Kem(f"kem-38/lib/libUVW-KEM-{level}.so", b"UVW padding alias witness")
    pk, sk = kem.keygen()
    rc, ct, key = kem.enc(pk)
    c1_bytes = (n * qbits + 7) // 8
    pad = 8 * c1_bytes - n * qbits
    assert rc == 0 and len(ct) == c1_bytes + 2 * (m // 8) and 0 < pad < 8
    assert kem.dec(sk, ct) == (0, key)
    masks = [1 << b for b in range(pad)] + [(1 << pad) - 1]
    alias = 0
    for mask in masks:
        c = bytearray(ct); c[c1_bytes - 1] ^= mask
        rc, k = kem.dec(sk, bytes(c))
        alias += rc == 0 and k == key
    c = bytearray(ct); c[-1] ^= 1                  # control: a changed d bit
    rc, k = kem.dec(sk, bytes(c))
    print(f"UVW-KEM-{level}: {pad} unused bits in c1 byte {c1_bytes - 1}")
    print(f"CONFIRMED: {alias}/{len(masks)} unused-bit changes return rc 0 and the honest key")
    print(f"CONTROL: changed d bit returns rc {rc}, honest key {rc == 0 and k == key}")
    assert alias == len(masks) and not (rc == 0 and k == key)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("levels", nargs="*", default=["128"], help="128 (seconds) and/or 512 (minutes)")
    for level in parser.parse_args().levels:
        check(level)


if __name__ == "__main__":
    main()
