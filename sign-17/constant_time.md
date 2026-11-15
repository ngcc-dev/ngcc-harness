# Constant-time review — 17 OPS-SIG

Scope: reference implementation, representative instance only: `Implementations and Test_Vectors/Implementations/Reference_Implementation/OPSsig-512/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/OPSsig-512/sign.c:131` — `evaluate_cs1_cs2_early_check_ops512` controls an early rejection on the signing response; source shows a variable path, not yet a demonstrated key leak. Inside it, `mult.c:59-69` branches on each challenge coefficient and reads the secret tables `s1_table`/`s2_table` at offsets set by the challenge support (`evaluate_ct0_ops512`, `mult.c:132-142`, does the same with the `t0` tables), and `mult.c:85-88` returns at the first coefficient of `w0'` or `z` over its bound; the challenge at that point belongs to an attempt that may still be rejected.
- Key generation: `sign.c:45-46` samples `s1`, `s2` through `poly_uniform_eta`/`rej_eta` (`poly.c:166-222`), which rejects nibbles of the stream expanded from the secret seed `rhoprime` and refills a data-dependent number of times.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Hint follow-up: `sign.c:142-147` calls `polyveck_make_hint` on private signing intermediates and rejects when the count exceeds `OMEGA`; `rounding.c:36-52` has value-dependent `make_hint`/`use_hint` tests. The accepted signature makes its hint public, but rejected-attempt traces are not part of that output. Whether this reveals an exploitable predicate beyond the public transcript remains untested.
