#!/usr/bin/env python3
"""Show that malformed QIMEN-PIKE public keys hang or abort encapsulation."""

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


LIBS = ["kem-31/lib/libNGCC-1.so", "kem-31/lib/libNGCC-2.so", "kem-31/lib/libNGCC-3.so"]
CURVE_BYTES = {"kem-31/lib/libNGCC-1.so": 128}   # Fp2 curve coefficient at the start of pk


def child(path, variant):
    kem = Kem(path, b"QIMEN-PIKE malformed public key witness")
    pk, _ = kem.keygen()
    key = bytearray(pk)
    if variant == "hints":
        key[len(key) - 10:len(key) - 2] = bytes(8)       # zero the two basis hints
    elif variant == "curve":
        key[:CURVE_BYTES[path]] = bytes(CURVE_BYTES[path])
    start = time.time()
    rc, _, _ = kem.enc(bytes(key))
    print(f"rc={rc} time={time.time() - start:.2f}s")


def run(path, variant, timeout):
    try:
        res = subprocess.run([sys.executable, __file__, "--child", path, variant],
                             capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return "timeout", f"no return within {timeout} s"
    if res.returncode:
        detail = (res.stderr.strip().splitlines() or ["signal"])[-1][-90:]
        return ("signal" if res.returncode < 0 else "error"), f"aborted: {detail}"
    return "ok", res.stdout.strip()


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--child":
        child(sys.argv[2], sys.argv[3]); return
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=int, default=10)
    args = parser.parse_args()
    for path in LIBS:
        honest, detail = run(path, 'honest', args.timeout)
        print(f"{path}: CONTROL honest key: {detail}")
        assert honest == "ok" and detail.startswith("rc=0 "), f"{path}: honest encapsulation failed"
        hints, detail = run(path, 'hints', args.timeout)
        print(f"{path}: CONFIRMED zeroed hints: {detail}")
        assert hints == "timeout", f"{path}: zeroed hints did not hang"
        if path in CURVE_BYTES:
            curve, detail = run(path, 'curve', args.timeout)
            print(f"{path}: CONFIRMED zeroed curve coefficient: {detail}")
            assert curve == "signal", f"{path}: zeroed curve did not terminate by signal"


if __name__ == "__main__":
    main()
