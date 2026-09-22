#!/usr/bin/env python3
"""Reproduce Tianyuan Xie's public HEP-QC EPC-P fingerprint."""

import argparse, collections, ctypes, random
from pathlib import Path

PARAMS = {"1": (128, 17669, 46, 384), "3": (192, 35851, 56, 640),
          "5": (256, 57637, 90, 640), "7": (512, 197123, 220, 896)}

def histogram(data, rows, columns):
    row_bytes = columns // 8
    packed = [bytearray((rows + 7) // 8) for _ in range(columns)]
    for r in range(rows):
        row = data[r * row_bytes:(r + 1) * row_bytes]
        target, mask = r >> 3, 1 << (r & 7)
        for i, value in enumerate(row):
            while value:
                bit = (value & -value).bit_length() - 1
                packed[8 * i + bit][target] |= mask
                value &= value - 1
    return collections.Counter(collections.Counter(map(bytes, packed)).values())

def main():
    p = argparse.ArgumentParser(); p.add_argument("--param", choices=PARAMS, default="1")
    p.add_argument("--library", type=Path); args = p.parse_args()
    rows, n, n1, n2 = PARAMS[args.param]; columns = 64 * ((n + 63) // 64)
    path = args.library or Path(__file__).with_name("lib") / f"libhep-qc-{args.param}.so"
    lib = ctypes.CDLL(str(path.resolve()))
    lib.kem_get_pk_len_bytes.restype = lib.kem_get_sk_len_bytes.restype = ctypes.c_ulonglong
    pn, sn = lib.kem_get_pk_len_bytes(), lib.kem_get_sk_len_bytes()
    pk, sk = (ctypes.c_ubyte * pn)(), (ctypes.c_ubyte * sn)()
    a, b = ctypes.c_ulonglong(), ctypes.c_ulonglong()
    if lib.kem_keygen(pk, ctypes.byref(a), sk, ctypes.byref(b)): return 2
    offset = 32 + (n + 7) // 8; size = rows * (columns // 8)
    got = histogram(bytes(pk)[offset:offset + size], rows, columns)
    expected = collections.Counter({n2 // 128: n1 * 128, 1: 64})
    rng = random.Random(0); control = histogram(bytes(rng.getrandbits(8) for _ in range(size)), rows, columns)
    print(f"HEP-QC-{args.param}: {dict(got)}; expected {dict(expected)}")
    print(f"uniform control: {dict(control)}")
    ok = got == expected and control == collections.Counter({1: columns})
    print("CONFIRMED: public G' is distinguishable" if ok else "NOT CONFIRMED")
    return 0 if ok else 1

if __name__ == "__main__": raise SystemExit(main())
