#!/usr/bin/env python3
"""Recompute Facto-DSA's unsalted target-space collision bounds."""

from math import log2, log
from pathlib import Path
import re

base = Path(__file__).resolve().parent / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
affected = []
for level in (128, 256, 512):
    tree = base / f"Facto-DSA-{level}"
    header = (tree / "SIG_AlgorithmInstance.h").read_text()
    source = (tree / "SIG_AlgorithmInstance.c").read_text()
    m = int(re.search(r"^#define FACTO_M (\d+)$", header, re.M).group(1))
    q = int(re.search(r"^#define FACTO_Q (\d+)u$", header, re.M).group(1))
    assert q == 65519
    assert source.count("xof_field(pkh, m, m_len_bytes, h)") == 2
    body = source.split("static int xof_field(", 1)[1].split("static int quad_index", 1)[0]
    assert "memcpy(buf + off, pkh, FACTO_PKH_BYTES)" in body
    assert "memcpy(buf + off, m, (size_t)m_len)" in body
    bits = m * log2(q)
    collision = bits / 2 + log2(2 * log(2)) / 2  # 50% birthday success
    if collision < level:
        affected.append(level)
    print(f"level={level} q={q} m={m} target_bits={bits:.3f} collision_50pct_bits={collision:.3f}")

assert affected == [128, 512]
print("CONFIRMED sign-10-3: unsalted target collisions fall below the 128- and 512-bit claims")
