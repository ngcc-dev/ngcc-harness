#!/usr/bin/env python3
"""Source and arithmetic certificate for Lynxer's quantum TCCR proof term."""

from __future__ import annotations

import math
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations" / "Reference_Implementation"
BLOCK_BITS = {160: 128, 256: 128, 384: 256, 512: 256}
TARGET_BITS = {160: 80, 256: 128, 384: 192, 512: 256}


def parameter(source: str, name: str) -> int:
    match = re.search(rf"^#define {re.escape(name)} ([0-9]+)$", source, re.M)
    assert match, name
    return int(match.group(1))


def main() -> None:
    for level in (160, 256, 384, 512):
        for profile in "sf":
            tag = f"LYNXER_{level}{profile.upper()}"
            directory = REF / f"Lynxer-{level}{profile}"
            parameters = (directory / "parameters.h").read_text()
            lam = parameter(parameters, tag + "_CSP")
            tau = parameter(parameters, tag + "_TAU")
            grind = parameter(parameters, tag + "_POW_LEVEL")
            k = (lam - grind) // tau + 1
            tau1 = (lam - grind) % tau
            length = tau1 * (1 << k) + (tau - tau1) * (1 << (k - 1))

            tccr = (directory / "tccr.c").read_text()
            enc = (directory / "enc.c").read_text()
            assert "const size_t x_l_bytes   = block_bytes - 1;" in tccr
            assert "for (size_t i = 0; i != 2; ++i)" in tccr
            assert f"case {lam}: return {BLOCK_BITS[level]} / 8;" in enc

            d = (2 * math.ceil(math.log2(length)) + 5) * tau
            error_bits = lam - BLOCK_BITS[level] - math.log2(2 * d)
            assert error_bits < TARGET_BITS[level]
            print(
                f"Lynxer-{level}{profile}: L={length} tau={tau} D={d} "
                f"quantum_target={TARGET_BITS[level]} "
                f"one_signature_proof_error_bits={error_bits:.2f}"
            )
    print("PROOF GAP sign-14-4 CONFIRMED: the quantum TCCR term misses every target")


if __name__ == "__main__":
    main()
