#!/usr/bin/env python3
"""kem-33-4: optimized QUBE compares only a prefix of u and v in the FO check.

Usage: python3 kem-33/reproduce_opt_partial_compare.py OPT_LIBDIR [TRIALS]

OPT_LIBDIR holds libqube-opt-{1,3,4,5}.so built from the submitted
Optimized_Implementation (kem-33/reproduce_opt_partial_compare.sh does this in
a temporary directory). The prebuilt reference libraries kem-33/lib/libqube-*.so
are the control. NGCC_LONG=1 raises the default from 10 to 50 trials per set.

Static part: crypto_kem_dec (src/common/kem.c) passes the 64-bit-word counts
VEC_N_256_SIZE_64 and VEC_N1N2_256_SIZE_64 to vect_compare, whose loop in
src/x86/vector.c runs over bytes. The compared prefix sizes are computed from
each parameters.h.

Runtime part: for fresh keys and encapsulations, one bit flipped in the
unchecked tail of u or of v must still return the honest shared key, while one
bit flipped in the compared prefixes or in the salt must not. The same tail
flips against the reference libraries must not return the honest key.
"""
import ctypes
import os
import random
import re
import sys
from pathlib import Path

ID = "kem-33-4"
CAND = Path(__file__).resolve().parent
ROOT = CAND.parent
OPT = CAND / "Implementations" / "Optimized_Implementation"
REF = CAND / "Implementations" / "Reference_Implementation"
SETS = {1: "128", 3: "256", 4: "384", 5: "512"}   # optimized qube-p -> QUBE level

U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong


def fail(msg):
    print(f"NOT CONFIRMED {ID}: {msg}")
    sys.exit(1)


def cdiv(a, b):
    return -(-a // b)


def macros(path):
    out = {}
    for line in path.read_text(errors="replace").splitlines():
        m = re.match(r"\s*#define\s+(\w+)\s+(\d+)\b", line)
        if m:
            out[m.group(1)] = int(m.group(2))
    return out


def static_check():
    kem = (OPT / "src/common/kem.c").read_text(errors="replace").splitlines()
    want = {
        257: "result |= vect_compare((uint8_t *)c_kem_t.c_pke.u, (uint8_t *)c_kem_prime_t.c_pke.u, VEC_N_256_SIZE_64);",
        258: "result |= vect_compare((uint8_t *)c_kem_t.c_pke.v, (uint8_t *)c_kem_prime_t.c_pke.v, VEC_N1N2_256_SIZE_64);",
        259: "result |= vect_compare(c_kem_t.salt, c_kem_prime_t.salt, SALT_BYTES);",
        240: "pseudohash((SHARED_SECRET_BYTES+SEED_BYTES)*8,randomness,8*(PARAM_SECURITY_BYTES+32+SALT_BYTES),K_theta_prime);",
    }
    for n, s in want.items():
        if kem[n - 1].strip() != s:
            fail(f"src/common/kem.c:{n} does not match: {kem[n - 1].strip()!r}")
    vec = (OPT / "src/x86/vector.c").read_text(errors="replace").splitlines()
    if not vec[282].startswith("uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size)"):
        fail("src/x86/vector.c:283 is not vect_compare")
    body = "\n".join(vec[283:290])
    if "for (size_t i = 0; i < size; i++)" not in body or "r |= v1[i] ^ v2[i];" not in body:
        fail("vect_compare does not loop over size bytes")
    print("source: kem.c:257-258 pass VEC_N_256_SIZE_64 / VEC_N1N2_256_SIZE_64 to "
          "vect_compare, whose loop (vector.c:283-290) counts bytes")

    layout = {}
    for p, lvl in SETS.items():
        hdr = OPT / f"src/x86/qube-{p}/parameters.h"
        text = hdr.read_text(errors="replace")
        if "(CEIL_DIVIDE(PARAM_N_MULT, 256) << 2)" not in text or \
           "(CEIL_DIVIDE(PARAM_N1N2, 256) << 2)" not in text:
            fail(f"qube-{p}/parameters.h: unexpected VEC_*_256_SIZE_64 definition")
        m = macros(hdr)
        ub, vb = cdiv(m["PARAM_N"], 8), cdiv(m["PARAM_N1N2"], 8)
        ucmp = cdiv(m["PARAM_N_MULT"], 256) << 2
        vcmp = cdiv(m["PARAM_N1N2"], 256) << 2
        layout[p] = (ub, vb, ucmp, vcmp, m["SALT_BYTES"])
        print(f"source: QUBE-{lvl} (qube-{p}) compares u {ucmp}/{ub} bytes, "
              f"v {vcmp}/{vb} bytes, salt {m['SALT_BYTES']}/{m['SALT_BYTES']} bytes")
    return layout


class Kem:
    def __init__(self, path):
        self.lib = lib = ctypes.CDLL(str(path))
        for f in ("pk", "sk", "ct", "ss"):
            getattr(lib, f"kem_get_{f}_len_bytes").restype = ULL
        self.sz = {f: int(getattr(lib, f"kem_get_{f}_len_bytes")()) for f in ("pk", "sk", "ct", "ss")}
        seed = os.urandom(48)
        if hasattr(lib, "ngcc_seed"):          # prebuilt reference (harness shim)
            lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
            lib.ngcc_seed((U8 * 48)(*seed), 48)
        elif hasattr(lib, "prng_init"):        # optimized tree: seed its SHAKE PRNG
            # as tests/qube-test.c does; the API wrapper never seeds it.
            lib.prng_init((U8 * 48)(*seed), None, 48, 0)
        else:
            fail(f"{path.name}: no seeding entry point")

    @staticmethod
    def buf(data):
        b = (U8 * len(data))()
        ctypes.memmove(b, bytes(data), len(data))
        return b

    def keygen(self):
        pk, sk = (U8 * self.sz["pk"])(), (U8 * self.sz["sk"])()
        pl, sl = ULL(0), ULL(0)
        if self.lib.kem_keygen(pk, ctypes.byref(pl), sk, ctypes.byref(sl)) != 0:
            fail("kem_keygen failed")
        return bytes(pk), bytes(sk)

    def enc(self, pk):
        ct, ss = (U8 * self.sz["ct"])(), (U8 * self.sz["ss"])()
        cl, sl = ULL(0), ULL(0)
        if self.lib.kem_enc(self.buf(pk), ULL(len(pk)), ss, ctypes.byref(sl), ct, ctypes.byref(cl)) != 0:
            fail("kem_enc failed")
        return bytes(ct), bytes(ss)

    def dec(self, sk, ct):
        ss, sl = (U8 * self.sz["ss"])(), ULL(0)
        self.lib.kem_dec(self.buf(sk), ULL(len(sk)), self.buf(ct), ULL(len(ct)), ss, ctypes.byref(sl))
        return bytes(ss)


def flip(ct, i, bit):
    c = bytearray(ct)
    c[i] ^= 1 << bit
    return bytes(c)


def run_opt(libdir, p, layout, trials, rnd):
    ub, vb, ucmp, vcmp, salt = layout
    k = Kem(Path(libdir) / f"libqube-opt-{p}.so")
    if k.sz["ct"] != ub + vb + salt:
        fail(f"qube-{p}: ciphertext size {k.sz['ct']} != {ub}+{vb}+{salt}")
    st = dict(honest=0, u_tail=0, v_tail=0, u_prefix=0, v_prefix=0, salt=0)
    for _ in range(trials):
        pk, sk = k.keygen()
        ct, ss = k.enc(pk)
        st["honest"] += k.dec(sk, ct) == ss
        # u's last byte holds padding bits, so the tail test stops one byte short.
        st["u_tail"] += k.dec(sk, flip(ct, rnd.randrange(ucmp, ub - 1), rnd.randrange(8))) == ss
        st["v_tail"] += k.dec(sk, flip(ct, ub + rnd.randrange(vcmp, vb), rnd.randrange(8))) == ss
        st["u_prefix"] += k.dec(sk, flip(ct, rnd.randrange(0, ucmp), rnd.randrange(8))) == ss
        st["v_prefix"] += k.dec(sk, flip(ct, ub + rnd.randrange(0, vcmp), rnd.randrange(8))) == ss
        st["salt"] += k.dec(sk, flip(ct, ub + vb + rnd.randrange(salt), rnd.randrange(8))) == ss
    print(f"optimized QUBE-{SETS[p]}: honest key kept {st['honest']}/{trials}; "
          f"u-tail flip {st['u_tail']}/{trials}, v-tail flip {st['v_tail']}/{trials}; "
          f"controls u-prefix {st['u_prefix']}/{trials}, v-prefix {st['v_prefix']}/{trials}, "
          f"salt {st['salt']}/{trials}")
    return (st["honest"] == trials and st["u_tail"] == trials and st["v_tail"] == trials
            and st["u_prefix"] == 0 and st["v_prefix"] == 0 and st["salt"] == 0)


def run_ref(lvl, opt_layout, trials, rnd):
    m = macros(REF / f"qube-{lvl}/src/ref/parameters.h")
    ub, vb, salt = cdiv(m["PARAM_N"], 8), cdiv(m["PARAM_N1N2"], 8), m["PARAM_SECURITY_BYTES"]
    _, _, ucmp, vcmp, _ = opt_layout
    k = Kem(CAND / "lib" / f"libqube-{lvl}.so")
    if k.sz["ct"] != ub + vb + salt:
        fail(f"reference qube-{lvl}: ciphertext size {k.sz['ct']} != {ub}+{vb}+{salt}")
    honest = u_tail = v_tail = 0
    for _ in range(trials):
        pk, sk = k.keygen()
        ct, ss = k.enc(pk)
        honest += k.dec(sk, ct) == ss
        u_tail += k.dec(sk, flip(ct, rnd.randrange(ucmp, ub - 1), rnd.randrange(8))) == ss
        v_tail += k.dec(sk, flip(ct, ub + rnd.randrange(vcmp, vb), rnd.randrange(8))) == ss
    print(f"control reference QUBE-{lvl}: honest key kept {honest}/{trials}; "
          f"same tail flips kept it u {u_tail}/{trials}, v {v_tail}/{trials}")
    return honest == trials and u_tail == 0 and v_tail == 0


def main():
    if len(sys.argv) < 2:
        print(__doc__.strip().splitlines()[2])
        sys.exit(2)
    libdir = sys.argv[1]
    trials = int(sys.argv[2]) if len(sys.argv) > 2 else (50 if os.environ.get("NGCC_LONG") == "1" else 10)
    rnd = random.Random(int.from_bytes(os.urandom(8), "little"))
    layout = static_check()
    ok = True
    for p in SETS:
        ok &= run_opt(libdir, p, layout[p], trials, rnd)
    for p, lvl in SETS.items():
        ok &= run_ref(lvl, layout[p], trials, rnd)
    if not ok:
        fail("an alias or control did not behave as expected")
    print(f"CONFIRMED {ID}: optimized QUBE-128/256/384/512 accept ciphertexts changed in the "
          f"unchecked u and v tails with the honest key; the reference rejects them")


if __name__ == "__main__":
    main()
