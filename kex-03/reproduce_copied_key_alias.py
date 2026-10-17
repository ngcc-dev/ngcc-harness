#!/usr/bin/env python3
"""Reproduce kex-03-10: a copied certified key misbinds CreTAKE sessions.

CreTAKE names the peers only by the public-key bytes hashed into the session
key. If a registration authority certifies Bob's public key also under the
name Eve, an initiator that addresses "Eve" completes with Bob, and both sides
derive the same key while Bob's session names a different peer. Eve does
nothing beyond registering the duplicate key. Control: an independently
generated responder key does not agree.

This is a deployment-model witness, not a break of distinct-key sessions.
"""

import ctypes
import glob
import os
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CAND = REPO / "kex-03"
U = ctypes.c_ulonglong
INSTANCES = 25


def buf(n):
    return ctypes.create_string_buffer(max(1, n))


class Kex:
    def __init__(self, path):
        lib = ctypes.CDLL(str(path), mode=ctypes.RTLD_LOCAL)
        for name in ("kex_get_pk_len_bytes", "kex_get_sk_len_bytes", "kex_get_sta_len_bytes",
                     "kex_get_stb_len_bytes", "kex_get_ss_len_bytes", "kex_get_total_msg_len_bytes"):
            getattr(lib, name).restype = U
        self.lib = lib
        self.pkl = lib.kex_get_pk_len_bytes()
        self.skl = lib.kex_get_sk_len_bytes()
        self.stl = max(lib.kex_get_sta_len_bytes(), lib.kex_get_stb_len_bytes()) + 4096
        self.ssl = lib.kex_get_ss_len_bytes()
        self.msgl = lib.kex_get_total_msg_len_bytes() + 64
        seed = bytes(range(48))
        lib.ngcc_seed(seed, U(len(seed)))

    def keygen(self, fn):
        pk, sk, st = buf(self.pkl), buf(self.skl), buf(self.stl)
        pl, sl, tl = U(), U(), U()
        if fn(pk, ctypes.byref(pl), sk, ctypes.byref(sl), st, ctypes.byref(tl)) != 0:
            raise RuntimeError("keygen")
        return pk.raw[:pl.value], sk.raw[:sl.value]

    def exchange(self, sk_a, pk_for_a, sk_b, pk_a):
        """Initiator addresses pk_for_a; the responder holding sk_b answers."""
        lib = self.lib
        sta, sl = buf(self.stl), U()
        m1, m1l = buf(self.msgl), U()
        if lib.kex_generate_pass1_msg_a(sk_a, U(len(sk_a)), pk_for_a, U(len(pk_for_a)),
                                        sta, ctypes.byref(sl), m1, ctypes.byref(m1l)) != 0:
            return None
        stb, tl = buf(self.stl), U()
        m2, m2l = buf(self.msgl), U()
        if lib.kex_generate_pass2_msg_b(sk_b, U(len(sk_b)), pk_a, U(len(pk_a)), m1, m1l,
                                        stb, ctypes.byref(tl), m2, ctypes.byref(m2l)) != 1:
            return None
        ka, kal = buf(self.ssl + 64), U()
        kb, kbl = buf(self.ssl + 64), U()
        ra = lib.kex_derive_ss_a(sk_a, U(len(sk_a)), pk_for_a, U(len(pk_for_a)), m2, m2l,
                                 sta, sl, ka, ctypes.byref(kal))
        rb = lib.kex_derive_ss_b(sk_b, U(len(sk_b)), pk_a, U(len(pk_a)), m1, m1l,
                                 stb, tl, kb, ctypes.byref(kbl))
        return ra, ka.raw[:kal.value], rb, kb.raw[:kbl.value]


def main():
    libs = sorted(glob.glob(str(CAND / "lib" / "libCreTAKE-*.so")))
    if len(libs) != INSTANCES:
        print(f"SKIP kex-03-10: expected {INSTANCES} CreTAKE libraries, found {len(libs)}; run make -C kex-03")
        return 77
    agreed = rejected = 0
    for path in libs:
        name = os.path.basename(path)[len("libCreTAKE-"):-3]
        x = Kex(path)
        pk_a, sk_a = x.keygen(x.lib.kex_init_a)
        pk_b, sk_b = x.keygen(x.lib.kex_init_b)
        pk_o, _ = x.keygen(x.lib.kex_init_b)
        certificates = {"Bob": pk_b, "Eve": pk_b}          # duplicate registration
        assert certificates["Eve"] == certificates["Bob"] and pk_o != pk_b

        honest = x.exchange(sk_a, certificates["Bob"], sk_b, pk_a)
        honest_ok = honest is not None and honest[0] == 0 and honest[2] == 0 and honest[1] == honest[3]
        alias = x.exchange(sk_a, certificates["Eve"], sk_b, pk_a)     # Alice believes she talks to Eve
        alias_ok = alias is not None and alias[0] == 0 and alias[2] == 0 and alias[1] == alias[3] and len(alias[1]) > 0
        control = x.exchange(sk_a, pk_o, sk_b, pk_a)                  # a genuinely different key
        control_ok = control is None or control[0] != 0 or control[2] != 0 or control[1] != control[3]
        agreed += honest_ok and alias_ok
        rejected += control_ok
        print(f"{name:28s} honest={'agree' if honest_ok else 'FAIL'} "
              f"alias(A names Eve, B answers)={'agree' if alias_ok else 'no'} "
              f"distinct-key control={'no agreement' if control_ok else 'AGREED'}")
    if agreed == INSTANCES and rejected == INSTANCES:
        print(f"CONFIRMED kex-03-10: copied-key misbinding agrees on {agreed}/{INSTANCES} CreTAKE instances; "
              f"distinct-key control fails to agree on {rejected}/{INSTANCES}")
        return 0
    print(f"NOT CONFIRMED kex-03-10: alias agreed {agreed}/{INSTANCES}, control clean {rejected}/{INSTANCES}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
