#!/usr/bin/env python3
"""Check sign-31-7: the TSUOV-128 salt parameter against its stated proof loss."""

from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent
SOURCE = (
    "Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/"
    "API_PKC/Implementations/Reference_Implementation/TSUOV_128/tsuov_params.h"
)
OPT_SOURCE = SOURCE.replace("x86-Reference_Implementation", "x86-Performance_Optimized_Implementation")

for relative in (SOURCE, OPT_SOURCE):
    params = (ROOT / relative).read_text()
    assert "#define TSUOV_SEED_LEN 16" in params, relative
    assert "#define TSUOV_SALT_LEN TSUOV_SEED_LEN" in params, relative
    print(f"PASS {relative}: 128-bit salt")

pdf_text = subprocess.run(
    ["pdftotext", "-layout", str(ROOT / "sign-31-spec.pdf"), "-"],
    capture_output=True, text=True, check=True,
).stdout
for label in ("Lemma 6.10.", "Theorem 6.12."):
    start = pdf_text.index(label)
    excerpt = pdf_text[start:start + 1700]
    assert "Qh +" in excerpt and "−λ" in excerpt, label
    print(f"PASS {label.rstrip('.')}: salt collision term found")

salt_bits = 128
trials = 1 << 64
term = trials * trials / (1 << salt_bits)  # Q_h = 0
assert term == 1
print("Bound at Q_h=0, Q'_s=2^64: 1 (vacuous); no forgery implied")
print("PROOF GAP sign-31-7 CONFIRMED: submitted salt-collision bound is vacuous at 2^64 trials")
