#!/usr/bin/env python3
"""Reproduce kex-09-3: TriQ-KEX authenticates fixed role labels, so a copied
certified key gives an unknown-key-share.

The session transcript is ST = ("TriQ-FSXY-ST", "ID_A", "ID_B", ek_A, ek_B,
CT_A, ek_T, CT_B, CT_T) (KEX_AlgorithmInstance.c:62-63 and the APPEND_ST block).
U_A and U_B are the constants "ID_A" and "ID_B", so a party's identity enters
the key only through its key bytes. Eve registers a copy of Bob's key (or of
Alice's key) under the name Eve and relays one honest exchange unchanged.

Only the uniform KEX ABI of lib/libTriQ-KEX-*.so is used.
"""

import ctypes
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LEVELS = (128, 256, 384, 512)
SEEDS = (0, 1)
U = ctypes.c_ulonglong
SRC = "TriQ-KEX/Implementations/Reference_Implementation/TriQ-KEX-{}/KEX_AlgorithmInstance.c"


def buffer(data_or_size):
    if isinstance(data_or_size, int):
        return ctypes.create_string_buffer(max(1, data_or_size))
    b = ctypes.create_string_buffer(max(1, len(data_or_size)))
    ctypes.memmove(b, data_or_size, len(data_or_size))
    return b


class TriQ:
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
        return pk.raw[:pl.value], sk.raw[:sl.value]

    def session(self, sk_a, pk_for_a, sk_b, pk_for_b):
        """Initiator (sk_a) believes its peer has pk_for_a; responder (sk_b) believes pk_for_b."""
        L = self.lib
        sta, stal, m1, m1l = buffer(self.stl), U(), buffer(self.msgl), U()
        rc1 = L.kex_generate_pass1_msg_a(buffer(sk_a), U(len(sk_a)), buffer(pk_for_a), U(len(pk_for_a)),
                                         sta, ctypes.byref(stal), m1, ctypes.byref(m1l))
        stb, stbl, m2, m2l = buffer(self.stl), U(), buffer(self.msgl), U()
        rc2 = L.kex_generate_pass2_msg_b(buffer(sk_b), U(len(sk_b)), buffer(pk_for_b), U(len(pk_for_b)),
                                         m1, m1l, stb, ctypes.byref(stbl), m2, ctypes.byref(m2l))
        ka, kal, kb, kbl = buffer(self.ssl), U(), buffer(self.ssl), U()
        rca = L.kex_derive_ss_a(buffer(sk_a), U(len(sk_a)), buffer(pk_for_a), U(len(pk_for_a)),
                                m2, m2l, sta, stal, ka, ctypes.byref(kal))
        rcb = L.kex_derive_ss_b(buffer(sk_b), U(len(sk_b)), buffer(pk_for_b), U(len(pk_for_b)),
                                m1, m1l, stb, stbl, kb, ctypes.byref(kbl))
        rcs = (rc1, rc2, rca, rcb)
        return rcs, ka.raw[:self.ssl], kb.raw[:self.ssl], m1.raw[:m1l.value], m2.raw[:m2l.value]


def source_check(level):
    path = ROOT / SRC.format(level)
    if not path.is_file():
        return "source not present (check skipped)"
    text = path.read_text()
    lines = text.splitlines()
    assert "{ 'I', 'D', '_', 'A' }" in lines[61] and "{ 'I', 'D', '_', 'B' }" in lines[62], "lines 62-63"
    st = text.split('st_label[] = "TriQ-FSXY-ST"', 1)[1].split("#undef APPEND_ST", 1)[0]
    order = ["APPEND_ST(st_label", "APPEND_ST(triq_kex_id_a", "APPEND_ST(triq_kex_id_b",
             "APPEND_ST(pka", "APPEND_ST(pkb", "APPEND_ST(ct_a", "APPEND_ST(pk_t",
             "APPEND_ST(ct_b", "APPEND_ST(ct_t"]
    pos = [st.index(tok) for tok in order]
    calls = [ln.strip() for ln in st.splitlines() if ln.strip().startswith("APPEND_ST(")]
    assert pos == sorted(pos) and len(calls) == len(order), "ST layout"
    return "ST = label, 'ID_A', 'ID_B', ek_A, ek_B, CT_A, ek_T, CT_B, CT_T"


def run(level, seed):
    x = TriQ(ROOT / f"lib/libTriQ-KEX-{level}.so")
    x.seed(bytes((seed * 7 + i) & 255 for i in range(48)))
    pk_a, sk_a = x.keygen(responder=False)   # Alice
    pk_b, sk_b = x.keygen(responder=True)    # Bob
    pk_x, _ = x.keygen(responder=True)       # an independently generated key for Eve (control)
    ok_rc = (0, 1, 0, 0)

    # honest control: Alice <-> Bob
    rc, ka, kb, m1, m2 = x.session(sk_a, pk_b, sk_b, pk_a)
    honest = rc == ok_rc and ka == kb
    labels = m1[:8] == b"ID_AID_B" and m2[:8] == b"ID_AID_B"

    # variant R: Eve's certificate carries Bob's key. Alice's peer is "Eve"; Bob's peer is Alice.
    directory = {"Alice": pk_a, "Bob": pk_b, "Eve": pk_b}
    rc, ka, kb, _, _ = x.session(sk_a, directory["Eve"], sk_b, directory["Alice"])
    uks_r = rc == ok_rc and ka == kb and len(ka) == level // 8

    # variant I: Eve's certificate carries Alice's key. Alice's peer is Bob; Bob's peer is "Eve".
    directory = {"Alice": pk_a, "Bob": pk_b, "Eve": pk_a}
    rc, ka, kb, _, _ = x.session(sk_a, directory["Bob"], sk_b, directory["Eve"])
    uks_i = rc == ok_rc and ka == kb

    # control: Eve's own independently generated key does not substitute
    rc, ka, kb, _, _ = x.session(sk_a, pk_x, sk_b, pk_a)
    ctl = ka != kb

    print(f"TriQ-KEX-{level} seed {seed}: role labels on the wire={labels}; "
          f"[Alice->'Eve'(copy of Bob), Bob->Alice] equal={uks_r}; "
          f"[Alice->Bob, Bob->'Eve'(copy of Alice)] equal={uks_i}; "
          f"distinct-key control differs={ctl}; honest control equal={honest}")
    return labels and uks_r and uks_i and ctl and honest


def main():
    t0 = time.time()
    present = [lv for lv in LEVELS if (ROOT / f"lib/libTriQ-KEX-{lv}.so").is_file()]
    missing = [lv for lv in LEVELS if lv not in present]
    if not present:
        print("SKIP kex-09-3: no lib/libTriQ-KEX-*.so (build them: make -C kex-09)")
        return 77
    for lv in present:
        print(f"TriQ-KEX-{lv} source: {source_check(lv)}")
    results = [run(lv, s) for lv in present for s in SEEDS]
    note = f"; not built: {', '.join(f'TriQ-KEX-{lv}' for lv in missing)}" if missing else ""
    sets = ", ".join(f"TriQ-KEX-{lv}" for lv in present)
    if all(results):
        print(f"CONFIRMED kex-09-3: {sets}: copied certified keys give equal session keys under "
              f"different peer identities in {sum(results)}/{len(results)} runs{note}")
        rc = 0
    else:
        print(f"NOT CONFIRMED kex-09-3: {sum(results)}/{len(results)} runs on {sets}{note}")
        rc = 1
    print(f"runtime {time.time() - t0:.1f} s")
    return rc


if __name__ == "__main__":
    sys.exit(main())
