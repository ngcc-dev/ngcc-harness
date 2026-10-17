#!/usr/bin/env python3
"""sign-26-4 SQIsign2D-push: noncanonical field, matrix and challenge encodings verify.

signature_decode (sqisign.c:349-378) reduces the four basis-change matrix
entries modulo 2^e and the challenge modulo 3^f instead of rejecting
out-of-range values, and fp_decode (gf/ref/gfx/fp.c:195-208) reduces an Fp
coordinate instead of rejecting values >= p.  For each level the script makes
one key pair and one honest signature, then builds byte-distinct aliases:

  * sig A + p   - the auxiliary-curve coefficient E_aux.A with one Fp
                  coordinate replaced by its value plus p;
  * pk  A + p   - the same edit on the public-key curve coefficient;
  * chl + 3^f   - the challenge field plus 3^f;
  * -M          - the four matrix entries negated modulo 2^e.

Every verification is the first library call in its own fresh process.
Controls: the honest signature verifies, a changed message rejects, A + 1 in
the signature and in the public key, chl + 1 and M[0][0] + 1 reject, and 16
appended bytes reject (the wrapper checks the length).

Levels 1-3 run by default.  NGCC_LONG=1 (or --all) also runs level 4, whose
key generation and signing take about a minute.
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
CAND = ROOT / "sign-26"
ID = "sign-26-4"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
FPFILE = {1: "fp_13178.c", 2: "fp_191118.c", 3: "fp_263156.c", 4: "fp_527324.c"}   # as in sign-26/Makefile

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
                           capture_output=True, text=True, timeout=900)
    except subprocess.TimeoutExpired:
        return "timeout"
    r = [ln.split()[1] for ln in p.stdout.splitlines() if ln.startswith("RESULT ")]
    return int(r[-1]) if r else f"rc={p.returncode}"


def get(b, o, n):
    return int.from_bytes(b[o:o + n], "little")


def put(b, o, n, v):
    if not 0 <= v < 1 << (8 * n):
        return None
    b = bytearray(b)
    b[o:o + n] = v.to_bytes(n, "little")
    return bytes(b)


def first_fit(b, offsets, n, delta):
    for o in offsets:
        r = put(b, o, n, get(b, o, n) + delta)
        if r is not None:
            return r
    return None


def params(level):
    src = CAND / f"Implementations/sqisign2d_lvl{level}/src"
    words = re.search(r"const uint64_t p\[NWORDS_FIELD\]\s*=\s*\{([^}]*)\}",
                      (src / f"gf/ref/lvl{level}/{FPFILE[level]}").read_text()).group(1)
    p = sum(int(w, 16) << (64 * i) for i, w in enumerate(re.findall(r"0x[0-9a-fA-F]+", words)))
    pre = src / f"precomp/ref/lvl{level}"
    hdr = "".join(f.read_text() for f in (pre / "include").glob("*.h"))
    fpb = int(re.search(r"#define FP_ENCODED_BYTES (\d+)", hdr).group(1))
    t2 = int(re.search(r"#define TORSION_2POWER_BYTES (\d+)", hdr).group(1))
    t3 = int(re.search(r"#define TORSION_3POWER_BYTES (\d+)", hdr).group(1))
    tc = (pre / "torsion_constants.c").read_text()
    e = int(re.search(r"TORSION_PLUS_EVEN_POWER = (0x[0-9a-fA-F]+)", tc).group(1), 16)
    f = int(re.search(r"TORSION_PLUS_ODD_POWERS\[1\] = \{(0x[0-9a-fA-F]+)\}", tc).group(1), 16)
    return p, fpb, t2, t3, e, f


def run(level):
    lib = CAND / f"lib/libSQIsign2D-lvl{level}.so"
    p, fpb, t2, t3, e, f = params(level)
    S = Sig(lib, seed=f"{ID} lvl{level}".encode())
    pk, sk = S.keygen()
    m = b"SQIsign2D-push encoding alias witness"
    sn = S.sign(sk, m)
    assert len(sn) == S.sz["sn"] == 2 * fpb + 4 * t2 + t3 + 2 and len(pk) == 2 * fpb
    mo, co = 2 * fpb, 2 * fpb + 4 * t2
    neg = sn
    for k in range(4):
        neg = put(neg, mo + k * t2, t2, (-get(sn, mo + k * t2, t2)) % (1 << e))
    rows = [("honest", pk, sn, m, True),
            ("control: changed message", pk, sn, m + b"!", False),
            ("sig E_aux.A coordinate + p", pk, first_fit(sn, (0, fpb), fpb, p), m, True),
            ("pk A coordinate + p", first_fit(pk, (0, fpb), fpb, p), sn, m, True),
            (f"chl + 3^{f}", pk, put(sn, co, t3, get(sn, co, t3) + 3 ** f), m, True),
            (f"-M mod 2^{e}", pk, neg, m, True),
            ("control: sig E_aux.A coordinate + 1", pk, first_fit(sn, (0,), fpb, 1), m, False),
            ("control: pk A coordinate + 1", first_fit(pk, (0,), fpb, 1), sn, m, False),
            ("control: chl + 1", pk, put(sn, co, t3, (get(sn, co, t3) + 1) % 3 ** f), m, False),
            ("control: M[0][0] + 1", pk, put(sn, mo, t2, (get(sn, mo, t2) + 1) % (1 << e)), m, False),
            ("control: sig || 16 trailing bytes", pk, sn + bytes(16), m, False)]
    ok = True
    print(f"lvl{level}: |pk|={len(pk)} |sig|={len(sn)} p={p.bit_length()} bits in {fpb} bytes, 2^{e}, 3^{f}", flush=True)
    for label, k, s, msg, want in rows:
        if k is None or s is None:
            print(f"  {label}: no room for the edit")
            ok = False
            continue
        r = fresh_verify(lib, k, s, msg)
        distinct = (k, s) != (pk, sn)
        good = isinstance(r, int) and (r == 0) == want
        ok &= good
        print(f"  {label}: distinct={distinct} verify={r} expected={'accept' if want else 'reject'}"
              f"{'' if good else '  <-- UNEXPECTED'}", flush=True)
    return ok


def main():
    full = os.environ.get("NGCC_LONG") == "1" or "--all" in sys.argv[1:]
    levels = (1, 2, 3, 4) if full else (1, 2, 3)
    missing = [l for l in levels if not (CAND / f"lib/libSQIsign2D-lvl{l}.so").is_file()]
    if missing:
        print(f"SKIP {ID}: libSQIsign2D-lvl{missing[0]}.so missing (build it with make -C sign-26)")
        return 77
    results = {l: run(l) for l in levels}
    bad = [f"lvl{l}" for l, v in results.items() if not v]
    names = "/".join(str(l) for l in levels)
    if not bad:
        print(f"CONFIRMED {ID}: SQIsign2D-push levels {names} accept byte-distinct A + p, pk A + p, chl + 3^f "
              "and -M aliases in fresh processes; all controls reject")
        return 0
    print(f"NOT CONFIRMED {ID}: unexpected results for {', '.join(bad)}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
