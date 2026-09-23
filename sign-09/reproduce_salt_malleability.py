#!/usr/bin/env python3
"""Flip DOVE's trailing salt byte through the official signature API."""

import ctypes
from pathlib import Path


LIBRARY = Path(__file__).resolve().parent / "lib/libdove_classic_128.so"


def main() -> None:
    lib = ctypes.CDLL(str(LIBRARY))
    for name in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
        getattr(lib, name).restype = ctypes.c_ulonglong
    pk_len = lib.sig_get_pk_len_bytes()
    sk_len = lib.sig_get_sk_len_bytes()
    sig_capacity = lib.sig_get_sn_len_bytes()

    seed = (ctypes.c_ubyte * 48)(*range(48))
    assert lib.ngcc_seed(seed, len(seed)) == 0
    pk = (ctypes.c_ubyte * pk_len)()
    sk = (ctypes.c_ubyte * sk_len)()
    actual_pk = ctypes.c_ulonglong()
    actual_sk = ctypes.c_ulonglong()
    assert lib.sig_keygen(pk, ctypes.byref(actual_pk), sk, ctypes.byref(actual_sk)) == 0
    assert (actual_pk.value, actual_sk.value) == (pk_len, sk_len)

    message = (ctypes.c_ubyte * 16).from_buffer_copy(b"DOVE salt check!")
    other_message = (ctypes.c_ubyte * 16).from_buffer_copy(b"DOVE salt check?")
    signature = (ctypes.c_ubyte * sig_capacity)()
    actual_sig = ctypes.c_ulonglong()
    assert lib.sig_sign(sk, sk_len, message, len(message), signature, ctypes.byref(actual_sig)) == 0
    assert actual_sig.value == sig_capacity
    assert lib.sig_verify(pk, pk_len, signature, actual_sig.value, message, len(message)) == 0

    changed = (ctypes.c_ubyte * sig_capacity).from_buffer_copy(bytes(signature))
    changed[-1] ^= 1  # Last byte belongs to the encoded salt.
    assert bytes(changed) != bytes(signature)
    assert lib.sig_verify(pk, pk_len, changed, actual_sig.value, message, len(message)) == 0
    assert lib.sig_verify(pk, pk_len, changed, actual_sig.value, other_message, len(other_message)) != 0
    print("CONFIRMED sign-09-1: salt-flipped signature accepted; changed-message control rejected")


if __name__ == "__main__":
    main()
