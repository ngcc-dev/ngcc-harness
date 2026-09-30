#!/usr/bin/env python3
"""Find an honest DKEX-512 shared-secret mismatch."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def buf(size: int):
    return ctypes.create_string_buffer(max(1, size))


def main() -> None:
    lib = ctypes.CDLL(str(ROOT / "lib/libDKEX-512.so"), mode=ctypes.RTLD_LOCAL)
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

    pka, ska, sta0 = buf(pk_n), buf(sk_n), buf(sta_n)
    pkb, skb, stb0 = buf(pk_n), buf(sk_n), buf(stb_n)
    pkan, skan, sta0n = ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong()
    pkbn, skbn, stb0n = ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong()
    assert lib.kex_init_a(pka, ctypes.byref(pkan), ska, ctypes.byref(skan),
                          sta0, ctypes.byref(sta0n)) == 0
    assert lib.kex_init_b(pkb, ctypes.byref(pkbn), skb, ctypes.byref(skbn),
                          stb0, ctypes.byref(stb0n)) == 0

    for trial in range(6000):
        sta, stb = buf(sta_n), buf(stb_n)
        ctypes.memmove(sta, sta0, sta0n.value)
        ctypes.memmove(stb, stb0, stb0n.value)
        stan, stbn = ctypes.c_ulonglong(sta0n.value), ctypes.c_ulonglong(stb0n.value)
        m1, m2, m3 = buf(msg_n), buf(msg_n), buf(msg_n)
        m1n, m2n, m3n = ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong()
        ssa, ssb = buf(ss_n), buf(ss_n)
        ssan, ssbn = ctypes.c_ulonglong(), ctypes.c_ulonglong()

        assert lib.kex_generate_pass1_msg_a(ska, skan.value, pkb, pkbn.value,
                                             sta, ctypes.byref(stan),
                                             m1, ctypes.byref(m1n)) == 0
        assert lib.kex_generate_pass2_msg_b(skb, skbn.value, pka, pkan.value,
                                             m1, m1n.value, stb, ctypes.byref(stbn),
                                             m2, ctypes.byref(m2n)) == 0
        assert lib.kex_generate_pass3_msg_a(ska, skan.value, pkb, pkbn.value,
                                             m2, m2n.value, sta, ctypes.byref(stan),
                                             m3, ctypes.byref(m3n)) == 1
        assert lib.kex_derive_ss_a(ska, skan.value, pkb, pkbn.value,
                                   m2, m2n.value, sta, stan.value,
                                   ssa, ctypes.byref(ssan)) == 0
        assert lib.kex_derive_ss_b(skb, skbn.value, pka, pkan.value,
                                   m3, m3n.value, stb, stbn.value,
                                   ssb, ctypes.byref(ssbn)) == 0
        if ssa.raw[:ssan.value] != ssb.raw[:ssbn.value]:
            print(f"CONFIRMED kex-04-1 DKEX-512 honest mismatch trial={trial}")
            return
    raise SystemExit("NOT-CONFIRMED: no mismatch in 6000 trials")


if __name__ == "__main__":
    main()
