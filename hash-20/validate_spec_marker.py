#!/usr/bin/env python3
# Finding: hash-20-2
"""Check the MOZI-768/1024 final-marker positions in the submitted code."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
REF = ROOT / "Mozi/Implementations/Reference_Implementation"
INSTANCES = {"MOZI-768": 152, "MOZI-1024": 120}
SPEC_MARKER_BIT = 2047


for instance, rate_bytes in INSTANCES.items():
    source = (REF / instance / "CryptHash_AlgorithmInstance.c").read_text()
    call = f"hash_Fsponge(digest_len_bits / 8, {rate_bytes},"
    assert call in source, f"{instance}: expected rate not found"
    assert "state[rate] ^= 0x80; // domain separation" in source
    code_marker_bit = 8 * rate_bytes
    assert code_marker_bit != SPEC_MARKER_BIT
    print(
        f"{instance}: code marker bit {code_marker_bit}; "
        f"Algorithm 2 marker bit {SPEC_MARKER_BIT}"
    )

print("CONFIRMED: the implementation uses rate-dependent marker positions")
