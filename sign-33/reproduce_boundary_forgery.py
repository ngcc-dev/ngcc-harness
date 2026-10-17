#!/usr/bin/env python3
"""sign-33-7 VDOO: moving bytes across the detached message/signature boundary.

sig_verify ignores sn_len_bytes, concatenates m || sn and calls
crypto_sign_open, which always takes the last CRYPTO_BYTES as the signature
(SIG_AlgorithmInstance.c:61-83, api.c:36-48).  From one honest signature on m,
the attacker submits a shorter (prefix) or longer (extended) message that was
never signed, with the bytes moved into or out of the detached signature.

Every verification runs as the first library call in a fresh child process,
so stale process state cannot explain an acceptance.  Controls: the honest
signature verifies, a changed message rejects, the prefix with the unmodified
signature rejects, and each forged signature with one bit flipped rejects.
"""
import ctypes
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-33"
ID = "sign-33-7"
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
a = json.loads(sys.stdin.read()); pk, sn, m = (bytes.fromhex(a[k]) for k in ("pk", "sn", "m"))
L.sig_verify.argtypes = [ctypes.POINTER(U8), ULL] * 3; L.sig_verify.restype = ctypes.c_int
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


def fresh_verify(lib, pk, sn, m):
    try:
        p = subprocess.run([sys.executable, "-I", "-c", CHILD, str(lib)],
                           input=json.dumps({"pk": pk.hex(), "sn": sn.hex(), "m": m.hex()}),
                           capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        return "timeout"
    r = [ln.split()[1] for ln in p.stdout.splitlines() if ln.startswith("RESULT ")]
    return int(r[-1]) if r else f"rc={p.returncode}"


def flip(b, i=0):
    b = bytearray(b)
    b[i] ^= 1
    return bytes(b)


def main():
    libs = [CAND / f"lib/libvdoo_{lv}.so" for lv in (128, 256, 512)]
    if not all(p.is_file() for p in libs):
        print(f"SKIP {ID}: VDOO libraries missing (build them with make -C sign-33)")
        return 77
    ok = True
    for lib in libs:
        name = lib.stem[3:]
        S = Sig(lib, seed=b"sign-33-7 " + name.encode())
        pk, sk = S.keygen()
        m = b"Transfer 1000 CNY to account 42"
        sn = S.sign(sk, m)
        assert len(sn) == S.sz["sn"]
        pre = b"Transfer 100"                      # never signed
        sig_pre = m[len(pre):] + sn                # moved message tail into the signature
        ext = m + sn[:3]                           # never signed
        sig_ext = sn[3:]                           # moved signature head into the message
        rows = [
            ("honest (m, sn)", fresh_verify(lib, pk, sn, m), True),
            ("control: changed message", fresh_verify(lib, pk, sn, m[:-1] + b"3"), False),
            (f"FORGERY prefix {pre!r} with tail||sn ({len(sig_pre)} bytes)",
             fresh_verify(lib, pk, sig_pre, pre), True),
            (f"FORGERY one-byte prefix {m[:-1]!r} with '2'||sn",
             fresh_verify(lib, pk, m[-1:] + sn, m[:-1]), True),
            (f"FORGERY extension m||sn[:3] ({len(ext)} bytes) with sn[3:] ({len(sig_ext)} bytes)",
             fresh_verify(lib, pk, sig_ext, ext), True),
            ("control: prefix with the unmodified signature", fresh_verify(lib, pk, sn, pre), False),
            ("control: prefix forgery with one bit flipped", fresh_verify(lib, pk, flip(sig_pre, len(sig_pre) - 20), pre), False),
            ("control: extension forgery with one bit flipped", fresh_verify(lib, pk, flip(sig_ext, 0), ext), False),
        ]
        print(f"{name}: CRYPTO_BYTES={S.sz['sn']}", flush=True)
        for label, r, want in rows:
            good = (r == 0) == want
            ok &= good and isinstance(r, int)
            print(f"  {label}: verify={r} expected={'accept' if want else 'reject'}{'' if good else '  <-- UNEXPECTED'}",
                  flush=True)
    if ok:
        print(f"CONFIRMED {ID}: VDOO-128/256/512 accept prefix and extension messages that were never signed, "
              "by moving bytes across the m||sn boundary (fresh-process verification; all controls reject)")
        return 0
    print(f"NOT CONFIRMED {ID}: a forgery rejected or a control accepted")
    return 1


if __name__ == "__main__":
    sys.exit(main())
