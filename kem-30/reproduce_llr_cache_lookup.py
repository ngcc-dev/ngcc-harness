#!/usr/bin/env python3
"""Certify PolarLAC's secret-indexed LLR table lookup in every source tree."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent
POLYS = sorted((ROOT / "Implementations").glob("**/poly.c"))

assert len(POLYS) == 20, f"expected 20 PolarLAC poly.c files, found {len(POLYS)}"

for path in POLYS:
    text = path.read_text(encoding="utf-8", errors="replace")
    assert re.search(r"centered\s*=\s*\(int16_t\)\(hatm\[i\]\s*-\s*half_2\)", text)
    assert re.search(r"llr\[i\]\s*=\s*llr_table\[centered\s*\+\s*RATIO\s*-\s*1\]", text)

print(
    "CONFIRMED kem-30-2: all 20 PolarLAC source trees index llr_table "
    "with a decrypted-coefficient-derived value"
)
