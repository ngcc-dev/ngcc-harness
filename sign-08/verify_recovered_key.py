#!/usr/bin/env python3
"""Use a recovered DARTS-128 algebraic key for a fresh-message forgery."""

from __future__ import annotations

import ctypes
import sys
from pathlib import Path


def cbuf(value: bytes):
    return (ctypes.c_ubyte * len(value)).from_buffer_copy(value)


def main() -> int:
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} LIB PK RECOVERED_SK", file=sys.stderr)
        return 2

    lib = ctypes.CDLL(str(Path(sys.argv[1]).resolve()))
    u64 = ctypes.c_ulonglong
    lib.sig_get_pk_len_bytes.restype = u64
    lib.sig_get_sk_len_bytes.restype = u64
    lib.sig_get_sn_len_bytes.restype = u64

    pk = Path(sys.argv[2]).read_bytes()
    sk = Path(sys.argv[3]).read_bytes()
    assert len(pk) == lib.sig_get_pk_len_bytes()
    assert len(sk) == lib.sig_get_sk_len_bytes()

    message = b"fresh-message forgery from recovered DARTS algebraic key"
    changed = b"fresh-message forgery from recovered DARTS algebraic kez"
    signature = (ctypes.c_ubyte * lib.sig_get_sn_len_bytes())()
    signature_len = u64(len(signature))

    rc = lib.sig_sign(
        cbuf(sk), len(sk), cbuf(message), len(message),
        signature, ctypes.byref(signature_len),
    )
    valid = lib.sig_verify(
        cbuf(pk), len(pk), signature, signature_len.value,
        cbuf(message), len(message),
    )
    control = lib.sig_verify(
        cbuf(pk), len(pk), signature, signature_len.value,
        cbuf(changed), len(changed),
    )
    if rc != 0 or valid != 0 or control == 0:
        print(
            f"DARTS recovery replay failed: sign={rc} verify={valid} "
            f"changed-message={control}",
            file=sys.stderr,
        )
        return 1
    print(
        "ATTACK sign-08-2 DARTS-128 CONFIRMED: recovered algebraic key "
        "signs a fresh message; changed-message control rejects"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
