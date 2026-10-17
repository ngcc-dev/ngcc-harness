#!/usr/bin/env python3
"""sign-27-6 SQIsignTriangle: high bits of the auxiliary-basis coefficients are ignored.

A signature is E_aux.A (2*FP_ENCODED_BYTES), four basis coefficients r1, r2,
s1, s2 of SQISIGNTRIANGLE_BASIS_COEFF_BYTES each, and q of the same width
(new_signature_from_bytes, encode_verification.c:284-297).  The decoder
reads each coefficient into a full-width integer but uses it only through
ec_biscalar_mul(..., e_rsp, ...) and mp_mod_2exp(..., e_rsp, ...), where
e_rsp = bitsize(q) (encode_verification.c:202-241).  Coefficient bits at
positions >= e_rsp are therefore never authenticated.

For each level the script signs once and then verifies, each as the first
library call in its own fresh process: every unused high bit flipped on its
own, all unused high bits set at once, and random multiples of 2^e_rsp added
to all four coefficients.  Controls: the honest signature verifies, a changed
message rejects, and flipping bit 0 or bit e_rsp // 2 of any coefficient
rejects.
"""
import ctypes
import hashlib
import json
import random
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-27"
REF = CAND / "Implementations/Reference_Implementation"
ID = "sign-27-6"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
LEVELS = (1, 2, 5, 6)

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


def get(b, o, n):
    return int.from_bytes(b[o:o + n], "little")


def put(b, o, n, v):
    assert 0 <= v < 1 << (8 * n)
    b = bytearray(b)
    b[o:o + n] = v.to_bytes(n, "little")
    return bytes(b)


def params(level):
    inc = REF / f"SQIsignTriangle_lvl{level}/sqisigntriangle/src/precomp/ref/lvl{level}/include"
    hdr = "".join(f.read_text() for f in inc.glob("*.h"))
    fpb = int(re.search(r"#define FP_ENCODED_BYTES (\d+)", hdr).group(1))
    cb = int(re.search(r"#define SQISIGNTRIANGLE_BASIS_COEFF_BYTES (\d+)", hdr).group(1))
    qb = int(re.search(r"#define SQISIGNTRIANGLE_Q_BYTES (\d+)", hdr).group(1))
    return fpb, cb, qb


def run(level):
    lib = CAND / f"lib/libSQIsignTriangle_lvl{level}.so"
    fpb, cb, qb = params(level)
    S = Sig(lib, seed=f"{ID} lvl{level}".encode())
    pk, sk = S.keygen()
    m = b"SQIsignTriangle coefficient witness"
    base, qo = 2 * fpb, 2 * fpb + 4 * cb
    for _ in range(8):                  # need e_rsp < 8*cb so that unused bits exist
        sn = S.sign(sk, m)
        e = get(sn, qo, qb).bit_length()
        if e < 8 * cb:
            break
    assert len(sn) == S.sz["sn"] == qo + qb
    free = 8 * cb - e
    coeffs = [get(sn, base + k * cb, cb) for k in range(4)]
    print(f"lvl{level}: |sig|={len(sn)} coefficient field={8 * cb} bits, e_rsp=bitsize(q)={e}, "
          f"unused high bits={free} per coefficient, {4 * free} in total", flush=True)
    ok = True

    def check(label, s, msg, want, quiet=False):
        nonlocal ok
        r = fresh_verify(lib, pk, s, msg)
        good = isinstance(r, int) and (r == 0) == want
        ok &= good
        if not quiet or not good:
            print(f"  {label}: verify={r} expected={'accept' if want else 'reject'}{'' if good else '  <-- UNEXPECTED'}",
                  flush=True)
        return good

    check("honest", sn, m, True)
    check("control: changed message", sn, m + b"!", False)
    n_ok = 0
    for k in range(4):
        for bit in range(e, 8 * cb):
            n_ok += check(f"coefficient {k} bit {bit} flipped", put(sn, base + k * cb, cb, coeffs[k] ^ (1 << bit)), m, True, True)
    print(f"  each unused high bit flipped alone: {n_ok}/{4 * free} accepted")
    s = sn
    for k in range(4):
        s = put(s, base + k * cb, cb, coeffs[k] | ((1 << (8 * cb)) - (1 << e)))
    check(f"all {4 * free} unused high bits set ({sum(a != b for a, b in zip(s, sn))} bytes changed)", s, m, True)
    rnd = random.Random(level)
    s = sn
    for k in range(4):
        s = put(s, base + k * cb, cb, coeffs[k] + (rnd.randrange(1, 1 << free) << e))
    check("random multiples of 2^e_rsp added to all four coefficients", s, m, True)
    for k in range(4):
        check(f"control: coefficient {k} bit 0 flipped", put(sn, base + k * cb, cb, coeffs[k] ^ 1), m, False)
        check(f"control: coefficient {k} bit {e // 2} flipped", put(sn, base + k * cb, cb, coeffs[k] ^ (1 << (e // 2))), m, False)
    return ok and free > 0, 4 * free


def main():
    libs = [CAND / f"lib/libSQIsignTriangle_lvl{l}.so" for l in LEVELS]
    if not all(x.is_file() for x in libs):
        print(f"SKIP {ID}: SQIsignTriangle libraries missing (build them with make -C sign-27)")
        return 77
    res = {l: run(l) for l in LEVELS}
    if all(v[0] for v in res.values()):
        counts = ", ".join(f"lvl{l}: {v[1]}" for l, v in res.items())
        print(f"CONFIRMED {ID}: SQIsignTriangle ignores coefficient bits >= bitsize(q); unused bits accepted "
              f"({counts}) in fresh processes, used-bit controls reject")
        return 0
    print(f"NOT CONFIRMED {ID}: unexpected results at "
          + ", ".join(f"lvl{l}" for l, v in res.items() if not v[0]))
    return 1


if __name__ == "__main__":
    sys.exit(main())
