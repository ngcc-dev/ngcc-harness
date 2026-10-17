#!/usr/bin/env python3
"""sign-01-9 Aigis-Sig+: 32-bit message-length arithmetic gives forgeries.

With USE_ICCS, kdf128/256/512(out, int outlen, in, int inlen) call
pseudoXOF(outlen * 8, in, inlen * 8, out) (hashkdf.c:8-45), and signing and
verification compute mu = KDF(CRH(pk) || M) with inlen = CRHBYTES + |M|
(sign.c:87-98 and 167-174).  The bit length inlen * 8 is a 32-bit int:

  A. |M| = 2^29 + j: inlen * 8 wraps to 8 * (CRHBYTES + j), so mu depends only
     on CRH(pk) || M[:j].  A signature on a short j-byte message m verifies
     for every 2^29 + j byte message that starts with m.
  B. 2^28 <= CRHBYTES + |M| < 2^29: inlen * 8 is negative, pseudoXOF's malloc
     fails, kdf ignores the error and mu stays CRH(pk).  One signature on any
     message in this range verifies for every other message in the range.
  C. (AVX2) sig_sign and msig_verf take int32_t mlen, so |M| = 2^32 + j is
     truncated to j bytes.

The prebuilt reference libraries (sign-01/lib, USE_ICCS) are always tested.
If gcc and an AVX2/BMI2 CPU are available, the three AVX2 trees are also
built with -DUSE_ICCS in a temporary directory and tested, including case C.
Long messages are anonymous mmap buffers.  Every verification runs as the
first library call in its own fresh process.  Controls: the honest signature
verifies, a changed short message rejects, the long forgeries with their
first byte changed reject, and a message just below range B rejects.
Memory: about 1.5 GiB peak.
"""
import ctypes
import hashlib
import json
import mmap
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-01"
IMPL = CAND / "Implementations/Implementations"
API = ROOT / "api"
ID = "sign-01-9"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
CRH = {1: 48, 2: 48, 3: 96}                       # CRHBYTES, params.h
TREE = {1: "Aigis-Sig+-I", 2: "Aigis-Sig+-II", 3: "Aigis-Sig+-III"}

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


def run(label, lib, level, avx2):
    S = Sig(lib, seed=f"{ID} {label}".encode())
    pk, sk = S.keygen()
    m = b"Transfer 10 CNY to Bob, ref 0042"
    j = len(m)
    sig = S.sign(sk, m)
    print(f"{label}: |pk|={len(pk)} |sig|={len(sig)} CRHBYTES={CRH[level]}", flush=True)
    X = b"X" + m[1:]
    A = (1 << 29) + j
    B1, B2 = (1 << 28) + 12345, (1 << 28) + 999999
    below = (1 << 28) - CRH[level] - 1
    p1, p2 = b"contract v1: pay 1", b"contract v2: pay 1000000"
    sigB = S.sign(sk, p1, B1, fill=0x41)          # one signing query on a 256 MiB message
    print("  (an 'ERROR: Memory allocation failed' line from pseudoXOF during this 256 MiB signing query is "
          "expected; kdf ignores the error)", flush=True)
    rows = [
        ("honest: sig(m) on m", sig, m, None, 0, True),
        ("control: sig(m) on m||'x'", sig, m + b"x", None, 0, False),
        (f"A FORGERY: sig(m) on m||0x00^(2^29) ({A} bytes)", sig, m, A, 0, True),
        (f"A FORGERY: sig(m) on m||0xa5^(2^29) ({A} bytes)", sig, m, A, 0xA5, True),
        ("A control: same length, first byte changed", sig, X, A, 0, False),
        (f"B honest: sigB on its own {B1}-byte message", sigB, p1, B1, 0x41, True),
        (f"B FORGERY: sigB on other content, {B1} bytes", sigB, p2, B1, 0x5A, True),
        (f"B FORGERY: sigB on other content, {B2} bytes", sigB, b"anything", B2, 0, True),
        (f"B control: sigB on the short message {p2[:11]!r}", sigB, p2[:11], None, 0, False),
        (f"B control: sigB on {below} bytes (just below the range)", sigB, p1, below, 0x41, False),
    ]
    if avx2:
        C = (1 << 32) + j
        rows += [(f"C FORGERY: sig(m) on m||0x00^(2^32) ({C} bytes)", sig, m, C, 0, True),
                 ("C control: same length, first byte changed", sig, X, C, 0, False)]
    ok = True
    for text, s, pre, N, fill, want in rows:
        r, err = fresh_verify(lib, pk, s, pre, N, fill)
        good = isinstance(r, int) and (r == 0) == want
        ok &= good
        note = "  [stderr: pseudoXOF malloc failed]" if "Memory allocation failed" in err else ""
        print(f"  {text}: verify={r} expected={'accept' if want else 'reject'}{note}"
              f"{'' if good else '  <-- UNEXPECTED'}", flush=True)
    return ok


def build_avx2(tmp):
    """Build the three unmodified AVX2 trees (USE_ICCS) as NGCC shared libraries."""
    srcs = ("sign.c polyvec.c packing.c poly.c ntt.c fips202.c hashkdf.c pspm.c fips202x4.c "
            "keccak4x/KeccakP-1600-times4-SIMD256.c auxfunc.c SIG_AlgorithmInstance.c drng.c sample.c rng.c").split()
    flags = ["-O2", "-fPIC", "-shared", "-std=gnu11", "-w", "-fwrapv", "-mavx2", "-mbmi2", "-mpopcnt",
             "-Wno-error=incompatible-pointer-types", "-Wno-error=implicit-function-declaration",
             "-Wno-error=int-conversion", "-DUSE_ICCS", "-DNGCC_BUILD_SIG",
             '-DNGCC_ID="sign-01"', '-DNGCC_ALG="Aigis-Sig+"']
    libs = {}
    for level, tree in TREE.items():
        src = IMPL / "Optimized_Implementation/avx2" / tree
        work = Path(tmp) / tree
        shutil.copytree(src, work, symlinks=False)
        for o in (work / "keccak4x").glob("*.o"):
            o.unlink()                                  # rebuild the shipped object from source
        assert "int32_t msig_verf" in (work / "sign.c").read_text()
        assert "pseudoXOF(outlen * 8, in, inlen * 8, out);" in (work / "hashkdf.c").read_text()
        out = work / f"libaigis{level}-avx2.so"
        cmd = ["gcc", *flags, f"-DPARAMS={level}", "-I.", "-Ikeccak4x", f"-I{API}", *srcs,
               str(API / "link_shim.c"), "-lm", "-Wl,--no-undefined", "-o", str(out)]
        p = subprocess.run(cmd, cwd=work, capture_output=True, text=True)
        if p.returncode != 0:
            print(p.stderr[-2000:])
            return None
        libs[level] = out
    return libs


def main():
    ref = {l: CAND / f"lib/libAigis-sig{l}.so" for l in (1, 2, 3)}
    if not all(p.is_file() for p in ref.values()):
        print(f"SKIP {ID}: reference libraries missing (build them with make -C sign-01)")
        return 77
    for l in (1, 2, 3):
        src = (IMPL / f"Reference_Implementation/{TREE[l]}/hashkdf.c").read_text()
        assert src.count("pseudoXOF(outlen * 8, in, inlen * 8, out);") == 3
    ok = all([run(f"reference {TREE[l]} (lib/libAigis-sig{l}.so)", ref[l], l, False) for l in (1, 2, 3)])
    cpu = Path("/proc/cpuinfo").read_text() if Path("/proc/cpuinfo").exists() else ""
    avx2_status = "skipped"
    if not shutil.which("gcc"):
        print("SKIP AVX2 part: gcc unavailable")
    elif not all(f" {x}" in cpu for x in ("avx2", "bmi2", "popcnt")):
        print("SKIP AVX2 part: CPU lacks AVX2/BMI2")
    else:
        with tempfile.TemporaryDirectory(prefix="sign-01-9.") as tmp:
            libs = build_avx2(tmp)
            if libs is None:
                print(f"NOT CONFIRMED {ID}: AVX2 build failed")
                return 1
            ok &= all([run(f"AVX2 {TREE[l]} (temporary build)", libs[l], l, True) for l in (1, 2, 3)])
            avx2_status = "confirmed"
    if ok:
        extra = " and all three AVX2 trees (also at 2^32 + j)" if avx2_status == "confirmed" else \
                " (AVX2 part skipped)"
        print(f"CONFIRMED {ID}: Aigis-Sig+ I/II/III reference USE_ICCS libraries{extra} accept a short-message "
              "signature on a 2^29 + j byte message and transfer one signature across the negative-length range")
        return 0
    print(f"NOT CONFIRMED {ID}: a forgery rejected or a control accepted")
    return 1


if __name__ == "__main__":
    sys.exit(main())
