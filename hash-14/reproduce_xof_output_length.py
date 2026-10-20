#!/usr/bin/env python3
"""Show that Laurus-XOF writes its fixed 256-byte output for a 64-byte request."""

import ctypes
from pathlib import Path

lib = ctypes.CDLL(str(Path(__file__).resolve().parent / "lib/libLaurus-XOF.so"))
lib.CryptHash.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p]
message = ctypes.create_string_buffer(b"NGCC Laurus length control")
out = (ctypes.c_ubyte * 256)(*([0xA5] * 256))
assert lib.CryptHash(512, message, 8 * (len(message.raw) - 1), out) == 0
assert any(b != 0xA5 for b in out[64:]), "no write beyond requested 64 bytes"
full = (ctypes.c_ubyte * 256)()
assert lib.CryptHash(2048, message, 8 * (len(message.raw) - 1), full) == 0
assert bytes(out) == bytes(full), "short request did not write the fixed full digest"
print("CONFIRMED hash-14-2: 512-bit request wrote the full 2048-bit XOF output")
