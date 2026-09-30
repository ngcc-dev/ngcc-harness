#!/usr/bin/env python3
"""Locate the two TRIKE proof gaps in the submitted specification."""

from pathlib import Path
import re
import subprocess


HERE = Path(__file__).resolve().parent
result = subprocess.run(
    ["pdftotext", "-layout", str(HERE / "kem-36-spec.pdf"), "-"],
    check=True,
    stdout=subprocess.PIPE,
    text=True,
)
text = result.stdout

# Algorithm 4 publishes (sigma,r2), but Theorem 4 assumes the hidden parity
# checks themselves are the public key and evaluates the private predicate.
assert re.search(r"pk\s*←\s*\(σ,\s*r2\s*\)", text)
assert "public key pk = (h0 , h1 , h2 )" in text
assert "determines whether the key passes the weak key test" in text

# The adjacent, unfiltered Type-II/III boundary classes receive only the cited
# zero-failure experiments, not a target-level analytic DFR bound.
boundary = "boundary keys that remain unfiltered (where m = m0 − 1)"
assert text.count(boundary) >= 2
assert text.count("zero decoding failures observed") >= 2
assert text.count("2−21.67 and 2−18.35 at a 95% confidence level") >= 2

print("TRIKE_BOUNDARY_DFR_EVIDENCE_GAP_CONFIRMED")
print("TRIKE_WEAK_KEY_REDUCTION_USES_SECRET_AS_PUBLIC_CONFIRMED")
