#!/usr/bin/env python3
"""Reproduce kex-05-8: Loom-KEM encryption reuses lifting PRF substreams for r.

Algorithm 11 (spec p. 10) lifts the public key from one stream PRF(seed1, 0)
and samples r[i] from PRF(seed1, 1..k); the IND-CPA proof (Game 2, pp. 24-25)
relies on these counters being distinct. The code passes the caller's nonce
by value to polyvec_invq (indcpa.c:351), which advances its own copy once per
public-key polynomial (poly_invq.c:97, 102), and then samples r[i] on nonces
1..k (indcpa.c:365). The ICCS prf (pseudoXOF(seed||nonce)) is
prefix-consistent, so each r[i]'s CBD input begins with a lifting stream.

The witness copies the submitted reference sources to a temporary directory,
renames the PRF symbol in the compiled symmetric-iccs object with objcopy, and
links a logging wrapper around the unchanged PRF. Control: for the same DRBG
seed, the instrumented build produces byte-identical messages and session
keys to the prebuilt lib/libLoomKEX-<L>.so. A second control checks that
streams on distinct (seed, nonce) inputs share no prefix.
"""

import ctypes
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ID = "kex-05-8"
ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "kex-05"
IMPL = CAND / "Implementations and Test_Vectors" / "Implementations"
# level: LOOM_MODE, Shuttle NTT set, K, N, eta2, SYMBYTES
LEVELS = {128: (1, "q15361n256", 5, 128, 2, 32),
          256: (3, "q61441n512", 4, 256, 4, 32),
          512: (5, "q59393n1024", 4, 512, 8, 64)}
U = ctypes.c_ulonglong
KEM = "cbd.c indcpa.c kem_cpaf.c msgenc.c ntt.c poly.c poly_invq.c polyvec.c reduce.c verify.c".split()
SIG = ("rounding.c sign.c polyvec.c sampler.c sampler_u.c irs.c packing.c poly.c "
       "poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c").split()
PROTO = "loom.c prf_mac.c state_serialize.c drng.c auxfunc.c".split()
NLOG, MAXB = 64, 4096

HOOK = r"""
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "drng.h"
DRNG_ctx drng_algorithm;
int ngccw_seed(const unsigned char *s, unsigned long long n)
{ return init_random_number(&drng_algorithm, s, n); }
void ngccw_real_prf(uint8_t *out, size_t outlen, const uint8_t *key, uint8_t nonce);
int ngccw_logging, ngccw_n;
unsigned int ngccw_nonce[NLOG];
unsigned long long ngccw_len[NLOG];
uint8_t ngccw_key[NLOG][SYM];
uint8_t ngccw_out[NLOG][MAXB];
void PRFNAME(uint8_t *out, size_t outlen, const uint8_t *key, uint8_t nonce)
{
    ngccw_real_prf(out, outlen, key, nonce);
    if (ngccw_logging && ngccw_n < NLOG) {
        int i = ngccw_n++;
        ngccw_nonce[i] = nonce;
        ngccw_len[i] = outlen;
        memcpy(ngccw_key[i], key, SYM);
        memcpy(ngccw_out[i], out, outlen < MAXB ? outlen : MAXB);
    }
}
"""


def skip(msg):
    print(f"SKIP {ID}: {msg}")
    sys.exit(77)


def fail(msg):
    print(f"NOT CONFIRMED {ID}: {msg}")
    sys.exit(1)


def require(cond, msg):
    if not cond:
        fail(msg)


def sh(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        fail(f"command failed: {' '.join(cmd[:3])} ...\n{r.stderr[-2000:]}")
    return r.stdout


def static_check():
    """The cited lines are present, identically, in all six trees."""
    seen = set()
    for tree in ("Reference_Implementation", "Optimized_Implementation"):
        for level in LEVELS:
            kem = IMPL / tree / f"LoomKEX-{level}" / "kem"
            enc = kem.joinpath("indcpa.c").read_text(errors="replace").splitlines()
            inv = kem.joinpath("poly_invq.c").read_text(errors="replace").splitlines()
            off = 2 if tree == "Optimized_Implementation" else 0
            require(enc[350 + off].strip() == "polyvec_invq(&pkpv, coins, nonce++);" and
                    enc[364 + off].strip() == "poly_getnoise_eta2(sp.vec+i, coins, nonce++);",
                    f"{tree} LoomKEX-{level} indcpa.c lifting/r lines moved")
            require("#define INV_Q_LIFTING" in "\n".join(enc[:20]), "INV_Q_LIFTING not default")
            require(inv[96].strip() == "prf(buf, GEN_INVQ_RAND_BYTES, seed, nonce++);" and
                    inv[101].strip() == "prf(buf, GEN_INVQ_RAND_BYTES, seed, nonce++);",
                    f"{tree} LoomKEX-{level} poly_invq.c:97/102 changed")
            seen.add(tree)
    print("static: indcpa.c:351 polyvec_invq(&pkpv, coins, nonce++) and :365 r[i] on nonce 1..k "
          "(optimized :353/:367); poly_invq.c:97,102 advance a by-value nonce copy; all 6 trees")


def build(level, tmp):
    mode, ntt, _, _, _, sym = LEVELS[level]
    src = tmp / f"LoomKEX-{level}"
    shutil.copytree(IMPL / "Reference_Implementation" / f"LoomKEX-{level}", src, symlinks=False)
    obj = tmp / f"obj{level}"
    obj.mkdir()
    prfname = f"loom{mode}_ref_weaver_iccs_prf"
    hook = HOOK.replace("PRFNAME", prfname).replace("NLOG", str(NLOG)).replace(
        "MAXB", str(MAXB)).replace("SYM]", f"{sym}]").replace("SYM)", f"{sym})")
    (src / "loom" / "ngccw_hook.c").write_text(hook)
    cflags = ["-O2", "-w", "-fPIC", "-std=gnu11", "-fwrapv", "-D_GNU_SOURCE",
              "-Wno-error=incompatible-pointer-types", "-Wno-error=implicit-function-declaration",
              f"-DLOOM_MODE={mode}", f"-I{src / 'loom'}", f"-I{src / 'kem'}", f"-I{src / 'sig'}"]
    files = [src / "loom" / f for f in PROTO + [f"KEX_LoomKEX-{level}.c", "ngccw_hook.c"]]
    files += [src / "kem" / f for f in KEM] + [src / "sig" / f for f in SIG]
    files.append(src / "sig" / "ntt" / ntt / "ntt_ref.c")
    objs = []
    for i, f in enumerate(files):
        o = obj / f"{i:02d}_{f.stem}.o"
        sh(["gcc"] + cflags + ["-c", str(f), "-o", str(o)])
        objs.append(o)
    sym_o = obj / "symmetric-iccs.o"
    sh(["gcc"] + cflags + ["-c", str(src / "loom" / "symmetric-iccs.c"), "-o", str(sym_o)])
    require(re.search(rf"\bT {prfname}$", sh(["nm", str(sym_o)]), re.M), f"{prfname} not found")
    sh(["objcopy", f"--redefine-sym={prfname}=ngccw_real_prf", str(sym_o)])
    out = tmp / f"libloom-instr-{level}.so"
    sh(["gcc", "-shared", "-o", str(out)] + [str(o) for o in objs + [sym_o]] + ["-lm"])
    return ctypes.CDLL(str(out), mode=ctypes.RTLD_LOCAL)


def buf(n):
    return ctypes.create_string_buffer(max(1, n))


class Kex:
    def __init__(self, lib, seedfn):
        self.lib = L = lib
        self.seedfn = getattr(L, seedfn)
        for n in ("kex_get_pk_len_bytes", "kex_get_sk_len_bytes", "kex_get_sta_len_bytes",
                  "kex_get_stb_len_bytes", "kex_get_ss_len_bytes", "kex_get_total_msg_len_bytes"):
            getattr(L, n).restype = U
        self.pkl, self.skl = L.kex_get_pk_len_bytes(), L.kex_get_sk_len_bytes()
        self.stl = max(L.kex_get_sta_len_bytes(), L.kex_get_stb_len_bytes()) + 4096
        self.msgl = L.kex_get_total_msg_len_bytes() + 64
        self.ssl = L.kex_get_ss_len_bytes() + 64

    def init(self, side):
        pk, sk, st = buf(self.pkl), buf(self.skl), buf(self.stl)
        pl, sl, tl = U(), U(), U()
        rc = getattr(self.lib, f"kex_init_{side}")(pk, ctypes.byref(pl), sk, ctypes.byref(sl),
                                                   st, ctypes.byref(tl))
        require(rc == 0, f"kex_init_{side} rc={rc}")
        return pk.raw[:pl.value], sk.raw[:sl.value], st.raw[:tl.value]

    def step(self, name, sk, pk, mi, st_in):
        st = buf(self.stl); ctypes.memmove(st, st_in, len(st_in)); stl = U(len(st_in))
        m, ml = buf(self.msgl), U()
        f = getattr(self.lib, name)
        if name.endswith("pass1_msg_a"):
            rc = f(sk, U(len(sk)), pk, U(len(pk)), st, ctypes.byref(stl), m, ctypes.byref(ml))
        else:
            mib = buf(len(mi)); ctypes.memmove(mib, mi, len(mi))
            rc = f(sk, U(len(sk)), pk, U(len(pk)), mib, U(len(mi)), st, ctypes.byref(stl), m,
                   ctypes.byref(ml))
        return rc, st.raw[:stl.value], m.raw[:ml.value]

    def derive(self, side, sk, pk, m, st):
        ss, sl = buf(self.ssl), U()
        mb = buf(len(m)); ctypes.memmove(mb, m, len(m))
        stb = buf(len(st)); ctypes.memmove(stb, st, len(st))
        rc = getattr(self.lib, f"kex_derive_ss_{side}")(sk, U(len(sk)), pk, U(len(pk)), mb, U(len(m)),
                                                        stb, U(len(st)), ss, ctypes.byref(sl))
        return rc, ss.raw[:sl.value]

    def exchange(self, seed, during_pass2=None):
        require(self.seedfn(seed, U(len(seed))) == 0, "seed")
        pkA, skA, stA = self.init("a")
        pkB, skB, stB = self.init("b")
        rc, stA, m1 = self.step("kex_generate_pass1_msg_a", skA, pkB, b"", stA)
        require(rc == 0, f"pass1 rc={rc}")
        if during_pass2:
            during_pass2(True)
        rc, stB, m2 = self.step("kex_generate_pass2_msg_b", skB, pkA, m1, stB)
        if during_pass2:
            during_pass2(False)
        require(rc == 0, f"pass2 rc={rc}")
        rc, stA, m3 = self.step("kex_generate_pass3_msg_a", skA, pkB, m2, stA)
        require(rc == 0, f"pass3 rc={rc}")
        rc, stB, m4 = self.step("kex_generate_pass4_msg_b", skB, pkA, m3, stB)
        require(rc == 1, f"pass4 rc={rc}")
        ra, sa = self.derive("a", skA, pkB, m4, stA)
        rb, sb = self.derive("b", skB, pkA, m3, stB)
        require(ra == 0 and rb == 0 and sa == sb, "honest exchange did not agree")
        return (m1, m2, m3, m4, sa)


def common_prefix(a, b):
    n = 0
    for x, y in zip(a, b):
        if x != y:
            break
        n += 1
    return n


def run_level(level, tmp):
    mode, _, K, N, eta2, sym = LEVELS[level]
    rlen = eta2 * N // 4
    lib = build(level, tmp)
    instr = Kex(lib, "ngccw_seed")
    shipped = Kex(ctypes.CDLL(str(CAND / "lib" / f"libLoomKEX-{level}.so"), mode=ctypes.RTLD_LOCAL),
                  "ngcc_seed")
    seed = bytes((level + 3 * i) & 255 for i in range(48))
    logging = ctypes.c_int.in_dll(lib, "ngccw_logging")
    count = ctypes.c_int.in_dll(lib, "ngccw_n")

    def toggle(on):
        if on:
            count.value = 0
        logging.value = 1 if on else 0

    t_instr = instr.exchange(seed, toggle)
    t_ship = shipped.exchange(seed)
    require(t_instr == t_ship, f"LoomKEX-{level}: instrumented build differs from the shipped library")
    n = count.value
    nonces = (ctypes.c_uint * NLOG).in_dll(lib, "ngccw_nonce")
    lens = (ctypes.c_ulonglong * NLOG).in_dll(lib, "ngccw_len")
    keys = ((ctypes.c_ubyte * sym) * NLOG).in_dll(lib, "ngccw_key")
    outs = ((ctypes.c_ubyte * MAXB) * NLOG).in_dll(lib, "ngccw_out")
    calls = [(nonces[i], lens[i], bytes(keys[i]), bytes(outs[i])[:min(lens[i], MAXB)]) for i in range(n)]
    # Pass 2 (responder encapsulation): lifting calls first, then the k r[i] calls.
    rcalls = calls[-K:]
    require(all(c[1] == rlen for c in rcalls) and [c[0] for c in rcalls] == list(range(1, K + 1)),
            f"LoomKEX-{level}: r[i] calls not on nonces 1..{K}: {[(c[0], c[1]) for c in calls]}")
    lift = calls[:-K]
    llen = lift[0][1]
    require(lift and all(c[1] == llen for c in lift) and
            [c[0] for c in lift] == list(range(len(lift))) and len(lift) >= K,
            f"LoomKEX-{level}: unexpected lifting call pattern {[(c[0], c[1]) for c in lift]}")
    coins = rcalls[0][2]
    require(all(c[2] == coins for c in calls), "PRF key is not the encryption coins seed1")
    reused = []
    for i, (nonce, _, _, rout) in enumerate(rcalls):
        if nonce < len(lift):
            shared = common_prefix(lift[nonce][3], rout)
            require(shared == min(llen, rlen),
                    f"LoomKEX-{level}: r[{i}] stream does not extend lifting stream {nonce}")
            reused.append(i)
        else:   # no lifting call reached this nonce: the stream must be unrelated
            require(max(common_prefix(l[3], rout) for l in lift) < 4, "unexpected prefix")
    # Control: distinct (seed, nonce) inputs share no meaningful prefix.
    ctrl = max(common_prefix(lift[0][3], r[3]) for r in rcalls)
    ctrl = max(ctrl, max(common_prefix(lift[j][3], lift[j + 1][3]) for j in range(len(lift) - 1)))
    require(ctrl < 4, f"control: distinct nonces share {ctrl} leading bytes")
    print(f"  LoomKEX-{level}: control instrumented build == shipped .so (m1..m4, ss identical); "
          f"lifting used nonces 0..{len(lift) - 1} ({llen} B each), r[0..{K - 1}] used nonces 1..{K} "
          f"({rlen} B each); r{reused} ({len(reused)}/{K}) CBD inputs begin with all "
          f"{min(llen, rlen)} bytes of a lifting stream; control distinct-nonce prefix <= {ctrl} B")
    require(len(reused) >= K - 1, f"LoomKEX-{level}: only {len(reused)}/{K} r[i] reuse lifting")
    return len(reused), K


def main():
    for tool in ("gcc", "objcopy", "nm"):
        if shutil.which(tool) is None:
            skip(f"{tool} unavailable")
    missing = [L for L in LEVELS if not (CAND / "lib" / f"libLoomKEX-{L}.so").is_file()]
    if missing:
        skip(f"build the libraries first (make -C kex-05): LoomKEX-{missing}")
    static_check()
    tot = got = 0
    with tempfile.TemporaryDirectory(prefix="ngcc-kex05-") as t:
        for level in LEVELS:
            r, k = run_level(level, Path(t))
            got += r
            tot += k
    print(f"CONFIRMED {ID}: {got}/{tot} ephemeral-secret polynomials r[i] at LoomKEX-128/256/512 "
          "are sampled from PRF(seed1, nonce) streams already consumed by public-key lifting")
    return 0


if __name__ == "__main__":
    sys.exit(main())
