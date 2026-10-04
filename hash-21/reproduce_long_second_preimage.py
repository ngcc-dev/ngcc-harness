#!/usr/bin/env python3
"""Certificate for hash-21-4's long-message second-preimage scan."""

from __future__ import annotations

import math
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SRC = (
    ROOT
    / "Neulaser"
    / "Implementations and Test_Vectors"
    / "API_CryptHash"
    / "Implementations"
    / "Reference_Implementation"
)
MESSAGE_BYTES = 12_000_000_111
P = 2**32 - 5


def merge_positions(mu: int, v_bits: int) -> list[int]:
    """Message-loaded global state words reduced after XOR with gamma."""
    first_message_word = v_bits // 32
    return [
        16 * module + word
        for module in range(mu)
        for word in (8, 12)
        if 16 * module + word >= first_message_word
    ]


def main() -> None:
    rows = ((512, 960, 576, 3), (768, 1216, 832, 4), (1024, 1472, 1088, 5))
    for bits, block_bits, v_bits, mu in rows:
        text = (SRC / f"Neulaser-{bits}" / "CryptHash_AlgorithmInstance.c").read_text()
        assert "#define NL_P 4294967291ULL" in text
        assert "T[i][7] = nl_redp32(S[i][8] ^" in text
        assert "T[i][11] = nl_redp32(S[i][12] ^" in text
        # gamma and phi read neither S[i][8] nor S[i][12].
        dependency_slice = text[text.index("for (int i = 0; i < mu; i++)\n    {\n        uint32_t lin") : text.index("for (int i = 0; i < mu; i++)\n    {\n        int ax")]
        assert "[8]" not in dependency_slice and "[12]" not in dependency_slice

        positions = merge_positions(mu, v_bits)
        assert len(positions) == {512: 4, 768: 5, 1024: 6}[bits]
        full_blocks = (8 * MESSAGE_BYTES) // block_bits
        trials = full_blocks * len(positions)
        success = -math.expm1(trials * math.log1p(-10 / 2**32))
        expected_blocks = 2**32 / (10 * len(positions))
        expected = {512: 0.6059677, 768: 0.6011094, 1024: 0.5979087}[bits]
        assert abs(success - expected) < 2e-7
        print(
            f"Neulaser-{bits}: positions/block={len(positions)} "
            f"eligible={trials} success={success:.6%} "
            f"log2_expected_blocks={math.log2(expected_blocks):.3f}"
        )

    fiber_inputs = {x for y in range(5) for x in (y, y + P)}
    assert len(fiber_inputs) == 10
    assert {x % P for x in fiber_inputs} == {0, 1, 2, 3, 4}
    print("ATTACK hash-21-4 CONFIRMED: long-message second-preimage scan")


if __name__ == "__main__":
    main()
