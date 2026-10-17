#!/usr/bin/env python3
"""sign-25-4 SQIsign2D2: noncanonical signature and public-key encodings verify.

For every one of the 16 submitted libraries the script makes one key pair and
one honest signature, then builds byte-distinct aliases:

  * sig A + p   - one Fp coordinate of a signature curve coefficient replaced
                  by its unreduced value plus p (fp_decode reduces, fp.c:169-183);
  * pk  A + p   - the same edit on the public-key curve coefficient;
  * trailing    - 16 bytes appended to the signature (sig_verify discards
                  sn_len_bytes, SIG_AlgorithmInstance.c:58-67);
  * -M          - uncompressed libraries only: the four basis-change matrix
                  entries negated modulo 2^e.

Every verification is the first library call in its own fresh process, so the
stale-stack verdict of sign-25-1 cannot explain an acceptance.  Controls per
library: the honest signature verifies, a changed message rejects, A + 1 in
the signature rejects, an independent public key rejects, and (uncompressed)
one matrix entry plus 1 rejects.  (A public key with A + 1 is not used as a
control: the compressed verifiers do not return on such a key.)
"""
import ctypes
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-25"
REF = CAND / "Implementations/Reference_Implementation"
ID = "sign-25-4"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
SETS = [f"Level{l}-{v}" for l in (1, 2, 3, 5) for v in ("eff", "sec")]

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
        self.seed(seed)

    def seed(self, seed):
        d = hashlib.sha512(seed).digest()
        self.L.ngcc_seed(arr(d), len(d))

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
                           capture_output=True, text=True, timeout=120)
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
            return r, o
    return None, None


def params(level):
    d = REF / f"SQISign2Dsquare-{level}/ec_arithmetic"
    src = next(d.glob("fp_p*.c")).read_text()
    words = re.search(r"const uint64_t p\[NWORDS_FIELD\]\s*=\s*\{([^}]*)\}", src).group(1)
    p = sum(int(w, 16) << (64 * i) for i, w in enumerate(re.findall(r"0x[0-9a-fA-F]+", words)))
    hdr = "".join(f.read_text() for f in (d / "include").glob("*.h"))
    fpb = int(re.search(r"#define FP_ENCODED_BYTES (\d+)", hdr).group(1))
    n2 = int(re.search(r"#define NWORDS_ORDER_2_BYTES (\d+)", hdr).group(1))
    n3 = int(re.search(r"#define NWORDS_ORDER_3_BYTES (\d+)", hdr).group(1))
    e = int(re.search(r"TORSION_PLUS_EVEN_POWER = (0x[0-9a-fA-F]+)", (d / "torsion_constants.c").read_text()).group(1), 16)
    return p, fpb, n2, n3, e


def run(level, comp):
    lib = CAND / f"lib/libSQISign2Dsquare-{level}_{comp}.so"
    p, fpb, n2, n3, e = params(level)
    S = Sig(lib, seed=f"{ID} {level} {comp}".encode())
    m = b"SQIsign2D2 encoding alias witness"
    for attempt in range(8):                       # a public key whose A has room for A + p
        pk, sk = S.keygen()
        pk_alias, _ = first_fit(pk, (0, fpb), fpb, p)
        if pk_alias is not None:
            break
        S.seed(f"{ID} {level} {comp} retry {attempt}".encode())
    sn = S.sign(sk, m)
    other_pk, _ = S.keygen()
    assert len(sn) == S.sz["sn"] and len(pk) == S.sz["pk"] and other_pk != pk
    curves = (0, fpb) if comp == "compressed" else (0, fpb, 2 * fpb, 3 * fpb)
    sig_alias, o = first_fit(sn, curves, fpb, p)
    sig_ctrl, _ = first_fit(sn, (o if o is not None else 0,), fpb, 1)
    rows = [("honest", pk, sn, m, True),
            ("control: changed message", pk, sn, m + b"!", False),
            (f"sig curve coordinate + p (offset {o})", pk, sig_alias, m, True),
            ("pk curve coordinate + p", pk_alias, sn, m, True),
            ("sig || 16 trailing bytes", pk, sn + bytes(range(16)), m, True),
            ("control: sig curve coordinate + 1", pk, sig_ctrl, m, False),
            ("control: independent public key", other_pk, sn, m, False)]
    if comp == "uncompressed":
        base = 4 * fpb
        neg = sn
        for k in range(4):
            neg = put(neg, base + k * n2, n2, (-get(sn, base + k * n2, n2)) % (1 << e))
        rows += [(f"-M mod 2^{e}", pk, neg, m, True),
                 ("control: M[0][0] + 1", pk, put(sn, base, n2, (get(sn, base, n2) + 1) % (1 << e)), m, False)]
    ok = True
    print(f"{level}-{comp}: |pk|={len(pk)} |sig|={len(sn)} p={p.bit_length()} bits in {fpb} bytes", flush=True)
    for label, k, s, msg, want in rows:
        if k is None or s is None:
            print(f"  {label}: no room for the edit")
            ok = False
            continue
        r = fresh_verify(lib, k, s, msg)
        distinct = (k, s) != (pk, sn)
        good = isinstance(r, int) and (r == 0) == want and (distinct or not want or label == "honest")
        ok &= good
        print(f"  {label}: distinct={distinct} verify={r} expected={'accept' if want else 'reject'}"
              f"{'' if good else '  <-- UNEXPECTED'}", flush=True)
    return ok


def main():
    libs = [CAND / f"lib/libSQISign2Dsquare-{s}_{c}.so" for s in SETS for c in ("uncompressed", "compressed")]
    missing = [str(x.relative_to(ROOT)) for x in libs if not x.is_file()]
    if missing:
        print(f"SKIP {ID}: libraries missing ({', '.join(missing[:3])} ...); build them with make -C sign-25")
        return 77
    results = {(s, c): run(s, c) for s in SETS for c in ("uncompressed", "compressed")}
    bad = [f"{s}-{c}" for (s, c), v in results.items() if not v]
    if not bad:
        print(f"CONFIRMED {ID}: all 16 SQIsign2D2 libraries accept byte-distinct signature and public-key aliases "
              "(A + p, trailing bytes, and -M when uncompressed) in fresh processes; all controls reject")
        return 0
    print(f"NOT CONFIRMED {ID}: unexpected results for {', '.join(bad)}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
