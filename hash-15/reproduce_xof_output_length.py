#!/usr/bin/env python3
"""Show that Litchi-XOF writes its fixed 128-byte output for a 64-byte request."""

import ctypes
from pathlib import Path

libdir = Path(__file__).resolve().parent / "lib"
library = next(p for p in (libdir / "libLitchi-XOF.so", libdir / "liblitchi_xof.so") if p.is_file())
lib = ctypes.CDLL(str(library))
lib.CryptHash.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p]
message = ctypes.create_string_buffer(b"NGCC Litchi length control")
out = (ctypes.c_ubyte * 128)(*([0xA5] * 128))
assert lib.CryptHash(512, message, 8 * (len(message.raw) - 1), out) == 0
assert any(b != 0xA5 for b in out[64:]), "no write beyond requested 64 bytes"
full = (ctypes.c_ubyte * 128)()
assert lib.CryptHash(1024, message, 8 * (len(message.raw) - 1), full) == 0
assert bytes(out) == bytes(full), "short request did not write the fixed full digest"
print("CONFIRMED hash-15-1: 512-bit request wrote the full 1024-bit XOF output")
