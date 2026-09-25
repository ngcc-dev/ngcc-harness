#!/usr/bin/env python3
"""Show that CMultiURAG-512 ciphertexts with changed padding bits decapsulate to the honest key."""

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


LIB = "kem-10/lib/libCMultiURAG-512.so"
M, U_ELEMS, V_ELEMS, U_BYTES, V_BYTES = 181, 67 * 21, 19 * 21, 31834, 9028


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trials", type=int, default=3)
    args = parser.parse_args()
    kem = Kem(LIB, b"CMultiURAG padding alias witness")
    pk, sk = kem.keygen()
    used_u, used_v = (U_ELEMS * M) % 8, (V_ELEMS * M) % 8
    alias = tests = control_same = controls = all_pad = 0
    for _ in range(args.trials):
        rc, ct, key = kem.enc(pk)
        assert rc == 0 and kem.dec(sk, ct) == (0, key)
        for last, used in ((U_BYTES - 1, used_u), (U_BYTES + V_BYTES - 1, used_v)):
            for bit in range(8):
                c = bytearray(ct); c[last] ^= 1 << bit
                rc, k = kem.dec(sk, bytes(c))
                if bit >= used:
                    tests += 1; alias += rc == 0 and k == key
                else:
                    controls += 1; control_same += k == key
        c = bytearray(ct)
        c[U_BYTES - 1] |= (0xFF << used_u) & 0xFF
        c[U_BYTES + V_BYTES - 1] |= (0xFF << used_v) & 0xFF
        rc, k = kem.dec(sk, bytes(c))
        all_pad += rc == 0 and k == key and bytes(c) != ct
    print(f"{LIB}: padding in U byte {U_BYTES - 1} ({8 - used_u} bits) and V byte "
          f"{U_BYTES + V_BYTES - 1} ({8 - used_v} bits)")
    print(f"CONFIRMED: {alias}/{tests} single padding-bit changes and {all_pad}/{args.trials} "
          "all-padding-set ciphertexts return the honest key")
    print(f"CONTROL: {control_same}/{controls} data-bit changes return the honest key")
    assert alias == tests and all_pad == args.trials and control_same == 0


if __name__ == "__main__":
    main()
