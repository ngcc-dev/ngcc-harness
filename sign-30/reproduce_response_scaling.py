#!/usr/bin/env python3
"""sign-30-5 TRINE: scalar multiples of the response vectors verify.

A TRINE response d_i is a representative of a projective point, and the
canonical form CF(phi, d_i) that verification recomputes is defined to be
invariant under d_i -> lambda * d_i (spec section 2.4; canonical.c:340-362,
trine.c:493-650).  The encoded signature, however, carries the K response
vectors themselves (TRINE_K * TRINE_n elements of 12 bits at offset 0), and
nothing makes the representative canonical.  Replacing any response vector by
a nonzero multiple therefore yields a different signature that verifies.

Default: TRINE-128-ShortSig.  NGCC_LONG=1 (or --all) adds TRINE-128-balanced
and TRINE-256-ShortSig, which take several minutes.  Every verification is
the first library call in its own fresh process.  Controls: the honest
signature verifies, a changed message rejects, one coordinate + 1 rejects,
and scaling only the first half of one vector rejects.
"""
import ctypes
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-30"
REF = CAND / "Implementations/Reference_Implementation"
ID = "sign-30-5"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
PARAMS = {"TRINE-128-balanced": 1, "TRINE-256-balanced": 2, "TRINE-512-balanced": 3,
          "TRINE-128-ShortSig": 4, "TRINE-256-ShortSig": 5, "TRINE-512-ShortSig": 6}   # sign-30/Makefile

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
                           capture_output=True, text=True, timeout=1800)
    except subprocess.TimeoutExpired:
        return "timeout"
    r = [ln.split()[1] for ln in p.stdout.splitlines() if ln.startswith("RESULT ")]
    return int(r[-1]) if r else f"rc={p.returncode}"


def params(name):
    txt = (REF / name / "params.h").read_text()
    block = txt.split(f"#if PARAMS == 1", 1)[1]
    block = re.split(r"#(?:el)?if PARAMS == \d", "#if PARAMS == 1" + block)
    sel = block[PARAMS[name]]
    val = {k: int(re.search(rf"#define TRINE_{k} (\d+)", sel).group(1)) for k in ("n", "q", "K", "r")}
    val["qbits"] = int(re.search(r"#define TRINE_q_bits (\d+)", txt).group(1))
    assert "#define TRINE_RESPONSE_OFFSET 0" in txt
    return val


class Responses:
    def __init__(self, sn, P):
        self.sn, self.P = bytearray(sn), P

    def get(self, i):
        qb = self.P["qbits"]
        return sum(((self.sn[(i * qb + k) // 8] >> ((i * qb + k) % 8)) & 1) << k for k in range(qb))

    def set(self, i, v):
        qb = self.P["qbits"]
        for k in range(qb):
            pos = i * qb + k
            if (v >> k) & 1:
                self.sn[pos // 8] |= 1 << (pos % 8)
            else:
                self.sn[pos // 8] &= ~(1 << (pos % 8)) & 0xFF

    def scale(self, vec, lam, coords=None):
        n, q = self.P["n"], self.P["q"]
        for j in (range(n) if coords is None else coords):
            self.set(vec * n + j, self.get(vec * n + j) * lam % q)
        return self

    def bytes(self):
        return bytes(self.sn)


def run(name):
    lib = CAND / f"lib/lib{name}.so"
    P = params(name)
    n, q, K = P["n"], P["q"], P["K"]
    S = Sig(lib, seed=f"{ID} {name}".encode())
    pk, sk = S.keygen()
    m = b"TRINE response scaling witness"
    sn = S.sign(sk, m)
    assert len(sn) == S.sz["sn"]
    allscaled = Responses(sn, P)
    for v in range(K):
        allscaled.scale(v, 2 + v)
    a = allscaled.bytes()
    one = Responses(sn, P).scale(0, q - 1).bytes()
    plus1 = Responses(sn, P)
    plus1.set(0, (plus1.get(0) + 1) % q)
    half = Responses(sn, P).scale(0, 2, range(n // 2)).bytes()
    print(f"{name}: n={n} q={q} K={K} |sig|={len(sn)}", flush=True)
    rows = [("honest", sn, m, True),
            ("control: changed message", sn, m + b"!", False),
            (f"all {K} response vectors scaled by 2..{K + 1} ({sum(x != y for x, y in zip(a, sn))} bytes changed)", a, m, True),
            ("response vector 0 scaled by -1", one, m, True),
            ("control: response coordinate 0 + 1", plus1.bytes(), m, False),
            (f"control: first {n // 2} coordinates of vector 0 scaled by 2", half, m, False)]
    ok = True
    for label, s, msg, want in rows:
        r = fresh_verify(lib, pk, s, msg)
        good = isinstance(r, int) and (r == 0) == want
        ok &= good
        print(f"  {label}: verify={r} expected={'accept' if want else 'reject'}{'' if good else '  <-- UNEXPECTED'}",
              flush=True)
    return ok


def main():
    full = os.environ.get("NGCC_LONG") == "1" or "--all" in sys.argv[1:]
    sets = ["TRINE-128-ShortSig"] + (["TRINE-128-balanced", "TRINE-256-ShortSig"] if full else [])
    missing = [s for s in sets if not (CAND / f"lib/lib{s}.so").is_file()]
    if missing:
        print(f"SKIP {ID}: lib{missing[0]}.so missing (build it with make -C sign-30)")
        return 77
    res = {s: run(s) for s in sets}
    if all(res.values()):
        print(f"CONFIRMED {ID}: {', '.join(sets)} accept signatures whose response vectors are replaced by "
              "nonzero scalar multiples (fresh-process verification; controls reject)")
        return 0
    print(f"NOT CONFIRMED {ID}: unexpected results for {', '.join(s for s, v in res.items() if not v)}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
