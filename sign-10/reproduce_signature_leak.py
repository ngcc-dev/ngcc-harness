#!/usr/bin/env python3
"""Facto-DSA: honest signatures fix values of secret linear forms (sign-10-4).

Signs distinct messages through the prebuilt lib/libFacto-DSA-*.so (harness
DRBG seeded deterministically), checks that every signature verifies, and then
uses the signer's own secret S^-1 (validation only) to compute XY = S z:

  * Y_{n-1} = (S z)_{2n-1} equals exactly 1 whenever the scale-1 first attempt
    succeeds (b is monic, Y = scale^-1 b); a uniform preimage gives 1/(q-1).
  * chi(2 a_0 X_0) = +1 in every signature (principal square root first, and the
    -root subtree mirrors the +root subtree at level 0).

Controls: mutated signatures reject; a fixed random public linear form r gives
r.z = 1 essentially never and chi(r.z) = +1 about half the time; the secret
forms of an unrelated key give no Y_{n-1} = 1 hits and chi about one half.

usage: python3 sign-10/reproduce_signature_leak.py   (NGCC_LONG=1: larger N)
"""
import collections
import ctypes
import hashlib
import os
import random
import re
import sys
import time
from pathlib import Path

ID = "sign-10-4"
ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-10"
REF = CAND / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
U8, ULL = ctypes.c_ubyte, ctypes.c_ulonglong
LONG = os.environ.get("NGCC_LONG") == "1"
COUNTS = {128: 8000, 256: 1000, 512: 300} if LONG else {128: 2000, 256: 300, 512: 60}


def arr(b):
    a = (U8 * max(1, len(b)))()
    ctypes.memmove(a, b, len(b))
    return a


class Lib:
    def __init__(self, path, seed):
        self.lib = lib = ctypes.CDLL(str(path))
        for f in ("pk", "sk", "sn"):
            getattr(lib, f"sig_get_{f}_len_bytes").restype = ULL
        self.L = {f: int(getattr(lib, f"sig_get_{f}_len_bytes")()) for f in ("pk", "sk", "sn")}
        lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
        d = hashlib.sha512(seed).digest()
        if lib.ngcc_seed(arr(d), len(d)) != 0:
            raise RuntimeError("ngcc_seed failed")

    def keygen(self):
        pk, sk = (U8 * self.L["pk"])(), (U8 * self.L["sk"])()
        pl, sl = ULL(len(pk)), ULL(len(sk))
        if self.lib.sig_keygen(pk, ctypes.byref(pl), sk, ctypes.byref(sl)) != 0:
            raise RuntimeError("keygen failed")
        return bytes(pk)[:pl.value], bytes(sk)[:sl.value]

    def sign(self, sk, m):
        sn, nl = (U8 * self.L["sn"])(), ULL(self.L["sn"])
        rc = self.lib.sig_sign(arr(sk), ULL(len(sk)), arr(m), ULL(len(m)), sn, ctypes.byref(nl))
        return rc, bytes(sn)[:nl.value]

    def verify(self, pk, sn, m):
        return self.lib.sig_verify(arr(pk), ULL(len(pk)), arr(sn), ULL(len(sn)), arr(m), ULL(len(m)))


def params(level):
    h = (REF / f"Facto-DSA-{level}/SIG_AlgorithmInstance.h").read_text()
    n = int(re.search(r"^#define FACTO_N (\d+)$", h, re.M).group(1))
    m = int(re.search(r"^#define FACTO_M (\d+)$", h, re.M).group(1))
    q = int(re.search(r"^#define FACTO_Q (\d+)u$", h, re.M).group(1))
    return n, m, q


def source_check():
    """The leak's three code paths, in every Facto-DSA tree."""
    impl = REF.parent
    trees = sorted(impl.glob("*/Facto-DSA-*/SIG_AlgorithmInstance.c"))
    pats = {
        "monic b": r"poly_one\(b\);",
        "Y = scale^-1 b": r"Y\[i\] = f_mul\(ainv, Y\[i\]\);",
        "scale 1 first": r"fe_t scale = 1;\s*\n\s*if \(sc != 0 && random_nonzero_fe\(&scale\)",
        "principal root first": r"root = f_sqrt\(delta\);\s*\n\s*x\[i\] = f_mul\(f_sub\(root, b\), inv2a\);",
        "sqrt = d^((q+1)/4)": r"return f_pow\(a, \(P \+ 1u\) / 4u\);",
    }
    for t in trees:
        s = t.read_text(errors="replace")
        miss = [k for k, p in pats.items() if not re.search(p, s)]
        if miss:
            print(f"NOT CONFIRMED {ID}: {t.relative_to(ROOT)} lacks {miss}")
            sys.exit(1)
        lines = {k: s[:re.search(p, s).start()].count("\n") + 1 for k, p in pats.items()}
        print(f"source {t.relative_to(CAND)}: " + ", ".join(f"{k} @{v}" for k, v in lines.items()))
    return len(trees)


def secret_forms(sk, n, m, q):
    V, D = 2 * n, 2 * n - 1
    fe = lambda off: sk[off] | (sk[off + 1] << 8)
    Sinv = [[fe(2 * (r * V + c)) for c in range(V)] for r in range(V)]
    # Gauss-Jordan inverse mod q
    M = [row[:] + [int(i == j) for j in range(V)] for i, row in enumerate(Sinv)]
    for c in range(V):
        p = next(r for r in range(c, V) if M[r][c])
        M[c], M[p] = M[p], M[c]
        iv = pow(M[c][c], q - 2, q)
        M[c] = [x * iv % q for x in M[c]]
        for r in range(V):
            if r != c and M[r][c]:
                f = M[r][c]
                M[r] = [(x - f * y) % q for x, y in zip(M[r], M[c])]
    S = [row[V:] for row in M]
    qoff = 2 * (V * V + D * m + (D - m) * D)  # Sinv, U, K, then the Q coefficients
    a0 = fe(qoff)
    return S, a0


def chi(x, q):
    return pow(x % q, (q - 1) // 2, q) == 1


def run_level(level, N):
    n, m, q = params(level)
    V = 2 * n
    path = CAND / f"lib/libFacto-DSA-{level}.so"
    lib = Lib(path, b"ngcc-sign-10-4-" + str(level).encode())
    _, sk_ctl = lib.keygen()  # unrelated key: control secret forms
    pk, sk = lib.keygen()
    S, a0 = secret_forms(sk, n, m, q)
    Sc, a0c = secret_forms(sk_ctl, n, m, q)
    r = [random.Random(level).randrange(q) for _ in range(V)]
    cnt = collections.Counter()
    mut_acc = 0
    for i in range(N):
        msg = b"facto-leak-%d" % i
        rc, sn = lib.sign(sk, msg)
        if rc != 0:
            cnt["signfail"] += 1
            continue
        if lib.verify(pk, sn, msg) != 0:
            print(f"NOT CONFIRMED {ID}: honest Facto-DSA-{level} signature rejected")
            sys.exit(1)
        if i < 20:
            bad = bytearray(sn)
            bad[(7 * i) % len(bad)] ^= 1 << (i % 8)
            mut_acc += lib.verify(pk, bytes(bad), msg) == 0
            mut_acc += lib.verify(pk, sn, msg + b"!") == 0
        cnt["ok"] += 1
        z = [sn[2 * j] | (sn[2 * j + 1] << 8) for j in range(V)]
        XY = [sum(a * b for a, b in zip(row, z)) % q for row in S]
        XYc = [sum(a * b for a, b in zip(row, z)) % q for row in Sc]
        rz = sum(a * b for a, b in zip(r, z)) % q
        cnt["Y==1"] += XY[V - 1] == 1
        cnt["chi"] += chi(2 * a0 * XY[0], q)
        cnt["ctlkey Y==1"] += XYc[V - 1] == 1
        cnt["ctlkey chi"] += chi(2 * a0c * XYc[0], q)
        cnt["r.z==1"] += rz == 1
        cnt["chi(r.z)"] += chi(rz, q)
    ok = cnt["ok"]
    print(f"Facto-DSA-{level}: {ok}/{N} signatures verified, sign failures {cnt['signfail']}, "
          f"mutated/changed-message accepted {mut_acc}/40")
    print(f"  secret (S z)_(2n-1) == 1: {cnt['Y==1']}/{ok} = {cnt['Y==1']/ok:.4f}  (uniform 1/(q-1) = {1/(q-1):.2e})")
    print(f"  secret chi(2 a0 (S z)_0) = +1: {cnt['chi']}/{ok}")
    print(f"  control, unrelated key's forms: Y==1 {cnt['ctlkey Y==1']}/{ok}, chi=+1 {cnt['ctlkey chi']}/{ok}")
    print(f"  control, random public form r: r.z==1 {cnt['r.z==1']}/{ok}, chi(r.z)=+1 {cnt['chi(r.z)']}/{ok}")
    controls = (mut_acc == 0 and cnt["ctlkey Y==1"] <= 1 and cnt["r.z==1"] <= 1
                and 0.3 * ok < cnt["ctlkey chi"] < 0.7 * ok and 0.3 * ok < cnt["chi(r.z)"] < 0.7 * ok)
    return ok, cnt, controls


def main():
    for lv in COUNTS:
        if not (CAND / f"lib/libFacto-DSA-{lv}.so").is_file():
            print(f"SKIP {ID}: sign-10/lib/libFacto-DSA-{lv}.so missing (make -C sign-10)")
            sys.exit(77)
    t0 = time.time()
    ntrees = source_check()
    leak, ctl = True, True
    for lv, N in COUNTS.items():
        ok, cnt, c = run_level(lv, N)
        ctl &= c
        # chi invariant in every signature at every level (chance 2^-ok)
        leak &= cnt["chi"] == ok
        # fixed-value hits far above 1/(q-1): at 128 require >= 1% of signatures
        if lv == 128:
            leak &= cnt["Y==1"] >= 0.01 * ok
    print(f"runtime {time.time() - t0:.1f} s")
    if leak and ctl:
        print(f"CONFIRMED {ID}: honest Facto-DSA signatures fix secret linear-form values "
              f"(chi(2a0 X0)=+1 in every signature at 128/256/512; (Sz)_(2n-1)=1 far above 1/(q-1)); "
              f"source path present in all {ntrees} trees; controls clean")
        sys.exit(0)
    print(f"NOT CONFIRMED {ID}: leak={leak} controls={ctl}")
    sys.exit(1)


main()
