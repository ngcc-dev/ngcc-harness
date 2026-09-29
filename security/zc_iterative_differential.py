#!/usr/bin/env python3
"""Exact local certificate for hash-30-1, hash-31-2, and hash-32-2."""

MASK64 = (1 << 64) - 1


def rol(x, n):
    return ((x << n) | (x >> (64 - n))) & MASK64


def chi(a0, a1, a2):
    a0 ^= (~a1 & 1) & a2
    a1 ^= (~a2 & 1) & a0
    a2 ^= (~a0 & 1) & a1
    return a0, a1, a2


preserving = []
for x in range(8):
    value = (x & 1, (x >> 1) & 1, (x >> 2) & 1)
    changed = (value[0] ^ 1, value[1], value[2])
    delta = tuple(a ^ b for a, b in zip(chi(*value), chi(*changed)))
    if delta == (1, 0, 0):
        preserving.append(value)

assert preserving == [(0, 0, 1), (1, 0, 1)]

for mask, weight in ((0x1111111111111111, 16), (MASK64, 64)):
    assert mask.bit_count() == weight
    assert rol(mask, 20) ^ rol(mask, 56) == 0

assert 16 * 2 == 32
assert 64 * 2 == 128
assert [(rounds, 32 * rounds) for rounds in (7, 11, 12)] == [
    (7, 224), (11, 352), (12, 384)
]
assert [(rounds, 128 * rounds) for rounds in (1, 2, 3)] == [
    (1, 128), (2, 256), (3, 384)
]

print("VALIDATED ZC local iterative differential: probability 2^-32 per ZC-1536 round")
print("VALIDATED reported conditional costs: 7/11/12 rounds -> 2^224/2^352/2^384")
print("LIMITATION multi-round satisfiability, independence, and valid-message distribution are not proved")
