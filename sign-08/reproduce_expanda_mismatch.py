#!/usr/bin/env python3
"""Source certificate for sign-08-3's reference/AVX2 ExpandA divergence."""
from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations"
checked = 0
for level in (128, 256, 512):
    reference = (ROOT / f"Reference_Implementation/DARTS{level}/poly.c").read_text()
    optimized = (ROOT / f"Optimized_Implementation/DARTS_AVX2/DARTS{level}/poly.c").read_text()
    ref_uniform = reference.split("void poly_uniform(", 1)[1].split("\n}", 1)[0]
    opt_uniform = optimized.split("void poly_uniform(", 1)[1].split("\n}", 1)[0]
    opt_four = optimized.split("void poly_uniform_4x(", 1)[1].split("\n}", 1)[0]
    assert "blocks_numbers = 9" in ref_uniform
    assert "off = ((8 * buflen) % 16 + 7) / 8" in ref_uniform
    assert "blocks_numbers = 7" in opt_uniform
    assert "off = ((8 * buflen) % Q_BITS + 7) / 8" in opt_uniform
    assert "blocks_numbers = 7" in opt_four
    assert "shake128x4_squeezeblocks" in opt_four
    checked += 1

assert checked == 3
print("CONFIRMED sign-08-3: all three tiers have divergent reference/AVX2 ExpandA sampling schedules")
