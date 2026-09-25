#!/usr/bin/env python3
"""Show that a one-bit ciphertext change drives Amoeba's Hamming correction outside its stack buffer."""

import argparse
import collections
import os
import subprocess
import sys
import time
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


# (bytes of c1, bits per compressed c2 coefficient) per parameter set
PARAMS = {"128": (720, 5), "192": (1188, 6), "256": (1440, 6), "384": (2376, 6), "512": (3456, 7)}
CHECK_ROW = 5   # Hamming check bit 512 + 2^5 - 1 = 543 > 523-byte correction buffer
CONTROL_ROW = 0  # neighboring check bit maps inside the correction buffer


def child(level, row):
    kem = Kem(f"kem-02/lib/libAmoeba{level}.so", b"Amoeba ECC stack write witness")
    pk, sk = kem.keygen()
    rc, ct, key = kem.enc(pk)
    c1_bytes, bits = PARAMS[level]
    bit = c1_bytes * 8 + bits * (512 + row)
    c = bytearray(ct); c[bit // 8] ^= 0x80 >> (bit % 8)   # top bit of c2 coefficient 512+row
    rc, _ = kem.dec(sk, bytes(c))
    print(f"rc={rc}")


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--child":
        child(sys.argv[2], int(sys.argv[3])); return
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    for level in PARAMS:
        control = subprocess.run([sys.executable, __file__, "--child", level, str(CONTROL_ROW)],
                                 capture_output=True, text=True)
        assert control.returncode == 0, f"Amoeba{level}: in-bounds check-bit control crashed"
        print(f"Amoeba{level}: CONTROL nearby check bit returned without abort ({control.stdout.strip()})")
        res = subprocess.run([sys.executable, __file__, "--child", level, str(CHECK_ROW)],
                             capture_output=True, text=True)
        crashed = res.returncode < 0
        detail = res.stderr.strip().splitlines()[-1] if crashed and res.stderr.strip() else res.stdout.strip()
        print(f"Amoeba{level}: {'CONFIRMED: decapsulation aborted' if crashed else 'returned'} ({detail})")
        assert crashed, f"Amoeba{level}: expected a signal during decapsulation (rc={res.returncode})"


if __name__ == "__main__":
    main()
