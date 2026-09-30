#!/usr/bin/env python3
"""Certify that literal squared-count Algorithm 9 rejects every TRIKE key."""

from math import comb
from pathlib import Path
import re


HERE = Path(__file__).resolve().parent
# Table 11: instance -> (d, s, s').  TRIKE-1 and -3 are specified but no
# implementation of those two parameter sets is present in the archive.
PARAMETERS = {
    "TRIKE-1": (27, 28, 50),
    "TRIKE-2": (35, 46, 83),
    "TRIKE-3": (41, 59, 102),
    "TRIKE-5": (55, 105, 172),
    "TRIKE-7": (83, 235, 405),
    "TRIKE-9": (111, 433, 737),
}

for name, (d, s, ss) in PARAMETERS.items():
    # For nonnegative integer multiplicities, sum x_i^2 >= sum x_i.
    # Intra-block multiplicities sum to C(d,2); inter-block ones to d^2.
    intra_lb = comb(d, 2)
    inter_lb = d * d
    assert intra_lb > s and inter_lb > ss
    print(
        f"{name}: literal intra >= {intra_lb} > {s}; "
        f"literal inter >= {inter_lb} > {ss}"
    )

base = HERE / "Implementations and Test_Vectors/Implementations"
for tree in ("Reference_Implementation", "Optimized_Implementation"):
    for name in ("TRIKE-2", "TRIKE-5", "TRIKE-7", "TRIKE-9"):
        source = (base / tree / name / "src/sample.c").read_text(errors="replace")
        params = (base / tree / name / "src/trike_params.h").read_text(
            errors="replace"
        )
        d, s, ss = PARAMETERS[name]
        assert re.search(r"for \(uint64_t i = 2, weight = 1;", source)
        assert "weight += i++" in source
        assert int(re.search(r"#define PARAM_D\s+(\d+)", params).group(1)) == d
        assert int(re.search(r"#define PARAM_S\s+(\d+)", params).group(1)) == s
        assert int(re.search(r"#define PARAM_SS\s+(\d+)", params).group(1)) == ss

print("TRIKE_LITERAL_WEAK_KEY_TEST_REJECTS_ALL_CONFIRMED")
