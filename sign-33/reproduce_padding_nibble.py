#!/usr/bin/env python3
"""sign-33-8 VDOO-128: the high nibble of response byte 68 is never read.

VDOO-128 packs N = 137 GF(16) response elements into VDOO_N_BYTE = 69 bytes,
followed by the 16-byte salt.  vdoo_verify reads elements 0..136 only, so the
four high bits of byte 68 are unauthenticated padding.

A child process (separate from the signer) flips each of the 680 signature
bits in turn and reports which flips still verify; exactly bits 4..7 of byte
68 are expected.  Every one of the 15 nonzero high-nibble values, and the
low-nibble and salt controls, are then verified again, each as the first call
in its own fresh process, together with the honest and changed-message
controls.
"""
import ctypes
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-33"
ID = "sign-33-8"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong

CHILD = r'''
import ctypes, hashlib, json, sys
U8 = ctypes.c_ubyte; ULL = ctypes.c_ulonglong
def arr(d, extra=64):
    b = (U8 * (len(d) + extra))(); ctypes.memmove(b, bytes(d), len(d)); return b
L = ctypes.CDLL(sys.argv[1])
L.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
d = hashlib.sha512(b"fresh verifier").digest(); L.ngcc_seed(arr(d), len(d))
L.sig_verify.argtypes = [ctypes.POINTER(U8), ULL] * 3; L.sig_verify.restype = ctypes.c_int
a = json.loads(sys.stdin.read()); pk, m = bytes.fromhex(a["pk"]), bytes.fromhex(a["m"])
for sn in map(bytes.fromhex, a["sns"]):
    print("RESULT", L.sig_verify(arr(pk), len(pk), arr(sn), len(sn), arr(m), len(m)), flush=True)
'''


def arr(d, extra=64):
    b = (U8 * (len(d) + extra))()
    ctypes.memmove(b, bytes(d), len(d))
    return b


class Sig:
    def __init__(self, path, seed):
        self.path = str(path)
        L = self.L = ctypes.CDLL(self.path)
        for f in ("pk", "sk", "sn"):
            getattr(L, f"sig_get_{f}_len_bytes").restype = ULL
        self.sz = {f: int(getattr(L, f"sig_get_{f}_len_bytes")()) for f in ("pk", "sk", "sn")}
        P, PL = ctypes.POINTER(U8), ctypes.POINTER(ULL)
        L.ngcc_seed.argtypes = [P, ULL]
        L.sig_keygen.argtypes = [P, PL, P, PL]
        L.sig_sign.argtypes = [P, ULL, P, ULL, P, PL]
        d = hashlib.sha512(seed).digest()
        L.ngcc_seed(arr(d), len(d))

    def keygen(self):
        pk, sk = (U8 * (self.sz["pk"] + 64))(), (U8 * (self.sz["sk"] + 64))()
        pl, sl = ULL(self.sz["pk"]), ULL(self.sz["sk"])
        assert self.L.sig_keygen(pk, ctypes.byref(pl), sk, ctypes.byref(sl)) == 0
        return bytes(pk)[:pl.value], bytes(sk)[:sl.value]

    def sign(self, sk, m):
        sn, nl = (U8 * (self.sz["sn"] + 4096))(), ULL(self.sz["sn"])
        assert self.L.sig_sign(arr(sk), len(sk), arr(m), len(m), sn, ctypes.byref(nl)) == 0
        return bytes(sn)[:nl.value]


def child_verify(lib, pk, sns, m):
    p = subprocess.run([sys.executable, "-I", "-c", CHILD, str(lib)],
                       input=json.dumps({"pk": pk.hex(), "m": m.hex(), "sns": [s.hex() for s in sns]}),
                       capture_output=True, text=True, timeout=900)
    r = [int(ln.split()[1]) for ln in p.stdout.splitlines() if ln.startswith("RESULT ")]
    return r if len(r) == len(sns) else None


def fresh_verify(lib, pk, sn, m):
    r = child_verify(lib, pk, [sn], m)
    return "crash" if r is None else r[0]


def xor(b, i, v):
    b = bytearray(b)
    b[i] ^= v
    return bytes(b)


def main():
    lib = CAND / "lib/libvdoo_128.so"
    if not lib.is_file():
        print(f"SKIP {ID}: {lib.relative_to(ROOT)} missing (build it with make -C sign-33)")
        return 77
    S = Sig(lib, seed=b"sign-33-8 vdoo_128")
    pk, sk = S.keygen()
    m = b"VDOO-128 padding nibble witness"
    sn = S.sign(sk, m)
    nbyte, salt = 69, 16                                       # VDOO_N_BYTE, SALT_BYTES
    assert len(sn) == nbyte + salt == S.sz["sn"]
    ok = True

    flips = [xor(sn, i // 8, 1 << (i % 8)) for i in range(8 * len(sn))]
    res = child_verify(lib, pk, flips, m)
    if res is None:
        print(f"NOT CONFIRMED {ID}: bit-scan child crashed")
        return 1
    accepted = [(i // 8, i % 8) for i, r in enumerate(res) if r == 0]
    print(f"bit scan of all {len(flips)} single-bit flips (one child process): accepted={accepted}")
    ok &= accepted == [(68, 4), (68, 5), (68, 6), (68, 7)]

    rows = [("honest", sn, True), ("control: changed message", None, False)]
    rows += [(f"byte 68 ^= 0x{v << 4:02x} (high nibble)", xor(sn, 68, v << 4), True) for v in range(1, 16)]
    rows += [("control: byte 68 ^= 0x01 (element 136)", xor(sn, 68, 0x01), False),
             ("control: byte 68 ^= 0x0f (element 136)", xor(sn, 68, 0x0F), False),
             ("control: byte 67 ^= 0x10 (element 135)", xor(sn, 67, 0x10), False),
             ("control: first salt byte ^= 0x01", xor(sn, nbyte, 0x01), False)]
    print("fresh-process verification (one process per call):")
    for label, s, want in rows:
        r = fresh_verify(lib, pk, sn, m + b"!") if s is None else fresh_verify(lib, pk, s, m)
        good = (r == 0) == want
        ok &= good
        print(f"  {label}: verify={r} expected={'accept' if want else 'reject'}{'' if good else '  <-- UNEXPECTED'}")
    if ok:
        print(f"CONFIRMED {ID}: VDOO-128 accepts all 16 values of the unread high nibble of response byte 68; "
              "every other single-bit flip rejects")
        return 0
    print(f"NOT CONFIRMED {ID}: unexpected acceptance pattern")
    return 1


if __name__ == "__main__":
    sys.exit(main())
