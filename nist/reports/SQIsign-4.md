# SQIsign-4 — Low: free-memory attack extrapolation falls below category targets

Severity: Low
Status: Lead
Layer: Design
Affected: On-Ramp Round 3 p324_3, p500_27, and p664_17 concrete-security claims
Discovery: Moderate
Exploitation: Model-dependent projected margin; no end-to-end attack at the claimed cost
Remediation: Architectural
Credit: Original attack by Benjamin Wesolowski; independent parameter extrapolation and cost-model reassessment by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

SQIsign's pinned [specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/sqisign_specification.pdf) incorporates the new
`p^(1/3+o(1))` supersingular endomorphism-ring attack.  Section 1.1 (physical
pp. 4–5) calls its concrete implications uncertain, notes its exponential
memory, and describes the parameters as a middle-of-the-road choice. The
submitters say they are optimistic that the parameters meet or surpass NIST's
targets, barring improvements such as a memory-free `p^(1/3+o(1))` attack;
they do not say that the current parameters would fail. Section 8.2.1.2
(physical pp. 108–109) uses fixed memory caps of `2^80`, `2^120`, and `2^160`
stored j-invariants.

## The model-dependent shortfall

The specification models a memory-`w` attack as generating at least
`sqrt(p/w)` j-invariants and assigns each generated value `2^c_j` binary
operations, where `c_j` is 27.3, 29.8, or 31.4.  At the submitted caps, using
the exact On-Ramp Round 3 primes gives 150.09, 222.18, and 285.44 bits of work.  These
are above NIST's 143-, 207-, and 272-gate category thresholds and agree with
the specification's rounded 149.8, 221.8, and 285.4 figures.

If the same lower-bound formula is extrapolated to `w = p^(1/3)` and memory is
free, its time exponent becomes `log2(p)/3 + c_j`:

| parameter | free-memory work | category target | shortfall | stored entries |
|---|---:|---:|---:|---:|
| p324_3 | 135.83 | 143 | 7.17 | `2^108.53` |
| p500_27 | 198.05 | 207 | 8.95 | `2^168.25` |
| p664_17 | 254.10 | 272 | 17.90 | `2^222.70` |

This is the concrete-margin lead.  It is an uncapped, free-random-access-memory
extrapolation of a bound that the specification states only for its considered
parameter range.  The listed memory is in stored field elements, not bits.

## Memory and cost-model caveats

The conclusion reverses when memory access is charged.  As an explicit
sensitivity test, charging `w^(1/3)` binary operations per generated
j-invariant raises the optimized exponents to 144.71, 224.34, and 296.94,
above all three targets.  NIST's call §4.B.3 (physical p. 16) permits category
comparisons to account for the cost of accessing extremely large memory.  The
specification's hard cap is its own concrete convention rather than a NIST
metric, but it also yields positive margins.

Wesolowski's own rough Section 4.1 model in
[ePrint 2026/1486](https://eprint.iacr.org/2026/1486) supplies a second caveat.
Re-evaluated at the exact On-Ramp Round 3 primes, it gives approximately `2^135.6`,
`2^203.4`, and `2^264.1` Fp2 operations using `2^117.3`, `2^178.9`, and
`2^234.7` entries.  Charging `log2(p)^2` bit operations per Fp2 operation raises
the work to 152.3, 221.4, and 282.9 bits.  This is another explicit cost-model
choice rather than a lower bound on an Fp2 operation, but under it the concrete
model does not demonstrate a category shortfall.  The reproducer calibrates its
implementation of the model to within 2.3 bits of the paper's printed examples.

The paper also makes clear that its asymptotic result is conditional on a
smoothness heuristic, carries a superpolynomial overhead hidden in the
`o(1)` exponent, and needs high memory. Its abstract says that the impact on
concrete parameters remains to be clarified. The table above is therefore a
cost-model extrapolation, not a consequence that the paper claims for these
three parameter sets.

## Classification and limits

This is Low / Lead because the free-memory arithmetic is below every target,
but no accepted resource accounting establishes an attack below a claimed
category.  No attack implementation was run at scale, the hidden `o(1)` cost
is material, and both alternative cost models above retain positive margins:
one charges memory access, while the other charges bit operations for each Fp2
operation. The specification itself acknowledges this uncertainty. Parameter
growth or a better-supported time-memory-area analysis would resolve the lead.

Recompute the models with:

```sh
python3 SQIsign/reproduce_sqisign_4.py
```

The script checks the pinned specification, exact primes, capped and uncapped
specification formulas, a charged-memory sensitivity model, and the retained
Wesolowski Section 4.1 approximation.  It is a deterministic cost-model check,
not a cryptanalytic attack.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-4
```
