#!/usr/bin/env python3
"""Recompute SQIsign2D-push1/2's implemented challenge-space bounds."""

from math import log2
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent
TARGETS = {1: 128, 2: 160, 3: 256, 4: 512}
expected_exponents = {1: 78, 2: 118, 3: 156, 4: 324}

affected = []
for level, target in TARGETS.items():
    base = ROOT / "Implementations" / f"sqisign2d_lvl{level}" / "src"
    params = base / "precomp" / "ref" / f"lvl{level}" / "include" / "ec_params.h"
    source = base / "sqisigndim2" / "ref" / "sqisigndim2x" / "sign.c"
    text = params.read_text()
    exponent = int(re.search(r"^#define POWER_OF_3\s+(\d+)$", text, re.M).group(1))
    assert exponent == expected_exponents[level]

    sign_text = source.read_text()
    body = sign_text.split("void hash_to_challenge", 1)[1].split("int protocols_sign", 1)[0]
    assert body.count("sqisign_xof(") == 1
    assert "for (" not in body and "while (" not in body
    iccs = (base / "iccs" / "xof_iccs.c").read_text()
    shake = (base / "common" / "generic" / "xof_shake.c").read_text()
    assert iccs.count("pseudoXOF(") == 1
    assert shake.count("SHAKE256(") == 1

    space_bits = exponent * log2(3)
    denominator = 3**exponent
    ratio = ((1 << target) + denominator - 1) // denominator
    specified_bits = space_bits + log2(ratio)
    misses = space_bits < target
    if misses:
        affected.append(level)
    print(
        f"level={level} target={target} e2={exponent} "
        f"implemented_bits={space_bits:.6f} specified_iterations={ratio} "
        f"specified_bits={specified_bits:.6f} below_target={str(misses).lower()}"
    )

assert affected == [1, 3]
print("ATTACK sign-26-2 CONFIRMED missing grinding leaves Levels 1 and 3 below target")
