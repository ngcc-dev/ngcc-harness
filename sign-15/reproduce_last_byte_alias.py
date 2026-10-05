#!/usr/bin/env python3
"""Check that the final ATLAS-512 signature byte is ignored by verification."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent
LIBRARY = ROOT / "lib/liblwrdsa512.so"


def verify(lib: ctypes.CDLL, pk: bytes, signature: bytes, message: bytes) -> int:
    pk_buf = ctypes.create_string_buffer(pk)
    sig_buf = ctypes.create_string_buffer(signature)
    msg_buf = ctypes.create_string_buffer(message)
    return lib.sig_verify(
        pk_buf, len(pk), sig_buf, len(signature), msg_buf, len(message)
    )


def main() -> int:
    if not LIBRARY.is_file():
        print(f"SKIP sign-15-11: build {LIBRARY} first")
        return 77

    lib = ctypes.CDLL(str(LIBRARY))
    for name in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
        getattr(lib, name).restype = ctypes.c_ulonglong
    pk_len = lib.sig_get_pk_len_bytes()
    sk_len = lib.sig_get_sk_len_bytes()
    sig_len = lib.sig_get_sn_len_bytes()
    pk = (ctypes.c_ubyte * pk_len)()
    sk = (ctypes.c_ubyte * sk_len)()
    got_pk = ctypes.c_ulonglong()
    got_sk = ctypes.c_ulonglong()
    if lib.sig_keygen(pk, ctypes.byref(got_pk), sk, ctypes.byref(got_sk)) != 0:
        raise RuntimeError("key generation failed")
    assert got_pk.value == pk_len and got_sk.value == sk_len

    message = b"ATLAS-512 final signature byte"
    msg = ctypes.create_string_buffer(message)
    signed = (ctypes.c_ubyte * (sig_len + len(message)))()
    got_sig = ctypes.c_ulonglong()
    if lib.sig_sign(sk, sk_len, msg, len(message), signed, ctypes.byref(got_sig)) != 0:
        raise RuntimeError("signing failed")
    assert got_sig.value == len(signed)

    original = bytes(signed)
    changed = bytearray(original)
    changed[sig_len - 1] ^= 0x80
    if verify(lib, bytes(pk), original, message) != 0:
        raise RuntimeError("honest signature rejected")
    if verify(lib, bytes(pk), bytes(changed), message) != 0:
        raise RuntimeError("changed final signature byte rejected")
    if verify(lib, bytes(pk), bytes(changed), message + b"!") == 0:
        raise RuntimeError("changed-message control accepted")

    print("MALLEABILITY sign-15-11 CONFIRMED: final ATLAS-512 signature byte is ignored")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
