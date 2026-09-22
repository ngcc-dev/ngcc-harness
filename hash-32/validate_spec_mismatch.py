#!/usr/bin/env python3
# Finding: hash-32-1
"""Check ZC-EDMC's h/h construction and submitted round schedule."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
BASE = ROOT / "ZC-EDMC/Implementations"
REF = BASE / "Implementations/Reference_Implementation"
LIB = BASE / "lib/low"
INSTANCES = (
    "ZC-EDMC-1280-512",
    "ZC-EDMC-1280-768",
    "ZC-EDMC-1280-1024",
    "ZC-EDMC-1536-512",
    "ZC-EDMC-1536-768",
    "ZC-EDMC-1536-1024",
)
SPEC = (0x58, 0x38, 0x3C0, 0xD0, 0x60, 0x120, 0x14, 0x2C, 0xF0, 0x1A0, 0x380, 0x12)
CODE = (0x58, 0x38, 0x3C0, 0xD0, 0x120, 0x14, 0x60, 0x2C, 0x380, 0xF0, 0x1A0, 0x12)


for instance in INSTANCES:
    source = (REF / instance / "CryptHash_AlgorithmInstance.c").read_text()
    body = re.search(
        r"static void absorb_block_custom\([^\n]+\)\n\{(?P<body>.*?)\n\}",
        source,
        re.S,
    )
    assert body, f"{instance}: absorb_block_custom not found"
    calls = re.findall(r"^\s*permute_half\(state\);", body.group("body"), re.M)
    assert len(calls) == 2, f"{instance}: expected two h calls, found {len(calls)}"
    print(f"{instance}: compression calls the same six-round half twice")

for width in (1280, 1536):
    header = (LIB / f"ZuD-{width}" / f"ZuD{width}.h").read_text()
    constants = {
        int(n): int(value, 16)
        for n, value in re.findall(
            rf"#define ZUD{width}_rc(\d+)\s+0x([0-9A-Fa-f]+)ULL", header
        )
    }
    schedule = tuple(constants[n] for n in range(12, 0, -1))
    assert schedule == CODE, f"ZuD-{width}: unexpected submitted schedule"
    assert schedule != SPEC
    mismatches = [i + 1 for i, pair in enumerate(zip(SPEC, schedule)) if pair[0] != pair[1]]
    assert mismatches == [5, 6, 7, 9, 10, 11]
    print(f"ZuD-{width}: code/spec constants differ at rounds {mismatches}")

print("CONFIRMED: all instances implement h-after-h with the code constant schedule")
