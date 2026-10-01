#!/usr/bin/env python3
"""Evaluate the Aigis-Sig+ section 7.3 abort bound."""

from math import exp

Q = 4_171_777
SETS = {
    "I": dict(n=512, k=2, ell=2, eta1=1, beta1=24, beta2=120,
              gamma1=2**14, gamma2=(Q - 1) / 12, table_repetitions=6.41),
    "II": dict(n=512, k=4, ell=4, eta1=1, beta1=44, beta2=44,
               gamma1=2**16, gamma2=(Q - 1) / 12, table_repetitions=5.16),
    "III": dict(n=512, k=8, ell=7, eta1=1, beta1=118, beta2=118,
                gamma1=2**19, gamma2=(Q - 1) / 8, table_repetitions=5.71),
}

violations = []
for name, p in SETS.items():
    a = p["n"] * p["ell"] * p["beta1"] / p["gamma1"]
    b = p["n"] * p["k"] * (p["beta2"] + p["eta1"]) / p["gamma2"]
    pbar = (1 - exp(-a)) + (1 - exp(-b))
    # Section 3.4's repetition estimate in product form, before its exponential
    # approximation; it rests on the section's heuristic assumptions and is not
    # a uniform bound over every secret key.
    estimated_repetitions = ((1 - p["beta1"] / p["gamma1"]) ** (-p["n"] * p["ell"]) *
                             (1 - (p["beta2"] + p["eta1"]) / p["gamma2"]) ** (-p["n"] * p["k"]))
    heuristic_abort = 1 - exp(-(a + b))
    # The estimate rounds to Table 2.
    assert abs(estimated_repetitions - p["table_repetitions"]) < 0.005
    assert 0 < heuristic_abort < 1
    if not 0 < pbar < 1:
        violations.append(name)
    print(
        f"set={name} pbar={pbar:.9f} estimated_repetitions={estimated_repetitions:.6f} "
        f"heuristic_abort={heuristic_abort:.9f}"
    )

assert violations == ["I", "III"]
print("CONFIRMED sign-01-6: printed pbar violates 0 < pbar < 1 for sets I and III; "
      "the theorems hold for any bound below one")
