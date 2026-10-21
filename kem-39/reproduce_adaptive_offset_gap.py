#!/usr/bin/env python3
"""Check Weaver's fixed-offset versus adaptive-message proof quantifiers."""

from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
spec = subprocess.run(
    ["pdftotext", "-layout", str(HERE / "kem-39-spec.pdf"), "-"],
    check=True, capture_output=True, text=True,
).stdout
flat = " ".join(spec.split())
for anchor in (
    "Definition 4.2 (Decisional Shifted-MLWR Assumption)",
    "∆ ∈ Rℓq be a public offset",
    "∆ is public and fixed",
    "Theorem 4.5 (IND-CPA Security of Weaver PKE)",
    "Game 3 (Replace the Challenge Ciphertext via Shifted-MLWR)",
    "Corollary A.9 (Ciphertext specialization)",
    "for every fixed message",
    "For each fixed message",
):
    assert anchor in flat, f"missing frozen-spec anchor: {anchor}"

# Logical counterexample to exchanging quantifiers, not a model of Weaver.
# For each predetermined offset, real and random differ on just one of N
# uniformly sampled public matrices. Choosing the offset after seeing that
# matrix separates them every time.
n = 256
fixed_advantages = [sum(int(offset == public) for public in range(n)) / n
                    for offset in range(n)]
adaptive_advantage = sum(int(public == public) for public in range(n)) / n
assert max(fixed_advantages) == 1 / n
assert adaptive_advantage == 1
print(f"illustrative_fixed_advantage={max(fixed_advantages):.6f}")
print(f"illustrative_adaptive_advantage={adaptive_advantage:.0f}")
print("LIMITATION: logical quantifier example only; no Weaver distinguisher")
print("PROOF GAP kem-39-6 CONFIRMED: fixed-offset assumption does not directly cover adaptive challenge messages")
