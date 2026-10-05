#!/usr/bin/env python3
"""Check Lynxer's shared-tweak and fixed-leaf target-count bounds."""

from __future__ import annotations

import math
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations/Reference_Implementation"


def number(source: str, name: str) -> int:
    match = re.search(rf"^#define {re.escape(name)} ([0-9]+)$", source, re.M)
    assert match, name
    return int(match.group(1))


def main() -> None:
    for level in (160, 256, 384, 512):
        for profile in "sf":
            directory = ROOT / f"Lynxer-{level}{profile}"
            tag = f"LYNXER_{level}{profile.upper()}"
            parameters = (directory / "parameters.h").read_text()
            bavc = (directory / "bavc.c").read_text()
            tau = number(parameters, tag + "_TAU")
            t_open = number(parameters, tag + "_T_OPEN")
            assert "prg_2_lambda(key, iv, 0, com, lambda);" in bavc
            assert "for (unsigned int i = 0, offset = 0; i < params->tau; ++i)" in bavc
            assert "params->T_open * lambda_bytes" in bavc
            leaf_bound = level - math.log2(tau)
            tree_threshold = level - math.log2(t_open + 1)
            target = 128 if level == 160 else level
            assert (leaf_bound < target) == (level != 160)
            assert (tree_threshold < target) == (level != 160)
            print(
                f"Lynxer-{level}{profile}: tau={tau} T_open={t_open} "
                f"leaf_log2_work={leaf_bound:.2f} "
                f"tree_threshold_log2_work={tree_threshold:.2f}"
            )
    print("CONFIRMED sign-14-2/sign-14-3: target-count certificates")


if __name__ == "__main__":
    main()
