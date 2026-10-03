#!/usr/bin/env python3
"""Certificate for the COMPASS-KEM Biased-MLWR proof/instance mismatch."""

from math import log2
from pathlib import Path
import re
import subprocess

PDF = Path(__file__).resolve().parent / "kem-11-spec.pdf"

SETS = (
    # level, n, q, k, eta
    (128, 256, 3329, 2, 3),
    (256, 256, 3329, 4, 2),
    (384, 512, 7681, 3, 3),
    (512, 512, 7681, 4, 4),
)


def main():
    result = subprocess.run(["pdftotext", "-layout", str(PDF), "-"], check=True,
                            stdout=subprocess.PIPE, text=True)
    text = " ".join(result.stdout.split())
    required = (
        "Definition 2. (Biased MLWR)",
        "secret term s",
        "s = s1 + s2",
        "s1 is uniformly random",
        "kn log(2γ) − (n + λ) log q",
        "centered binomial distributions with parameters {2, 3, 4}",
        "under biased MLWR assumption, Game 2 and Game 3 are indistinguishable",
    )
    for needle in required:
        assert needle in text, needle

    # Parse the implementation constants and the concrete CBD sampling call.
    root = PDF.parent / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
    for level, n, q, k, eta in SETS:
        source = root / f"COMPASS-KEM-{level}"
        params = (source / "params.h").read_text()
        indcpa = (source / "indcpa.c").read_text()
        for macro, value in (("N", n), ("Q", q), ("K", k), ("ETA1", eta)):
            full = f"COMPASS_KEM_{macro}"
            block = re.search(rf"#(?:if|elif) \(SECURITY_LEVEL == {level}\)(.*?)(?:#elif|#else)",
                              params, re.S).group(1)
            assert re.search(rf"#define\s+{full}\s+{value}\b", block), (level, full)
        assert "poly_getnoise_eta1(sp.vec+i, coins, nonce++);" in indcpa

        # Give the concrete distribution the optimistic entropy of a uniform
        # draw over its entire CBD support.  Actual CBD min-entropy is lower.
        available = k * n * log2(2 * eta + 1)
        required = (2 * n + level) * log2(q) + 2 * level
        shortfall = required - available
        assert shortfall > 6000
        print(f"COMPASS-KEM-{level}: available={available:.3f} "
              f"required={required:.3f} shortfall={shortfall:.3f} bits")

    print("PROOF GAP kem-11-2 CONFIRMED: Game 2 to Game 3 invokes an uninstantiated assumption")


if __name__ == "__main__":
    main()
