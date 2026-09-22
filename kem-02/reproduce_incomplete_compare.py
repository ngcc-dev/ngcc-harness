#!/usr/bin/env python3
# Finding: kem-02-1
"""Reproduce Jinnuo Li's Amoeba incomplete-FO-comparison finding."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent
SETS = ("576", "864", "1152", "1728", "2304")


def source(bits: str) -> str:
    return (ROOT / "Implementations" / "Reference_Implementation" /
            f"Amoeba-{bits}" / "src" / "backend" / "ccakem.c").read_text()


def vulnerable_compare(a: bytes, b: bytes) -> int:
    return sum(a[i] != b[i] for i in range(0, len(a), 4))


def main() -> int:
    texts = [source(bits) for bits in SETS]
    normalized = [re.sub(r"\s+", " ", text) for text in texts]
    needle = "for (int32_t i = 0; i < len; i += 4)"
    assert all(needle in text for text in normalized)
    assert all("tag += (c1[i] != c2[i])" in text for text in normalized)

    length = 1047  # Amoeba-576 ciphertext length
    checked = tuple(range(0, length, 4))
    assert len(checked) == 262
    base = bytes(length)
    for position in (1, 2, 3, 1046):
        changed = bytearray(base)
        changed[position] = 1
        assert vulnerable_compare(base, changed) == 0
    changed = bytearray(base)
    changed[1044] = 1
    assert vulnerable_compare(base, changed) != 0

    print("affected source trees: 5/5")
    print(f"Amoeba-576 checked bytes: {len(checked)}/{length}")
    print(f"unchecked bytes: {length - len(checked)}/{length}")
    print("blind-position controls: PASS (1, 2, 3, 1046)")
    print("checked-position control: PASS (1044)")
    print("result: AMOEBA_INCOMPLETE_FO_COMPARISON_CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
