# MAYO design-review witnesses

These retained scripts reproduce the Informational design observations recorded
in [security_survey.md](../../security_survey.md). They load the unmodified
reference libraries through the shared harness ABI.

`mayo_buff.py` demonstrates non-resignability, single-key exclusive ownership,
single-key destructive exclusive ownership, and message-bound-signature
failures. `buff_controls.py` repeats those checks with wrong-key,
wrong-message, uncompensated-matrix, and Round-2-equation controls. MAYO claims
EUF-CMA and does not claim these BUFF properties, so the observations are not
EUF-CMA breaks.

```sh
python3 -I MAYO/analysis/design-review/buff_controls.py . mayo_1 5
python3 -I MAYO/analysis/design-review/buff_controls.py . mayo_2 5
python3 -I MAYO/analysis/design-review/buff_controls.py . mayo_3 5
python3 -I MAYO/analysis/design-review/buff_controls.py . mayo_5 5
```

`keyrec_rerun.py` demonstrates cross-parameter misuse: generating MAYO1 and
MAYO2 from the same 24-byte secret seed and signing common messages with the
same randomizer yields enough shared linear equations to recover both oil
spaces. It needs SageMath's Python environment.

```sh
sage -python MAYO/analysis/design-review/keyrec_rerun.py . 25 alpha
```

The 2026-10-18 independent review repeated the recovery with three seed labels.
It used 15 consistent signature pairs per recovery; extra input pairs allow the
script to discard pairs in which either signer retried. This is an out-of-model
cross-parameter key-management misuse, not a break of either parameter set's
EUF-CMA claim.
