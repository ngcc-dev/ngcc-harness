#!/usr/bin/env python3
"""sign-07-5 CS: 32-bit message lengths give long-message forgeries.

CS_Sign and CS_Verify take the message length as int len (cs.c:65-68 and
151-154; the wrapper passes the 64-bit m_len_bytes, SIG_AlgorithmInstance.c:
46-61), and H is pseudoXOF((OUTLEN) * 8, IN, (INLEN) * 8, OUT) with the bit
length computed in 32-bit int (symmetric.h:10).  mu = H(H(pk) || M) is
computed as H(mu, HBYTES, buf, HBYTES + len) (cs.c:86-89 and 173-177):

  * |M| = 2^29 + j: (HBYTES + len) * 8 wraps to 8 * (HBYTES + j);
  * |M| = 2^32 + j: len itself is truncated to j.

In both cases mu depends only on H(pk) || M[:j], so one signature on a short
j-byte message m verifies for every such long message that starts with m.
Long messages are anonymous mmap buffers.  Every verification runs as the
first library call in its own fresh process.  Controls: the honest signature
verifies, m || 'x' rejects, each long forgery with its first byte changed
rejects, and the length 2^32 + j + 1 (which hashes m || 0x00) rejects.
Memory: about 1 GiB peak.
"""
import ctypes
import hashlib
import json
import mmap
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-07"
IMPL = CAND / "Implementations"
ID = "sign-07-5"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
SETS = ("CS-128", "CS-256", "CS-512")

CHILD = r'''
import ctypes, hashlib, json, mmap, sys
U8 = ctypes.c_ubyte; ULL = ctypes.c_ulonglong
def arr(d, extra=64):
    b = (U8 * (len(d) + extra))(); ctypes.memmove(b, bytes(d), len(d)); return b
L = ctypes.CDLL(sys.argv[1])
L.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
d = hashlib.sha512(b"fresh verifier").digest(); L.ngcc_seed(arr(d), len(d))
L.sig_verify.argtypes = [ctypes.POINTER(U8), ULL] * 3; L.sig_verify.restype = ctypes.c_int
a = json.loads(sys.stdin.read()); pk, sn, pre = (bytes.fromhex(a[k]) for k in ("pk", "sn", "prefix"))
N, fill = a["N"], a["fill"]
mm = mmap.mmap(-1, max(N, 1), flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS)
if fill:
    chunk = bytes([fill]) * (1 << 24)
    for o in range(0, N, len(chunk)):
        mm[o:min(N, o + len(chunk))] = chunk[:min(N, o + len(chunk)) - o]
mm[:len(pre)] = pre
buf = (U8 * max(N, 1)).from_buffer(mm)
print("RESULT", L.sig_verify(arr(pk), len(pk), arr(sn), len(sn), buf, N), flush=True)
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

    def sign(self, sk, m, N=None, fill=0):
        """Sign m, or, with N, the N-byte message m || fill^(N - |m|) held in an mmap."""
        sn, nl = (U8 * (self.sz["sn"] + 4096))(), ULL(self.sz["sn"])
        if N is None:
            r = self.L.sig_sign(arr(sk), len(sk), arr(m), len(m), sn, ctypes.byref(nl))
        else:
            mm = mmap.mmap(-1, N, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS)
            if fill:
                chunk = bytes([fill]) * (1 << 24)
                for o in range(0, N, len(chunk)):
                    mm[o:min(N, o + len(chunk))] = chunk[:min(N, o + len(chunk)) - o]
            mm[:len(m)] = m
            buf = (U8 * N).from_buffer(mm)
            r = self.L.sig_sign(arr(sk), len(sk), buf, N, sn, ctypes.byref(nl))
            del buf
            mm.close()
        assert r == 0, r
        return bytes(sn)[:nl.value]


def fresh_verify(lib, pk, sn, prefix, N=None, fill=0):
    N = len(prefix) if N is None else N
    try:
        p = subprocess.run([sys.executable, "-I", "-c", CHILD, str(lib)],
                           input=json.dumps({"pk": pk.hex(), "sn": sn.hex(), "prefix": prefix.hex(),
                                             "N": N, "fill": fill}),
                           capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        return "timeout", ""
    r = [ln.split()[1] for ln in p.stdout.splitlines() if ln.startswith("RESULT ")]
    return (int(r[-1]) if r else f"rc={p.returncode}"), p.stderr


def run(name):
    lib = CAND / f"lib/lib{name}.so"
    S = Sig(lib, seed=f"{ID} {name}".encode())
    pk, sk = S.keygen()
    m = b"Transfer 10 CNY to Bob, ref 0042"
    j = len(m)
    sig = S.sign(sk, m)
    print(f"{name}: |pk|={len(pk)} |sig|={len(sig)}", flush=True)
    X = b"X" + m[1:]
    A, C = (1 << 29) + j, (1 << 32) + j
    rows = [
        ("honest: sig(m) on m", sig, m, None, 0, True),
        ("control: sig(m) on m||'x'", sig, m + b"x", None, 0, False),
        (f"FORGERY: sig(m) on m||0x00^(2^29) ({A} bytes)", sig, m, A, 0, True),
        (f"FORGERY: sig(m) on m||0xa5^(2^29) ({A} bytes)", sig, m, A, 0xA5, True),
        ("control: 2^29 + j bytes, first byte changed", sig, X, A, 0, False),
        (f"FORGERY: sig(m) on m||0x00^(2^32) ({C} bytes)", sig, m, C, 0, True),
        ("control: 2^32 + j bytes, first byte changed", sig, X, C, 0, False),
        ("control: 2^32 + j + 1 bytes", sig, m, C + 1, 0, False),
    ]
    ok = True
    for text, s, pre, N, fill, want in rows:
        r, err = fresh_verify(lib, pk, s, pre, N, fill)
        good = isinstance(r, int) and (r == 0) == want
        ok &= good
        print(f"  {text}: verify={r} expected={'accept' if want else 'reject'}"
              f"{'' if good else '  <-- UNEXPECTED'}", flush=True)
    return ok


def main():
    libs = [CAND / f"lib/lib{s}.so" for s in SETS]
    if not all(p.is_file() for p in libs):
        print(f"SKIP {ID}: CS libraries missing (build them with make -C sign-07)")
        return 77
    for s in SETS:
        for tree in ("Reference_Implementation", "Optimized_Implementation"):
            d = IMPL / tree / s
            assert "#define H(OUT, OUTLEN, IN, INLEN) pseudoXOF((OUTLEN) * 8, IN, (INLEN) * 8, OUT)" in \
                (d / "symmetric.h").read_text()
            cs = (d / "cs.c").read_text()
            assert cs.count("int len)") == 1 and cs.count("int len,") == 1
    if all([run(s) for s in SETS]):
        print(f"CONFIRMED {ID}: CS-128/256/512 accept a signature on a short message m for fresh messages "
              "of 2^29 + j and 2^32 + j bytes that start with m; all controls reject")
        return 0
    print(f"NOT CONFIRMED {ID}: a forgery rejected or a control accepted")
    return 1


if __name__ == "__main__":
    sys.exit(main())
