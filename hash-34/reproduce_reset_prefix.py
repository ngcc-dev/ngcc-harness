#!/usr/bin/env python3
"""Check WChain's reset-prefix algebra in a reduced-width permutation model."""

from hashlib import sha256
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "WChain/Implementations and Test_Vectors/Implementations/Reference_Implementation/WChain-V1-512/wchain_c.c"
text = SOURCE.read_text(encoding="utf-8")
other_source = ROOT / "WChain/Implementations and Test_Vectors/Implementations/Reference_Implementation/WChain-V2-1024/wchain_c.c"
assert other_source.read_text(encoding="utf-8") == text
for fragment in (
    "out[i] = x[i] ^ b_final[i] ^ v[i]",
    "h_prev1[i] = c[i] ^ h_prev2[i]",
    "h_prev2[i] = old_h1",
    "sigma_words[i] ^= w0",
    "sigma_words[9 + i] ^= w1",
    "wchain_v1_compress_loaded(h, h_prev1, b0, b1)",
):
    assert fragment in text, f"submitted source changed: {fragment}"

MASK = (1 << 16) - 1


def rotl(value: int, shift: int) -> int:
    return ((value << shift) | (value >> (16 - shift))) & MASK if shift else value


def rotr(value: int, shift: int) -> int:
    return ((value >> shift) | (value << (16 - shift))) & MASK if shift else value


def parameters(block: int) -> tuple[int, int]:
    digest = sha256(block.to_bytes(4, "big")).digest()
    return digest[0] & 15, int.from_bytes(digest[1:3], "big")


def permutation(block: int, state: int) -> int:
    shift, constant = parameters(block)
    return rotl(state, shift) ^ constant


def inverse_at_zero(block: int) -> int:
    shift, constant = parameters(block)
    return rotr(constant, shift)


def compress(state: int, block: int) -> int:
    return permutation(block, state) ^ state


def model_hash(message: bytes) -> int:
    padded = message + b"\x80"
    padded += b"\x00" * (-len(padded) % 4)
    previous, current, checksum = 0, 0, 0
    for offset in range(0, len(padded), 4):
        block = int.from_bytes(padded[offset : offset + 4], "big")
        checksum ^= block
        previous, current = current, compress(current, block) ^ previous
    return compress(current, checksum)


forward: dict[int, int] = {}
for candidate in range(2048):
    block = (candidate * 0x9E3779B1) & 0xFFFFFFFF
    forward.setdefault(permutation(block, 0), block)

pair = next(
    (
        (forward[required], block)
        for candidate in range(2048, 8192)
        for block in [(candidate * 0x9E3779B1) & 0xFFFFFFFF]
        for required in [inverse_at_zero(block)]
        if required in forward
    ),
    None,
)
assert pair is not None, "reduced-width birthday search found no pair"
x, y = pair
a = permutation(x, 0)
assert a == inverse_at_zero(y) and permutation(y, a) == 0
states = [0, 0]
for block in (x, y, y, x):
    states.append(compress(states[-1], block) ^ states[-2])
assert states[2:] == [a, a, 0, 0]
assert x ^ y ^ y ^ x == 0

prefix = b"".join(block.to_bytes(4, "big") for block in (x, y, y, x))
for message in (b"", b"a", b"abc", b"abcd", b"WChain reset test"):
    assert model_hash(prefix + message) == model_hash(message)

print("STRUCTURAL hash-34-1 CONFIRMED: source paths and reduced-width reset prefix")
print("FULL-SIZE hash-34-1 NOT RUN: birthday-like matching remains an assumption")
