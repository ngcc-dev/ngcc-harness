#!/usr/bin/env python3
"""Certify the nonzero one-column MSIS witness in COMPASS-SIG Theorem 2."""

import math
from pathlib import Path
import shutil
import subprocess
import sys


spec = Path(__file__).resolve().with_name("sign-06-spec.pdf")
if shutil.which("pdftotext") is None:
    print("SKIP sign-06-7: pdftotext is unavailable")
    sys.exit(77)

text = " ".join(subprocess.check_output(["pdftotext", "-layout", str(spec), "-"], text=True).split())
checks = (
    "Definition 2 (MSIS)" in text,
    "Theorem 2." in text,
    "γ ′ := γ2 − ηe · τ" in text,
    "AdvSUF-CMA" in text,
    "AdvMSIS" in text,
    "Algorithm 3: Verify" in text,
)
if not all(checks):
    raise SystemExit("NOT REPRODUCED sign-06-7: proof or verifier premise missing")

# Table 1: level, q, n, k, l, eta_s, eta_e, d, gamma1, gamma2, tau, B.
rows = (
    (128, 2081281, 256, 3, 4, 1, 1, 4, 215, 130080, 30, 560),
    (256, 2081281, 256, 7, 7, 1, 1, 5, 217, 520320, 60, 1120),
    (384, 8380417, 512, 5, 6, 1, 1, 5, 218, 1047552, 78, 1760),
    (512, 8380417, 512, 7, 7, 1, 1, 5, 219, 2095104, 120, 2240),
)
for row in rows:
    if " ".join(map(str, row)) not in text:
        raise SystemExit(f"NOT REPRODUCED sign-06-7: Table 1 row {row[0]} differs")

for level, q, n, k, _l, _eta_s, eta_e, _d, _gamma1, gamma2, tau, _B in rows:
    residual_bound = 2 * (gamma2 - eta_e * tau)
    centered_max = (q - 1) // 2
    if residual_bound > centered_max:
        raise SystemExit(f"NOT REPRODUCED sign-06-7: residual bound out of range at {level}")
    # z=(1,0,...,0) is nonzero. For uniformly random A, its first column
    # has k*n independent uniform coefficients, each fitting in [-B,B]
    # with probability (2B+1)/q.
    log2_probability = k * n * math.log2((2 * residual_bound + 1) / q)
    if level in (256, 512):
        probability = 2**log2_probability
        if not (0.81 < probability < 0.82 and 2 * gamma2 == centered_max):
            raise SystemExit(f"NOT REPRODUCED sign-06-7: affected-set arithmetic differs at {level}")
        print(f"COMPASS-SIG-{level}: one-column success {probability:.6f}; 2γ2 covers every centered residue")
    else:
        if log2_probability >= -128:
            raise SystemExit(f"NOT REPRODUCED sign-06-7: control arithmetic differs at {level}")
        print(f"COMPASS-SIG-{level} [control]: one-column log2 success {log2_probability:.1f}")

print("PROOF GAP sign-06-7 CONFIRMED: the SUF-CMA MSIS premise is easy at 256 and 512; no forgery shown")
