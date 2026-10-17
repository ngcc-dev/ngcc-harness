#!/usr/bin/env python3
"""hash-19-2: short-message MoFang-768 and MoFang-1024 outputs are related.

Algorithm 6 initializes L with domain value 1010 (MoFang-768) or 1011
(MoFang-1024) and XORs 2 into L before the last block.  For a one-block
padded message (at most 1151 bits) the two instances therefore call the
Hirose compression function on L values that differ exactly in the bit that
its second branch flips (L xor 1).  They make the same two cipher calls with
the branches swapped, and

    MoFang-768(M) = MoFang-1024(M)[64:112] || MoFang-1024(M)[0:48].

The script checks that identity through the submitted libraries for every
length 0..1151 bits (random and all-zero messages), checks that it fails for
every length 1152..1399 bits, uses MoFang-768-XOF as an unrelated-profile
control, and checks the two domain constants in the submitted sources.
"""

import ctypes
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAND = ROOT / "hash-19"
ID = "hash-19-2"
SOURCES = {
    # (instance, tree): (line, expected statement)
    ("MoFang-768", "Reference_Implementation"): (264, "in_g[2][2] = 10;"),
    ("MoFang-1024", "Reference_Implementation"): (263, "in_g[2][2] = 11;"),
    ("MoFang-768", "Optimized_Implementation"): (279, "in_g[2][2] = 10;"),
    ("MoFang-1024", "Optimized_Implementation"): (278, "in_g[2][2] = 11;"),
}


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
    libs = [CAND / "lib" / f"lib{n}.so" for n in ("MoFang-768", "MoFang-1024", "MoFang-768-XOF")]
    missing = [str(p) for p in libs if not p.is_file()]
    if missing:
        print(f"SKIP {ID}: missing {', '.join(missing)} (build: make -C hash-19)")
        return 77
    h768 = load("MoFang-768", 96)
    h1024 = load("MoFang-1024", 128)
    xof768 = load("MoFang-768-XOF", 96)

    def related(message, bits):
        long = h1024(message, bits)
        return h768(message, bits) == long[64:112] + long[0:48]

    ok = True
    for label, source in (("random", bytes(random.Random(1).getrandbits(8) for _ in range(176))),
                          ("all-zero", bytes(176))):
        inside = sum(related(canonical(source, n), n) for n in range(0, 1152))
        outside = sum(related(canonical(source, n), n) for n in range(1152, 1400))
        print(f"[relation] {label:8s}: holds for {inside}/1152 lengths 0..1151; "
              f"{outside}/248 hits at 1152..1399 (control)")
        ok &= inside == 1152 and outside == 0

    message = b"abc"
    a, b = h768(message, 24), h1024(message, 24)
    print(f"[example]  MoFang-768('abc')  = {a.hex()}")
    print(f"[example]  MoFang-1024('abc') = {b.hex()}")
    x = xof768(message, 24)
    xof_related = x == b[64:112] + b[0:48]
    print(f"[control]  MoFang-768-XOF('abc') related in the same way: {xof_related}")
    ok &= a == b[64:112] + b[0:48] and not xof_related

    tree = CAND / "MoFang" / "Implementations"
    for (instance, kind), (line, statement) in SOURCES.items():
        path = tree / kind / instance / "CryptHash_AlgorithmInstance.c"
        if not path.is_file():
            print(f"[source]   {kind}/{instance}: not present, skipped")
            continue
        text = path.read_text(errors="replace").splitlines()
        found = len(text) >= line and text[line - 1].strip() == statement
        print(f"[source]   {kind}/{instance}/CryptHash_AlgorithmInstance.c:{line}: "
              f"{statement} {'present' if found else 'NOT FOUND'}")
        ok &= found

    if ok:
        print(f"CONFIRMED {ID}: MoFang-768(M) = MoFang-1024(M)[64:112] || MoFang-1024(M)[0:48] "
              "for every tested length 0..1151 bits; no hit at 1152..1399; XOF control unrelated")
        return 0
    print(f"NOT CONFIRMED {ID}: relation or control did not behave as expected")
    return 1


if __name__ == "__main__":
    sys.exit(main())
