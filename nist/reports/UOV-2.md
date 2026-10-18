# UOV-2 — Low: Polynomial XL places uov-Ip near the Category-1 boundary

Severity: Low
Status: Lead
Layer: Design
Affected: uov-Ip in classic, PKC and PKC+SKC formats
Discovery: Non-trivial
Exploitation: Cost/model check for a public-key-only direct forgery; no attack execution
Remediation: Moderate
Credit: Polynomial XL analysis by Hiroki Furue and Momonari Kudo; independent uov-Ip application and cost-model reassessment by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

## Result

The Round-3 specification prices the uov-Ip direct forgery at about `2^146`
binary gates using hybrid Wiedemann-XL on the Thomae–Wolf-reduced system
(§4.2, physical p.24). It does not analyze Furue and Kudo's Polynomial XL
(PXL), whose elimination is performed over a polynomial ring before guessing
the remaining variables.

Applying PXL equation (4.8) with the paper's Hilbert-series conventions to the
reduced system of 44 equations in 44 variables over GF(256) gives:

| Matrix exponent | Field operations | Spec gate conversion | Dense-memory upper bound |
|---:|---:|---:|---:|
| 2.81 | `2^132.9` | `2^140.0` | about `2^110.6` bits |
| 3.00 | `2^137.2` | `2^144.3` | about `2^110.6` bits |

The specification converts a field operation to `2r^2+r` gates for
`r=log2(q)`. NIST's Category-1 classical gate reference is about `2^143`.
Thus the estimate is roughly three bits below the reference only when a matrix
exponent near 2.81 applies to the very large dense elimination with omitted
constants. At cubic elimination it remains above the reference.

## Evidence and limitations

The public harness implementation of the Furue–Kudo formulas first reproduces the
paper's published Table-1 values, then evaluates uov-Ip. The focused numbered
witness is:

```sh
python3 UOV/reproduce_uov_2.py
```

For the complete calibration and sensitivity sweep, run:

```sh
python3 -I UOV/analysis/pxl_verify.py
python3 -I UOV/analysis/pxl_sensitivity.py
```

These are deterministic formula checks, not forgery executions. The PXL paper
reports asymptotic costs with dropped constants and notes that its experimental
Fix and Linearize phase did not follow the estimate at much smaller dimensions.
Its stated dense-memory use is also extreme. NIST considers the cost of
accessing very large memory, while the submission's MEM-2D calculation covers
its sparse PWXL schedules rather than dense PXL. The estimate therefore remains
a Lead and is rated Low.

The appropriate follow-up is an attack-specific memory/access model and an
implementation study at the reduced uov-Ip dimensions. If those confirm a cost
below Category 1, the parameter set needs a larger margin.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-2
```
