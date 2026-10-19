# UOV cost-model checks

`pxl_verify.py` independently implements the Polynomial XL formulas and
Hilbert-series conventions from Furue--Kudo, ePrint 2021/1609 v2. It first
checks its outputs against every relevant cell of the paper's Table 1, then
evaluates the Thomae--Wolf-reduced On-Ramp Round 3 UOV systems and prints field-operation,
gate-conversion, and dense-memory exponents.

`pxl_sensitivity.py` imports that implementation and records the uov-Ip
sensitivity to the matrix-multiplication exponent, omitted constants, and a
lower estimate for the paper's omitted first cost term.

Run:

```sh
python3 -I UOV/analysis/pxl_verify.py
python3 -I UOV/analysis/pxl_sensitivity.py
```

These are cost/model checks, not attack executions. The PXL estimates use
heuristic semi-regularity and dense linear algebra with very large memory.
Their durable interpretation and focused reproducer are in
[`UOV-2.md`](../UOV-2.md). The wider review is recorded in
[`security/20261018-design-survey-review.md`](../../security/20261018-design-survey-review.md).

`row_selection_toy.sage` tests the partial divided-power Hilbert-series
assumption used by the sparse-row UOV-4 lead. Its default GF(256)
`(v,a,m)=(4,10,1)` instance is a counterexample to that formula as a general
identity: it predicts a one-dimensional kernel after divided squares, while
three planted-UOV trials leave dimension 42. This shape has more oil than
vinegar and only one form, unlike uov-V. Run:

```sh
sage UOV/analysis/row_selection_toy.sage
```

`row_selection_verifier.sage` independently computes plain and divided-square
ranks and tests selected square submatrices. Its `inter` mode constructs the
intersection transform, then replaces the transformed family with the
specified number of random linear combinations before projection. For example:

```sh
sage UOV/analysis/row_selection_verifier.sage inter 8 20261018 1 11 9 5 12
```

That example may take several minutes. It tests only a subset of the
transformed form family at degree five, not the full uov-V construction at
degree 27. `planted` and `generic` modes accept `(v,a,m)` and `(n,a,m)`,
respectively, after the field exponent, seed and trial count.

Neither toy refutes Merz--Ran's full divided-power-tower conjecture. Together
they show positive evidence for small, favorable shapes and counterexamples
to a universal partial-tower rule. Full-scale rank and row-selection costs
remain untested.
