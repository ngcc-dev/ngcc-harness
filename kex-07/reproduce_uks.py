#!/usr/bin/env python3
"""Reproduce the kex-07-1 unknown-key-share through the NEV-AKE uniform API.

The wrappers pass all-zero identities (kat_test/KEX_AlgorithmInstance.c:105-106,
165-166) into GC-AKE, whose key K = H(m_i, m_j, K~, i, j, M, M') (ake.c:29-48)
contains no static public key. Eve registers (a) a copy of Alice's public key,
or (b) a byte-distinct non-canonical encoding of it that the decoder maps to the
same polynomial (pack.c:140-141 for q=1409; the raw 10-bit tail coefficients at
pack.c:329-350 for q=769). Alice initiates to Bob; Eve relays M and tells Bob
the initiator is Eve, so Bob runs pass 2 with Eve's certified key. Both derive
the same key while Alice's peer is Bob and Bob's peer is Eve.

Usage: python3 kex-07/reproduce_uks.py [trials-per-set]   (default 20)
"""

import ctypes
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
SETS = ("NEV_AKE_512_769_C", "NEV_AKE_1024_769_C", "NEV_AKE_2048_769_C",
        "NEV_AKE_512_1409", "NEV_AKE_1024_1409", "NEV_AKE_2048_1409",
        "NEV_AKE_512_769", "NEV_AKE_1024_769", "NEV_AKE_2048_769")
U = ctypes.c_ulonglong
OK_RC = (0, 1, 0, 0)


def buffer(data_or_size):
    if isinstance(data_or_size, int):
        return ctypes.create_string_buffer(max(1, data_or_size))
    b = ctypes.create_string_buffer(max(1, len(data_or_size)))
    ctypes.memmove(b, data_or_size, len(data_or_size))
    return b


class Nev:
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

    def keygen(self):
        pk, sk, st = buffer(self.pkl), buffer(self.skl), buffer(self.stl)
        pl, sl, tl = U(), U(), U()
        assert self.lib.kex_init_a(pk, ctypes.byref(pl), sk, ctypes.byref(sl), st, ctypes.byref(tl)) == 0
        return pk.raw[:self.pkl], sk.raw[:self.skl]

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
        return (rc1, rc2, rca, rcb), ka.raw[:self.ssl], kb.raw[:self.ssl]


def alias(pk, q, n, delta):
    """Re-encode one coefficient x as x + delta (delta = q gives a congruent alias)."""
    b = bytearray(pk)
    if q == 1409:
        # 21-bit groups t = x1*1409 + x0; decode16 takes x1 = t/1409 with no range check
        for g in range(n // 16):
            bits = int.from_bytes(b[21 * g:21 * g + 21], "little")
            for i in range(8):
                t = (bits >> (21 * i)) & 0x1FFFFF
                x0, x1 = t % 1409, t // 1409
                t2 = (x1 + delta) * 1409 + x0
                if t2 < 1 << 21:
                    bits = (bits & ~(0x1FFFFF << (21 * i))) | (t2 << (21 * i))
                    b[21 * g:21 * g + 21] = bits.to_bytes(21, "little")
                    return bytes(b)
    else:
        # the trailing n mod 5 coefficients are stored raw in 10 bits
        off = 6 * (n // 5)
        bits = int.from_bytes(b[off:], "little")
        for i in range(n % 5):
            x = (bits >> (10 * i)) & 0x3FF
            if x + delta < 1024:
                bits = (bits & ~(0x3FF << (10 * i))) | ((x + delta) << (10 * i))
                b[off:] = bits.to_bytes(len(b) - off, "little")
                return bytes(b)
    return None


def source_check():
    if not REF.is_dir():
        return "source not present (check skipped)"
    for d in ("NEV-AKE-C1", "NEV-AKE-C1-c", "NEV-AKE-C2", "NEV-AKE-C2-c", "NEV-AKE-C3",
              "NEV-AKE-C3-c", "NEV-AKE-R1", "NEV-AKE-R2", "NEV-AKE-R3"):
        w = (REF / d / "kat_test/KEX_AlgorithmInstance.c").read_text().splitlines()
        for ln in (105, 106, 165, 166):
            assert w[ln - 1].strip() in ("unsigned char idi[SEED_BYTES] = {0};",
                                          "unsigned char idj[SEED_BYTES] = {0};"), f"{d} wrapper:{ln}"
        h = (REF / d / "ake.c").read_text().split("void ake_hash_h(", 1)[1].split("void ake_hash_hi(", 1)[0]
        assert "pk" not in h and "idi" in h and "idj" in h, f"{d} ake_hash_h inputs"
    return "all nine wrappers hash zero identities; ake_hash_h has no public-key input"


def run(name, trials):
    q = 1409 if name.endswith("1409") else 769
    n = int(name.split("_")[2])
    x = Nev(ROOT / f"lib/lib{name}.so")
    r = dict(honest=0, copy=0, alias=0, aliasable=0, ctl=0, mut=0)
    for t in range(trials):
        x.seed(bytes([t]) + name.encode().ljust(47, b"\0"))
        pk_a, sk_a = x.keygen()      # Alice
        pk_b, sk_b = x.keygen()      # Bob
        pk_x, _ = x.keygen()         # an independent key for Eve (control)
        rc, ka, kb = x.session(sk_a, pk_b, sk_b, pk_a)
        r["honest"] += rc == OK_RC and ka == kb
        # (a) Eve's certificate carries an exact copy of Alice's key; Bob's peer is Eve.
        rc, ka, kb = x.session(sk_a, pk_b, sk_b, bytes(pk_a))
        r["copy"] += rc == OK_RC and ka == kb
        # (b) a byte-distinct congruent re-encoding of Alice's key
        pk_alias = alias(pk_a, q, n, q)
        if pk_alias is not None:
            assert pk_alias != pk_a
            r["aliasable"] += 1
            rc, ka, kb = x.session(sk_a, pk_b, sk_b, pk_alias)
            r["alias"] += rc == OK_RC and ka == kb
            # mutated control: the same position re-encoded as x + 1 (not congruent)
            pk_mut = alias(pk_a, q, n, 1)
            rc, ka, kb = x.session(sk_a, pk_b, sk_b, pk_mut)
            r["mut"] += ka != kb
        # control: Eve's own independently generated key
        rc, ka, kb = x.session(sk_a, pk_b, sk_b, pk_x)
        r["ctl"] += ka != kb
    ok = (r["honest"] == r["copy"] == r["ctl"] == trials and r["alias"] == r["aliasable"] > 0
          and r["mut"] == r["aliasable"])
    print(f"{name}: honest {r['honest']}/{trials}; copied-key UKS {r['copy']}/{trials}; "
          f"byte-distinct alias UKS {r['alias']}/{r['aliasable']} alias-able keys; "
          f"non-congruent re-encoding differs {r['mut']}/{r['aliasable']}; "
          f"independent-key control differs {r['ctl']}/{trials}")
    return ok


def main():
    trials = int(sys.argv[1]) if len(sys.argv) > 1 else 20
    t0 = time.time()
    present = [s for s in SETS if (ROOT / f"lib/lib{s}.so").is_file()]
    missing = [s for s in SETS if s not in present]
    if not present:
        print("SKIP kex-07-1: no lib/libNEV_AKE_*.so (build them: make -C kex-07)")
        return 77
    print(f"source: {source_check()}")
    results = [run(s, trials) for s in present]
    note = f"; not built: {', '.join(missing)}" if missing else ""
    if all(results):
        print(f"CONFIRMED kex-07-1: {len(present)}/{len(SETS)} sets: copied and byte-distinct aliased "
              f"keys give equal session keys under different peer identities{note}")
        rc = 0
    else:
        print(f"NOT CONFIRMED kex-07-1: {sum(results)}/{len(present)} sets{note}")
        rc = 1
    print(f"runtime {time.time() - t0:.1f} s")
    return rc


if __name__ == "__main__":
    sys.exit(main())
