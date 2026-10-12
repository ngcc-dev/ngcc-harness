#!/usr/bin/env python3
"""Check the frozen TRINE invariant-attack estimate, not the attack itself."""

import math
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SETS = (
    ("128", 22, 4093, 145),
    ("256", 40, 4093, 256),
    ("512", 81, 4093, 512),
)


def check_source(level: str, n: int, q: int) -> None:
    for variant in ("balanced", "ShortSig"):
        source = ROOT / "Implementations" / "Reference_Implementation" / f"TRINE-{level}-{variant}" / "params.h"
        data = source.read_text()
        selected = {"128": 1, "256": 2, "512": 3}[level]
        if variant == "ShortSig":
            selected += 3
        assert re.search(rf"^#define PARAMS {selected}$", data, re.MULTILINE), source
        branch = re.search(rf"^#elif PARAMS == {selected}\n(.*?)(?=^#elif |^#else|^#endif)", data, re.MULTILINE | re.DOTALL)
        if selected == 1:
            branch = re.search(r"^#if PARAMS == 1\n(.*?)(?=^#elif )", data, re.MULTILINE | re.DOTALL)
        assert branch is not None, source
        assert re.search(rf"^#define TRINE_n {n}$", branch.group(1), re.MULTILINE), source
        assert re.search(rf"^#define TRINE_q {q}$", branch.group(1), re.MULTILINE), source


def estimate(n: int, q: int) -> tuple[float, float]:
    # §5.3 and §6.2: O(q^((n-2)/2) n^4 (log q)^2).
    # Take log q in base two, as for the displayed security-bit estimates.
    list_bits = (n - 2) / 2 * math.log2(q)
    work_bits = list_bits + 4 * math.log2(n) + 2 * math.log2(math.log2(q))
    return list_bits, work_bits


def main() -> None:
    for level, n, q, printed in SETS:
        check_source(level, n, q)
        list_bits, work_bits = estimate(n, q)
        print(f"TRINE-{level}: list={list_bits:.3f}, formula={work_bits:.3f}, Table 8={printed}")
        if level != "512":
            assert abs(round(work_bits) - printed) <= 1
        else:
            assert 5 < printed - work_bits < 6
            assert (n - 2) / 2 == 39.5
            # Replacing the legitimate half exponent by 40 explains the table.
            assert abs((work_bits + math.log2(q) / 2) - printed) < 1
    print("LEAD sign-30-4: Table 8 uses about 5.5 more bits than its formula")
    print("ATTACK NOT RUN: the q-free point-sampling heuristic and big-O constant are untested")


if __name__ == "__main__":
    main()
