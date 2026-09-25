#!/usr/bin/env python3
"""Show that BAG-Piglet rejection keys do not bind the received ciphertext."""

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



def check(path, salt_bytes, limit):
    kem = Kem(path, b"rejection key binding witness")
    pk, sk = kem.keygen()
    rc, ct, key = kem.enc(pk)
    assert rc == 0 and kem.dec(sk, ct)[1] == key
    core_bits = 8 * (len(ct) - salt_bytes)
    step = max(1, core_bits // limit)
    keys, codes = collections.Counter(), collections.Counter()
    for bit in range(0, core_bits, step):
        c = bytearray(ct); c[bit // 8] ^= 1 << (bit % 8)
        rc, k = kem.dec(sk, bytes(c))
        codes[rc] += 1
        assert k != key
        keys[k] += 1
    salt_keys = set()
    for bit in range(core_bits, 8 * len(ct)):
        c = bytearray(ct); c[bit // 8] ^= 1 << (bit % 8)
        salt_keys.add(kem.dec(sk, bytes(c))[1])
    tested, largest = sum(keys.values()), keys.most_common(1)[0][1]
    print(f"{path}: {tested} distinct invalid ciphertexts (single core-bit changes), return codes {dict(codes)}")
    print(f"CONFIRMED: they yield only {len(keys)} distinct rejection keys; "
          f"the largest group of ciphertexts sharing one key has {largest}")
    print(f"CONTROL: {8 * salt_bytes} salt-bit changes yield {len(salt_keys)} distinct keys")
    assert largest > 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("libraries", nargs="+")
    parser.add_argument("--salt-bytes", type=int, default=16)
    parser.add_argument("--limit", type=int, default=1500)
    args = parser.parse_args()
    for path in args.libraries:
        check(path, args.salt_bytes, args.limit)


if __name__ == "__main__":
    main()
