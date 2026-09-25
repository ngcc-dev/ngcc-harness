#!/usr/bin/env python3
"""Show that BRA ciphertexts with changed padding bits decapsulate to the honest key."""

import argparse
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


CONFIG = {  # library: (vector bytes, n, m)
    "kem-06/lib/libBRA-128.so": (1014, 121, 67),
    "kem-06/lib/libBRA-256.so": (1671, 161, 83),
    "kem-06/lib/libBRA-512.so": (3509, 221, 127),
}


def pad_bits(vec_bytes, n, m):
    return 8 * vec_bytes - n * m


def check(path, vec_bytes, n, m, trials):
    pad = pad_bits(vec_bytes, n, m)
    assert 0 < pad < 8
    kem = Kem(path, b"rbc padding alias witness")
    alias = tests = control_same = controls = all_pad = 0
    for _ in range(trials):
        pk, sk = kem.keygen()
        rc, ct, key = kem.enc(pk)
        assert rc == 0 and kem.dec(sk, ct) == (0, key)
        for last in (vec_bytes - 1, 2 * vec_bytes - 1):   # last bytes of u and v
            for bit in range(8 - pad, 8):                  # unused padding bits
                c = bytearray(ct); c[last] ^= 1 << bit
                rc, k = kem.dec(sk, bytes(c)); tests += 1
                alias += rc == 0 and k == key
            c = bytearray(ct); c[last] ^= 1 << (7 - pad)   # control: last data bit
            rc, k = kem.dec(sk, bytes(c)); controls += 1
            control_same += k == key
        c = bytearray(ct)
        for last in (vec_bytes - 1, 2 * vec_bytes - 1):
            c[last] |= (0xFF << (8 - pad)) & 0xFF
        rc, k = kem.dec(sk, bytes(c))
        all_pad += rc == 0 and k == key and bytes(c) != ct
    print(f"{path}: {pad} padding bits per vector (bytes {vec_bytes - 1}, {2 * vec_bytes - 1})")
    print(f"CONFIRMED: {alias}/{tests} single padding-bit changes and {all_pad}/{trials} "
          "all-padding-set ciphertexts return the honest key")
    print(f"CONTROL: {control_same}/{controls} data-bit changes return the honest key")
    assert alias == tests and all_pad == trials and control_same == 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trials", type=int, default=5)
    args = parser.parse_args()
    for path, params in CONFIG.items():
        check(path, *params, args.trials)


if __name__ == "__main__":
    main()
