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
