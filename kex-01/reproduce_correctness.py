#!/usr/bin/env python3
"""Find an honest ADKEX-512 shared-secret mismatch."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def buf(size: int):
    return ctypes.create_string_buffer(max(1, size))


def main() -> None:
    lib = ctypes.CDLL(str(ROOT / "lib/libADKEX-512.so"), mode=ctypes.RTLD_LOCAL)
    getters = [
        "kex_get_pk_len_bytes", "kex_get_sk_len_bytes", "kex_get_sta_len_bytes",
        "kex_get_stb_len_bytes", "kex_get_ss_len_bytes", "kex_get_total_msg_len_bytes",
    ]
    for name in getters:
        getattr(lib, name).restype = ctypes.c_ulonglong
    pk_n, sk_n = lib.kex_get_pk_len_bytes(), lib.kex_get_sk_len_bytes()
    sta_n, stb_n = lib.kex_get_sta_len_bytes(), lib.kex_get_stb_len_bytes()
    ss_n, msg_n = lib.kex_get_ss_len_bytes(), lib.kex_get_total_msg_len_bytes()
    state = (ctypes.c_ubyte * 165).in_dll(lib, "drng_algorithm")
    seed_value = bytes(range(64))
    seed = ctypes.create_string_buffer(seed_value)
    assert lib.init_random_number(state, seed, len(seed_value)) == 0

    pkb, skb, stb = buf(pk_n), buf(sk_n), buf(stb_n)
    pn, sn, stbn = ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong()
    assert lib.kex_init_b(pkb, ctypes.byref(pn), skb, ctypes.byref(sn),
                          stb, ctypes.byref(stbn)) == 0
    dummy = buf(1)
    for trial in range(3000):
        sta, m1, m2 = buf(sta_n), buf(msg_n), buf(msg_n)
        ssa, ssb = buf(ss_n), buf(ss_n)
        stan = ctypes.c_ulonglong()
        m1n, m2n = ctypes.c_ulonglong(), ctypes.c_ulonglong()
        ssan, ssbn = ctypes.c_ulonglong(), ctypes.c_ulonglong()
        assert lib.kex_generate_pass1_msg_a(dummy, 0, pkb, pn.value,
                                             sta, ctypes.byref(stan),
                                             m1, ctypes.byref(m1n)) == 0
        assert lib.kex_generate_pass2_msg_b(skb, sn.value, dummy, 0,
                                             m1, m1n.value,
                                             stb, ctypes.byref(stbn),
                                             m2, ctypes.byref(m2n)) == 1
        assert lib.kex_derive_ss_a(dummy, 0, pkb, pn.value,
                                   m2, m2n.value, sta, stan.value,
                                   ssa, ctypes.byref(ssan)) == 0
        assert lib.kex_derive_ss_b(skb, sn.value, dummy, 0,
                                   m1, m1n.value, stb, stbn.value,
                                   ssb, ctypes.byref(ssbn)) == 0
        if ssa.raw[:ssan.value] != ssb.raw[:ssbn.value]:
            print(f"CONFIRMED kex-01-2 ADKEX-512 honest mismatch trial={trial}")
            return
    raise SystemExit("NOT-CONFIRMED: no mismatch in 3000 trials")


if __name__ == "__main__":
    main()
