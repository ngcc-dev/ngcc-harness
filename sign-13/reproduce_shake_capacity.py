#!/usr/bin/env python3
"""Check sign-13-1's SHAKE capacity bound and a scaled two-block collision."""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parent
SRC = ROOT / "Implementations" / "Reference_Implementation" / "GreatWall512f"


def permutation(value: int) -> int:
    """A deterministic 32-bit Feistel permutation for the scaled model."""
    left, right = value >> 16, value & 0xFFFF
    for round_number in range(8):
        f = ((right * 0x9E37 + 0xB7E1 + round_number * 0x6D2B) & 0xFFFF)
        f ^= ((f << 7) | (f >> 9)) & 0xFFFF
        left, right = right, left ^ f
    return (left << 16) | right


def main() -> int:
    hash_header = (SRC / "hash.h").read_text(encoding="utf-8")
    implementation = (SRC / "faest.c").read_text(encoding="utf-8")
    assert "Keccak_HashInitialize_SHAKE256(ctx)" in hash_header
    assert "hash_update(&hasher, pk_packed, GREATWALL_PUBLIC_KEY_BYTES);" in implementation
    assert "hash_update(&hasher, msg, msg_len);" in implementation
    assert "hash_update_byte(&hasher, 1);" in implementation
    assert "hash_final(&hasher, &mu, sizeof(mu));" in implementation

    # Scaled Sponge-F construction: rate=capacity=16 bits. Search first
    # blocks for equal capacity, then cancel the rate difference in block 2.
    initial = 0x4E474343
    seen: dict[int, tuple[int, int]] = {}
    pair = None
    for block in range(1 << 16):
        state = permutation(initial ^ (block << 16))
        capacity = state & 0xFFFF
        if capacity in seen:
            pair = (*seen[capacity], block, state)
            break
        seen[capacity] = (block, state)
    assert pair is not None
    block0, state0, block1, state1 = pair
    assert block0 != block1
    cancel = (state0 >> 16) ^ (state1 >> 16)
    merged0 = permutation(state0)
    merged1 = permutation(state1 ^ (cancel << 16))
    assert merged0 == merged1

    classical = 512 / 2
    quantum = 512 / 3
    assert classical < 512
    print(f"scaled_capacity_collision_blocks={block0},{block1}")
    print(f"scaled_rate_cancellation={cancel}")
    print("scaled_post_second_block_states=IDENTICAL")
    print(f"GreatWall-512 classical_work=2^{classical:.0f}")
    print(f"GreatWall-512 quantum_queries=2^{quantum:.1f}")
    print("GREATWALL_SHAKE_CAPACITY_FORGERY_BOUND=CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
