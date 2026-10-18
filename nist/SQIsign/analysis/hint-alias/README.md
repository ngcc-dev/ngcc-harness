# SQIsign auxiliary-hint and Montgomery-model alias witness

This optional three-stage witness reproduces the auxiliary-basis-hint and
`Eaux.A -> -Eaux.A` portions of [SQIsign-1](../../SQIsign-1.md). It keeps all
generated signatures and intermediate point data outside the repository.

Requirements are Python 3, the built SQIsign reference libraries, and SageMath.
For one parameter set, run from the repository root:

```sh
work=$(mktemp -d)
python3 -I SQIsign/analysis/hint-alias/hintdump.py \
  SQIsign/lib/p324_3.so p324_3 review-p324 "$work/dump.json"
sage SQIsign/analysis/hint-alias/hintalias.sage \
  "$work/dump.json" "$work/candidates.json"
python3 -I SQIsign/analysis/hint-alias/hintverify.py \
  SQIsign/lib/p324_3.so "$work/dump.json" "$work/candidates.json"
```

Replace `p324_3` with `p500_27` or `p664_17` to exercise another level. The
first stage signs a deterministic message and asks the submitted library to
decode every possible auxiliary hint on the `A` and `-A` models. The Sage stage
lifts the x-only bases, checks full order, derives the basis-change matrices,
and constructs normalized response matrices. The final Python stage sends each
candidate to the unmodified submitted verifier and checks a changed-message
control.

The 2026-10-18 independent review observed 38 and 91 alternative accepted hints
for two p324_3 signatures, 97 for p500_27, and 36 for p664_17. Every hint that
decoded to a full-order basis was accepted after compensating the matrix. The
`-A` model was accepted at all three levels. Counts depend on the generated
curve and are not fixed protocol parameters.
