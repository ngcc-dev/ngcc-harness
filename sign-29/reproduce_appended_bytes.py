#!/usr/bin/env python3
"""sign-29-3 extension: Tins ignores up to NODE_SIZE - 1 appended signature bytes.

sig_verify derives path_size = (sn_len_bytes - NTemp) / NODE_SIZE and then
parses fixed-size fields from sn (Tins128 SIG_TINS128.c:354-371; Tins256
:352-369; Tins512 :360-377).  For an honest signature sn_len_bytes - NTemp is
a multiple of NODE_SIZE, so appending r < NODE_SIZE bytes leaves path_size
and every parse offset unchanged: the appended bytes are never read and the
verifier sees exactly the inputs of the honest signature.  Appending
NODE_SIZE bytes adds a path node and shifts every later field (and overruns
node path[T_OPEN] when path_size is already T_OPEN).

The verdict alone is weak evidence here: the Tins128 verifier compares its
recomputed challenge with an uninitialized buffer, and the Tins512 verifier
decodes overwritten auxiliary data, so both reject honest signatures in a
fresh process.  Each child process therefore places the signature so that it
ends exactly at a PROT_NONE guard page and passes sn_len_bytes = |sn| + r.
If the verifier read any of the r appended bytes it would fault.  The verdict
is printed as well; Tins256 accepts its honest signature and every extension.
Controls: r = 0 completes; with the last real signature byte moved into the
guard page the verifier faults (so reads are detected); r = NODE_SIZE
terminates abnormally (the parser faults, or the prebuilt libraries'
_FORTIFY_SOURCE check aborts the oversized path copy).
"""
import ctypes
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "sign-29"
REF = CAND / "Implementations/Reference_Implementation"
ID = "sign-29-3"
U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong
LEVELS = (128, 256, 512)

CHILD = r'''
import ctypes, hashlib, json, mmap, sys
U8 = ctypes.c_ubyte; ULL = ctypes.c_ulonglong
def arr(d, extra=64):
    b = (U8 * (len(d) + extra))(); ctypes.memmove(b, bytes(d), len(d)); return b
L = ctypes.CDLL(sys.argv[1])
L.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
d = hashlib.sha512(b"fresh verifier").digest(); L.ngcc_seed(arr(d), len(d))
L.sig_verify.argtypes = [ctypes.POINTER(U8), ULL, ctypes.c_void_p, ULL, ctypes.POINTER(U8), ULL]
L.sig_verify.restype = ctypes.c_int
libc = ctypes.CDLL(None, use_errno=True)
libc.mprotect.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
a = json.loads(sys.stdin.read()); pk, sn, m = (bytes.fromhex(a[k]) for k in ("pk", "sn", "m"))
extra, shift = a["extra"], a["shift"]
P = mmap.PAGESIZE
npages = (len(sn) + P - 1) // P + 1
mm = mmap.mmap(-1, (npages + 1) * P, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS)
buf = (U8 * ((npages + 1) * P)).from_buffer(mm)
guard = ctypes.addressof(buf) + npages * P
assert libc.mprotect(guard, P, 0) == 0               # PROT_NONE
start = guard - len(sn) + shift                      # signature ends at the guard page
ctypes.memmove(start, sn, len(sn) - shift)
print("READY", flush=True)
print("RESULT", L.sig_verify(arr(pk), len(pk), start, len(sn) + extra, arr(m), len(m)), flush=True)
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


def guarded_verify(lib, pk, sn, m, extra, shift=0):
    """Return ('ok', verdict) if sig_verify returned, or ('signal N', None)."""
    try:
        p = subprocess.run([sys.executable, "-I", "-c", CHILD, str(lib)],
                           input=json.dumps({"pk": pk.hex(), "sn": sn.hex(), "m": m.hex(),
                                             "extra": extra, "shift": shift}),
                           capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        return "timeout", None
    if "READY" not in p.stdout:
        return f"setup failed rc={p.returncode}", None
    r = [ln.split()[1] for ln in p.stdout.splitlines() if ln.startswith("RESULT ")]
    if r and p.returncode == 0:
        return "returned", int(r[-1])
    why = "fortify abort" if "buffer overflow detected" in p.stderr else ""
    return (f"signal {-p.returncode}" if p.returncode < 0 else f"rc={p.returncode}") + (f" ({why})" if why else ""), None


def params(level):
    h = (REF / f"Tins{level}/params.h").read_text()
    lam = int(re.search(r"#define LAMBDA (\d+)", h).group(1))
    t_open = int(re.search(r"#define T_OPEN (\d+)", h).group(1))
    src = (REF / f"Tins{level}/SIG_TINS{level}.c").read_text()
    assert "path_size = (sn_len_bytes - NTemp) / NODE_SIZE;" in src
    assert "memcpy(path, sn + pos, NODE_SIZE * path_size);" in src
    return lam // 8, t_open


def run(level):
    lib = CAND / f"lib/libTins{level}.so"
    node, t_open = params(level)
    S = Sig(lib, seed=f"{ID} Tins{level}".encode())
    pk, sk = S.keygen()
    m = b"Tins appended-bytes witness"
    sn = S.sign(sk, m)
    print(f"Tins{level}: |sig|={len(sn)} NODE_SIZE={node} T_OPEN={t_open}", flush=True)
    rows = [("r=0 (honest length)", 0, 0, True)]
    rows += [(f"r={r} appended bytes beyond the guard", r, 0, True) for r in sorted({1, node // 2, node - 1})]
    rows += [("control: last signature byte inside the guard page", 0, 1, False),
             (f"control: r={node} (one more path node)", node, 0, False)]
    ok = True
    verdicts = set()
    for label, extra, shift, want in rows:
        status, verdict = guarded_verify(lib, pk, sn, m, extra, shift)
        returned = status == "returned"
        if returned:
            verdicts.add(verdict)
        good = returned == want
        ok &= good
        shown = f"returned, verdict={verdict}" if returned else status
        print(f"  {label}: {shown}; expected {'normal return' if want else 'abnormal termination'}"
              f"{'' if good else '  <-- UNEXPECTED'}", flush=True)
    print(f"  verdicts for r = 0 .. {node - 1}: {sorted(verdicts)}"
          f"{' (all accepted)' if verdicts == {0} else ''}", flush=True)
    return ok and len(verdicts) == 1, verdicts == {0}


def main():
    libs = [CAND / f"lib/libTins{l}.so" for l in LEVELS]
    if not all(p.is_file() for p in libs):
        print(f"SKIP {ID}: Tins libraries missing (build them with make -C sign-29)")
        return 77
    res = {l: run(l) for l in LEVELS}
    if all(v[0] for v in res.values()):
        acc = [f"Tins{l}" for l, v in res.items() if v[1]]
        print(f"CONFIRMED {ID}: Tins128/256/512 never read up to NODE_SIZE - 1 appended signature bytes "
              f"(guard-page test){'; ' + ', '.join(acc) + ' accepts the extended signatures' if acc else ''}")
        return 0
    print(f"NOT CONFIRMED {ID}: unexpected guard-page behaviour")
    return 1


if __name__ == "__main__":
    sys.exit(main())
