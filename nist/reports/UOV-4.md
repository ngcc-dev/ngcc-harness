# UOV-4 — Low: truncated-ring estimates leave almost no uov-V RAM margin

Severity: Low
Status: Lead
Layer: Design
Affected: uov-V and uov-Ip in classic, PKC and PKC+SKC formats
Discovery: Non-trivial
Exploitation: Cost/model margin observation for public-key key recovery; no attack execution
Remediation: Moderate
Credit: Original truncated-ring analysis by Hiroki Furue and Yasuhiko Ikematsu; independent On-Ramp Round 3 recomputation by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

## Result

Re-evaluating the specification's §4.6 truncated-polynomial-ring and
Furue–Ikematsu intersection formulas gives the following minimum RAM-gate
costs:

| Parameter set | Recomputed cost | Claimed reference | Margin |
|---|---:|---:|---:|
| uov-Ip | 144.75 | 143 | +1.75 |
| uov-Is | 158.55 | 143 | +15.55 |
| uov-III | 213.12 | 207 | +6.12 |
| uov-V | 272.05 | 272 | +0.05 |

The uov-V result is effectively on its claimed boundary under the
submission's primary RAM-gate model; uov-Ip has less than two bits of margin.
Table 5 reports `floor(log2(cost))`: 144, 158, 213 and 272, rather than values
rounded to the nearest integer. Flooring the recomputed values gives exactly
those four entries. The calculation does not place a On-Ramp Round 3 parameter set
below its claim.

The post-submission Schur–Macaulay estimator was separately reviewed with the
authors' full shape search. It gives about 143.9, 158.1, 216.6 and 280.7 for
Ip, Is, III and V, respectively, so it also does not lower a On-Ramp Round 3 set below
its target. An earlier 271.7 figure for uov-V came from a 0.32-bit row-counting
convention applied to plain wedge, rather than the new Schur attack.

## Reproduction and interpretation

```sh
python3 UOV/reproduce_uov_4.py
```

The script implements the specification formulas with exact integer power
series and prints every On-Ramp Round 3 optimum. It is a deterministic cost/model
check, not a key-recovery run. The specification also reports substantial
MEM-2D additions for these attacks, including 48 bits for uov-V, and warns
that the underlying semi-regularity heuristics have counterexamples.

This report records a Low Lead because the candidate-specific reproduced
calculation leaves only 0.05 bit of RAM-model margin for uov-V and 1.75 bits
for uov-Ip, while no current estimate establishes an attack below target under
a complete memory-aware model. The rating follows the methodology's Low / Lead
rule for a reproducible candidate-specific primary-model estimate with less
than two bits of margin and an unresolved modeling choice that could erase it.
Parameter changes would be warranted only after that missing step is resolved.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-4
```
