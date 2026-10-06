#!/usr/bin/env python3
"""Verify that submitted Phoenix-SM3 signatures disclose a SK.prf suffix."""

import ctypes
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent
MESSAGE = b"Phoenix public-randomizer disclosure certificate"
INSTANCES = ("384s", "384f", "512s", "512f")

targets = [f"lib/libPhoenix-SM3-{instance}.so" for instance in INSTANCES]
build = subprocess.run(["make", "-C", str(ROOT), *targets], capture_output=True, text=True)
if build.returncode:
    raise RuntimeError(f"submitted-library build failed:\n{build.stdout}{build.stderr}")


for instance in INSTANCES:
    library = ROOT / "lib" / f"libPhoenix-SM3-{instance}.so"
    if not library.exists():
        print(f"SKIP sign-19-5: missing {library}; build the reference libraries with make -C sign-19")
        raise SystemExit(77)
    scheme = ctypes.CDLL(str(library))
    for name in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
        getattr(scheme, name).restype = ctypes.c_ulonglong

    n = int(instance[:3]) // 8
    pk_len = ctypes.c_ulonglong(scheme.sig_get_pk_len_bytes())
    sk_len = ctypes.c_ulonglong(scheme.sig_get_sk_len_bytes())
    pk = (ctypes.c_ubyte * pk_len.value)()
    sk = (ctypes.c_ubyte * sk_len.value)()
    assert scheme.sig_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len)) == 0

    # The submitted maximum may be tight; the margin is confined to this witness.
    signature = (ctypes.c_ubyte * (scheme.sig_get_sn_len_bytes() + 4096))()
    signature_len = ctypes.c_ulonglong()
    message = (ctypes.c_ubyte * len(MESSAGE)).from_buffer_copy(MESSAGE)
    assert scheme.sig_sign(sk, sk_len, message, len(MESSAGE),
                           signature, ctypes.byref(signature_len)) == 0
    assert scheme.sig_verify(pk, pk_len, signature, signature_len,
                             message, len(MESSAGE)) == 0

    exposed = bytes(signature)[32:n]
    expected = bytes(byte ^ 0x5C for byte in bytes(sk)[n + 32:2 * n])
    assert exposed == expected, instance
    print(f"PASS Phoenix-SM3-{instance}: valid signature discloses {len(exposed)} SK.prf bytes")

print("CONFIRMED sign-19-5: partial SK.prf disclosure in all four high-level SM3 sets")
