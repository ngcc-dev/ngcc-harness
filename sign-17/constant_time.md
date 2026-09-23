# Constant-time review — 17 OPS-SIG

Scope: reference implementation, representative instance only: `Implementations and Test_Vectors/Implementations/Reference_Implementation/OPSsig-512/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/OPSsig-512/sign.c:131` — `evaluate_cs1_cs2_early_check_ops512` controls an early rejection on the signing response; source shows a variable path, not yet a demonstrated key leak.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Hint follow-up: `sign.c:142-147` calls `polyveck_make_hint` on private signing intermediates and rejects when the count exceeds `OMEGA`; `rounding.c:36-52` has value-dependent `make_hint`/`use_hint` tests. The accepted signature makes its hint public, but rejected-attempt traces are not part of that output. Whether this reveals an exploitable predicate beyond the public transcript remains untested.
