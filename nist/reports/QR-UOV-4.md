# QR-UOV-4 — Low: zero-guess Hashimoto–PXL composition conditionally falls below two targets

**Severity:** Low
**Status:** Lead
**Layer:** Design
**Affected:** On-Ramp Round 3 1q127L10 and 5q127L10, with either public-expansion backend, under the submission's omega=2.37 free-memory PXL convention
**Discovery:** Hard
**Exploitation:** Conditional public-key direct-forgery cost/model path; structured toy forgeries but no PXL or full-scale attack execution
**Remediation:** Moderate
**Credit:** Underdetermined reduction by Yasufumi Hashimoto and geometric formulation by Massimo Ostuzzi; Polynomial XL by Hiroki Furue and Momonari Kudo; zero-guess composition and On-Ramp evaluation by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Date:** 2026-10-18

## Result

QR-UOV specification §7.2, physical pp.42–43, estimates Polynomial XL
(PXL) after the Thomae–Wolf reduction. It says that PXL is difficult to
combine with the stronger partition-based underdetermined solvers because
both guess variables before substituting their values. That explanation does
not cover Hashimoto's zero-guess boundary.

In [Ostuzzi's geometric formulation](https://eprint.iacr.org/2026/1122),
[Hashimoto](https://eprint.iacr.org/2021/1045) extraction with `k_H=0` and
partition parameter `b` leaves a square `MQ(m-b-1,m-b-1)` residual. No
variable value has been guessed at this point, so PXL can make its own
symbolic choice of variables on the residual. Substitution in Ostuzzi's
equation (7), the original Hashimoto condition, and [Asanuma et al.'s
restatement](https://eprint.iacr.org/2026/1054) gives:

| Parameter set | Original `(n,m)` | `b` | Hashimoto residual | Thomae–Wolf residual |
|---|---:|---:|---:|---:|
| 1q127L10 | (600,60) | 10 | 49 | 51 |
| 5q127L10 | (1150,110) | 9 | 100 | 101 |
| 3q127L3 | (306,78) | 2 | 75 | 76 |
| 5q31L10 | (1240,120) | 9 | 110 | 111 |

Applying the specification's affine Hilbert-series formula, its gate
conversion, and the [PXL paper's](https://eprint.iacr.org/2021/1609)
asymptotic `omega=2.37` matrix exponent gives:

| Parameter set | Recomputed TW→PXL | Hashimoto(0)→PXL | Target | Margin |
|---|---:|---:|---:|---:|
| 1q127L10 | 148.64 | **141.95** | 143 | **-1.05** |
| 5q127L10 | 272.98 | **270.05** | 272 | **-1.95** |
| 3q127L3 | 210.58 | 208.74 | 207 | +1.74 |
| 5q31L10 | 275.45 | 273.90 | 272 | +1.90 |

The centrifugation and initial square-system costs are below 49 bits for the
two affected sets in the same model and do not change the displayed totals.
The result corrects the submission's chosen estimator: under that estimator,
the stated reason for using only Thomae–Wolf before PXL misses two
below-target rows.

Those figures price one residual solve. If each conditioned square residual
has the independent random-system success probability `1-e^-1`, repeated
reductions add about `log2(1/(1-e^-1)) = 0.66` bit: approximately **142.61**
and **270.71** bits. This is a heuristic sensitivity, not a measured
full-size success rate. The ten small structured toys needed an average of
2.4 target attempts; they do not establish that unsuccessful full-size
residuals can be screened cheaply before PXL. At Level I the restart-adjusted
margin is only about 0.39 bit, so small omitted costs could erase it.

## Why this remains a lead

The figures are not practical finite-size attack costs. They depend on three
material assumptions:

1. `omega=2.37` is an asymptotic existence bound. Neither the PXL paper, the
   submission, nor this study supplies an algorithm or crossover showing that
   it applies to these dense matrices at dimensions 49 and 100. Replacing it
   by the PXL paper's `omega=2.81` sensitivity gives 148.24 and 288.33 bits,
   both above target.
2. The modeled PXL storage is about `2^110.27` and `2^211.15` field elements,
   or `2^113.08` and `2^213.96` bits. A specific logarithmic memory-access
   charge gives about 148.73 and 277.77 bits. This is a sensitivity model;
   the size of a defensible access charge has not been fixed by the toy runs.
3. PXL assumes suitable semi-regular behavior. Five retained Sage trials
   validate zero-guess centrifugation and a lifted forgery for random
   quadratic maps over GF(31). A separate dependency-free toy constructs the
   submitted q=31, ell=3 quotient-ring public-map distribution at reduced
   dimensions `(V,M)=(3,2)`. Seeds 20261018 through 20261027 all produced the
   predicted 4-by-4 residual, Hilbert function `[1,4,6,4,1,0]`, and a valid
   lifted forgery. This is useful structured evidence, but it does not
   establish semi-regularity at dimensions 49 or 100 and uses brute-force
   residual solving rather than PXL. The specification's Table 13 experiments
   likewise concern the original QR-MQ system rather than the conditioned
   residual.

The alternative CryptographicEstimators implementation gives lower numbers
under a different homogeneous convention. It is a sensitivity check, not
independent confirmation of the specification's affine calculation.

This is therefore **Low / Lead** under the project's security-margin rule.
It is not a demonstrated category break or full-scale forgery. If full-size
structured-residual tests and a realizable finite-size cost establish a
below-target path, the direct EUF-CMA consequence would require a
substantially higher severity assessment.

## Other parameter observations

Recalculation of the submission and Round-2 tables shows that the previously
reported thinnest published QR-UOV margin is 5q127L10 at 273 bits, one bit
above its 272-bit target. The q=7 and q=31 sets remain above their targets
under the specification's formula; another estimator puts 5q31L10 at 271.9
under a different convention, so that value is not presented as a separate
attack.

The characteristic-two divided-square row-selection mechanism studied for
UOV does not transfer directly to odd-characteristic QR-UOV. Higher
characteristic-p divided-power methods exist, but no sparse-row gain for the
submitted QR-UOV sets was established here.

## Reproduction

```sh
python3 QR-UOV/reproduce_qruov_4.py
```

The dependency-free script checks the two Hashimoto applicability
inequalities, residual dimensions, specification-formula PXL optima, omitted
preprocessing cost, memory units, and `omega=2.81` sensitivity. It is a
deterministic cost/model check, not PXL execution.

The optional mechanism tests are:

```sh
python3 QR-UOV/analysis/hashimoto_pxl/structured_toy.py --trials 10
sage QR-UOV/analysis/hashimoto_pxl/toy_hashimoto.sage
```

The first command constructs small quotient-ring QR-UOV maps without external
packages. The second uses random MQ maps. Neither runs PXL or validates the
full-size cost.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py QR-UOV-4
```
