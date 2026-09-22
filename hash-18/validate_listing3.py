#!/usr/bin/env python3
# Finding: hash-18-2
"""Check the AVX-512 ternary truth tables printed in MEGASCON Listing 3."""

BIT_PERM = (1, 7, 5, 2, 4, 6, 0, 3)


def ternary(a: int, b: int, c: int, immediate: int) -> int:
    # VPTERNLOG indexes the immediate by DEST:a, SRC1:b, SRC2:c.
    return (immediate >> (4 * a + 2 * b + c)) & 1


def sbox(x: int, regular: int, special: int, permute: bool = False) -> int:
    q = [(x >> i) & 1 for i in range(8)]
    calls = (
        (q[1], q[2], q[0], regular),
        (q[2], q[0], q[4], regular),
        (q[0], q[1], q[3], regular),
        (q[4], q[5], q[1], special),
        (q[5], q[6], q[2], regular),
        (q[6], q[7], q[5], regular),
        (q[7], q[3], q[6], regular),
        (q[3], q[4], q[7], regular),
    )
    out = [0] * 8
    for row, call in enumerate(calls):
        out[BIT_PERM[row] if permute else row] = ternary(*call)
    return sum(bit << i for i, bit in enumerate(out))


bad = [sbox(x, 0xB4, 0x1E) for x in range(256)]
fixed = [sbox(x, 0xA6, 0x56) for x in range(256)]

assert bad[0x23] == bad[0x24] == 0x1B
assert len(set(bad)) == 175
assert len(set(fixed)) == 256

print("Listing 3: S_bad(23) = S_bad(24) = 1b; distinct outputs = 175")
print("Corrected immediates a6/56: distinct outputs = 256")
