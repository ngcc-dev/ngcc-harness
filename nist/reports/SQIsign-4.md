# SQIsign-4 — Low: validated free-memory attack model falls below category targets

Severity: Low
Status: Lead
Layer: Design
Affected: On-Ramp Round 3 p324_3, p500_27, and p664_17 concrete-security claims
Discovery: Hard
Exploitation: Validated finite-size cost model with exponential memory; no full-scale attack at the claimed cost
Remediation: Architectural
Credit: Original attack by Benjamin Wesolowski; conjugate-isogeny lattice analysis by Yves Aubry, Roger Oyono, and Christelle Vincent; unconditional asymptotic analyses by Yi Ouyang, Wei Wei, Zheng Xu, and Jose Luis Delgado; finite-size validation and On-Ramp evaluation by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

SQIsign's pinned [specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/sqisign_specification.pdf) incorporates the
`p^(1/3+o(1))` supersingular endomorphism-ring attack. Section 1.1 (physical
pp. 4--5) calls its concrete implications uncertain, notes its exponential
memory, and describes the parameters as a middle-of-the-road choice. Section
8.2.1.2 (physical pp. 108--109) caps the attack at `2^80`, `2^120`, and
`2^160` stored j-invariants.

## Validated finite-size model

The new review validates the attack's two main finite-size ingredients.
Isogenies from `E` to its Frobenius conjugate of degree below `p/4` lie in a
rank-three lattice whose degree-form determinant is `p/4`. The expected count
through degree `D` is

```text
(4*pi/(3*zeta(3))) * D^(3/2) / sqrt(p)
  = 3.484685... * D^(3/2) / sqrt(p).
```

Exact Sage experiments at all three submitted primes match this count within
about five percent at the well-populated radii. They also reproduce the
`(p/2)^(1/3)` upper bound from [Aubry, Oyono and
Vincent](https://arxiv.org/abs/2607.14624); the measured median shortest
degree is 0.39--0.44 times that bound.

The success model counts every way that a short degree can split between the
two smooth-degree tables, including the measured multiplication-table loss.
Across the nonzero p324_3 and p500_27 test points, its predicted success
probability agrees with exact conjugate-isogeny samples within about 0.5 bit,
with mixed sign. Small real-isogeny toy runs give similar agreement. These
tests cover the free-memory operating region. At the submitted memory caps,
the relevant maximum degrees are about 28, 47 and 62 bits below the natural
lattice radius and probe rare tails beyond the sampled range. The capped
results therefore retain that extrapolation. The p664_17 end-to-end validation
timed out, adding a separate Level-V gap.

## Concrete costs

The table gives base-2 work exponents. “Specification” charges each generated
table entry `log2(B)^2 log2(p)^2` bit operations, as in Remark 8.2.1.
“One Fp2 multiplication” charges the explicit circuit cost of one field
multiplication per entry; it is a sensitivity convention, not a lower bound on
full entry generation. `log2(p)^2` is a still more optimistic per-entry
sensitivity. Ranges show the result without and with charged modular
polynomial precomputation.

| Resource convention | p324_3 | p500_27 | p664_17 |
|---|---:|---:|---:|
| Specification cost, at submitted memory cap | 150.69 | 222.90 | 286.32 |
| One Fp2 multiplication, at cap | 143.28--143.47 | 214.10--214.26 | 276.45--276.80 |
| `log2(p)^2` per entry, at cap | **140.20--140.48** | 211.28--211.48 | 273.78--274.17 |
| Specification cost, free memory | **137.50** | **199.98--199.99** | **255.84--256.17** |
| Free-memory table entries | `2^109.90` | `2^169.88` | `2^223.80` |
| Charge `w^(1/3)` per entry | 145.72--146.76 | 225.40--226.52 | 298.13--299.07 |
| NIST category target | 143 | 207 | 272 |

The free-memory shortfall is therefore 5.50, 7.02 and 15.83--16.16 bits. It
is now supported by lattice and success-probability experiments rather than a
direct `p/3` exponent substitution.

The Level-I result at the submitted cap depends on entry generation cost. The
attack crosses the 143-bit target if an entry costs only `log2(p)^2` bit
operations, while charging one Fp2 multiplication leaves 0.28--0.47 bit and
the specification's Cantor--Zassenhaus analysis leaves 7.69 bits. At the
optimized Level-I point, reaching the category boundary requires roughly
`2^19.4` bit operations per entry. Known root-finding and field arithmetic are
more expensive; the specification uses approximately `2^27.3`. This is a
sensitivity boundary, not an implemented below-target attack.

The specification says its more detailed analysis costs at least
`3*sqrt(p/w)` generated j-invariants before deliberately using the weaker
`sqrt(p/w)` expression. The experiments support the square-root scaling but
measure constants around 1.4--2.4 in the tested region rather than reproducing
the factor three. They do not reach the rare-tail cap regime and therefore do
not disprove the specification's stronger constant. The optimized table above
uses the measured success model and records this extrapolation explicitly.

## What changed from the earlier report

Wesolowski's rough §4.1 calculation is not a lower bound on attack work. Once
the lattice density and all smooth-degree splits are included, its free-memory
`log2(p)^2` convention gives about 126--128, 187--189 and 243--244 bits, not
the earlier 152, 221 and 283-bit conversion. It cannot be used as an
above-target defence. Memory cost is the remaining reason that the validated
time estimates do not establish a NIST-category failure.

Ouyang, Wei and Xu [prove the relevant smoothness law
asymptotically](https://arxiv.org/abs/2610.21488), and Delgado gives an
[independent unconditional Las Vegas
algorithm](https://arxiv.org/abs/2609.22018). These remove the original
asymptotic heuristic while retaining `p^(1/3+o(1))` memory. Mamah's [concrete
analysis](https://eprint.iacr.org/2026/1821) concerns earlier SQIsign primes;
its table cannot simply be relabelled with the On-Ramp parameters.

Van Oorschot--Wiener collision search is at least 16 bits slower in the
reviewed model. Pollard rho, dissection and asymmetric degree sets give no
improvement. The optimistic quantum attacks also remain above their NIST
depth-limited benchmarks.

## Classification and reproduction

This remains **Low / Lead**. The finite-size free-memory attack model is now
well supported, but all three attacks require roughly `2^110` to `2^224`
table entries. NIST permits category comparisons to charge access to enormous
memory, and the illustrative `w^(1/3)` sensitivity charge puts every set above
target. It is not a prescribed NIST metric. No full-scale attack was executed,
the capped rare-tail extrapolation is unvalidated, and the Level-V success
check remains open.
An accepted below-target area-time model or practical entry-generation method
would justify reassessing the severity.

Run the portable check with:

```sh
python3 SQIsign/reproduce_sqisign_4.py
```

It hash-checks the retained compact evidence and verifies the stated capped,
free-memory and memory-charged comparisons. The full Sage, PARI and numerical
tooling and regeneration instructions are published in
[ngcc-harness](https://github.com/ngcc-dev/ngcc-harness) under
`nist/SQIsign/analysis/p13/`. These are resource-model experiments, not an
implementation of the full-scale attack.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-4
```
