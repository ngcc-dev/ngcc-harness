#!/usr/bin/env python3
"""Scaled, padding-aware witness for the C-Hash-1024 backward-tree route.

The full C-Engine search is infeasible. This checks the archived mode source and
executes the same VIL/FIL algebra with a 20-bit state and a toy permutation.
"""

from collections import defaultdict
from hashlib import blake2s
from pathlib import Path
from random import Random


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "C Hash/Implementations/Reference_Implementation/CHash_1024/CryptHash_AlgorithmInstance.c"
source = SOURCE.read_text(encoding="utf-8")
for fragment in (
    "#define CH_N_BITS       ((size_t)1472)",
    "#define CH_HALF_N_BITS  ((size_t)736)",
    "v->mode         = CH_CTR_FUNC;",
    "xor_inplace(y_n, x_n, CH_N_BYTES);",
    "xor_inplace(h2, m_i, CH_HALF_N_BYTES);",
    "make_CTR_bytes(v->lane_inject, (uint64_t)i, ctr);",
    "make_CTR_bytes(v->lane_gen, 1, ctr);",
):
    assert fragment in source, f"submitted mode changed: {fragment}"


class Toy:
    state_bits = 20
    block_bits = state_bits // 2
    counter_bits = 8
    state_mask = (1 << state_bits) - 1
    block_mask = (1 << block_bits) - 1
    feistel_mask = (1 << ((state_bits + counter_bits) // 2)) - 1
    pad = 1 << (block_bits - 1)  # M || 1 || 0* for a block-aligned message

    def permutation(self, value):
        half = (self.state_bits + self.counter_bits) // 2
        left, right = value >> half, value & self.feistel_mask
        for index in range(10):
            digest = blake2s(
                bytes((index,)) + right.to_bytes(2, "big"),
                digest_size=2,
                person=b"CHashToy",
            ).digest()
            left, right = right, left ^ (int.from_bytes(digest, "big") & self.feistel_mask)
        return (left << half) | right

    def g(self, state, counter):
        out = self.permutation((state << self.counter_bits) | counter)
        return (out >> self.counter_bits) ^ state

    def step(self, previous, block, position):
        x = previous ^ (block << self.block_bits)
        return self.g(x, 0x40 | position) ^ block

    def initial(self):
        iv = self.state_bits
        return self.g(iv << self.block_bits, 0x40) ^ iv

    def chain(self, blocks):
        state = self.initial()
        for position, block in enumerate(blocks, 1):
            state = self.step(state, block, position)
        return state

    def digest(self, blocks):
        return self.g(self.chain(blocks), 0x81) & self.state_mask


toy = Toy()
rng = Random(20261009)
probes = 1 << 14
found = None
for _attempt in range(6):
    challenge = tuple(rng.getrandbits(toy.block_bits) for _ in range(4))
    target = toy.chain(challenge)  # state before the fixed fifth, padded block
    level = {target: ()}
    for position in (4, 3):
        buckets = defaultdict(list)
        for state, tail in level.items():
            buckets[state >> toy.block_bits].append((state, tail))
        next_level = {}
        for _ in range(probes):
            x = rng.getrandbits(toy.state_bits)
            g = toy.g(x, 0x40 | position)
            for state, tail in buckets.get(g >> toy.block_bits, ()):
                block = (g & toy.block_mask) ^ (state & toy.block_mask)
                predecessor = x ^ (block << toy.block_bits)
                assert toy.step(predecessor, block, position) == state
                next_level[predecessor] = (block,) + tail
        level = next_level
    for _ in range(probes):
        prefix = (rng.getrandbits(toy.block_bits), rng.getrandbits(toy.block_bits))
        tail = level.get(toy.chain(prefix))
        if tail is not None:
            forged = prefix + tail
            if forged != challenge:
                found = (challenge, forged)
                break
    if found:
        break

assert found is not None, "scaled backward-tree search found no join"
challenge, forged = found
assert len(challenge) == len(forged) == 4 and challenge != forged
assert toy.chain(challenge) == toy.chain(forged)
assert toy.digest(challenge + (toy.pad,)) == toy.digest(forged + (toy.pad,))
assert toy.digest(forged + (toy.pad ^ 1,)) != toy.digest(challenge + (toy.pad,))
assert any(
    toy.digest((forged[0] ^ bit,) + forged[1:] + (toy.pad,))
    != toy.digest(challenge + (toy.pad,))
    for bit in (1, 2, 4)
)

print("SCALED hash-03-1 CONFIRMED: distinct same-length messages with the same fixed padding collide")
print("FULL-SIZE hash-03-1 NOT RUN: 2^983 cost is modeled, not measured")
