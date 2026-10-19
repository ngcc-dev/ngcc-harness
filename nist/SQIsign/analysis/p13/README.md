# SQIsign p^(1/3) concrete-model validation

These scripts support the refined finite-size analysis in
[`SQIsign-4.md`](../../SQIsign-4.md). They validate three parts that the
focused reproducer treats as retained evidence:

- `conj_lattice.sage` constructs the lattice of isogenies from a random
  supersingular curve to its Frobenius conjugate. `analyze_lat.py` compares
  its exact ranks, determinants, degree counts and successive minima with the
  rank-three volume law.
- `pair_mc.py`, `toy_mitm.sage` and `validate_success.py` test the probability
  that a short isogeny degree splits into the two smooth table degrees needed
  by the meet-in-the-middle attack.
- `cost_model.py` and `cost_model2.py` optimize the attack under the
  specification, one-field-multiplication, `log2(p)^2`, free-memory and
  memory-charged conventions. `fp2_gates.py`, `smooth_check.py` and
  `psibar.gp` supply independent cost and density checks.

The compact `model-data/` inputs and `validation-results/` summaries are
retained. The raw lattice samples total about 25 MiB and are deterministic
regenerable output, so they are not retained. The result summaries record 700
p324_3 samples, 240 p500_27 samples and 100 p664_17 samples. The end-to-end
success check completed for p324_3 and p500_27; the p664_17 run timed out.

Requirements for the full analysis are SageMath, NumPy, PARI/GP and
`cypari2`. From this directory, the optimized table can be regenerated with:

```sh
python3 cost_model2.py > validation-results/cost_model2.json
```

The numbered report's portable reproducer needs only Python 3. It verifies
the hashes of these retained artifacts and checks their stated numerical
conclusions:

```sh
python3 SQIsign/reproduce_sqisign_4.py
```

The model is a concrete resource estimate, not an implementation of the
full-scale attack.
