#!/usr/bin/env python3
"""Reproduce a natural Origami-128 tail-zone signing abort."""

import ctypes
import hashlib
from pathlib import Path


def main():
    lib_path = Path(__file__).with_name("lib") / "libOrigami-128.so"
    if not lib_path.exists():
        print("SKIP sign-18-6: build libOrigami-128.so first")
        return 77

    lib = ctypes.CDLL(str(lib_path.resolve()))
    for name in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
        getattr(lib, name).restype = ctypes.c_ulonglong
    pk_len = lib.sig_get_pk_len_bytes()
    sk_len = lib.sig_get_sk_len_bytes()
    sig_len = lib.sig_get_sn_len_bytes()

    def seed(value):
        buf = (ctypes.c_ubyte * len(value)).from_buffer_copy(value)
        if lib.ngcc_seed(buf, len(value)) != 0:
            raise RuntimeError("ngcc_seed failed")

    seed(bytes(48))
    pk = (ctypes.c_ubyte * pk_len)()
    sk = (ctypes.c_ubyte * sk_len)()
    written_pk = ctypes.c_ulonglong()
    written_sk = ctypes.c_ulonglong()
    if lib.sig_keygen(pk, ctypes.byref(written_pk), sk, ctypes.byref(written_sk)) != 0:
        raise RuntimeError("key generation failed")

    successes = failures = 0
    first_failure = None
    # Trial 16 is the first abort under this deterministic, unmodified call
    # sequence; trial 0 is a successful-signature control.
    for trial in (0, 16):
        seed(hashlib.shake_256(b"origami-tail-abort" + trial.to_bytes(4, "little")).digest(48))
        raw_message = trial.to_bytes(16, "little")
        message = (ctypes.c_ubyte * len(raw_message)).from_buffer_copy(raw_message)
        signature = (ctypes.c_ubyte * sig_len)()
        written_sig = ctypes.c_ulonglong()
        rc = lib.sig_sign(sk, sk_len, message, len(raw_message), signature, ctypes.byref(written_sig))
        if rc == 0:
            if lib.sig_verify(pk, pk_len, signature, written_sig.value, message, len(raw_message)) != 0:
                raise RuntimeError("successful signature did not verify")
            successes += 1
        elif rc == -4:
            failures += 1
            if first_failure is None:
                first_failure = trial
        else:
            raise RuntimeError(f"unexpected signing return code {rc}")

    ok = successes == 1 and failures == 1 and first_failure == 16
    print(f"natural calls: {successes} succeeded, {failures} aborted; first abort at trial {first_failure}")
    print("CONFIRMED sign-18-6: unmodified signer naturally exhausts its retries" if ok else
          "NOT CONFIRMED sign-18-6")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
