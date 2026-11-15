# SDitH-4 — Low: Division branch on witness positions determines the witness under an ideal branch trace

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: All 12 parameter sets; the reference tree, and the optimized tree whenever its run-time dispatch does not select AVX2
Discovery: Moderate
Exploitation: Emulated ideal branch trace of honest key generation plus the public key recovers the witness for 9 of 9 keys; no measured channel
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

[SDitH-1](/nist/reports/SDitH-1.html) records that the reference helper
`div_rem_u32_ct_ref` branches on `rem >= public_d`
(`ct_utils.c:33` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/ct_utils.c#L33`)) and cites its use while sampling
witness positions (`rsd.c:46-64`). There the branch outcomes are the quotient
of a random draw. Key generation also calls the helper on the secret witness
positions themselves while encoding the solution
(`rsd.c:264` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/rsd.c#L264`)). At that call site the first-level outcomes
equal `pos // mu_1` for each secret position.

## Evidence

The reproducer builds cat1_fast, cat3_fast and cat5_fast from the reference
sources, with a temporary copy of `ct_utils.c` whose helper logs its branch
outcomes, and runs honest key generation for three seeds per category.

- At `rsd.c:264` the logged outcomes are exactly `pos // mu_1` and match an
  emulation of the helper; at `rsd.c:63` they are the draw quotient.
- From those outcomes, the public matrix and `y = Hx`, a GF(2) linear solve
  recovers the secret witness for 9 of 9 keys (224 to 416 unknowns, 488 to 904
  equations, kernel dimension 0).
- Control: without the trace the same system has kernel dimension 9,872 to
  18,960.

## Classification and limits

Low / Confirmed / Side-channel. This strengthens SDitH-1 with a second call
site and a candidate-specific map from an ideal trace to the key, but the
trace is emulated in software and no timing, cache or power channel was
measured. The logging changes the compiled code, so the result says nothing
about a particular compiler's output. The AVX2 helper uses masks and has no
such branch (source check). Only the `*_fast` sets were run.

Use a constant-time division for the witness encoding, as the AVX2 helper
does.

## Reproduction

```sh
python3 SDitH/reproduce_sdith_4.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SDitH-4
```
