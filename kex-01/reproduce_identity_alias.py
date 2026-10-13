#!/usr/bin/env python3
"""Reproduce kex-01-4 with a duplicated responder-key registration.

The identity labels belong to the certificate layer, not the ADKEX API.
This is a deployment-model witness, not a break of distinct-key CK sessions.
"""

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SRC = ROOT / "Implementations/Reference_Implementation"


def buffer(size):
    return ctypes.create_string_buffer(max(1, size))


def require(condition, label):
    if not condition:
        raise AssertionError(label)


def exchange(lib, pk_for_a, pk_len, sk_b, sk_len, state_b, state_b_len):
    state_a = buffer(lib.kex_get_sta_len_bytes())
    m1 = buffer(lib.kex_get_total_msg_len_bytes())
    m2 = buffer(lib.kex_get_total_msg_len_bytes())
    ss_a = buffer(lib.kex_get_ss_len_bytes())
    ss_b = buffer(lib.kex_get_ss_len_bytes())
    state_a_len = ctypes.c_ulonglong()
    m1_len = ctypes.c_ulonglong()
    m2_len = ctypes.c_ulonglong()
    ss_a_len = ctypes.c_ulonglong()
    ss_b_len = ctypes.c_ulonglong()
    dummy = buffer(1)

    require(lib.kex_generate_pass1_msg_a(dummy, 0, pk_for_a, pk_len,
                                         state_a, ctypes.byref(state_a_len),
                                         m1, ctypes.byref(m1_len)) == 0, "pass 1")
    require(lib.kex_generate_pass2_msg_b(sk_b, sk_len, dummy, 0,
                                         m1, m1_len.value, state_b,
                                         ctypes.byref(state_b_len), m2,
                                         ctypes.byref(m2_len)) == 1, "pass 2")
    require(lib.kex_derive_ss_a(dummy, 0, pk_for_a, pk_len,
                                m2, m2_len.value, state_a, state_a_len.value,
                                ss_a, ctypes.byref(ss_a_len)) == 0, "derive A")
    require(lib.kex_derive_ss_b(sk_b, sk_len, dummy, 0,
                                m1, m1_len.value, state_b, state_b_len.value,
                                ss_b, ctypes.byref(ss_b_len)) == 0, "derive B")
    require(ss_a_len.value == ss_b_len.value, "key lengths")
    return ss_a.raw[:ss_a_len.value], ss_b.raw[:ss_b_len.value]


def run_level(level):
    source = (SRC / f"ADKEX-{level}" / "adkex_derand.c").read_text()
    binder = source.split("static void adkex_transcript(", 1)[1].split(
        "void ADKEX_init_b_derand(", 1)[0]
    require("memcpy(p, pk_B" in binder, "public-key binding")
    require("memcpy(p, m1" in binder and "memcpy(p, m2" in binder,
            "message binding")
    require("id_A" not in binder and "id_B" not in binder and "identity" not in binder,
            "identity unexpectedly bound")

    lib_path = ROOT / f"lib/libADKEX-{level}.so"
    if not lib_path.is_file():
        raise SystemExit(f"SKIP 77: build {lib_path.name} with make -C kex-01")
    lib = ctypes.CDLL(str(lib_path), mode=ctypes.RTLD_LOCAL)
    for name in ("kex_get_pk_len_bytes", "kex_get_sk_len_bytes",
                 "kex_get_sta_len_bytes", "kex_get_stb_len_bytes",
                 "kex_get_ss_len_bytes", "kex_get_total_msg_len_bytes"):
        getattr(lib, name).restype = ctypes.c_ulonglong
    rng = (ctypes.c_ubyte * 165).in_dll(lib, "drng_algorithm")
    seed = ctypes.create_string_buffer(bytes(range(64)))
    require(lib.init_random_number(rng, seed, 64) == 0, "seed RNG")

    pk_len = lib.kex_get_pk_len_bytes()
    sk_len = lib.kex_get_sk_len_bytes()
    pk_b, sk_b, state_b = buffer(pk_len), buffer(sk_len), buffer(lib.kex_get_stb_len_bytes())
    pk_b_len, sk_b_len, state_b_len = (ctypes.c_ulonglong() for _ in range(3))
    require(lib.kex_init_b(pk_b, ctypes.byref(pk_b_len), sk_b,
                           ctypes.byref(sk_b_len), state_b,
                           ctypes.byref(state_b_len)) == 0, "keygen B")
    require(pk_b_len.value == pk_len and sk_b_len.value == sk_len, "key sizes")

    # The registration authority assigns this same key to two distinct names.
    certificates = {"Bob": pk_b.raw[:pk_len], "Eve": pk_b.raw[:pk_len]}
    require(certificates["Bob"] == certificates["Eve"], "duplicate certificate")
    initiator_peer = "Eve"
    actual_responder = "Bob"
    require(initiator_peer != actual_responder, "identity control")
    alias_pk = ctypes.create_string_buffer(certificates[initiator_peer])
    a_key, b_key = exchange(lib, alias_pk, pk_len, sk_b, sk_len,
                            state_b, state_b_len)
    require(a_key == b_key, "duplicated-key exchange did not agree")

    # A genuinely different public key does not substitute for the duplicate.
    pk_other = buffer(pk_len)
    sk_other = buffer(sk_len)
    state_other = buffer(lib.kex_get_stb_len_bytes())
    other_pk_len, other_sk_len, other_state_len = (ctypes.c_ulonglong() for _ in range(3))
    require(lib.kex_init_b(pk_other, ctypes.byref(other_pk_len), sk_other,
                           ctypes.byref(other_sk_len), state_other,
                           ctypes.byref(other_state_len)) == 0, "keygen control")
    require(pk_other.raw[:pk_len] != pk_b.raw[:pk_len], "distinct-key control")
    mismatch_a, mismatch_b = exchange(lib, pk_other, pk_len, sk_b, sk_len,
                                      state_b, state_b_len)
    require(mismatch_a != mismatch_b, "distinct-key control unexpectedly agreed")
    print(f"CONFIRMED kex-01-4 ADKEX-{level}: duplicate-certified-key alias agrees; distinct-key control does not agree")


def main():
    missing = [level for level in (128, 256, 512)
               if not (ROOT / f"lib/libADKEX-{level}.so").is_file()]
    if missing:
        print(f"SKIP 77: build ADKEX libraries for {missing} with make -C kex-01")
        raise SystemExit(77)
    for level in (128, 256, 512):
        run_level(level)


if __name__ == "__main__":
    main()
