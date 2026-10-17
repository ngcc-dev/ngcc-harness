#!/usr/bin/env python3
"""Reproduce kex-05-7: Loom's ICCS wrappers use constant party identities.

All honest traffic runs through the prebuilt lib/libLoomKEX-<L>.so. Eve is an
ordinary registered party with her own honestly generated key. Her only
computation is one Shuttle signature with that key, made by a scratch build of
the submitted reference sources (copied to a temporary directory; nothing is
built in-tree).

Variant I: A believes its peer is B; B believes its initiator is Eve.
Variant R: A believes its responder is Eve; B believes its initiator is A.
In both, A and B accept and derive the same session key.

Controls: an honest A-B exchange agrees; the unmodified message is rejected
when the receiver expects Eve; and the submitted loom.c core with distinct
per-party identities rejects the same substitution (MAC failure, -11) while
an honest distinct-ID exchange completes.
"""

import ctypes
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ID = "kex-05-7"
ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "kex-05"
IMPL = CAND / "Implementations and Test_Vectors" / "Implementations"
LEVELS = {128: (1, "q15361n256", 752, 736),
          256: (3, "q61441n512", 1312, 1344),
          512: (5, "q59393n1024", 2880, 2944)}
SPI = 8
U = ctypes.c_ulonglong
KEM = "cbd.c indcpa.c kem_cpaf.c msgenc.c ntt.c poly.c poly_invq.c polyvec.c reduce.c verify.c".split()
SIG = ("rounding.c sign.c polyvec.c sampler.c sampler_u.c irs.c packing.c poly.c "
       "poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c").split()
PROTO = "loom.c prf_mac.c state_serialize.c drng.c auxfunc.c symmetric-iccs.c".split()


LIBC = ctypes.CDLL(None)


def cflush():
    """Keep the library's own '**Error' diagnostics in order with our output."""
    LIBC.fflush(None)
    sys.stdout.flush()


def skip(msg):
    print(f"SKIP {ID}: {msg}")
    sys.exit(77)


def fail(msg):
    print(f"NOT CONFIRMED {ID}: {msg}")
    sys.exit(1)


def require(cond, msg):
    if not cond:
        fail(msg)


def static_check():
    """Lines 19-20 of all six wrappers hard-code the identities used both ways."""
    n = 0
    for tree in ("Reference_Implementation", "Optimized_Implementation"):
        for level in LEVELS:
            path = IMPL / tree / f"LoomKEX-{level}" / "loom" / f"KEX_LoomKEX-{level}.c"
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            require(lines[18].strip() == '#define ID_INITIATOR "initiator"' and
                    lines[19].strip() == '#define ID_RESPONDER "responder"',
                    f"{path.name} ({tree}) lines 19-20 changed")
            text = "\n".join(lines)
            require("LOOM_INITIATOR, (const uint8_t*)ID_INITIATOR" in text and
                    "LOOM_RESPONDER, (const uint8_t*)ID_RESPONDER" in text and
                    "crypto_loom_auth_resp(resp, pka, (const uint8_t*)ID_INITIATOR" in text and
                    "crypto_loom_last_init(init, pkb, (const uint8_t*)ID_RESPONDER" in text,
                    f"{path.name} ({tree}) no longer uses the constants as own and peer IDs")
            n += 1
    print(f"static: {n}/6 wrappers define ID_INITIATOR=\"initiator\", ID_RESPONDER=\"responder\" "
          "(lines 19-20) and use them as both own and expected-peer identity")


def build_scratch(level, tmp):
    mode, ntt, _, _ = LEVELS[level]
    src = tmp / f"LoomKEX-{level}"
    shutil.copytree(IMPL / "Reference_Implementation" / f"LoomKEX-{level}", src, symlinks=False)
    (src / "loom" / "ngccw_drng.c").write_text('#include "drng.h"\nDRNG_ctx drng_algorithm;\n')
    files = [src / "loom" / f for f in PROTO + [f"KEX_LoomKEX-{level}.c", "ngccw_drng.c"]]
    files += [src / "kem" / f for f in KEM] + [src / "sig" / f for f in SIG]
    files.append(src / "sig" / "ntt" / ntt / "ntt_ref.c")
    out = tmp / f"libeve-{level}.so"
    cmd = ["gcc", "-O2", "-w", "-fPIC", "-shared", "-std=gnu11", "-fwrapv", "-D_GNU_SOURCE",
           "-Wno-error=incompatible-pointer-types", "-Wno-error=implicit-function-declaration",
           f"-DLOOM_MODE={mode}", f"-I{src / 'loom'}", f"-I{src / 'kem'}", f"-I{src / 'sig'}",
           "-o", str(out)] + [str(f) for f in files] + ["-lm"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        fail(f"scratch build of LoomKEX-{level} failed:\n{r.stderr[-2000:]}")
    return ctypes.CDLL(str(out), mode=ctypes.RTLD_LOCAL)


def buf(n):
    return ctypes.create_string_buffer(max(1, n))


class Kex:
    def __init__(self, path):
        self.lib = L = ctypes.CDLL(str(path), mode=ctypes.RTLD_LOCAL)
        for n in ("kex_get_pk_len_bytes", "kex_get_sk_len_bytes", "kex_get_sta_len_bytes",
                  "kex_get_stb_len_bytes", "kex_get_ss_len_bytes", "kex_get_total_msg_len_bytes"):
            getattr(L, n).restype = U
        self.pkl, self.skl = L.kex_get_pk_len_bytes(), L.kex_get_sk_len_bytes()
        self.stl = max(L.kex_get_sta_len_bytes(), L.kex_get_stb_len_bytes()) + 4096
        self.msgl = L.kex_get_total_msg_len_bytes() + 64
        self.ssl = L.kex_get_ss_len_bytes() + 64

    def seed(self, s):
        require(self.lib.ngcc_seed(s, U(len(s))) == 0, "seed")

    def init(self, side):
        pk, sk, st = buf(self.pkl), buf(self.skl), buf(self.stl)
        pl, sl, tl = U(), U(), U()
        rc = getattr(self.lib, f"kex_init_{side}")(pk, ctypes.byref(pl), sk, ctypes.byref(sl),
                                                   st, ctypes.byref(tl))
        require(rc == 0, f"kex_init_{side} rc={rc}")
        return pk.raw[:pl.value], sk.raw[:sl.value], st.raw[:tl.value]

    def pass1(self, ska, pkb, sta):
        st = buf(self.stl); ctypes.memmove(st, sta, len(sta)); stl = U(len(sta))
        m, ml = buf(self.msgl), U()
        rc = self.lib.kex_generate_pass1_msg_a(ska, U(len(ska)), pkb, U(len(pkb)), st,
                                               ctypes.byref(stl), m, ctypes.byref(ml))
        return rc, st.raw[:stl.value], m.raw[:ml.value]

    def passn(self, k, side, sk, pk, mi, st_in):
        st = buf(self.stl); ctypes.memmove(st, st_in, len(st_in)); stl = U(len(st_in))
        m, ml = buf(self.msgl), U()
        mib = buf(len(mi)); ctypes.memmove(mib, mi, len(mi))
        rc = getattr(self.lib, f"kex_generate_pass{k}_msg_{side}")(
            sk, U(len(sk)), pk, U(len(pk)), mib, U(len(mi)), st, ctypes.byref(stl), m, ctypes.byref(ml))
        return rc, st.raw[:stl.value], m.raw[:ml.value]

    def derive(self, side, sk, pk, m, st):
        ss, sl = buf(self.ssl), U()
        mb = buf(len(m)); ctypes.memmove(mb, m, len(m))
        stb = buf(len(st)); ctypes.memmove(stb, st, len(st))
        rc = getattr(self.lib, f"kex_derive_ss_{side}")(sk, U(len(sk)), pk, U(len(pk)), mb, U(len(m)),
                                                        stb, U(len(st)), ss, ctypes.byref(sl))
        return rc, ss.raw[:sl.value]


def eve_sign(eve, level, sk, dtbs):
    f = getattr(eve, f"shuttle{level}_ref_signature_rnd")
    sig, sl = buf(16384), ctypes.c_size_t()
    rnd = buf(64); ctypes.memmove(rnd, b"\x5a" * 64, 64)
    require(f(sig, ctypes.byref(sl), dtbs, ctypes.c_size_t(len(dtbs)), sk, rnd) == 0, "Eve signing")
    return sig.raw[:sl.value]


def patch_sig(m, sig):
    """Replace sigma in an auth message idlen||ID||siglen||sigma||tau; ID and tau unchanged."""
    ilen = m[0] | (m[1] << 8)
    so = 2 + ilen
    require((m[so] | (m[so + 1] << 8)) == len(sig), "signature length")
    return m[:so + 2] + sig + m[so + 2 + len(sig):]


def honest(k, seed):
    k.seed(seed)
    pkA, skA, stA = k.init("a")
    pkB, skB, stB = k.init("b")
    rc, stA, m1 = k.pass1(skA, pkB, stA); require(rc == 0, "honest pass1")
    rc, stB, m2 = k.passn(2, "b", skB, pkA, m1, stB); require(rc == 0, "honest pass2")
    rc, stA, m3 = k.passn(3, "a", skA, pkB, m2, stA); require(rc == 0, "honest pass3")
    rc, stB, m4 = k.passn(4, "b", skB, pkA, m3, stB); require(rc == 1, "honest pass4")
    ra, sa = k.derive("a", skA, pkB, m4, stA)
    rb, sb = k.derive("b", skB, pkA, m3, stB)
    require(ra == 0 and rb == 0 and sa == sb, "honest exchange did not agree")


def misbind(k, eve, level, variant, seed):
    _, _, ekl, ctl = LEVELS[level]
    k.seed(seed)
    pkA, skA, stA = k.init("a")
    pkB, skB, stB = k.init("b")
    pkE, skE, _ = k.init("a")                      # Eve: her own honest key
    require(len({pkA, pkB, pkE}) == 3, "distinct long-term keys")
    a_peer = pkB if variant == "I" else pkE
    b_peer = pkE if variant == "I" else pkA
    rc, stA, m1 = k.pass1(skA, a_peer, stA); require(rc == 0, "pass1")
    rc, stB, m2 = k.passn(2, "b", skB, b_peer, m1, stB); require(rc == 0, "pass2")
    rc, stA, m3 = k.passn(3, "a", skA, a_peer, m2, stA); require(rc == 0, "pass3")
    ek, ct, spi = m1[SPI:SPI + ekl], m2[SPI:SPI + ctl], m2[:SPI]
    if variant == "I":
        rc0, _, _ = k.passn(4, "b", skB, b_peer, m3, stB)
        require(rc0 != 1, "control: unmodified msg3 accepted by B expecting Eve")
        ctrl = rc0
        m3 = patch_sig(m3, eve_sign(eve, level, skE, spi + b"\x00" + ek + ct))
    rc, stB, m4 = k.passn(4, "b", skB, b_peer, m3, stB)
    require(rc == 1, f"B rejected msg3 rc={rc}")
    if variant == "R":
        rc0, _ = k.derive("a", skA, a_peer, m4, stA)
        require(rc0 != 0, "control: unmodified msg4 accepted by A expecting Eve")
        ctrl = rc0
        m4 = patch_sig(m4, eve_sign(eve, level, skE, spi + b"\x01" + ct + ek))
    ra, sa = k.derive("a", skA, a_peer, m4, stA)
    rb, sb = k.derive("b", skB, b_peer, m3, stB)
    require(ra == 0 and rb == 0, f"derive rcA={ra} rcB={rb}")
    require(sa == sb and len(sa) == level // 8, "keys differ")
    cflush()
    who = "A(peer=B) B(peer=Eve)" if variant == "I" else "A(peer=Eve) B(peer=A)"
    print(f"  LoomKEX-{level} {variant}: {who} both accept, same {len(sa)}-byte key "
          f"{sa.hex()[:16]}..; control unmodified message rc={ctrl}")


def distinct_id_control(eve, level):
    """Same core (loom.c) with distinct per-party IDs: the MAC rejects Eve's substitution."""
    mode = LEVELS[level][0]
    _, _, ekl, ctl = LEVELS[level]
    ns = f"loom{mode}_ref_"
    fn = {n: getattr(eve, ns + n) for n in
          ("initialize_state", "init", "respond", "auth_init", "auth_resp", "last_init", "finalize")}
    eve.create_ctx.restype = ctypes.c_void_p
    for f in fn.values():
        f.restype = ctypes.c_int
    fn["initialize_state"].argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p,
                                       ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t]
    fn["auth_resp"].argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t,
                                ctypes.c_char_p, ctypes.c_char_p]
    fn["last_init"].argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t,
                                ctypes.c_char_p]
    for n in ("init",):
        fn[n].argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    for n in ("respond", "auth_init"):
        fn[n].argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
    fn["finalize"].argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_size_t)]
    seed = bytes(range(100, 148))
    require(eve.init_random_number(ctypes.c_void_p(ctypes.addressof(
        ctypes.c_ubyte.in_dll(eve, "drng_algorithm"))), seed, U(48)) == 0, "seed scratch DRNG")

    keep = []                                     # the core stores pk/sk pointers

    def party(role, name):
        ctx = ctypes.c_void_p(eve.create_ctx())
        pk, sk = buf(16384), buf(16384)
        keep.extend((pk, sk))
        require(fn["initialize_state"](ctx, pk, sk, role, name, len(name)) == 0, "core init")
        return ctx, pk, sk

    def run(substitute):
        a, pa, _ = party(1, b"Alice")
        b, pb, _ = party(0, b"Bobby")
        e, pe, se = party(1, b"Evely")
        n1, n2, n3, n4 = (buf(16384) for _ in range(4))
        rcs = (fn["init"](a, n1), fn["respond"](b, n1, n2), fn["auth_init"](a, n2, n3))
        require(rcs == (0, 0, 0), f"core passes 1-3 rc={rcs}")
        if not substitute:
            rc = fn["auth_resp"](b, pa, b"Alice", 5, n3, n4)
            rc2 = fn["last_init"](a, pb, b"Bobby", 5, n4) if rc == 0 else None
            ka, kb = buf(128), buf(128)
            la, lb = ctypes.c_size_t(), ctypes.c_size_t()
            ok = (rc == 0 and rc2 == 0 and fn["finalize"](a, ka, ctypes.byref(la)) == 0 and
                  fn["finalize"](b, kb, ctypes.byref(lb)) == 0 and ka.raw[:la.value] == kb.raw[:lb.value])
            return rc, ok
        m1, m2 = n1.raw, n2.raw
        m3 = bytearray(n3.raw)
        m3[2:7] = b"Evely"                        # Eve claims the initiator identity
        dtbs = m2[:SPI] + b"\x00" + m1[SPI:SPI + ekl] + m2[SPI:SPI + ctl]
        m3 = patch_sig(bytes(m3), eve_sign(eve, level, se.raw, dtbs))
        return fn["auth_resp"](b, pe, b"Evely", 5, m3, n4), None

    rc_h, ok_h = run(False)
    require(rc_h == 0 and ok_h, f"distinct-ID honest core exchange failed rc={rc_h}")
    rc_s, _ = run(True)
    cflush()
    require(rc_s == -11, f"distinct-ID control: substitution not rejected by MAC (rc={rc_s})")
    print(f"  LoomKEX-{level} distinct-ID control: honest core exchange agrees; "
          f"Eve's substitution rejected rc={rc_s} (MAC failure)")


def main():
    sys.stdout.reconfigure(line_buffering=True)
    if shutil.which("gcc") is None:
        skip("gcc unavailable")
    libs = {L: CAND / "lib" / f"libLoomKEX-{L}.so" for L in LEVELS}
    missing = [str(p) for p in libs.values() if not p.is_file()]
    if missing:
        skip(f"build the libraries first (make -C kex-05): {missing}")
    print("(rejected controls make the submitted library print '**Error' lines; they are expected)")
    static_check()
    with tempfile.TemporaryDirectory(prefix="ngcc-kex05-") as t:
        tmp = Path(t)
        count = 0
        for level in LEVELS:
            eve = build_scratch(level, tmp)
            k = Kex(libs[level])
            honest(k, bytes(range(48)))
            print(f"  LoomKEX-{level} control: honest A-B exchange agrees")
            for variant in "IR":
                misbind(k, eve, level, variant, bytes((ord(variant) + i) & 255 for i in range(48)))
                count += 1
            distinct_id_control(eve, level)
    print(f"CONFIRMED {ID}: {count}/6 misbinding sessions (3 levels x 2 directions) accepted "
          "with Eve's own key; distinct-ID core rejects the same substitution")
    return 0


if __name__ == "__main__":
    sys.exit(main())
