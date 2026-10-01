#!/usr/bin/env python3
"""Validate the public square-coset constraint in QIMEN-PIKE."""

from __future__ import annotations

import math
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SPEC = ROOT / "kem-31-spec.pdf"
SOURCE = Path("src/pike/ref/pikex_compressed/pike_compressed.c")
TREES = (
    ROOT / "Implementations" / "Implementations",
    ROOT / "Implementations" / "Optimized_Implementation",
)


def normalized_pdf() -> str:
    try:
        output = subprocess.run(
            ["pdftotext", "-layout", str(SPEC), "-"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
    except FileNotFoundError as exc:
        raise SystemExit("pdftotext is required") from exc
    return " ".join(output.split())


def units(modulus: int) -> set[int]:
    return {x for x in range(modulus) if math.gcd(x, modulus) == 1}


def main() -> int:
    spec = normalized_pdf()
    assert "Algorithm 9 QIMEN-PIKE.PKE.KeyGen" in spec
    assert "RA , SA ← [γ]RA" in spec and "[γ]S" in spec
    assert "independent masking scalars with uniformly distributed determinants" in spec
    assert "cannot be recovered via pairings" in spec

    for tree in TREES:
        source = (tree / SOURCE).read_text(encoding="utf-8")
        assert source.count("ibz_random_unit(&gamma, &TORSION_ODD_PLUS, NULL);") == 1
        assert "ibz_crt(&alpha, &alpha, &gamma" in source
        assert "ibz_crt(&beta, &beta, &gamma" in source

    # The exponent of e_C(R_A,S_A), relative to the public base pairing, is
    # deg(phi)*gamma^2.  The following exact enumeration illustrates the
    # general group statement on an odd composite modulus.
    modulus = 3**2 * 5
    group = units(modulus)
    degree = 2  # any unit selects one coset of the square subgroup
    squares = {(g * g) % modulus for g in group}
    shared_mask = {(degree * g * g) % modulus for g in group}
    independent_masks = {
        (degree * g_r * g_s) % modulus for g_r in group for g_s in group
    }

    assert shared_mask == {(degree * square) % modulus for square in squares}
    assert shared_mask < group
    assert independent_masks == group
    assert len(group) // len(shared_mask) == 4  # two distinct odd prime factors

    print(f"toy modulus C={modulus}: |U(C)|={len(group)}, |QR(C)|={len(squares)}")
    print(f"common-mask coset index: {len(group) // len(shared_mask)}")
    print("independent-mask products cover U(C): yes")
    print("ATTACK kem-31-4 QIMEN-PIKE PUBLIC PAIRING CONSTRAINT: CONFIRMED")
    print("LIMITATION: formal-parameter key recovery and cost are not reproduced.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
