#!/usr/bin/env python3
"""Static and scaled witnesses for SQIsignTriangle's modular challenge."""

from __future__ import annotations

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE = (
    ROOT
    / "Implementations/Reference_Implementation/SQIsignTriangle_lvl1"
    / "sqisigntriangle/src/verification/ref/lvlx"
)


def is_prime(value: int) -> bool:
    if value < 2:
        return False
    factor = 2
    while factor * factor <= value:
        if value % factor == 0:
            return value == factor
        factor += 1
    return True


def challenge(prefix: bytes, message: bytes, security_bits: int) -> tuple[int, int]:
    byte_len = security_bits // 16
    block_len = 2 * byte_len
    # Python exposes SHAKE output as a prefix rather than incremental squeezes;
    # split one long prefix into the same consecutive blocks used by the C code.
    stream = hashlib.shake_256(prefix + message).digest(4096)
    for offset in range(0, len(stream) - block_len + 1, block_len):
        left = int.from_bytes(stream[offset : offset + byte_len], "big")
        right = int.from_bytes(stream[offset + byte_len : offset + block_len], "big")
        if left % 4 == 3 and is_prime(left):
            return left, right
        if right % 4 == 3 and is_prime(right):
            return right, left
    raise RuntimeError("scaled challenge stream contained no suitable prime")


def main() -> int:
    common = (SOURCE / "common.c").read_text(encoding="utf-8")
    verify = (SOURCE / "verify.c").read_text(encoding="utf-8")
    for expression in (
        "size_t byte_len = SECURITY_BITS / 16;",
        "shake256_inc_absorb(&ctx, message, length);",
        "mpz_import(*c1, byte_len",
        "mpz_import(*c2, byte_len",
    ):
        assert expression in common, expression
    for expression in (
        "ibz_sub(&rem, &q, &c2);",
        "ibz_mod(&rem, &rem, &c1);",
        "verify = ibz_is_zero(&rem);",
    ):
        assert expression in verify, expression

    for level in (128, 160, 256, 512):
        print(f"level {level}: challenge-transfer work about 2^{level // 2}")

    q = 1_048_583
    pair_a = (251, q % 251)
    pair_b = (239, q % 239)
    assert pair_a != pair_b
    assert q % pair_a[0] == pair_a[1]
    assert q % pair_b[0] == pair_b[1]
    print(f"same-response proof witness: q={q}, challenges={pair_a} and {pair_b}")

    prefix = b"scaled SQIsignTriangle public-key and commitment"
    for counter in range(1_000_000):
        message = f"transferred-message-{counter}".encode()
        c1, c2 = challenge(prefix, message, 16)
        if q % c1 == c2:
            print(
                f"scaled transfer found after {counter + 1} messages: "
                f"c1={c1}, c2={c2}, q mod c1={q % c1}"
            )
            print("ATTACK sign-27-3/sign-27-4 CONFIRMED")
            return 0
    print("scaled transfer search unexpectedly failed")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
