#!/usr/bin/env python3
"""hash-14-1 (extension): Laurus-512 equals the first 512 bits of Laurus-XOF.

Laurus-512 = Laurus[512,0] and Laurus-XOF = Laurus[512,1] (specification
Sections 2.4-2.5).  The function identifier is placed only in the last state
word S0[23].  At c=512, g0 copies S[8..23] to the feed-forward half without
passing them through the permutation, so each absorption moves the fid
difference down by 8 words (S0[23] -> S1[15] -> S2[7]); g1 never reads S[7].
After exactly two absorptions, i.e. for 1537..2560-bit messages, the output is
independent of fid and the two named functions agree on their common prefix.

The script checks the identity through the submitted libraries for every
length 0..3199 bits on three random streams and the all-zero stream, requires
the hit set to be exactly 1537..2560, and checks the two named wrappers in the
submitted sources.
"""

import ctypes
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAND = ROOT / "hash-14"
ID = "hash-14-1"
REF = CAND / "Laurus" / "Implementations" / "Reference_Implementation"
SOURCES = (
    (REF / "Laurus-512" / "laurus_512.c", 260, "laurus(h, 512, in, inlen, 512, 0);"),
    (REF / "Laurus-XOF" / "laurus_xof.c", 261, "laurus(out, outlen, in, inlen, 512, 1);"),
)


def load(instance, size):
    function = ctypes.CDLL(str(CAND / "lib" / f"lib{instance}.so")).CryptHash
    function.argtypes = (ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p)
    function.restype = ctypes.c_int

    def digest(message, bits):
        data = bytes(message[:(bits + 7) // 8]) + bytes(8)
        source = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        output = (ctypes.c_ubyte * size)()
        if function(8 * size, source, bits, output) != 0:
            raise RuntimeError(f"{instance}: CryptHash failed")
        return bytes(output)

    return digest


def canonical(source, bits):
    message = bytearray(source[:(bits + 7) // 8])
    if bits % 8:
        message[-1] &= (0xFF << (8 - bits % 8)) & 0xFF
    return bytes(message)


def main():
    libs = [CAND / "lib" / f"lib{n}.so" for n in ("Laurus-512", "Laurus-XOF")]
    missing = [str(p) for p in libs if not p.is_file()]
    if missing:
        print(f"SKIP {ID}: missing {', '.join(missing)} (build: make -C hash-14)")
        return 77
    h512 = load("Laurus-512", 64)
    xof = load("Laurus-XOF", 256)   # the submitted XOF wrapper returns 2048 bits

    window = list(range(1537, 2561))
    ok = True
    streams = [(f"random seed {s}", bytes(random.Random(s).getrandbits(8) for _ in range(400)))
               for s in (1, 2, 3)] + [("all-zero", bytes(400))]
    for label, source in streams:
        hits = [n for n in range(0, 3200)
                if h512(canonical(source, n), n) == xof(canonical(source, n), n)[:64]]
        exact = hits == window
        outside = len([n for n in hits if n not in window])
        print(f"[relation] {label:13s}: {len(hits)} hits in 0..3199; exactly 1537..2560: {exact}; "
              f"out-of-window hits (control): {outside}")
        ok &= exact

    message = bytes(256)
    a, x = h512(message, 2048), xof(message, 2048)
    print(f"[example]  Laurus-512(0^2048)      = {a.hex()}")
    print(f"[example]  Laurus-XOF(0^2048)[:64] = {x[:64].hex()}")
    ok &= a == x[:64]

    for path, line, statement in SOURCES:
        if not path.is_file():
            print(f"[source]   {path.name}: not present, skipped")
            continue
        text = path.read_text(errors="replace").splitlines()
        found = len(text) >= line and text[line - 1].strip() == statement
        print(f"[source]   {path.parent.name}/{path.name}:{line}: {statement} "
              f"{'present' if found else 'NOT FOUND'}")
        ok &= found

    if ok:
        print(f"CONFIRMED {ID}: Laurus-512(M) = MSB_512(Laurus-XOF(M)) for exactly the "
              "1537..2560-bit messages tested; no out-of-window hit")
        return 0
    print(f"NOT CONFIRMED {ID}: relation window or control did not behave as expected")
    return 1


if __name__ == "__main__":
    sys.exit(main())
